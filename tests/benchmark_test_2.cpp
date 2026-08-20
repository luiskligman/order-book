#include "order.h"
#include "order_book.h"
#include "matching_engine.h"

#include <chrono>
//#include <sstream>
#include <iostream>
#include <vector>

/*
  The goal of 'benchmark_test_2.cpp' is to provide an
  intial benchmark insight for the efficiency of the traversal
  through price levels and the creation of new price levels into 
  a std::map datastructure. std::map is a read and black tree O(log n) 
  average insertion and lookup. Before changing this data structure, it is 
  ideal to benchmark to quantify the new data structures effectiveness. 
*/

struct Benchmark {
  std::chrono::duration<double, std::milli> insert_price_level;
  std::chrono::duration<double, std::milli> find_price_level;
};

/*
  Populate only one side of the book, since we are testing the books internal data structure
  we only need to worry about one side, populating both sides would be redundant and std::greaterthan
  for our comparator should not change anything.
*/
void populate_book(MatchingEngine& engine, uint64_t num_levels) {
  uint64_t uid { 0 };
  double starting_price { 1.00 };

  while (uid < num_levels) {
    engine.submit(std::make_shared<LimitOrder>(uid, Side::SELL, 1, starting_price));
    ++uid;
    ++starting_price;
  }

  return;
}

std::chrono::duration<double, std::milli> insert_price_level(uint64_t num_levels) {
  OrderBook book;
  MatchingEngine engine(book);

  populate_book(engine, num_levels);

  uint16_t trials { 30 };
  std::chrono::duration<double, std::milli> time { 0 };
  int i { 0 };

  while (i < trials) {
    Price p = num_levels + i + 1;
    auto start = std::chrono::steady_clock::now();
    engine.submit(std::make_shared<LimitOrder>(p, Side::SELL, 1, double(p)));
    auto end = std::chrono::steady_clock::now();
    time += end - start;
    ++i;
  }

  return time / trials;
  
}

int main() {
  
  std::cout << insert_price_level(10'000).count() << std::endl;
  std::cout << insert_price_level(100'000).count() << std::endl;
  std::cout << insert_price_level(1'000'000).count() << std::endl;
  


  return 0;
}

