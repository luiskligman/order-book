
#include "../include/order_book.h"

#include <iostream>
#include <iomanip>

void OrderBook::add_order(OrderPtr order) {
    // Route to the correct side
    if (order->side() == Side::BUY) {
        bids_[order->price()].push_back(order);  // Use .push_back() to ensure price-time priority
    } else {
        asks_[order->price()].push_back(order);
    }
    // Index it for fast cancel lookup
    order_index_[order->id()] = order;
}

bool OrderBook::cancel_order(OrderID id) {
    auto index_entry = order_index_.find(id);  
        // If not found .find(id) returns container at .end()
    if (index_entry == order_index_.end()) return false;

    // index_entry->first is the OrderID, index_entry->second is a pointer to the Order with OrderID
    OrderPtr order = index_entry->second; 

    // Remove from the correct price level on the correct side
    if (order->side() == Side::BUY) {
        auto price_level = bids_.find(order->price());
        if (price_level != bids_.end()) {
            auto& queue = price_level->second;  // store bids_ dequeue as queue
            // use order_iter to iterate through orders in the deque 
            for (auto order_iter = queue.begin(); order_iter != queue.end(); ++order_iter) {  
                if ((*order_iter)->id() == id) { queue.erase(order_iter); break; }
            }
            if (queue.empty()) bids_.erase(price_level);
        }
    } else {
        auto price_level = asks_.find(order->price());
        if (price_level != asks_.end()) {
            auto& queue = price_level->second;  // store asks_ dequeue as queue
            // use order_iter to iterate through orders in the deque 
            for (auto order_iter = queue.begin(); order_iter != queue.end(); ++order_iter) {
                if ((*order_iter)->id() == id) { queue.erase(order_iter); break; }
            }
            if (queue.empty()) asks_.erase(price_level);
        }
    }

    order_index_.erase(index_entry);
    return true; 
    
}

std::optional<double> OrderBook::best_bid() const {
    if (bids_.empty()) return std::nullopt;
    return bids_.begin()->first;
}

std::optional<double> OrderBook::best_ask() const {
    if (asks_.empty()) return std::nullopt;
    return asks_.begin()->first;
}

void OrderBook::print() const {
    std::cout << "\n=== ORDER BOOK ===\n";
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "  ASKS (lowest first):\n";
    // Print asks in reverse so highest ask is at the top
    for (auto price_level = asks_.rbegin(); price_level != asks_.rend(); ++price_level) {
        int total_qty = 0;
        for (const auto& order : price_level->second) total_qty += order->quantity();
        std::cout << "    $" << price_level->first << "  qty=" << total_qty
            << "  (" << price_level->second.size() << " order(s))\n";
    }

    auto bid = best_bid();
    auto ask = best_ask();
    if (bid && ask)
        std::cout << "  --- spread: $" << (*ask - *bid) << " ---\n";
    else
        std::cout << "  --- no spread (book not crossed) ---\n";

    std::cout <<  "  BIDS (highest first):\n";
    for (const auto& [price, orders] : bids_) {
        int total_qty = 0;
        for (const auto& order : orders) total_qty += order->quantity();
        std::cout << "    $" << price << "  qty=" << total_qty
            << "  (" << orders.size() << " order(s))\n";
    }
    std::cout << "==================\n\n";
}

void OrderBook::print(const std::unordered_map<OrderID, int>& remaining_qty) const {
    std::cout << "\n=== ORDER BOOK ===\n";
    std::cout << std::fixed << std::setprecision(2);

    std::cout << "  ASKS (lowest first):\n";
    // Print asks in reverse so highest ask is at the top
    for (auto price_level = asks_.rbegin(); price_level != asks_.rend(); ++price_level) {
        int total_qty = 0;
        for (const auto& order : price_level->second) {
            auto remaining = remaining_qty.find(order->id());
            total_qty += (remaining != remaining_qty.end()) ? remaining->second : order->quantity();
        }
        std::cout << "    $" << price_level->first << "  qty=" << total_qty
            << "  (" << price_level->second.size() << " order(s))\n";
    }

    auto bid = best_bid();
    auto ask = best_ask();
    if (bid && ask)
        std::cout << "  --- spread: $" << (*ask - *bid) << " ---\n";
    else
        std::cout << "  --- no spread (book not crossed) ---\n";

    std::cout <<  "  BIDS (highest first):\n";
    for (const auto& [price, orders] : bids_) {
        int total_qty = 0;
        for (const auto& order : orders) {
            auto remaining = remaining_qty.find(order->id());
            total_qty += (remaining != remaining_qty.end()) ? remaining->second : order->quantity();
        }
        std::cout << "    $" << price << "  qty=" << total_qty
            << "  (" << orders.size() << " order(s))\n";
    }
    std::cout << "==================\n\n";
}
