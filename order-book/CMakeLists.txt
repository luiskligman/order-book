cmake_minimum_required(VERSION 3.15)
project(OrderBook)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

# Locate the platform threading library
find_package(Threads REQUIRED)

add_executable(order_book
    src/main.cpp
    src/order_book.cpp
    src/matching_engine.cpp
    src/order_queue.cpp
)

# Link the threading library into my executables
target_link_libraries(order_book PRIVATE Threads::Threads)

# activates CTest, which is CMake's built-in test runner
enable_testing()

set(TESTS
    test_matching_engine0
    test_matching_engine1
)

foreach(TEST ${TESTS})
    # Compiles the test as its own binary
    add_executable(${TEST}
        tests/${TEST}.cpp
        src/order_book.cpp
        src/matching_engine.cpp
    )
    target_link_libraries(${TEST} PRIVATE Threads::Threads)

    # Registers the binary with CTest so ctest knows to run it
    add_test(NAME ${TEST} COMMAND ${TEST})
endforeach()