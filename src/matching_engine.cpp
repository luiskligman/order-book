#include "matching_engine.h"

#include <iostream>
#include <iomanip>

MatchingEngine::MatchingEngine(OrderBook& book) : book_(book) {}

std::vector<Trade> MatchingEngine::submit(OrderPtr incoming) {
    // Stop orders are not marketable, they sit in pending until triggered
    if (!incoming->is_marketable()) {
        incoming->side() == Side::BUY ? 
                buy_stops_[incoming->price()].push_back(incoming) 
                : sell_stops_[incoming->price()].push_back(incoming);
        return {};
    }

    int remaining = incoming->quantity();
    std::vector<Trade> trades = match(incoming, remaining);

    // Any unfilled quantity on a limit order rests in the book
    // Market orders that can't fully fill will expire, they never rest
    if (remaining > 0 && incoming->type_str() == "LIMIT") {
        book_.add_order(incoming);
        remaining_qty_[incoming->id()] = remaining;
    }
    
    return trades; 
}

void MatchingEngine::print() const {
    std::cout << "\n=== ORDER BOOK ===\n";
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "  ASKS (lowest first):\n";
    for (auto price_level = book_.asks_.rbegin(); price_level != book_.asks_.rend(); ++price_level) {
        int total_qty = 0;
        for (const auto& order : price_level->second) total_qty += order->quantity();
        std::cout << "    $" << price_level->first << "  qty=" << total_qty
                  << "  (" << price_level->second.size() << " order(s))\n";
    }

    auto bid = book_.best_bid();
    auto ask = book_.best_ask();

    if (bid && ask) {
        std::cout << "  --- spread: $" << (*ask - *bid) << " ---\n";
    } else {
        std::cout << "  --- no spread (book not crossed) ---\n";
    }

    std::cout << "  BIDS (higest first):\n";
    for (const auto& [price, orders] : book_.bids_) {
        int total_qty = 0;
        for (const auto& order : orders) {
            auto it = remaining_qty_.find(order->id());
            total_qty += (it != remaining_qty_.end()) ? it->second : order->quantity();
            std::cout << "    $" << price << "  qty=" << total_qty
                      << "  (" << orders.size() << " order(s))\n"; 
        }
    }

    std::cout << "  RESTING STOP ORDERS (normally hidden):\n";
    if (buy_stops_.empty()  && sell_stops_.empty()) {
        std::cout << " no resting stop orders\n";
    } else {
        std::cout << "  SELL STOPS (descending):\n";
        for (const auto& [price, orders] : sell_stops_) {
            for (const auto& order : orders) {
                std::cout << "    $" << order->price() << "  qty=" << order->quantity()
                          << "  side=" << (order->side() == Side::BUY ? "BUY" : "SELL") << "\n"; 
            }
        }
        std::cout << "  BUY STOPS (ascending):\n";
        for (const auto& [price, orders] : buy_stops_) {
            for (const auto& order : orders) {
                std::cout << "    $" << order->price() << "  qty=" << order->quantity()
                          << "  side=" << (order->side() == Side::BUY ? "BUY" : "SELL") << "\n"; 
            }
        }
    }

    std::cout << "\n==================\n";
}

std::vector<Trade> MatchingEngine::match(OrderPtr incoming, int& remaining) {
    std::vector<Trade> trades;

    // Lambda function: will this incoming order accept a fill at resting_price
    // The answer depends on order type and side
    auto price_acceptable = [&](double resting_price) -> bool {
        if (incoming->type_str() == "MARKET") return true;
        if (incoming->side() == Side::BUY) return resting_price <= incoming->price();
        return resting_price >= incoming->price();  // Handle Side::SELL
    }; 

    // BUY orders match against asks; SELL order match against bids 
    if (incoming->side() == Side::BUY) {
        while (remaining > 0 && !book_.asks_.empty()) {
            auto best_level = book_.asks_.begin();  // lowest ask first
            if (!price_acceptable(best_level->first)) break;

            auto& queue = best_level->second;
            OrderPtr maker = queue.front();  // oldest order at this price level (FIFO)

            int fill_qty = std::min(remaining, remaining_qty_[maker->id()]);

            trades.push_back({maker->id(), incoming->id(),
                            best_level->first, fill_qty,
                            std::chrono::steady_clock::now()});

            remaining -= fill_qty;
            remaining_qty_[maker->id()] -= fill_qty;

            // If the maker is fully filled, remove it from the book entirely
            if (remaining_qty_[maker->id()] == 0) {
                queue.pop_front();  // remove best ask from the deque, no more available quantity to offer
                remaining_qty_.erase(maker->id());
                book_.order_index_.erase(maker->id());
                if (queue.empty()) book_.asks_.erase(best_level);  // no more asks in the price level
            }
            // check for stops with the last executed trading price
            auto stop_trades = check_stops(trades.back().price);
            trades.insert(trades.end(), stop_trades.begin(), stop_trades.end());
        }
    } else {
        while (remaining > 0 && !book_.bids_.empty()) {
            auto best_level = book_.bids_.begin();  // highest bid first
            if (!price_acceptable(best_level->first)) break;

            auto& queue = best_level->second;
            OrderPtr maker = queue.front();

            int fill_qty = std::min(remaining, remaining_qty_[maker->id()]);

            trades.push_back({maker->id(), incoming->id(),
                            best_level->first, fill_qty,
                            std::chrono::steady_clock::now()});

            remaining -= fill_qty;
            remaining_qty_[maker->id()] -= fill_qty;

            if (remaining_qty_[maker->id()] == 0) {
                queue.pop_front();  // remove best bid from the deque, no more available quantity to offer
                remaining_qty_.erase(maker->id());
                book_.order_index_.erase(maker->id());
                if (queue.empty()) book_.bids_.erase(best_level);
            }

            // check for stops with the last executed trading price
            auto stop_trades = check_stops(trades.back().price);
            trades.insert(trades.end(), stop_trades.begin(), stop_trades.end());
        }
    }

    return trades;
}

std::vector<Trade> MatchingEngine::check_stops(double last_price) {
    std::vector<Trade> trades;

    // check if there are no stops
    if (buy_stops_.empty() && sell_stops_.empty()) { return trades;}
    
    // get the price level of the best buy stop order if one exists
    double best_buy = buy_stops_.empty() ? std::numeric_limits<double>::max() : buy_stops_.begin()->first;

    // get the price level of the best sell stop order if one exists
    double best_sell = sell_stops_.empty() ? std::numeric_limits<double>::lowest() : sell_stops_.begin()->first;

    // invalid conditions for stops to be changed into market orders
    if (best_buy > last_price && best_sell < last_price) { return trades; }

    // collect and erase all triggered buy stops
    std::vector<OrderPtr> to_trigger;
    for (auto stop_iter = buy_stops_.begin(); stop_iter != buy_stops_.end() && stop_iter->first <= last_price; ) {
        for (const auto& stop : stop_iter->second)
            to_trigger.push_back(stop);
        stop_iter = buy_stops_.erase(stop_iter);
    }

    // now match the stop orders that became market orders
    for (const auto& stop : to_trigger) {
        auto market = std::make_shared<MarketOrder>(stop->id(), stop->side(), stop->quantity());
        int remaining = market->quantity();
        auto stop_trades = match(market, remaining);
        trades.insert(trades.end(), stop_trades.begin(), stop_trades.end());
    }

    return trades;
}


