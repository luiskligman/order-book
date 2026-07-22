# include "../include/order.h"

#include <iostream>
#include <iomanip>

int main() {
  LimitOrder limitorder = LimitOrder(1, Side::BUY, 100, 0.00);

  std::cout << std::fixed 
            << std::setprecision(2) 
            << limitorder.toString() 
            << std::endl;
  
  return 1;
}