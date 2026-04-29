#include "matching_engine.h"

#include <iostream>
#include <iomanip>

MatchingEngine::MatchingEngine(OrderBook& book) : book_(book) {}

std::vector<Trade> MatchingEngine::submit(OrderPtr incoming) {
    // Stop orders are not marketable, they sit in pending until triggered
    if (!incoming->is_marketable()) {
        pending_stops_.push_back(incoming);
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

            // Check if the last fill price triggers any waiting stop orders
            auto stop_trades = check_stops(trades.back().price);
            trades.insert(trades.end(), stop_trades.begin(), stop_trades.end());

            // If the maker is fully filled, remove it from the book entirely
            if (remaining_qty_[maker->id()] == 0) {
                queue.pop_front();  // remove best ask from the deque, no more available quantity to offer
                remaining_qty_.erase(maker->id());
                book_.order_index_.erase(maker->id());
                if (queue.empty()) book_.asks_.erase(best_level);  // no more asks in the price level
            }
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

            // Check if the last fill price triggers any waiting stop orders
            auto stop_trades = check_stops(trades.back().price);
            trades.insert(trades.end(), stop_trades.begin(), stop_trades.end());

            if (remaining_qty_[maker->id()] == 0) {
                queue.pop_front();  // remove best bid from the deque, no more available quantity to offer
                remaining_qty_.erase(maker->id());
                book_.order_index_.erase(maker->id());
                if (queue.empty()) book_.bids_.erase(best_level);
            }
        }
    }

    return trades;
}

std::vector<Trade> MatchingEngine::check_stops(double last_price) {
    std::vector<Trade> trades;

    for (auto stop_iter = pending_stops_.begin(); stop_iter != pending_stops_.end(); ) {
        OrderPtr stop = *stop_iter;
        double trigger = stop->price();

        // BUY stop triggers when price rises to or above the trigger
        // SELL stop triggers when price falls to or below the trigger
        bool triggered = (stop->side() == Side::BUY && last_price >= trigger) ||
                         (stop->side() == Side::SELL && last_price <= trigger);

        // Convert the triggered stop into a market order and match it immediately
        // We cannot mutate the stop order (fields are const), therefore, we must create a new
        // MarketOrder with the same ID so fills can be traced back to the original stop
        if (triggered) {
            // Orders are immutable so we can't convert in place.
            // Create a new market order carrying the same ID for traceability
            auto market = std::make_shared<MarketOrder>(
                stop->id(), stop->side(), stop->quantity()
            );

            int remaining = market->quantity();
            auto stop_trades = match(market, remaining);
            trades.insert(trades.end(), stop_trades.begin(), stop_trades.end());
            stop_iter = pending_stops_.erase(stop_iter);
        } else {
            ++stop_iter;
        }
    }

    return trades;
}


