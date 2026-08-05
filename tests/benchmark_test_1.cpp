#include "../include/order_book.h"
#include "../include/matching_engine.h"

#include <chrono>
#include <sstream>
#include <iostream>
#include <vector>

constexpr bool print_orders { false };  // optionally print information about each order
constexpr int64_t num_orders { 10000 };  // number of orders at each price level
constexpr Qty qty_per_order { 10 };
constexpr int64_t price_levels { 5 };  // number of price levels above and below starting price
constexpr Price starting_price { 100 };

/*
  The goal of 'benchmark_test_1.cpp is to provide an intial benchmark
  insight for the efficiency of the program at this stage. This is done
  to enable verification proving that changes are increasing the runtime
  efficiency of this application.
*/

/*
  Initial Averages: Cancel Middle: .364696 +- .005    Cancel Last: .260171 +- .005
                                  Longer due to shifting of subsequent elements
  
*/

struct Benchmark {
    std::chrono::duration<double, std::milli> cancel_middle_order;
    std::chrono::duration<double, std::milli> cancel_last_order;
};


std::chrono::duration<double, std::milli> populate_book(MatchingEngine& engine, int64_t& uid) {
  Price ask_price { starting_price };
  Price bid_price { starting_price };

  auto start = std::chrono::steady_clock::now();

  for (int level = 0; level < price_levels; ++level) {

    ask_price += 1;
    bid_price-= 1;

    for (int i = 0; i < num_orders; ++i) {

      engine.submit(std::make_shared<LimitOrder>(uid, Side::SELL, qty_per_order, ask_price));
      engine.submit(std::make_shared<LimitOrder>(uid + 1, Side::BUY, qty_per_order, bid_price));
      uid += 2;

    }
  }

  auto end = std::chrono::steady_clock::now();

  return end - start;
}

std::chrono::duration<double, std::milli> time_cancel_middle() {
  OrderBook book;
  MatchingEngine engine(book);
  int64_t uid { 1 } ;
  populate_book(engine, uid);

  auto start = std::chrono::steady_clock::now();
  book.cancel_order(num_orders);
  auto end = std::chrono::steady_clock::now();
  return end - start;
}

std::chrono::duration<double, std::milli> time_cancel_last() {
  OrderBook book;
  MatchingEngine engine(book);
  int64_t uid { 1 } ;
  populate_book(engine, uid);

  auto start = std::chrono::steady_clock::now();
  book.cancel_order(num_orders * 2);
  auto end = std::chrono::steady_clock::now();
  return end - start;
}


int main() {

  std::cout << "Size of Limit Order: " << sizeof(LimitOrder) << std::endl;
  std::cout << "Size of Market Order: " << sizeof(MarketOrder) << std::endl;
  std::cout << "Size of Stop Order: " << sizeof(StopOrder) << std::endl;
  std::cout << "Size of Stop Limit Order: " << sizeof(StopLimitOrder) << std::endl;

  std::cout << "\n";

  std::vector<Benchmark> tests {};

  for (int i = 0; i < 30; ++i) {
    Benchmark b {};
    b.cancel_middle_order = time_cancel_middle();
    b.cancel_last_order = time_cancel_last();
    tests.push_back(b);
  }

  auto cancel_middle_sum { 0.0 };
  auto cancel_last_sum { 0.0 };
  for (const auto& test : tests) {
    cancel_middle_sum += test.cancel_middle_order.count();
    cancel_last_sum += test.cancel_last_order.count();
  }

  std::cout << "\n";

  std::cout << "AVERAGES" << std::endl;
  std::cout << "  cancel middle avg: " << cancel_middle_sum / double(size(tests)) << std::endl;
  std::cout << "  cancel last avg: " << cancel_last_sum / double(size(tests)) << std::endl;

  return 1;

}