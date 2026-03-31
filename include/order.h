#pragma once

#include <chrono>
#include <string>


// OrderId is a type alias for unsigned int
using OrderID = unsigned int;

// Side represents which direction an order is: buying or selling
enum class Side { BUY, SELL };


// Order<PriceT, QtyT> is the abstract base for all order types
template<typename PriceT, typename QtyT>
class Order {
 public:
    
    // Constructor takes all fields. Every field is const as orders are immutable once created
    Order(OrderID id, Side side, QtyT quantity, PriceT price) 
        : id_(id)
        , side_(side)
        , quantity_(quantity)
        , price_(price)
        , timestamp_(std::chrono::steady_clock::now())
        {}

    // Virtual destructor is required any time you delete through a base class pointer
    // Without, deleting a LimitOrder* through an Order* would be undefined behavior
    virtual ~Order() = default;


    // Const getter methods
    OrderID id() const { return id_; }
    Side side() const { return side_; }
    QtyT quantity() const { return quantity_; }
    PriceT price() const { return price_; }
    std::chrono::time_point<std::chrono::steady_clock> timestamp() const { return timestamp_; }


    // Pure virtual methods: every concrete order type must answer these
    // is_marketable(): can this order match immediately against the book
    // type_str(): human-readable name for printing
    virtual bool is_marketable() const = 0;
    virtual std::string type_str() const = 0;

 private: 
    // All fields are const. Once an order is created, nothing changes.
    const OrderID id_;
    const Side side_;
    const QtyT quantity_;
    const PriceT price_;
    const std::chrono::time_point<std::chrono::steady_clock> timestamp_;

};


// LimitOrder
// Matches only at the limit price or better
// is_marketable() returns true because we allow it to attempt a match
// the engine will enforce the price constraint
class LimitOrder : public Order<double, int> {
 public: 
    LimitOrder(OrderID id, Side side, int quantity, double price)
        : Order<double, int>(id, side, quantity, price)
    {}
    
    bool is_marketable() const override { return true; }
    std::string type_str() const override { return "LIMIT"; }
    
};


// MarketOrder
// Matches at whatever price is available. No price constraint
// Price is stored as 0.0 since it is irrelevant for market orders
class MarketOrder : public Order<double, int> {
 public:
    MarketOrder(OrderID id, Side side, int quantity)
        : Order<double, int>(id, side, quantity, 0.0)
    {}

    bool is_marketable() const override { return true; }
    std::string type_str() const override { return "MARKET"; }

};


// StopOrder
// Sits dormant until the market price crosses the trigger price, then
// it becomes a market order
class StopOrder : public Order<double, int> {
 public:
    StopOrder(OrderID id, Side side, int quantity, double stop_price)
        : Order<double, int>(id, side, quantity, stop_price)
    {}

    double stop_price() const { return price(); }

    // Not marketable until triggered, dormant by default.
    bool is_marketable() const override { return false; }
    std::string type_str() const override { return "STOP"; }

};


