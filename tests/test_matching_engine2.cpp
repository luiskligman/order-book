#include "matching_engine.h"

#include <iostream>
#include <iomanip>

/*
    This test is used to make sure that sell market stop orders are working 
    properly in a controlled setting.
*/

int main() {
    std::cout << "\n--- TESTING SELL MARKET STOP ORDER ---\n";

    OrderBook book;
    MatchingEngine engine(book);

    std::cout << std::fixed << std::setprecision(2);

    /*
        To test the sell stop market order functionality
        we want to load the book to be more heavy on the bid side, since
        bids and sell stops are both below the current trading price
    */

    // 1 Limit Buy Order - 100 resting bids @ 99.00
    engine.submit(std::make_shared<LimitOrder>(1, Side::BUY, 100, 99.00));
    // 1 Limit Buy Order - 100 resting bids @ 98.00
    engine.submit(std::make_shared<LimitOrder>(2, Side::BUY, 100, 98.00));
    // 1 Limit Buy Order - 100 resting bids @ 97.00
    engine.submit(std::make_shared<LimitOrder>(3, Side::BUY, 100, 97.00));
    // 1 Limit Buy Order - 100 resting bids @ 96.00
    engine.submit(std::make_shared<LimitOrder>(4, Side::BUY, 100, 96.00));

    // make a sell order so that the book will show a spread
    engine.submit(std::make_shared<LimitOrder>(5, Side::SELL, 100, 102.00));

    // 1 StopOrder - 50 resting asks @ 97.50
    engine.submit(std::make_shared<StopOrder>(6, Side::SELL, 50, 97.50));

    std::cout << "--- Initial Book ---";
    engine.print();

    std::cout << "\n--- SELL MARKET ---\n";
    auto trades = engine.submit(std::make_shared<MarketOrder>(7, Side::SELL, 201));
    for (const auto& t : trades)
        std::cout << "  TRADE: maker=" << t.maker_id << " taker=" << t.taker_id
            << " price=$" << t.price << " qty=" << t.quantity << "\n";

    engine.print();

    return 0;
}