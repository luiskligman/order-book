# include "../include/order.h"
#include "../include/order_book.h"

#include <iostream>
#include <iomanip>

int main() {

  // Testing toString() function
  LimitOrder limitorder = LimitOrder(1, Side::BUY, 100, 0.00);

  std::cout << std::fixed 
            << std::setprecision(2) 
            << limitorder.toString() 
            << std::endl;

  OrderBook book;
  
  book.add_order(std::make_shared<LimitOrder>(100, Side::SELL, 250, 104));
  book.add_order(std::make_shared<LimitOrder>(101, Side::SELL, 150, 103));
  book.add_order(std::make_shared<LimitOrder>(102, Side::SELL, 100, 102));
  book.add_order(std::make_shared<LimitOrder>(103, Side::SELL, 100, 101));
  book.add_order(std::make_shared<LimitOrder>(104, Side::SELL, 100, 101));

  book.add_order(std::make_shared<LimitOrder>(105, Side::BUY, 100, 99));
  book.add_order(std::make_shared<LimitOrder>(106, Side::BUY, 100, 99));
  book.add_order(std::make_shared<LimitOrder>(107, Side::BUY, 150, 98));
  book.add_order(std::make_shared<LimitOrder>(108, Side::BUY, 100, 97));
  book.add_order(std::make_shared<LimitOrder>(109, Side::BUY, 300, 96));

  std::cout << book.print();

  book.cancel_order(109);

  std::cout << book.print();

  return 1;
}

