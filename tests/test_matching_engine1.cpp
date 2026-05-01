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

    // Create many resting BIDs

    // 3 LimitOrders - 100 resting bids @ 150.00
    engine.submit(std::make_shared<LimitOrder>(1, Side::BUY, 25, 150.00));
    engine.submit(std::make_shared<LimitOrder>(2, Side::BUY, 50, 150.00));
    engine.submit(std::make_shared<LimitOrder>(3, Side::BUY, 25, 150.00));
    // 1 LimitOrder - 50 resting bids @ 149.50
    engine.submit(std::make_shared<LimitOrder>(4, Side::BUY, 50, 149.50));
    // 2 LimitOrders - 100 resting bids @ 149.00
    engine.submit(std::make_shared<LimitOrder>(5, Side::BUY, 75, 149.00));
    engine.submit(std::make_shared<LimitOrder>(6, Side::BUY, 25, 149.00));
    // 1 LimitOrder - 100 resting bids @ 148.50
    engine.submit(std::make_shared<LimitOrder>(7, Side::BUY, 100, 148.50));

    // 1 StopOrder - 50 resting bids @ 149.50
    engine.submit(std::make_shared<StopOrder>(8, Side::BUY, 50, 149.50));

    std::cout << "--- Initial Book ---";
    engine.print();
    // book.print();  // should be identical at this step

    // std::cout << "\n--- SELL MARKET 101 ---\n";
    // auto trades = engine.submit(std::make_shared<MarketOrder>(9, Side::SELL, 101));
    // for (const auto& t : trades)
    //      std::cout << "  TRADE: maker=" << t.maker_id << " taker=" << t.taker_id
    //          << " price=$" << t.price << " qty=" << t.quantity << "\n";

    engine.print();


    // Incoming SELL limit crosses the best bid - partial fill on order 1
    // std::cout << "--- SELL 80 @ 150.00 ---\n";
    // auto trades = engine.submit(std::make_shared<LimitOrder>(5, Side::SELL, 80, 150));
    // for (const auto& t : trades)
    //     std::cout << "  TRADE: maker=" << t.maker_id << " taker=" << t.taker_id
    //         << " price=$" << t.price << " qty=" << t.quantity << "\n";
    
    // book.print(engine.get_remaining());

    // // Market order sweeps two ask levels
    // std::cout << "--- BUY MARKET 120 ---\n";
    // trades = engine.submit(std::make_shared<MarketOrder>(6, Side::BUY, 120));
    // for (const auto& t : trades)
    //     std::cout << "  TRADE: maker=" << t.maker_id << " taker=" << t.taker_id
    //         << " price=$" << t.price << " qty=" << t.quantity << "\n";
    
    // book.print(engine.get_remaining());

    return 0;
}