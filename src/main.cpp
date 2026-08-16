#include "../include/order.h"
#include "../include/order_book.h"
#include "../include/matching_engine.h"

#include <iostream>
#include <iomanip>

constexpr bool print_orders { false };  // optionally print information about each order
constexpr int64_t num_orders { 3 };  // number of orders at each price level
constexpr Qty qty_per_order { 10 };
constexpr int64_t price_levels { 5 };  // number of price levels above and below starting price
constexpr Price starting_price { 100 };
int64_t uid { 1 };

void populate_book(MatchingEngine& engine) {
  Price ask_price { starting_price };
  Price bid_price { starting_price };

  for (int level = 0; level < price_levels; ++level) {

    ask_price += 1;
    bid_price-= 1;

    for (int i = 0; i < num_orders; ++i) {

      engine.submit(std::make_shared<LimitOrder>(uid, Side::SELL, qty_per_order, ask_price));
      engine.submit(std::make_shared<LimitOrder>(uid + 1, Side::BUY, qty_per_order, bid_price));
      uid += 2;

    }
  }
}

int main() {

  // // Testing toString() function
  // LimitOrder limitorder = LimitOrder{1, Side::BUY, 100, 0.00};

  // std::cout << std::fixed 
  //           << std::setprecision(2) 
  //           << limitorder.toString() 
  //           << std::endl;

  OrderBook book;
  MatchingEngine engine(book);


  // engine.submit(std::make_shared<LimitOrder>(999, Side::BUY, 1, 99.9999));

  // thin resting liquidity: only 2 available at 101, stop order wants 10
  engine.submit(std::make_shared<LimitOrder>(1, Side::SELL, 2, 101.00));

  // dormant buy stop-limit: triggers at 100, limits at 101
  engine.submit(std::make_shared<StopLimitOrder>(2, Side::BUY, 10, 101.00, 100.00));

  // trip the stop
  engine.submit(std::make_shared<LimitOrder>(3, Side::SELL, 1, 100.00));
  auto trades = engine.submit(std::make_shared<LimitOrder>(4, Side::BUY, 1, 100.00));

  // populate_book(engine);
  std::cout << book.print();

  
  return 0;
  
}

