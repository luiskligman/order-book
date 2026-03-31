# Order Book Simulator

A Limit order book simulator written in C++17, modeled after the matching systems used at real exchanges such as NYSE and Nasdaq.

## What is a Limit Order Book?

Every exchange maintains an order book, being a live record of all outstanding buy and sell orders for a security. When a new order arrives, the exchange checks if it can be matched against an exisiting order on the opposite side. If yes, a trade executes. If no, the order rests in the order book until it can be filled or is cancelled.

The two sides of the book:
- **Bids** - buy orders, sorted highest price first (best bid at the top)
- **Asks** - sell orders, sorted lowest price first (best ask at the top)
- **Spread** - the gap between the best bid and best ask

Matching follows **price-time priority**: the best price fills first. At the same price, the earliest order fills first (FIFO).

## Build

Requirements: CMake 3.15+, C++17 compiler (`g++` or clang++)

```bash
cd build
cmake ..
make
./order_book
```

## Project Structure
```bash
order-book/
├── include/
│   ├── order.h         # Order base class and concrete order types
│   └── order_book.h    # OrderBook class declaration
├── src/ 
│   ├── order_book.cpp  # OrderBook implementation
│   └── main.cpp        # Manual test driver
├── tests/              # Unit/integration tests
├── data/               # Sample order data
├── docs/               # Design notes
├── CMakeLists.txt
└── README.md
```

## Architecture

### Order Hierarchy

`Order<PriceT, QtyT>` is an abstract base class templated on price and quantity types. 

Three types inherit from it:

| Type | Behavior |
|:-:|:-:|
| LimitOrder | Matches only at the specified price or better |
| MarketOrder | Matches at whatever price is available |
| StopOrder | Dormant until trigger price is crossed, then becomes a market order |

All order fields `(id, side, price, quantity, timestamp)` are `const` once created, an order is immutable. Changing price or quantity will require a cancel and subsequent repost, which is how real exchanges work.

### OrderBook
```cpp
bids_: std::map<double, std::deque<OrderPtr>, std::greater<double>>

asks_: std::map<double, std::deque<OrderPtr>>
```

Each side is a sorted map of price levels. Each price level holds a deque of order in arrival order (FIFO). `std::greater<double>` on the bid side keeps the highest price at the front, so `bids_.begin()` is always best bid, and `asks_.begin()` is always the best ask.

`OrderPtr` is `shared_ptr<const Order<double, int>>`:
- `shared_ptr` - shared ownership between the book and the order index

- `const` - neither the book nor the engine can mutate a resting order 

A flat `order_index_` map (OrderID -> OrderPtr) allows fast cancel lookup without scanning every price level

### Key Operations

| Operation | Complexity | Notes |
|:-:|:-:|:-:|
| add_order | O(log n) | Map insert by price |
| cancel_order | O(log n) | Index lookup + map erase |
| best_bid / best_ask | O(1) | Iterator to map front |

### C++ Concepts Implemented

| Concept | Where used |
|:-:|:-:|
| Templates | `Order<PriceT, QtyT>` - write once, type-paramterized |
| Virtual dispatch | `is_marketable()`, `type_str()` - each order type answers for itself |
| Abstract base class | `Order` cannot be instantiaed directly |
| `const` correctness | Immutable order fields; `const` getters; `const OrderPtr` |
| `shared_ptr` | Shared ownership of orders across book sides and index |
| `std::map` | Price-sorted sides; O(log n) insert and lookup |
| `std::deque` | Per_level FIFO queue for price-time priority |
| `std::optional` | `best_bid()`/`best_ask()` return nothing when book is empty |

