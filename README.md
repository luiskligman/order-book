# Order Book

A C++17 limit order book and matching engine that contains price-time priority matching, fixed-point pricing, stop and stop-limit orders, and a benchmark-driven approach to the hot paths.

## Features

**Order types**: Limit, Market, Stop, Stop-Limit  
**Price-time priority matching**: FIFO fill order within each price level  
**O(1) cancellation**: cancel any resting order, at any queue position, without a scanning  
**Fixed-point pricing**: prices stored as integer ticks, no floating-point rounding drift  
**Catch2 unit tests**: covering matching, cancellation, and stop-triggering behavior  
**Benchmarked hot paths**: used before/after times after data structures were changed in order to quantify code efficiency

## Architecture

| File | Responsibility |
|---|---|
| `include/order.h` | `Order` base class (Rule-of-Five: copy/move deleted, polymorphic, `shared_ptr` only) and the four order types |
| `include/order_book.h` / `src/order_book.cpp` | Resting-order storage: `bids_` / `asks_` (`std::map<Price, std::list<OrderPtr>>`), `order_index_` for O(1) lookup / cancel |
| `include/matching_engine.h` / `src/matching_engine.cpp` | `submit` / `match` / `check_stops` : contains the actual matching and stop-triggering logic |
| `include/trade.h` | `Trade` record produced by filling an order |

**Matching**: `MatchingEngine::submit` routes non-marketable orders (stops) into a dormant book, everything else into `match()`, which walks the resting side's best price level, filling in FIFO order against `queue.front()`, until the incoming order is exhausted or the book is no longer marketable at the incoming order's limit. Any unfilled `LimitOrder` quantity rests in the book on its respective side; an unfilled `MarketOrder` quantity is dropped.

**Cancellation**: `order_index_` maps an `OrderID` to both the `shared_ptr<Order>` and a cached `std::list<OrderPtr>::iterator` pointing directly at its slot. Cancelling is a hash lookup, an `O(log n)` price-level lookup, and a direct `list::erase(iterator)`, no scan of the level's queue.

**Fixed-point pricing**: prices are stored as `int64_t` ticks (`PRICE_SCALE = 10,000`), converted once at order construction by using `to_ticks(double)`. Every downstream comparison and match is integer arithmetic, therefore there's no float rounding error that may compound across repeated price comparisons.

## Prerequisites
* CMake ≥ 3.15
* A C++17 compiler (developed against g++ 11+)

## Build and Run

```bash
mkdir build && cd build
cmake ..
make
```

Run the demo:
```bash
./order_book
```
### Output
**Note**: this output was created using the `populate_book()` function.
```
  ORDER BOOK
       PRICE       QTY      ORD           CUM     DEPTH
  ----------------------------------------------------
    105.0000           30       3            150  
    104.0000           30       3            120  
    103.0000           30       3             90  
    102.0000           30       3             60  
    101.0000           30       3             30  
  ----------------------------------------------------
  spread 2.0000  mid 100.0000
  ----------------------------------------------------
     99.0000           30       3             30  
     98.0000           30       3             60  
     97.0000           30       3             90  
     96.0000           30       3            120  
     95.0000           30       3            150  
```

### Run the tests:
```bash
./unit_tests
```
### Output
```
Randomness seeded to: 3214156994
===============================================================================
All tests passed (17 assertions in 8 test cases)
```

## Benchmarks
```bash
./benchmark_test_1  # cancel_order: mid-queue vs tail-of-queue cancel cost
./benchmark_test_2  # price-level insert cost vs. book depth (10k / 100k / 1M Price Levels)
```

Both benchmark targets are built **without** AddressSanitizer (`unit_tests` is built with it). Mixing sanitizer tools into a nanosecond-scale benchmark proved to bury the real time under ASan's per-allocation bookkeeping.

### `cancel_order`: O(n) deque -> O(1) list

Resting orders per price level were originally a `std::deque` as an initial approach to `price-time priority`; cancelling anything that wasn't at the front or back required an `O(n)` shift. Switching to `std::list`, with the iterator into it cached in `order_index_` allows for `O(1)` lookup and cancel.

Measured on a release build, no address sanitizers, 30-trial average:  
| | Before (`deque`) | After (`list`) | Speedup |
|---|---|---|---|
| Cancel mid-queue | 0.141 ms | 0.00086 ms | **~164x** |
| Cancel tail-of-queue | 0.138 ms | 0.00082 ms | **~169x** |

### Price-level lookup: 

Measured before rewriting; upon reviewing the results, decided not to rewrite this structure as the number of price levels should remain relatively small and the search already seemed fast enough for the scope of this project

| Book depth | Time to insert a new price level |
|---|---|
| 10,000 | ~0.00089 ms |
| 100,000 | ~0.00093 ms | 
| 1,000,000 | ~0.00100 ms |

A 100x increase in book depth moved the measured cost by under 15%. `log(n)` does not grow fast enough over this range for the tree structure to become a bottleneck

## Testing

The Catch2 test (`tests/order_book_tests.cpp`) covers: cancelling on both sides, cancelling a nonexistent id, FIFO fill ordering within a price level, partial fills, full fills removing the resting order, a triggered `StopOrder` converting to and filling as a `MarketOrder`, and triggered buy and sell side `StopLimitOrder`s that partially fill and rest the remainder as a `LimitOrder`.

One of those assertions - checking `best_bid()` after a triggered `StopLimitOrder` caught a real bug where the trigger path was double-converting an already-tick-scaled price through a `double`-taking constructor, quietly resting orders off by a factor of `PRICE_SCALE`.

## What's Next
The largest remaining cost center is `shared_ptr<Order>` + virtual dispatch: `sizeof(Order) == 64` (one cache line - 56 bytes of data plus an 8-byte vtable pointer), and every fill / match walks that vtable. The planned next step is a rewrite to a `std::variant` based closed set of order types stored in a contiguous arena. No vtable, no atomic refcounting, no per-order heap allocation. Deliberately deferred until test coverage was solid enough to catch regressions in a rewrite at this magnitude.

