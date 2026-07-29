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

class Benchmark {
  public:

    // time taken to populate the book
    std::chrono::duration<double, std::milli> populate;

    std::string print() const {

      std::ostringstream os;
      
      os << "  Time taken to populate the book: " << populate.count();

      return os.str();
    }


};

Benchmark test() {
  Benchmark timing;  // create an empty benchmark

  OrderBook book;
  MatchingEngine engine(book);

  int64_t uid { 1 };

  const char* RED = "\033[31m";
  const char* GREEN = "\033[32m";
  const char* BLUE = "\033[34m";
  const char* DIM = "\033[90m";
  const char* BOLD = "\033[1m";
  const char* RST = "\033[0m";

  auto start = std::chrono::steady_clock::now();

  // Generate limit orders
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

  auto end = std::chrono::steady_clock::now();
  std::chrono::duration<double, std::milli> elapsed_ms = end - start;
  timing.populate = elapsed_ms;
  // std::cout << elapsed_ms.count() << std::endl;


  return timing;
}

int main() {

  std::cout << "Size of Limit Order: " << sizeof(LimitOrder) << std::endl;
  std::cout << "Size of Market Order: " << sizeof(MarketOrder) << std::endl;
  std::cout << "Size of Stop Order: " << sizeof(StopOrder) << std::endl;
  std::cout << "Size of Stop Limit Order: " << sizeof(StopLimitOrder) << std::endl;

  std::cout << "\n";

  std::vector<Benchmark> tests {};

  tests.push_back(test());
  tests.push_back(test());
  tests.push_back(test());
  tests.push_back(test());
  tests.push_back(test());

  auto populate_sum = 0.0;
  for (const auto& test : tests) {
    populate_sum += test.populate.count();
    std::cout << test.print() << std::endl;
    
  }

  std::cout << "\n";

  std::cout << "AVERAGES" << std::endl;
  std::cout << "  populate avg: " << populate_sum / double(size(tests)) << std::endl;

  return 1;

}