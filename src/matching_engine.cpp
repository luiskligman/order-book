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

  // BUY orders match against asks
  // SELL orders match against bids
  if (incoming->side() == Side::BUY) {
    while (incoming->quantity() > 0 && !book_.asks_.empty()) {

      auto best_level = book_.asks_.begin();  // lowest ask price level first

      if (!price_acceptable(best_level->first)) {
        break;
      }

      auto& queue = best_level->second;
      OrderPtr maker = queue.front();  // oldest order at this price level (FIFO)

      Qty fill_qty = std::min(incoming->quantity(), maker->quantity());

      trades.push_back({0,  // default TradeID will be 0, need an auto increment
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

    // Check if stops can become marketable after last trade executed
    if (!trades.empty()) {
      auto stop_trades = check_stops(trades.back().price);
      // trades.insert(trades.end(), stop_trades.begin(), stop_trades.end());
      for (auto& trade : stop_trades) {
        trades.push_back(trade);
      }
    }

  } else {
    while (incoming->quantity() > 0 && !book_.bids_.empty()) {

      auto best_level = book_.bids_.begin();  // highest bid fist

      if (!price_acceptable(best_level->first)) {
        break;
      }

      auto& queue = best_level->second;
      OrderPtr maker = queue.front();  // oldest order at this price level (FIFO)

      Qty fill_qty = std::min(incoming->quantity(), maker->quantity());

      trades.push_back({0,  // default TradeID will be 0, need an auto increment
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

    // Check if stops can become marketable after last trade executed
    if (!trades.empty()) {
      auto stop_trades = check_stops(trades.back().price);
      // trades.insert(trades.end(), stop_trades.begin(), stop_trades.end());
      for (auto& trade : stop_trades) {
        trades.push_back(trade);
      }
    }
  }

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

  // add buy market stops to to_trigger vector
  for (auto stop_iter = buy_stops_.begin(); stop_iter != buy_stops_.end() && stop_iter->first <= last_price; ) {
    for (const auto& stop : stop_iter->second) {
      to_trigger.push_back(stop);
    }
    stop_iter = buy_stops_.erase(stop_iter);
  }

  // add sell market stops to to_trigger vector
  for (auto stop_iter = sell_stops_.begin(); stop_iter != sell_stops_.end() && stop_iter->first >= last_price; ) {
    for (const auto& stop : stop_iter->second) {
      to_trigger.push_back(stop);
    }
    stop_iter = sell_stops_.erase(stop_iter);
  }

  for (const auto& stop : to_trigger) {
    auto market = std::make_shared<MarketOrder>(stop->id(), stop->side(), stop->quantity());
    
    auto stop_trades = match(market);

    for (const auto& trade : stop_trades) {
      trades.push_back(trade);
    }
  }

  std::cerr << "TEST\n";
  return trades;

};