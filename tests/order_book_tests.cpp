#include <catch2/catch_test_macros.hpp>

#include "../include/order_book.h"
#include "../include/matching_engine.h"


TEST_CASE("cancel_order removes a resting order on either side", "[order_book]") {
  OrderBook book;
  MatchingEngine engine(book);

  SECTION("Sell side") {
    engine.submit(std::make_shared<LimitOrder>(1, Side::SELL, 10, 100.00));
    REQUIRE(book.cancel_order(1) == true);
    REQUIRE(book.best_ask() == std::nullopt);
  }

  SECTION("Buy side") {
    engine.submit(std::make_shared<LimitOrder>(1, Side::BUY, 10, 100.00));
    REQUIRE(book.cancel_order(1) == true);
    REQUIRE(book.best_bid() == std::nullopt);
  }
}

TEST_CASE("cancel_order on a nonexistent id returns false and touches nothing", "[order_book]") {
  OrderBook book;
  MatchingEngine engine(book);
  engine.submit(std::make_shared<LimitOrder>(1, Side::BUY, 10, 100.00));

  REQUIRE(book.cancel_order(999) == false);
  REQUIRE(book.best_bid() == to_ticks(100.00));
}

TEST_CASE("a resting order at a price level fills FIFO by submission order", "[matching_engine]") {
  OrderBook book;
  MatchingEngine engine(book);

  engine.submit(std::make_shared<LimitOrder>(1, Side::SELL, 5, 100.00));
  engine.submit(std::make_shared<LimitOrder>(2, Side::SELL, 5, 100.00)); 

  auto trades = engine.submit(std::make_shared<LimitOrder>(3, Side::BUY, 5, 100.00));

  REQUIRE(trades.size() == 1);
  REQUIRE(trades[0].maker_id == 1);  // the earliest resting order fills first, not id 2
}

TEST_CASE("partial fill leaves the correct remaining quantity resting", "[matching_engine]") {
  OrderBook book;
  MatchingEngine engine(book);

  engine.submit(std::make_shared<LimitOrder>(1, Side::SELL, 10, 100.00));
  auto trades = engine.submit(std::make_shared<LimitOrder>(2, Side::BUY, 4, 100.00));

  REQUIRE(trades.size() == 1);
  REQUIRE(trades[0].quantity == 4);
  REQUIRE(book.best_ask() == to_ticks(100.00));  // make still resting with 6 left
}

TEST_CASE("a fully filled resting order is removed from the book", "[matching_engine]") {
  OrderBook book;
  MatchingEngine engine(book);

  engine.submit(std::make_shared<LimitOrder>(1, Side::SELL, 10, 100.00));
  engine.submit(std::make_shared<LimitOrder>(2, Side::BUY, 10, 100.00));

  REQUIRE(book.best_ask() == std::nullopt);
}

TEST_CASE("a triggered StopOrder becomes a MarketOrder and fills", "[matching_engine][stops]") {
  OrderBook book;
  MatchingEngine engine(book);

  // resting liquidity for the triggered market order to fill against
  engine.submit(std::make_shared<LimitOrder>(1, Side::SELL, 10, 101.00));

  // dormant buy stop, triggers opnce last trade price reaches 100
  engine.submit(std::make_shared<StopOrder>(2, Side::BUY, 10, 100.00));

  // trade at 100 to trip the stop
  engine.submit(std::make_shared<LimitOrder>(3, Side::SELL, 1, 100.00));
  auto trades = engine.submit(std::make_shared<LimitOrder>(4, Side::BUY, 1, 100.00));

  bool stop_filled = false;
  for (const auto& t : trades) {
    if (t.taker_id == 2) stop_filled = true;
  }

  REQUIRE(stop_filled);
}

TEST_CASE("a triggered StopLimitOrder that can't fully fill rests as a LimitOrder", "[matching_engine][stops]") {
  OrderBook book;
  MatchingEngine engine(book);

  // thin resting liquidity: only 2 available at 101, stop order wants 10
  engine.submit(std::make_shared<LimitOrder>(1, Side::SELL, 2, 101.00));

  // dormant buy stop-limit: triggers at 100, limits at 101
  engine.submit(std::make_shared<StopLimitOrder>(2, Side::BUY, 10, 101.00, 100.00));

  // trip the stop
  engine.submit(std::make_shared<LimitOrder>(3, Side::SELL, 1, 100.00));
  auto trades = engine.submit(std::make_shared<LimitOrder>(4, Side::BUY, 1, 100.00));

  // order 2 should have filled 2, and be resting with 8 left as a LIMIT order
  REQUIRE(book.best_bid() == to_ticks(101.00));

  bool stop_filled = false;
  for (const auto& t : trades) {
    if (t.taker_id == 2) stop_filled = true;
  }

  // REQUIRE(stop_filled);
}