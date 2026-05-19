#include "matching_engine.h"

#include <iostream>
#include <iomanip>

/*
    In this test, I am skewing there to be many orders on one side of the book, testing stop orders
    to see if they will properly activate if one market order sweeps through multiple price levels.

    My current analysis of the code leads me to believe that this is a critical error, but this test will
    prove if I am right or wrong. If I am right, I can use this to check my fixes.
*/

int main() {
    OrderBook book;
    MatchingEngine engine(book);

    std::cout << std::fixed << std::setprecision(2);

    // 1 LimitOrder - 100 resting asks @ 101.00
    engine.submit(std::make_shared<LimitOrder>(1, Side::SELL, 100, 101.00));
    // 1 LimitOrder - 100 resting asks @ 102
    engine.submit(std::make_shared<LimitOrder>(2, Side::SELL, 100, 102.00));
    // 1 LimitOrder - 100 resting asks @ 103.00
    engine.submit(std::make_shared<LimitOrder>(3, Side::SELL, 100, 103.00));

    // 1 LimitOrder - 100 resting bids @ 99.00
    engine.submit(std::make_shared<LimitOrder>(4, Side::BUY, 100 , 99.00));

    // 1 StopOrder - 50 resting bids @ 101.50
    engine.submit(std::make_shared<StopOrder>(8, Side::BUY, 50, 101.50));

    std::cout << "--- Initial Book ---";
    engine.print();
    // book.print();  // should be identical at this step

    std::cout << "\n--- SELL MARKET ---\n";
    auto trades = engine.submit(std::make_shared<MarketOrder>(9, Side::BUY, 150));
    for (const auto& t : trades)
         std::cout << "  TRADE: maker=" << t.maker_id << " taker=" << t.taker_id
             << " price=$" << t.price << " qty=" << t.quantity << "\n";

    engine.print();

    return 0;
}