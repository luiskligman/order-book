
#include "matching_engine.h"
#include "order_queue.h"

#include <iostream>
#include <iomanip>
#include <thread>

int main() {
    OrderBook book;
    MatchingEngine engine(book);
    OrderQueue queue;

    std::cout << std::fixed << std::setprecision(2);

    // Matching thread: blocks on queue.pop(), submits each order to the engine
    std::thread matcher([&]() {
        while (auto order = queue.pop()) {
            auto trades = engine.submit(*order);
            for (const auto& t : trades)
                std::cout << "  TRADE: maker=" << t.maker_id
                << " taker=" << t.taker_id
                << " price=$" << t.price
                << " qty=" << t.quantity << "\n";
        }
    });

    // Ingestion: main thread acts as the feed
    queue.push(std::make_shared<LimitOrder>(1, Side::BUY, 100, 150.00));
    queue.push(std::make_shared<LimitOrder>(2, Side::BUY, 50 , 149.50));
    queue.push(std::make_shared<LimitOrder>(3, Side::SELL, 75, 151.00));
    queue.push(std::make_shared<LimitOrder>(4, Side::SELL, 100, 152.00));
    queue.push(std::make_shared<LimitOrder>(5, Side::SELL, 80, 150.00));
    queue.push(std::make_shared<MarketOrder>(6, Side::BUY, 120));

    queue.stop();  // signal matcher to drain and exit
    matcher.join(); // wait for matcher to finish

    std::cout << "\n --- Final Book ---\n";
    book.print();


    return 0;
}