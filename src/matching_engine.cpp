#include "matching_engine.h"
#include "trade.h"

#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

MatchingEngine::MatchingEngine(OrderBook& book) : book_(book) {}

std::vector<Trade> MatchingEngine::submit(OrderVariant incoming) {
  // if (!incoming.is_marketable()) {
  //   incoming->side() == Side::BUY ?
  //     buy_stops_[incoming->stop_price()].push_back(incoming) :
  //     sell_stops_[incoming->stop_price()].push_back(incoming);
  //   return {};  // no trades take place when submitting a non marketable order
  // }
  if (!is_marketable(incoming)) {
    Price stop_trigger = std::visit(overloaded{
      [](const StopOrder& o) { return o.stop_price(); },
      [](const StopLimitOrder& o) { return o.stop_price(); },
      [](const auto &) -> Price {
        throw std::logic_error("non-marketable order missing a stop_price:");
      }
    }, incoming);

    side(incoming) == Side::BUY ?
      buy_stops_[stop_trigger].push_back(std::move(incoming)) :
      sell_stops_[stop_trigger].push_back(std::move(incoming));
    return {};  // no trades take place when submitting a non marketable order
  }

  std::vector<Trade> trades = match(incoming);

  // Any unfilled quantity on a limit order rests in the book
  // Market orders that can't completely fill will expire, they never rest
  if (quantity(incoming) > 0 && std::holds_alternative<LimitOrder>(incoming)) {
    book_.add_order(std::get<LimitOrder>(std::move(incoming)));
  }

  return trades;
};

std::vector<Trade> MatchingEngine::match(OrderVariant incoming) {

  std::vector<Trade> trades;

  auto price_acceptable = [&](Price resting_price) -> bool {
    return std::visit(overloaded {
      [](const MarketOrder&) { return true; },
      [&](const LimitOrder& o) {
        return side(incoming) == Side::BUY ? resting_price <= o.price() : resting_price >= o.price();
      },
      [&](const StopLimitOrder& o) {
        return side(incoming) == Side::BUY ? resting_price <= o.price() : resting_price >= o.price();
      },
      [](const StopOrder&) -> bool {
        throw std::logic_error("StopOrder should never reach match() directly");
      }
    }, incoming);
  };

  auto match_order = [&](auto& resting_side) {
    while (quantity(incoming) > 0 && !resting_side.empty()) {

      auto best_level = resting_side.begin();

      if (!price_acceptable(best_level->first)) {
        break;
      }

      auto& queue = best_level->second;
      LimitOrder& maker = queue.front();  // oldest order at this price level (FIFO)

      Qty fill_qty = std::min(quantity(incoming), maker.quantity());

      trades.push_back({trade_id++,
                        maker.id(), 
                        id(incoming),
                        side(incoming),
                        maker.price(),
                        fill_qty,
                        std::chrono::steady_clock::now()
                      });
      
      // incoming->fill(fill_qty);
      fill(incoming, fill_qty);
      // maker->fill(fill_qty);
      maker.fill(fill_qty);

      // If the maker is fully filled, remove it from the book
      book_.remove_if_filled(maker);
    }
  };

  auto check_marketable_stops = [&]() {
    // Check if stops can become marketable after last trade executed
    if (!trades.empty()) {
      auto stop_trades = check_stops(trades.back().price);

      for (auto& trade : stop_trades) {
        trades.push_back(trade);
      }
    }
  };


  // BUY orders match against asks
  // SELL orders match against bids
  side(incoming) == Side::BUY ? match_order(book_.asks_) : match_order(book_.bids_);

  check_marketable_stops();

  return trades;
  
};

std::vector<Trade> MatchingEngine::check_stops(Price last_price) {

  std::vector<Trade> trades;

  // Check if there are stops
  if (buy_stops_.empty() && sell_stops_.empty()) { return trades; }

  // Get the price level of the best buy stop order if one exists
  Price best_buy_stop = buy_stops_.empty() ? std::numeric_limits<Price>::max() : buy_stops_.begin()->first;

  // Get the price level of the best sell stop order if one exists
  Price best_sell_stop = sell_stops_.empty() ? std::numeric_limits<Price>::lowest() : sell_stops_.begin()->first;

  // If buy stops and sell stops are not ready to trigger 
  if (best_buy_stop > last_price && best_sell_stop < last_price) { return trades; }

  std::vector<OrderVariant> to_trigger;

  // take a sides stops and add to the to_trigger vector if the stop is ready to become live
  auto trigger = [&](auto& stop_side, Side side) {
    for (auto stop_iter = stop_side.begin(); 
          stop_iter != stop_side.end() && (side == Side::BUY ? stop_iter->first <= last_price : stop_iter->first >= last_price); ) {
      for (const auto& stop : stop_iter->second) {
        to_trigger.push_back(std::move(stop));
      }
      stop_iter = stop_side.erase(stop_iter);
    }
  };

  // add buy market stops to to_trigger vector
  trigger(buy_stops_, Side::BUY);

  // add sell market stops to to_trigger vector
  trigger(sell_stops_, Side::SELL);

  // for (const auto& stop : to_trigger) {

  //   std::shared_ptr<Order> market {};

  //   if (stop->type_str() == "STOP ORDER") {
  //      market = std::make_shared<MarketOrder>(stop->id(), stop->side(), stop->quantity());
  //   } else if (stop->type_str() == "STOP LIMIT ORDER") {
  //     market = std::make_shared<LimitOrder>(stop->id(), stop->side(), stop->quantity(), stop->price());
  //   } else {
  //     std::cerr << "error in check stops - order is neither stop or stop limit\n";
  //   }
    
  //   auto stop_trades = submit(market);

  //   for (const auto& trade : stop_trades) {
  //     trades.push_back(trade);
  //   }
  // }
  for (const auto& stop : to_trigger) {

    OrderVariant market = std::visit(overloaded {
      [](const StopOrder& o) -> OrderVariant {
        return MarketOrder(o.id(), o.side(), o.quantity());
      },
      [](const StopLimitOrder& o) -> OrderVariant {
        return LimitOrder(o.id(), o.side(), o.quantity(), o.price());
      },
      [](const auto&) -> OrderVariant {
        throw std::logic_error("check_stops: order is neither StopOrder or StopLimitOrder");
      }
    }, stop);

    auto stop_trades = submit(std::move(market));

    for (const auto& trade : stop_trades) {
      trades.push_back(trade);
    }
  }

  return trades;
};