#pragma once

#include "order.h"

#include <chrono>
#include <cstdint>
#include <sstream>
#include <string>

using TradeID = uint64_t;

struct Trade {
  
  const TradeID id;
  const OrderID maker_id;
  const OrderID taker_id;
  const Side taker_side;
  const Price price;
  const Qty quantity;
  const std::chrono::time_point<std::chrono::steady_clock> timestamp =
    std::chrono::steady_clock::now();

  std::string toString() const {
    std::ostringstream oss;

    oss << "Trade ID: " << id
        << "  Maker Order ID: " << maker_id 
        << "  Taker Order ID: " << taker_id 
        << "  Taker Side: " << taker_side
        << "  Price: " << price
        << "  Quantity: " << quantity
        << "  Timestamp: " << timestamp.time_since_epoch().count();
    
    return oss.str();

  }

};