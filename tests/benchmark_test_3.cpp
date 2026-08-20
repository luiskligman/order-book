#include "order.h"
#include "order_book.h"
#include "matching_engine.h"

#include <chrono>
#include <sstream>
#include <iostream>
#include <vector>

/*
  This text serves as a baseline for the current shared_ptr<Order> + virtual dispatch design, to be
  re-measured after the planned std::variant / arena rewrite

  Test isolates two things:
    1. construction: cost of make_shared<LimitOrder> alone (heap allocation + vtable ptr setup + shared_ptr control block),
       no book interaction.
    2. match_throughput: cost of fully crossing num_orders resting orders with one incoming order. 
       Exercises the per-fill hot path: the virtual type_str() call inside price_acceptable(), a shared_ptr
       copy of 'maker', and fill()/quantity() calls, once per fill.
*/

/*
  Initial Averages: (10,000 orders)
                    Construction: 1.1951 ms     Match_Sum: 3.90039 ms
  Averages After Changing:
                    Construction:               Match_Sum: 
*/

constexpr int64_t num_orders { 10'000 };
constexpr Qty qty_per_order { 1 };
constexpr double price { 100.00 };

struct Benchmark {
  std::chrono::duration<double, std::milli> construction;
  std::chrono::duration<double, std::milli> match_throughput;
};

std::chrono::duration<double, std::milli> time_construction() {
  std::vector<OrderPtr> orders;
  orders.reserve(num_orders);  // reserve num_orders space in vector to avoid allocation noise during measurement

  auto start = std::chrono::steady_clock::now();

  for (int64_t i = 0; i < num_orders; ++i) {
    orders.push_back(std::make_shared<LimitOrder>(i, Side::SELL, qty_per_order, price));
  }

  auto end = std::chrono::steady_clock::now();

  return end - start;
}

std::chrono::duration<double, std::milli> time_match_throughput() {
  OrderBook book;
  MatchingEngine engine(book);

  for (int64_t i = 0; i < num_orders; ++i) {
    engine.submit(std::make_shared<LimitOrder>(i, Side::SELL, qty_per_order, price));
  }

  // one incoming order sized to fully cross every resting order in the book
  // one at a time, maximizing the number of hot-path fill iterations
  auto incoming = std::make_shared<LimitOrder>(num_orders, Side::BUY, qty_per_order * num_orders, price);

  auto start = std::chrono::steady_clock::now();
  engine.submit(incoming);
  auto end = std::chrono::steady_clock::now();

  return end - start;
}

int main() {
  std::vector<Benchmark> tests {};

  for (int i = 0; i < 30; ++i) {
    Benchmark b {};
    b.construction = time_construction();
    b.match_throughput = time_match_throughput();
    tests.push_back(b);
  }

  double construction_sum { 0.0 };
  double match_sum { 0.0 };
  for (const auto& t : tests) {
    construction_sum += t.construction.count();
    match_sum += t.match_throughput.count();
  }

  std::cout << "AVERAGES over " << tests.size() << " trials\n";
  std::cout << "  construction (" << num_orders << " orders): " << construction_sum / tests.size() << " ms\n";
  std::cout << "  match_sum (" << num_orders << " fills): " << match_sum / tests.size() << " ms" << std::endl;

  return 0;
}
