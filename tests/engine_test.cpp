#include "engine.hpp"
#include <cassert>
#include <optional>

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

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Resting);
  assert(R.FilledQuantity == 0);
}

template <Side s> void TestLimitFullyFills() {
  MatchingEngine<int> Engine;

  Order<int> OrderSideA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderSideA);
  assert(R.OrderStatus == Status::Resting);

  Order<int> OrderSideB = {.price = 100,
                           .quantity = 10,
                           .direction = Opposite(s),
                           .type = Type::Limit};

  Result R2 = Engine.ExecuteOrder(OrderSideB);
  assert(R2.OrderStatus == Status::Filled);
  assert(R2.FilledQuantity == 10);
}

template <Side s> void TestLimitPartialFills() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Resting);

  Order<int> OrderB = {.price = 100,
                       .quantity = 5,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  Result R2 = Engine.ExecuteOrder(OrderB);
  assert(R2.OrderStatus == Status::Filled);

  Order<int> OrderC = {.price = 100,
                       .quantity = 10,
                       .direction = Opposite(s),
                       .type = Type::Limit};
  // We have to try a third order because the engine stores a copy of orders
  Result R3 = Engine.ExecuteOrder(OrderC);
  assert(R3.OrderStatus == Status::Resting);
  assert(R3.FilledQuantity == 5);
}

template <Side s> void TestLimitPriceOverflowRests() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Resting);

  Order<int> OrderB = {.price = 100,
                       .quantity = 15,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  Result R2 = Engine.ExecuteOrder(OrderB);
  assert(R2.OrderStatus == Status::Resting);
  assert(R2.FilledQuantity == 10);
}

template <Side s> void TestLimitNonCrossingBoundary() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {.price = 100 + GetSideVal<int>(Opposite(s)),
                       .quantity = 10,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Resting);

  Order<int> OrderB = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R2 = Engine.ExecuteOrder(OrderB);
  assert(R2.OrderStatus == Status::Resting);
  assert(R2.FilledQuantity == 0);
}

// Market Orders

template <Side s> void TestMarketOnEmptyBookCancelled() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {.quantity = 10, .direction = s, .type = Type::Market};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Cancelled);
}

template <Side s> void TestMarketPartialFill() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Resting);

  Order<int> OrderB = {
      .quantity = 15, .direction = Opposite(s), .type = Type::Market};

  Result R2 = Engine.ExecuteOrder(OrderB);
  assert(R2.OrderStatus == Status::PartialFill);
  assert(R2.FilledQuantity == 10);
}

template <Side s> void TestMarketOnMultiplePriceLevelsFills() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {.price = 100 + GetSideVal<int>(Opposite(s)),
                       .quantity = 10,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Resting);

  Order<int> OrderB = {.price = 100 + (2 * GetSideVal<int>(Opposite(s))),
                       .quantity = 10,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  Result R2 = Engine.ExecuteOrder(OrderB);
  assert(R2.OrderStatus == Status::Resting);

  Order<int> OrderC = {.quantity = 20, .direction = s, .type = Type::Market};

  Result R3 = Engine.ExecuteOrder(OrderC);
  assert(R3.OrderStatus == Status::Filled);

  Order<int> OrderD = {.quantity = 20, .direction = s, .type = Type::Market};

  Result R4 = Engine.ExecuteOrder(OrderD);
  assert(R4.OrderStatus == Status::Cancelled);
}

// Quantity

void TestNoQuantityRejects() {
  MatchingEngine<int> Engine;

  Order<int> Buy = {
      .price = 100, .quantity = 0, .direction = Side::Buy, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(Buy);
  assert(R.OrderStatus == Status::Rejected);
}

void TestPreFilledOrderRejected() {
  MatchingEngine<int> Engine;

  Order<int> Buy = {.price = 100,
                    .quantity = 10,
                    .FilledQuantity = 10,
                    .direction = Side::Buy,
                    .type = Type::Limit};

  Result R = Engine.ExecuteOrder(Buy);
  assert(R.OrderStatus == Status::Rejected);
}

void TestNegativeQuantityRejects() {
  MatchingEngine<int> Engine;

  Order<int> Buy = {.price = 100,
                    .quantity = -1,
                    .direction = Side::Buy,
                    .type = Type::Limit};

  Result R = Engine.ExecuteOrder(Buy);
  assert(R.OrderStatus == Status::Rejected);
}

void TestMarketWithPriceRejects() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {.price = 100,
                       .quantity = 10,
                       .direction = Side::Buy,
                       .type = Type::Market};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Rejected);
}

void TestLimitNoPriceRejects() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .quantity = 10, .direction = Side::Buy, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Rejected);
}

// Order Book Structure

template <Side s> void TestOrderBookSorting() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {.price = 100 + (2 * GetSideVal<int>(s)),
                       .quantity = 10,
                       .direction = s,
                       .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Resting);

  Order<int> OrderB = {.price = 100 + GetSideVal<int>(s),
                       .quantity = 10,
                       .direction = s,
                       .type = Type::Limit};

  Result R2 = Engine.ExecuteOrder(OrderB);
  assert(R2.OrderStatus == Status::Resting);

  Order<int> OrderC = {
      .quantity = 15, .direction = Opposite(s), .type = Type::Market};

  Result R3 = Engine.ExecuteOrder(OrderC);
  assert(R3.OrderStatus == Status::Filled);

  Order<int> OrderD = {.price = 100 + (GetSideVal<int>(s)),
                       .quantity = 5,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  Result R4 = Engine.ExecuteOrder(OrderD);
  assert(R4.OrderStatus == Status::Resting);
  assert(R4.FilledQuantity == 0);
}

// Cancel Order
template <Side s> void TestOrderCancelsByID() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Resting);

  CancelResult CancelR = Engine.CancelOrderById(R.OrderID);
  assert(CancelR.Status == CancelStatus::Cancelled);
  assert(CancelR.FilledQuantity == 0);
}

template <Side s> void TestCancelRejected() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Market};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Rejected);

  CancelResult CancelR = Engine.CancelOrderById(R.OrderID);

  assert(CancelR.Status == CancelStatus::Rejected);
  assert(CancelR.FilledQuantity == std::nullopt);
}

template <Side s> void TestCancelTwiceRejected() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Resting);

  CancelResult CancelR = Engine.CancelOrderById(R.OrderID);

  assert(CancelR.Status == CancelStatus::Cancelled);
  assert(CancelR.FilledQuantity == 0);

  CancelResult CancelR2 = Engine.CancelOrderById(R.OrderID);

  assert(CancelR2.Status == CancelStatus::Rejected);
  assert(CancelR2.FilledQuantity == std::nullopt);
}

template <Side s> void TestCancelLastOrderOnLevel() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);
  assert(R.OrderStatus == Status::Resting);

  CancelResult CancelR = Engine.CancelOrderById(R.OrderID);

  assert(CancelR.Status == CancelStatus::Cancelled);
  assert(CancelR.FilledQuantity == 0);

  Order<int> OrderB = {.price = 100,
                       .quantity = 10,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  Result R2 = Engine.ExecuteOrder(OrderB);
  assert(R2.OrderStatus == Status::Resting);
  assert(R2.FilledQuantity == 0);
}

template <Side s> void TestCancelOrderAfterFilled() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);

  assert(R.OrderStatus == Status::Resting);

  Order<int> OrderB = {.price = 100,
                       .quantity = 10,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  Result R2 = Engine.ExecuteOrder(OrderB);
  assert(R2.OrderStatus == Status::Filled);
  assert(R2.FilledQuantity == 10);

  CancelResult CancelR = Engine.CancelOrderById(R.OrderID);

  assert(CancelR.Status == CancelStatus::Rejected);
  assert(CancelR.FilledQuantity == std::nullopt);
}

template <Side s> void TestPartialFillThenCancel() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);

  assert(R.OrderStatus == Status::Resting);

  Order<int> OrderB = {.price = 100,
                       .quantity = 6,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  Result R2 = Engine.ExecuteOrder(OrderB);

  assert(R2.OrderStatus == Status::Filled);
  assert(R2.FilledQuantity == 6);

  CancelResult CancelR = Engine.CancelOrderById(R.OrderID);

  assert(CancelR.Status == CancelStatus::Cancelled);
  assert(CancelR.FilledQuantity == 6);

  Order<int> OrderC = {.price = 100,
                       .quantity = 4,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  Result R3 = Engine.ExecuteOrder(OrderC);

  assert(R3.OrderStatus == Status::Resting);
  assert(R3.FilledQuantity == 0);
}

template <Side s> void TestMultipleOrdersOnLevel() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);

  assert(R.OrderStatus == Status::Resting);

  Order<int> OrderB = {
      .price = 100, .quantity = 5, .direction = s, .type = Type::Limit};

  Result R2 = Engine.ExecuteOrder(OrderB);

  assert(R2.OrderStatus == Status::Resting);

  // Cancel the larger order

  CancelResult CancelR = Engine.CancelOrderById(R.OrderID);

  assert(CancelR.Status == CancelStatus::Cancelled);
  assert(CancelR.FilledQuantity == 0);

  Order<int> OrderC = {.price = 100,
                       .quantity = 7,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  Result R3 = Engine.ExecuteOrder(OrderC);

  assert(R3.OrderStatus == Status::Resting);
  assert(R3.FilledQuantity == 5);
}

// Get Order By ID
template <Side s> void TestGetOrderById() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);

  assert(R.OrderStatus == Status::Resting);

  GetOrderResult<int> OrderR = Engine.GetOrderById(R.OrderID);

  assert(OrderR.Status == GetOrderStatus::Found);
  assert(OrderR.Order->OrderID == R.OrderID);
  assert(OrderR.Order->direction == s);
  assert(OrderR.Order->QuantityRemaining == 10);
  assert(OrderR.Order->price == 100);
}

void TestGetOrderRejects() {
  MatchingEngine<int> Engine;

  GetOrderResult<int> OrderR = Engine.GetOrderById(15);

  assert(OrderR.Status == GetOrderStatus::NotFound);
  assert(OrderR.Order == std::nullopt);
}

template <Side s> void TestGetOrderUpdates() {
  MatchingEngine<int> Engine;

  Order<int> OrderA = {
      .price = 100, .quantity = 10, .direction = s, .type = Type::Limit};

  Result R = Engine.ExecuteOrder(OrderA);

  assert(R.OrderStatus == Status::Resting);
  assert(R.FilledQuantity == 0);

  Order<int> OrderB = {.price = 100,
                       .quantity = 5,
                       .direction = Opposite(s),
                       .type = Type::Limit};

  Result R2 = Engine.ExecuteOrder(OrderB);

  assert(R2.OrderStatus == Status::Filled);
  assert(R2.FilledQuantity == 5);

  GetOrderResult<int> OrderR = Engine.GetOrderById(R.OrderID);

  assert(OrderR.Status == GetOrderStatus::Found);
  assert(OrderR.Order->QuantityRemaining == 5);
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

  TestMultipleOrdersOnLevel<Side::Buy>();
  TestMultipleOrdersOnLevel<Side::Sell>();

  TestPartialFillThenCancel<Side::Buy>();
  TestPartialFillThenCancel<Side::Sell>();

  // Get Order By Id
  TestGetOrderById<Side::Buy>();
  TestGetOrderById<Side::Sell>();

  TestGetOrderRejects();

  TestGetOrderUpdates<Side::Buy>();
  TestGetOrderUpdates<Side::Sell>();

  return 0;
}