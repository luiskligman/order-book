
#include "matching_engine.h"

#include <iostream>
#include <iomanip>

int main() {
    OrderBook book;
    MatchingEngine engine(book);

    std::cout << std::fixed << std::setprecision(2);

    // Resting orders, none of these cross so no trades execute yet
    engine.submit(std::make_shared<LimitOrder>(1, Side::BUY, 100, 150.00));
    engine.submit(std::make_shared<LimitOrder>(2, Side::BUY, 50, 149.50));
    engine.submit(std::make_shared<LimitOrder>(3, Side::SELL, 75, 151.00));
    engine.submit(std::make_shared<LimitOrder>(4, Side::SELL, 100, 152.00));

    std::cout << "--- Initial Book ---";
    book.print(engine.get_remaining());

    // Incoming SELL limit crosses the best bid - partial fill on order 1
    std::cout << "--- SELL 80 @ 150.00 ---\n";
    auto trades = engine.submit(std::make_shared<LimitOrder>(5, Side::SELL, 80, 150));
    for (const auto& t : trades)
        std::cout << "  TRADE: maker=" << t.maker_id << " taker=" << t.taker_id
            << " price=$" << t.price << " qty=" << t.quantity << "\n";
    
    book.print(engine.get_remaining());

    // Market order sweeps two ask levels
    std::cout << "--- BUY MARKET 120 ---\n";
    trades = engine.submit(std::make_shared<MarketOrder>(6, Side::BUY, 120));
    for (const auto& t : trades)
        std::cout << "  TRADE: maker=" << t.maker_id << " taker=" << t.taker_id
            << " price=$" << t.price << " qty=" << t.quantity << "\n";
    
    book.print(engine.get_remaining());

    return 0;
}