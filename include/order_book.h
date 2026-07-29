#pragma once

#include <order.h>

#include <memory>
#include <optional>
#include <map>
#include <deque>
#include <functional>
#include <unordered_map>
#include <sstream>


using OrderPtr = std::shared_ptr<Order>;

class OrderBook {

  friend class MatchingEngine;

  public: 
    // Add an order to the appropriate side of the book
    void add_order(OrderPtr order);

    // Remove an order by ID. Returns true if found and removed
    bool cancel_order(OrderID id);

    // Remove an order if quantity is zero
    // Return true is removed, false if not
    bool remove_if_filled(OrderPtr order);

    // Returns the best bid price (highest buy), if any bids exist
    std::optional<Price> best_bid() const; 

    // Returns the best ask price (lowest sell), if any asks exist
    std::optional<Price> best_ask() const;

    // Print the current book state to stdout
    std::string print() const;

  private:
    // Bids - highest price first, use std::greater<double> to reverse the default
    // ascending order so bids_.cbegin() is always the best (highest) bid
    std::map<Price, std::deque<OrderPtr>, std::greater<Price>> bids_;

    // Asks - lowest price first (default order)
    // asks_.cbegin() is always the best (lowest) ask
    std::map<Price, std::deque<OrderPtr>> asks_;

    // Flat index - OrderID -> pointer into the book
    std::unordered_map<OrderID, OrderPtr> order_index_;

};