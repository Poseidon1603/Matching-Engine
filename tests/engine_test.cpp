#include "engine.hpp"
#include <cassert>

Side Opposite(Side s) {
  if (s == Side::Buy) {
    return Side::Sell;
  } else
    return Side::Buy;
}

template <typename T> T GetSideVal(Side s) {
  if (s == Side::Buy) {
    return -1;
  } else {
    return 1;
  }
}

// Limit Orders

template <Side s> void TestLimitRests() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Resting);
  assert(OrderA.GetRemainingQuantity() == OrderA.quantity);
}

template <Side s> void TestLimitFullyFills() {
  MatchingEngine<int> Engine;

  Order<int> OrderSideA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderSideA).OrderStatus == Status::Resting);

  Order<int> OrderSideB = {.price = 100,
                           .quantity = 10,
                           .direction = Opposite(s),
                           .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderSideB).OrderStatus == Status::Filled);
  assert(OrderSideB.FilledQuantity == 10);
}

template <Side s> void TestLimitPartialFills() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Resting);

  Order<int> OrderB = {.price = 100,
                       .quantity = 5,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderB).OrderStatus == Status::Filled);

  Order<int> OrderC = {.price = 100,
                       .quantity = 10,
                       .direction = Opposite(s),
                       .type = Type::Limit};
  // We have to try a third order because the engine stores a copy of orders

  assert(Engine.ExecuteOrder(OrderC).OrderStatus == Status::Resting);
  assert(OrderC.FilledQuantity == 5);
}

template <Side s> void TestLimitPriceOverflowRests() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Resting);

  Order<int> OrderB = {.price = 100,
                       .quantity = 15,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderB).OrderStatus == Status::Resting);
  assert(OrderB.FilledQuantity == 10);
}

template <Side s> void TestLimitNonCrossingBoundary() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {.price = 100 + GetSideVal<int>(Opposite(s)),
                       .quantity = 10,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Resting);

  Order<int> OrderB = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderB).OrderStatus == Status::Resting);
  assert(OrderB.FilledQuantity == 0);
}

// Market Orders

template <Side s> void TestMarketOnEmptyBookCancelled() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {.quantity = 10, .direction = s, .type = Type::Market};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Cancelled);
}

template <Side s> void TestMarketPartialFill() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Resting);

  Order<int> OrderB = {
      .quantity = 15, .direction = Opposite(s), .type = Type::Market};

  assert(Engine.ExecuteOrder(OrderB).OrderStatus == Status::PartialFill);
  assert(OrderB.FilledQuantity == 10);
}

template <Side s> void TestMarketOnMultiplePriceLevelsFills() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {.price = 100 + GetSideVal<int>(Opposite(s)),
                       .quantity = 10,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Resting);

  Order<int> OrderB = {.price = 100 + (2 * GetSideVal<int>(Opposite(s))),
                       .quantity = 10,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderB).OrderStatus == Status::Resting);

  Order<int> OrderC = {.quantity = 20, .direction = s, .type = Type::Market};

  assert(Engine.ExecuteOrder(OrderC).OrderStatus == Status::Filled);

  Order<int> OrderD = {.quantity = 20, .direction = s, .type = Type::Market};

  assert(Engine.ExecuteOrder(OrderD).OrderStatus == Status::Cancelled);
}

// Quantity

void TestNoQuantityRejects() {
  MatchingEngine<int> Engine;

  Order<int> Buy = {
      .price = 100, .quantity = 0, .direction = Side::Buy, .type = Type::Limit};

  assert(Engine.ExecuteOrder(Buy).OrderStatus == Status::Rejected);
}

void TestPreFilledOrderRejected() {
  MatchingEngine<int> Engine;

  Order<int> Buy = {.price = 100,
                    .quantity = 10,
                    .FilledQuantity = 10,
                    .direction = Side::Buy,
                    .type = Type::Limit};

  assert(Engine.ExecuteOrder(Buy).OrderStatus == Status::Rejected);
}

void TestNegativeQuantityRejects() {
  MatchingEngine<int> Engine;

  Order<int> Buy = {.price = 100,
                    .quantity = -1,
                    .direction = Side::Buy,
                    .type = Type::Limit};

  assert(Engine.ExecuteOrder(Buy).OrderStatus == Status::Rejected);
}

void TestMarketWithPriceRejects() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {.price = 100,
                       .quantity = 10,
                       .direction = Side::Buy,
                       .type = Type::Market};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Rejected);
}

void TestLimitNoPriceRejects() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .quantity = 10, .direction = Side::Buy, .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Rejected);
}

// Order Book Structure

template <Side s> void TestOrderBookSorting() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {.price = 100 + (2 * GetSideVal<int>(s)),
                       .quantity = 10,
                       .direction = s,
                       .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Resting);

  Order<int> OrderB = {.price = 100 + GetSideVal<int>(s),
                       .quantity = 10,
                       .direction = s,
                       .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderB).OrderStatus == Status::Resting);

  Order<int> OrderC = {
      .quantity = 15, .direction = Opposite(s), .type = Type::Market};

  assert(Engine.ExecuteOrder(OrderC).OrderStatus == Status::Filled);

  Order<int> OrderD = {.price = 100 + (GetSideVal<int>(s)),
                       .quantity = 5,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderD).OrderStatus == Status::Resting);
  assert(OrderD.FilledQuantity == 0);
}

// Cancel Order
template <Side s> void TestOrderCancelsByID() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Resting);
  assert(Engine.CancelOrderById(OrderA.id) == CancelResult::Cancelled);
}

template <Side s> void TestCancelRejected() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Market};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Rejected);
  assert(Engine.CancelOrderById(OrderA.id) == CancelResult::Rejected);
}

template <Side s> void TestCancelTwiceRejected() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Resting);
  assert(Engine.CancelOrderById(OrderA.id) == CancelResult::Cancelled);
  assert(Engine.CancelOrderById(OrderA.id) == CancelResult::Rejected);
}

template <Side s> void TestCancelLastOrderOnLevel() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Resting);
  assert(Engine.CancelOrderById(OrderA.id) == CancelResult::Cancelled);

  Order<int> OrderB = {.price = 100,
                       .quantity = 10,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderB).OrderStatus == Status::Resting);
}

template <Side s> void TestCancelOrderAfterFilled() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderA).OrderStatus == Status::Resting);

  Order<int> OrderB = {.price = 100,
                       .quantity = 10,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  assert(Engine.ExecuteOrder(OrderB).OrderStatus == Status::Filled);
  assert(OrderB.FilledQuantity == 10);
  assert(Engine.CancelOrderById(OrderA.id) == CancelResult::Rejected);
}

int main(void) {
  // Limit
  TestLimitRests<Side::Buy>();
  TestLimitRests<Side::Sell>();

  TestLimitFullyFills<Side::Buy>();
  TestLimitFullyFills<Side::Sell>();

  TestLimitPartialFills<Side::Buy>();
  TestLimitPartialFills<Side::Sell>();

  TestLimitPriceOverflowRests<Side::Buy>();
  TestLimitPriceOverflowRests<Side::Sell>();

  TestLimitNonCrossingBoundary<Side::Buy>();
  TestLimitNonCrossingBoundary<Side::Sell>();

  // Market
  TestMarketOnEmptyBookCancelled<Side::Buy>();
  TestMarketOnEmptyBookCancelled<Side::Sell>();

  TestMarketPartialFill<Side::Buy>();
  TestMarketPartialFill<Side::Sell>();

  TestMarketOnMultiplePriceLevelsFills<Side::Buy>();
  TestMarketOnMultiplePriceLevelsFills<Side::Sell>();

  // Quantity
  TestNoQuantityRejects();
  TestPreFilledOrderRejected();
  TestNegativeQuantityRejects();
  TestMarketWithPriceRejects();
  TestLimitNoPriceRejects();

  // Order Book Structure
  TestOrderBookSorting<Side::Buy>();
  TestOrderBookSorting<Side::Sell>();

  // Cancelling
  TestOrderCancelsByID<Side::Buy>();
  TestOrderCancelsByID<Side::Sell>();

  TestCancelRejected<Side::Buy>();
  TestCancelRejected<Side::Sell>();

  TestCancelTwiceRejected<Side::Buy>();
  TestCancelTwiceRejected<Side::Sell>();

  TestCancelLastOrderOnLevel<Side::Buy>();
  TestCancelLastOrderOnLevel<Side::Sell>();

  TestCancelOrderAfterFilled<Side::Buy>();
  TestCancelOrderAfterFilled<Side::Sell>();

  return 0;
}