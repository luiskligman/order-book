#pragma once

#include "order.h"

#include <memory>
#include <optional>
#include <map>
#include <deque>
#include <functional>
#include <unordered_map>

// Convenience alias - we use this type everywhere in the book.
// shared_ptr<const Order<double, int>> means:
//  - shared ownership (multiple things can hold a reference)
//  - const: nobody holding this pointer can mutate the order
using OrderPtr = std::shared_ptr<const Order<double, int>>;

class OrderBook {
   friend class MatchingEngine;

 public:
    // Add an order to the appropriate side of the book
    void add_order(OrderPtr order);

    // Remove an order by ID. Returns true if found and removed
    bool cancel_order(OrderID id);

    // Returns the best bid price (highest buy), if any bids exists
    std::optional<double> best_bid() const;  // const signifies that calling .best_bid() on an OrderBook object will not 
                                             // modify the contents of the OrderBook
    
    // Returns the best ask price (lowest sell), if any asks exist
    std::optional<double> best_ask() const;

    // Print the current book state to stdout
    void print() const;
    void print(const std::unordered_map<OrderID, int>& remaining_qty) const;

 private: 
    // Bids: highest price first, use std::greater<double> to reverse the default 
    // ascending order so bids_[0] is always the best (highest) bid.
    std::map<double, std::deque<OrderPtr>, std::greater<double>> bids_;
    
    // Asks: lowest price first (default order)
    // asks_[0] is always the best (lowest) ask
    std::map<double, std::deque<OrderPtr>> asks_;

    // Flat index: OrderID -> pointer into the book
    // O(1) lookup time complexity
    std::unordered_map<OrderID, OrderPtr> order_index_;

};