#include "../include/order_book.h"
#include "../include/matching_engine.h"

#include <chrono>
#include <sstream>
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