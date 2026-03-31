cmake_minimum_required(VERSION 3.15)
project(OrderBook)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

add_executable(order_book
    src/main.cpp
    src/order_book.cpp
)