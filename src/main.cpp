#include "../include/order.h"
#include "../include/order_book.h"
#include "../include/matching_engine.h"

#include <iostream>
#include <iomanip>

int main() {

  // Testing toString() function
  LimitOrder limitorder = LimitOrder{1, Side::BUY, 100, 0.00};

  std::cout << std::fixed 
            << std::setprecision(2) 
            << limitorder.toString() 
            << std::endl;

  OrderBook book;
  MatchingEngine engine(book);
  
  // Testing order book add / cancel order
  engine.submit(std::make_shared<LimitOrder>(100, Side::SELL, 250, 104));
  engine.submit(std::make_shared<LimitOrder>(101, Side::SELL, 150, 103));
  engine.submit(std::make_shared<LimitOrder>(102, Side::SELL, 100, 102));
  engine.submit(std::make_shared<LimitOrder>(103, Side::SELL, 100, 101));
  engine.submit(std::make_shared<LimitOrder>(104, Side::SELL, 100, 101));

  engine.submit(std::make_shared<LimitOrder>(105, Side::BUY, 100, 99));
  engine.submit(std::make_shared<LimitOrder>(106, Side::BUY, 100, 99));
  engine.submit(std::make_shared<LimitOrder>(107, Side::BUY, 150, 98));
  engine.submit(std::make_shared<LimitOrder>(108, Side::BUY, 100, 97));
  engine.submit(std::make_shared<LimitOrder>(109, Side::BUY, 300, 96));

  // engine.submit(std::make_shared<StopOrder>(98, Side::BUY, 100, 103));
  engine.submit(std::make_shared<StopOrder>(99, Side::SELL, 100, 98));

  // std::cout << book.print();

  // engine.submit(std::make_shared<MarketOrder>(110, Side::BUY, 301));
  std::cout << book.print();

  engine.submit(std::make_shared<MarketOrder>(111, Side::SELL, 201));
  std::cout << book.print();





  return 1;
}

