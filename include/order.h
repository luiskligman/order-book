#pragma once

#include <cstdint>
#include <chrono>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <iomanip>

// OrderId is a type alias for unsigned 64 int
using OrderID = uint64_t;
using Price = int64_t;
using Qty = int64_t;

// Side represents which direction an order is
enum class Side { BUY, SELL };

constexpr int64_t PRICE_SCALE = 10000;

inline std::ostream& operator<<(std::ostream& os, Side s) {
  switch(s) {
    case Side::BUY: return os << "BUY";
    case Side::SELL: return os << "SELL";
  }
  return os << "unknown";
}

inline Price to_ticks(const double real_price) {
  return static_cast<Price>(std::llround(real_price * PRICE_SCALE));
}

inline std::string ticks_to_string(Price ticks) {
  Price whole = ticks / PRICE_SCALE;
  Price frac = ticks % PRICE_SCALE;
  std::ostringstream oss;
  oss << whole << '.' << std::setfill('0') << std::setw(4) << frac;
  return oss.str();
}

class Order {
  public:
    
    Order(OrderID id, Side side, Qty quantity, Price price, Price stop_price)
      : id_(id)
      , side_(side)
      , original_qty_(quantity)
      , quantity_(quantity)
      , price_(price)
      , stop_price_(stop_price)
      , timestamp_(std::chrono::steady_clock::now())
      {}

    // Virtual destructor is required any time you delete through a base class pointer
    // Withjout, deleting a LimitOrder* through an Order* would be undefined behavior
    virtual ~Order() = default;

    // RULE OF 5
    // Manually delete copy ctor, copy assignment, move ctor, move assignment
    // Makes compiler errors instead of undefined behavior
    Order(const Order&) = delete;  // copy constructor
    Order& operator=(const Order&) = delete;  // copy assignment
    Order(Order&&) = delete;  // move constructor
    Order& operator=(Order&&) = delete;  // move assignment

    OrderID id() const { return id_; }
    Side side() const { return side_; }
    Qty original_qty() const { return original_qty_; }
    Qty quantity() const { return quantity_; }
    Price price() const { return price_; }
    Price stop_price() const { return stop_price_; }
    auto timestamp() const { return timestamp_; }  // might be able to just use auto

    void fill(Qty fill_qty) {
      if (quantity_ < fill_qty) {
          throw std::runtime_error("fill quantity exceeds available quanity");
          return;
      }
      quantity_ -= fill_qty;
      return;
    }

    std::string toString() const {
      std::ostringstream oss;
      oss << "Order ID: " << id()
          << "   Side: " << side()
          << "   Original Qty: " << original_qty()
          << "   Quantity: " << quantity()
          << "   Price: " << ticks_to_string(price())
          << "   Stop Price: " << ticks_to_string(stop_price())
          << "   Timestamp: " << timestamp().time_since_epoch().count();
      return oss.str();
    }

    // pure virtual methods: every concrete order type must answer these
    // is_marketable(): can this order match immediately against the book
    // type_str(): human-readable name for printing
    virtual bool is_marketable() const = 0;
    virtual std::string type_str() const = 0;

  private: 
    const OrderID id_;
    const Side side_;
    const Qty original_qty_;
    Qty quantity_;
    const Price price_;
    const Price stop_price_;
    const std::chrono::time_point<std::chrono::steady_clock> timestamp_;
};


// LimitOrder
// Matches only at the limit price or better
// is_marketable() returns true because we allow it to attempt a match
// the matching engine will enfore the price constraint
class LimitOrder : public Order {
  public: 
    LimitOrder(OrderID id, Side side, int quantity, double price)
      : Order(id, side, quantity, to_ticks(price), 0)
      {}

      bool is_marketable() const override { return true; }
      std::string type_str() const override { return "LIMIT"; }
};

// MarketOrder
// Matches at whatever price is available
// Price is stored as 0.0 since it is irrelevant for market orders
class MarketOrder : public Order {
  public:
    MarketOrder(OrderID id, Side side, int quantity)
    : Order(id, side, quantity, 0, 0)
    {}

    bool is_marketable() const override { return true; }
    std::string type_str() const override { return "MARKET"; }
};

// StopOrder
// Sits dormant until the market price crosses the trigger price
// Once crossed it then becomes a market order
class StopOrder : public Order {
  public:
    StopOrder(OrderID id, Side side, int quantity, double stop_price)
    : Order(id, side, quantity, 0, to_ticks(stop_price))
    {}

    bool is_marketable() const override { return false; }
    std::string type_str() const override { return "STOP ORDER"; }
};

// StopLimitOrder
// Sits dormant until the market price crosses the trigger price
// Then it becomes a limit order
class StopLimitOrder : public Order {
  public:
    StopLimitOrder(OrderID id, Side side, int quantity, double price, double stop_price)
    : Order(id, side, quantity, to_ticks(price), to_ticks(stop_price))
    {}

    bool is_marketable() const override { return false; }
    std::string type_str() const override { return "STOP LIMIT ORDER"; }
};