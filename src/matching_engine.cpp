#include "../include/matching_engine.h"
#include "../include/trade.h"

#include <vector>
#include <string>
#include <chrono>
#include <iostream>
#include <limits>

MatchingEngine::MatchingEngine(OrderBook& book) : book_(book) {}

std::vector<Trade> MatchingEngine::submit(OrderPtr incoming) {
  if (!incoming->is_marketable()) {
    incoming->side() == Side::BUY ?
      buy_stops_[incoming->stop_price()].push_back(incoming) :
      sell_stops_[incoming->stop_price()].push_back(incoming);
    return {};  // no trades take place when submitting a non marketable order
  }

  std::vector<Trade> trades = match(incoming);

  // Any unfilled quantity on a limit order rests in the book
  // Market orders that can't completely fill will expire, they never rest
  if (incoming->quantity() > 0 && incoming->type_str() == "LIMIT") {
    book_.add_order(incoming);
  }

  return trades;
};

std::vector<Trade> MatchingEngine::match(OrderPtr incoming) {

  std::vector<Trade> trades;

  auto price_acceptable = [&](Price resting_price) -> bool {
    if (incoming->type_str() == "MARKET") return true;

    if (incoming->side() == Side::BUY) return resting_price <= incoming->price();
    // Handle side::SELL
    return resting_price >= incoming->price(); 
  };

  auto match_order = [&](auto& resting_side) {
    while (incoming->quantity() > 0 && !resting_side.empty()) {

      auto best_level = resting_side.begin();

      if (!price_acceptable(best_level->first)) {
        break;
      }

      auto& queue = best_level->second;
      OrderPtr maker = queue.front();  // oldest order at this price level (FIFO)

      Qty fill_qty = std::min(incoming->quantity(), maker->quantity());

      trades.push_back({trade_id++,
                        maker->id(), 
                        incoming->id(),
                        incoming->side(),
                        maker->price(),
                        fill_qty,
                        std::chrono::steady_clock::now()
                      });
      
      incoming->fill(fill_qty);
      maker->fill(fill_qty);

      // If the maker is fully filled, remove it from the book
      if (book_.remove_if_filled(maker)) {
        std::cout << "Order Removed: " << maker->toString() << std::endl;
      }
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
  incoming->side() == Side::BUY ? match_order(book_.asks_) : match_order(book_.bids_);

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

  std::vector<OrderPtr> to_trigger;

  // take a sides stops and add to the to_trigger vector if the stop is ready to become live
  auto trigger = [&](auto& stop_side) {
    for (auto stop_iter = stop_side.begin(); stop_iter != stop_side.end() && stop_iter->first <= last_price; ) {
      for (const auto& stop : stop_iter->second) {
        to_trigger.push_back(stop);
      }
      stop_iter = stop_side.erase(stop_iter);
    }
  };

  // add buy market stops to to_trigger vector
  trigger(buy_stops_);

  // add sell market stops to to_trigger vector
  trigger(sell_stops_);

  for (const auto& stop : to_trigger) {

    std::shared_ptr<Order> market {};

    if (stop->type_str() == "STOP ORDER") {
       market = std::make_shared<MarketOrder>(stop->id(), stop->side(), stop->quantity());
    } else if (stop->type_str() == "STOP LIMIT ORDER") {
      market = std::make_shared<LimitOrder>(stop->id(), stop->side(), stop->quantity(), stop->price());
    } else {
      std::cerr << "error in check stops - order is neither stop or stop limit\n";
    }
    
    auto stop_trades = submit(market);

    for (const auto& trade : stop_trades) {
      trades.push_back(trade);
    }
  }

  return trades;

};