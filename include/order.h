#pragma once

#include <cstdint>
#include <chrono>
#include <string>
#include <stdexcept>
#include <sstream>

// OrderId is a type alias for unsigned 64 int
using OrderID = uint64_t;

// Side represents which direction an order is
enum class Side { BUY, SELL };

inline std::ostream& operator<<(std::ostream& os, Side s) {
  switch(s) {
    case Side::BUY: return os << "BUY";
    case Side::SELL: return os << "SELL";
  }
  return os << "unknown";
}

// Order<PriceT, QtyT> is the abstract base for all order types
template<typename PriceT, typename QtyT>
class Order {
  public:
    
    Order(OrderID id, Side side, QtyT quantity, PriceT price, PriceT stop_price)
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

    OrderID id() const { return id_; }
    Side side() const { return side_; }
    QtyT original_qty() const { return original_qty_; }
    QtyT quantity() const { return quantity_; }
    PriceT price() const { return price_; }
    PriceT stop_price() const { return stop_price_; }
    auto timestamp() const { return timestamp_; }  // might be able to just use auto

    void fill(QtyT fill_qty) {
      if (quantity_ < fill_qty) {
          throw std::runtime_error("fill quantity exceeds available quanity");
          return;
      }
      quantity_ -= fill_qty;
      return;
    }

    // std::string toString() {
    //   return "Order ID: " + id() + " Side: " + side() + " Original Qty: " + original_qty() +
    //     " Quantity: " + quantity() + " Price: " + price() + " Stop Price: " + stop_price() + 
    //     " TimeStamp: " + std::string(timestamp());
    // }
    std::string toString() const {
      std::ostringstream oss;
      oss << "Order ID: " << id()
          << "   Side: " << side()
          << "   Original Qty: " << original_qty()
          << "   Quantity: " << quantity()
          << "   Price: " << price()
          << "   Stop Price: " << stop_price()
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
    const QtyT original_qty_;
    QtyT quantity_;
    const PriceT price_;
    const PriceT stop_price_;
    const std::chrono::time_point<std::chrono::steady_clock> timestamp_;
};


// LimitOrder
// Matches only at the limit price or better
// is_marketable() returns true because we allow it to attempt a match
// the matching engine will enfore the price constraint
class LimitOrder : public Order<double, int> {
  public: 
    LimitOrder(OrderID id, Side side, int quantity, double price)
      : Order<double, int>(id, side, quantity, price, 0.0)
      {}

      bool is_marketable() const override { return true; }
      std::string type_str() const override { return "LIMIT"; }
};

// MarketOrder
// Matches at whatever price is available
// Price is stored as 0.0 since it is irrelevant for market orders
class MarketOrder : public Order<double, int> {
  public:
    MarketOrder(OrderID id, Side side, int quantity)
    : Order<double, int>(id, side, quantity, 0.0, 0.0)
    {}

    bool is_marketable() const override { return true; }
    std::string type_str() const override { return "MARKET"; }
};

// StopOrder
// Sits dormant until the market price crosses the trigger price
// Once crossed it then becomes a market order
class StopOrder : public Order<double, int> {
  public:
    StopOrder(OrderID id, Side side, int quantity, double stop_price)
    : Order<double, int>(id, side, quantity, 0.0, stop_price)
    {}

    bool is_marketable() const override { return false; }
    std::string type_str() const override { return "STOP ORDER"; }
};

// StopLimitOrder
// Sits dormant until the market price crosses the trigger price
// Then it becomes a limit order
class StopLimitOrder : public Order<double, int> {
  public:
    StopLimitOrder(OrderID id, Side side, int quantity, double price, double stop_price)
    : Order<double, int>(id, side, quantity, price, stop_price)
    {}

    bool is_marketable() const override { return false; }
    std::string type_str() const override { return "STOP LIMIT ORDER"; }
};