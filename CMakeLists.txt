cmake_minimum_required(VERSION 3.15)
project(OrderBook)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(order_book
  src/main.cpp
  src/order_book.cpp
  src/matching_engine.cpp

)

enable_testing()

set(TESTS
  benchmark_test_1

)

foreach(TEST ${TESTS})

  # Compiles each test as its own executable
  add_executable(${TEST}
    tests/${TEST}.cpp
    src/order_book.cpp
    src/matching_engine.cpp
  )

  # Register the binary with CTEST so it knows how to run
  add_test(NAME ${TEST} COMMAND ${TEST})

  target_compile_options(${TEST} PRIVATE -fsanitize=address -g)
  target_link_options(${TEST} PRIVATE -fsanitize=address)

endforeach()

