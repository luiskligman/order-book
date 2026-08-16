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

include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)

add_executable(unit_tests
  tests/order_book_tests.cpp
  src/order_book.cpp
  src/matching_engine.cpp
)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)
target_compile_options(unit_tests PRIVATE -fsanitize=address -g)
target_link_options(unit_tests PRIVATE -fsanitize=address)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)
catch_discover_tests(unit_tests)

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

