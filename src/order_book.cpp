# include <../include/order_book.h>

#include <iostream>
#include <iomanip>

void OrderBook::add_order(OrderPtr order) {

  // Route to the correct side
  if (order->side() == Side::BUY) {
    bids_[order->price()].push_back(order);
  } else {
    asks_[order->price()].push_back(order);
  }
  order_index_[order->id()] = order;
}

bool OrderBook::cancel_order(OrderID id) {
  auto index_entry = order_index_.find(id);

  if (index_entry == order_index_.end()) return false;

  // index_entry->first is the OrderID, index_entry->second is a 
  // pointer to the Order with OrderID
  OrderPtr order = index_entry->second;

  // Remove from the correct price level on the correct side
  if (order->side() == Side::BUY) {
    auto price_level = bids_.find(order->price());
    if (price_level != bids_.end()) {
      auto& queue = price_level->second;  // store bids_ deque as queue
      // use order_iter to iterate through orders in the deque
      for (auto order_iter = queue.begin(); order_iter != queue.end(); ++order_iter) {
        if ((*order_iter)->id() == id) { 
          queue.erase(order_iter); 
          break;
        }
      }
      if (queue.empty()) {
          bids_.erase(price_level);
        }
    } 
  } else {
      auto price_level = asks_.find(order->price());
      if (price_level != asks_.end()) {
        auto& queue = price_level->second;  // store asks_ deque as queue
        // use order_iter to iterate through orders in the deque
        for (auto order_iter = queue.begin(); order_iter != queue.end(); ++order_iter) {
          if ((*order_iter)->id() == id) {
            queue.erase(order_iter);
            break;
          }
        }
        if (queue.empty()) {
          asks_.erase(price_level);
      }
    }
  }

  order_index_.erase(index_entry);
  return true;

}

bool OrderBook::remove_if_filled(OrderPtr order) {
  // Order with quantity should not be removed
  if (order->quantity() != 0) {
    return false;
  }

  return cancel_order(order->id());
}


std::optional<Price> OrderBook::best_bid() const {
  if (bids_.empty()) return std::nullopt;
  return bids_.begin()->first;
}

std::optional<Price> OrderBook::best_ask() const {
  if (asks_.empty()) return std::nullopt;
  return asks_.begin()->first;
}

std::string OrderBook::print() const {

  const char* RED = "\033[31m";
  const char* GREEN = "\033[32m";
  const char* DIM = "\033[90m";
  const char* BOLD = "\033[1m";
  const char* RST = "\033[0m";

  int bar_width = 20;

  struct level { 
    Price price;
    Qty qty;
    std::size_t orders;
    Qty cumulative;
  };

  // return data for each price level of a side
  auto collect = [](const auto& side) {
    std::vector<level> levels;
    Qty running = 0;
    for (const auto& [price, orders] : side) {
      Qty q = 0;
      for (const auto& o : orders) q += o->quantity();
      running += q;
      levels.push_back(level{price, q, orders.size(), running});
    }
    return levels;
  };

  auto asks = collect(asks_);
  auto bids = collect(bids_);


  std::ostringstream os;
  os << std::fixed << std::setprecision(2);

  auto bar = [&](Qty q) {
    int count = static_cast<int>((q + 49) / 50);
    return std::string(count, '#');
  };

  auto row = [&](const level& l, const char* color) {
    os << "  " << color
       << std::setw(10) << l.price
       << std::setw(8) << l.qty
       << std::setw(5) << l.orders
       << DIM << std::setw(9) << l.cumulative << RST 
       << "  " << color << bar(l.qty) << RST << "\n";
  };

  os << "\n" << BOLD << "  ORDER BOOK" << RST << "\n";
  os << DIM << "       PRICE     QTY  ORD      CUM  DEPTH\n" << RST;
  os << DIM << "  " << std::string(52, '-') << RST << "\n";
 
  // Asks - worst price at top, best just above the spread line
  for (auto order = asks.crbegin(); order != asks.crend(); ++order) {
    row(*order, RED);
  };

  std::optional<Price> bid = best_bid();
  std::optional<Price> ask = best_ask();

  os << DIM << "  " << std::string(52, '-') << RST << "\n";

  if (!bid || !ask) {
    os << DIM << "  (one side empty)\n" << RST;
  } else if (*bid >= *ask) {
    os << BOLD << RED << "  *** CROSSED: bid " << *bid 
       << " >= ask " << *ask << " ***" << RST << "\n";
  } else {
    os << "  spread " << (*ask - *bid) << DIM
       << "  mid " << ((*bid + *ask) / 2 ) << RST << 
       "\n";
  }

  os << DIM << "  " << std::string(52, '-') << RST << "\n";

  for (const auto& l : bids) {
    row(l, GREEN);
  }

  os << "\n";

  return os.str();
  


}