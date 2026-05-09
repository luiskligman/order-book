#pragma once

#include "order.h"
#include <chrono>
#include <iostream>
#include <iomanip>

// Emitted by the matching engine each time two orders cross
// maker - the resting order (was already in the book)
// taker - the incoming order (arrived and triggered the match)
struct Trade {
    OrderID maker_id;
    OrderID taker_id;
    double price;
    int quantity;
    std::chrono::time_point<std::chrono::steady_clock> timestamp;

    void const print() const {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "  $" << price << "  qty=" << quantity
                  << "  maker_id=" << maker_id 
                  << "  taker_id= << taker_id\n";  
    }
};