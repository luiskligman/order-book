#include "matching_engine.h"

#include <iostream>
#include <iomanip>

/*
    This test is used to ensure that buy market stop orders are working properly. The
    code is currently set to show resting stop orders, something a normal exchange would not do.
*/

int main() {
    std::cout << "\n--- TESTING BUY MARKET STOP ORDER ---\n";

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
    engine.submit(std::make_shared<StopOrder>(5, Side::BUY, 50, 101.50));

    std::cout << "--- Initial Book ---";
    engine.print();
    // book.print();  // should be identical at this step

    std::cout << "\n--- BUY MARKET ---\n";
    auto trades = engine.submit(std::make_shared<MarketOrder>(6, Side::BUY, 150));
    for (const auto& t : trades)
        std::cout << "  TRADE: maker=" << t.maker_id << " taker=" << t.taker_id
            << " price=$" << t.price << " qty=" << t.quantity << "\n";

    engine.print();

    return 0;
}