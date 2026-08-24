#pragma once

#include <cstdint>
#include <chrono>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <variant>

// OrderId is a type alias for unsigned 64 int
using OrderID = uint64_t;
using Price = int64_t;
using Qty = int64_t;

/*
  Side represents which direction an order is

  Uses std::uint8_t (clang-tidy: performance-enum-size) insteaf of the default 
  int. 
*/ 
enum class Side : std::uint8_t { BUY, SELL };

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

class LimitOrder {
  public: 
    LimitOrder(OrderID id, Side side, Qty quantity, double price)
      : id_(id), side_(side), original_qty_(quantity)
      , quantity_(quantity), price_(to_ticks(price))
      , timestamp_(std::chrono::steady_clock::now()) 
      {}

    LimitOrder(OrderID id, Side side, Qty quantity, Price price) 
      : id_(id), side_(side), original_qty_(quantity)
      , quantity_(quantity), price_(price)
      , timestamp_(std::chrono::steady_clock::now()) {}

    OrderID id() const { return id_; }
    Side side() const { return side_; }
    Qty original_qty() const { return original_qty_; }
    Qty quantity() const { return quantity_; }
    Price price() const { return price_; }
    auto timestamp() const { return timestamp_; }

    void fill(Qty fill_qty) {
      if (quantity_ < fill_qty) {
          throw std::runtime_error("fill quantity exceeds available quanity");
      }
      quantity_ -= fill_qty;
    }

    bool is_marketable() const { return true; }
    std::string type_str() const { return "LIMIT"; }

    std::string toString() const {
      std::ostringstream oss;
      oss << "Order ID: " << id_ 
          << "  Side: " << side_
          << "  Original Qty: " << original_qty_
          << "  Quantity: " << quantity_
          << "  Price: " << price_ 
          << "  Timestamp: " << timestamp_.time_since_epoch().count();

      return oss.str();
    }

  private:
    OrderID id_;
    Side side_;
    Qty original_qty_;
    Qty quantity_;
    Price price_;
    std::chrono::time_point<std::chrono::steady_clock> timestamp_;
};

class MarketOrder {
  public:
    MarketOrder(OrderID id, Side side, Qty quantity)
    : id_(id), side_(side), original_qty_(quantity) 
    , quantity_(quantity), timestamp_(std::chrono::steady_clock::now())
    {}

    OrderID id() const { return id_; }
    Side side() const { return side_; }
    Qty original_qty() const { return original_qty_; }
    Qty quantity() const { return quantity_; }
    auto timestamp() const { return timestamp_; }

    void fill(Qty fill_qty) {
      if (quantity_ < fill_qty) {
          throw std::runtime_error("fill quantity exceeds available quanity");
      }
      quantity_ -= fill_qty;
    }

    bool is_marketable() const { return true; }
    std::string type_str() const { return "MARKET"; }

    std::string toString() const {
      std::ostringstream oss;
      oss << "Order ID: " << id_ 
          << "  Side: " << side_
          << "  Original Qty: " << original_qty_
          << "  Quantity: " << quantity_
          << "  Timestamp: " << timestamp_.time_since_epoch().count();

      return oss.str();
    }

  private:
    OrderID id_;
    Side side_;
    Qty original_qty_;
    Qty quantity_;
    std::chrono::time_point<std::chrono::steady_clock> timestamp_;

};

// Once the stop price is crossed the StopOrder will become a market order
class StopOrder {
  public:
    StopOrder(OrderID id, Side side, Qty quantity, double stop_price)
    : id_(id), side_(side), original_qty_(quantity)
    , quantity_(quantity), stop_price_(to_ticks(stop_price))
    , timestamp_(std::chrono::steady_clock::now())
    {}

    StopOrder(OrderID id, Side side, Qty quantity, Price stop_price)
    : id_(id), side_(side), original_qty_(quantity)
    , quantity_(quantity), stop_price_(stop_price)
    , timestamp_(std::chrono::steady_clock::now())
    {}

    OrderID id() const { return id_; }
    Side side() const { return side_; }
    Qty original_qty() const { return original_qty_; }
    Qty quantity() const { return quantity_; }
    Price stop_price() const { return stop_price_; }
    auto timestamp() const { return timestamp_; }

    void fill(Qty fill_qty) {
      if (quantity_ < fill_qty) {
          throw std::runtime_error("fill quantity exceeds available quanity");
      }
      quantity_ -= fill_qty;
    }

    bool is_marketable() const { return false; }
    std::string type_str() const { return "STOP ORDER"; }

    std::string toString() const {
      std::ostringstream oss;
      oss << "Order ID: " << id_ 
          << "  Side: " << side_
          << "  Original Qty: " << original_qty_
          << "  Quantity: " << quantity_
          << "  Stop Price: " << stop_price_ 
          << "  Timestamp: " << timestamp_.time_since_epoch().count();

      return oss.str();
    }

  private:
    OrderID id_;
    Side side_;
    Qty original_qty_;
    Qty quantity_;
    Price stop_price_;
    std::chrono::time_point<std::chrono::steady_clock> timestamp_;
};

// Sits dormant until the market price crosses the trigger price
// Then it becomes a limit order
class StopLimitOrder {
  public:

    // Main constructor the user is expected to use, since it accepts doubles
    StopLimitOrder(OrderID id, Side side, Qty quantity, double price, double stop_price)
    : id_(id), side_(side), original_qty_(quantity)
    , quantity_(quantity), price_(to_ticks(price)), stop_price_(to_ticks(stop_price))
    , timestamp_(std::chrono::steady_clock::now())
    {}

    /*
      Constructor used when converting the resting stop limit order to marketable, this
      way there is no unecessary conversion back to a true price then back to ticks, 
      just accept Price directly
    */ 
    StopLimitOrder(OrderID id, Side side, Qty quantity, Price price, Price stop_price)
    : id_(id), side_(side), original_qty_(quantity)
    , quantity_(quantity), price_(price), stop_price_(stop_price)
    , timestamp_(std::chrono::steady_clock::now())
    {}

    OrderID id() const { return id_; }
    Side side() const { return side_; }
    Qty original_qty() const { return original_qty_; }
    Qty quantity() const { return quantity_; }
    Price price() const { return price_; }
    Price stop_price() const { return stop_price_; }
    auto timestamp() const { return timestamp_; }

    void fill(Qty fill_qty) {
      if (quantity_ < fill_qty) {
          throw std::runtime_error("fill quantity exceeds available quanity");
      }
      quantity_ -= fill_qty;
    }

    std::string toString() const {
      std::ostringstream oss;
      oss << "Order ID: " << id_ 
          << "  Side: " << side_
          << "  Original Qty: " << original_qty_
          << "  Quantity: " << quantity_
          << "  Price: " << price_ 
          << "  Stop Price: " << stop_price_
          << "  Timestamp: " << timestamp_.time_since_epoch().count();

      return oss.str();
    }

    bool is_marketable() const { return false; }
    std::string type_str() const { return "STOP LIMIT ORDER"; }

  private:
    OrderID id_;
    Side side_;
    Qty original_qty_;
    Qty quantity_;
    Price price_;
    Price stop_price_;
    std::chrono::time_point<std::chrono::steady_clock> timestamp_;
};

using OrderVariant = std::variant<LimitOrder, MarketOrder, StopOrder, StopLimitOrder>;

/*
  template<class... Ts>  is a variadic template parameter where Ts can stand for any number of types. 
  This is what allows me to hand overloaded{} lambdas.

  struct overload : Ts...  inherits from every one of those lambda-classes simultaneously

  using Ts::operator()...  Explictly pulls all four base operator()s into overloaded's own overload set
  overloaded is a single object with four legitimate overloads of operator(), one per lambda. Calling it
  triggers ordinary overload resolution to pick the right one based on argument type
*/
template<class... Ts>
struct overloaded : Ts... { using Ts::operator()...; };

// deduction guide
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

// Writing auto as a lambda parameter type turns the lambda's call operator into a template
inline OrderID id(const OrderVariant& order) {
  return std::visit([](const auto& o) { return o.id(); }, order);
}

inline Side side(const OrderVariant& order) {
  return std::visit([](const auto& o) { return o.side(); }, order);
}

inline Qty quantity(const OrderVariant& order) {
  return std::visit([](const auto& o) { return o.quantity(); }, order);
}

inline Qty original_qty(const OrderVariant& order) {
  return std::visit([](const auto& o) { return o.original_qty(); }, order);
}

inline bool is_marketable(const OrderVariant& order) {
  return std::visit([](const auto& o) { return o.is_marketable(); }, order);
}

inline std::string type_str(const OrderVariant& order) {
  return std::visit([](const auto& o) { return o.type_str(); }, order);
}

inline std::string toString(const OrderVariant& order) {
  return std::visit([](const auto& o) { return o.toString(); }, order);
}

inline void fill(OrderVariant& order, Qty fill_qty) {
  return std::visit([fill_qty](auto& o) { o.fill(fill_qty); }, order);
}