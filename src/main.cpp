
#include "order_book.h"

#include <iostream>

int main() {
    OrderBook book;

    // Post limit orders on the buy side
    book.add_order(std::make_shared<LimitOrder>(1, Side::BUY, 100, 149.50));
    book.add_order(std::make_shared<LimitOrder>(2, Side::BUY, 200, 150.00));
    book.add_order(std::make_shared<LimitOrder>(3, Side::BUY, 50, 150.00));

    // Post limit orders on the sell side
    book.add_order(std::make_shared<LimitOrder>(4, Side::SELL, 150, 151.00));
    book.add_order(std::make_shared<LimitOrder>(5, Side::SELL, 75, 152.50));

    // Print intial state
    std::cout << "--- Initial book ---";
    book.print();

    // Cancel order 2 and reprint
    book.cancel_order(2);
    std::cout << "--- After cancelling order 2 ---";
    book.print();

    // Check best bid and ask explicitly
    auto bb = book.best_bid();
    auto ba = book.best_ask();
    std::cout << "Best bid: " << (bb ? std::to_string(*bb) : "none") << "\n";
    std::cout << "Best ask: " << (ba ? std::to_string(*ba) : "none") << "\n";

    return 0;
}