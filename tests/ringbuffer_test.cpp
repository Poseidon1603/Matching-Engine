
#include "engine.hpp"
#include "ringbuffer.hpp"
#include <cassert>
#include <cstddef>
#include <limits>

Side Opposite(Side s) {
  if (s == Side::Buy) {
    return Side::Sell;
  } else
    return Side::Buy;
}

template <Side s> void TestPush() {
    MatchingEngine<int> Engine;
    RingBuffer<Fill<int>, 2> Buffer;
    
    Order<int> OrderA = {.price=100, .quantity=10, .direction=s, .type=Type::Limit};

    Result ResultO = Engine.ExecuteOrder(OrderA);

    assert(ResultO.OrderStatus == Status::Resting);

    Order<int> OrderB = {.price=100, .quantity=10, .direction=Opposite(s),.type=Type::Limit};

    Result ResultO2 = Engine.ExecuteOrder(OrderB);

    assert(ResultO2.OrderStatus == Status::Filled);
    assert(ResultO2.FilledQuantity == 10);

    for (const auto& fill : Engine.LastFills()) {
      assert(Buffer.push(fill));
    }
    assert(Buffer.size() == 2);
}

template <Side s> void TestPop() {
  MatchingEngine<int> Engine;
  RingBuffer<Fill<int>, 2> Buffer;

  Order<int> OrderA {.price=100, .quantity=10, .direction=s, .type= Type::Limit};

  Result ResultO = Engine.ExecuteOrder(OrderA);

  assert(ResultO.OrderStatus == Status::Resting);

  Order<int> OrderB = {.price=100, .quantity=10, .direction=Opposite(s), .type=Type::Limit};

  Result ResultO2 = Engine.ExecuteOrder(OrderB);

  assert(ResultO2.OrderStatus == Status::Filled);
  assert(ResultO2.FilledQuantity == 10);

  for (const auto& fill : Engine.LastFills()) {
    Buffer.push(fill);
  }

  assert(Buffer.size() == 2);

  // Resting fill
  Fill<int> FillA = *Buffer.pop();

  // Active Fill
  Fill<int> FillB = *Buffer.pop();

  assert(FillA.TradeID == FillB.TradeID);
  assert(FillA.direction == OrderA.direction);
  assert(FillB.direction == OrderB.direction);
}


// Overflow
void TestOverflow() {
  RingBuffer<int, 8> Buffer(std::numeric_limits<std::size_t>::max() - 2);

  Buffer.push(1);
  Buffer.push(2);
  Buffer.push(3);
  Buffer.push(4);
  Buffer.push(5);

  assert(Buffer.size() == 5);

  for (int i = 1; i < 6; i++) {
    assert(Buffer.pop() == i);
  }

  assert(Buffer.empty());
}

int main(void) {
  TestPop<Side::Buy>();
  TestPop<Side::Sell>();

  TestPush<Side::Buy>();
  TestPush<Side::Sell>();


  // Overflow
  TestOverflow();

}