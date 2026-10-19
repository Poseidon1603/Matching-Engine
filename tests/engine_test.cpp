#include "../include/engine.hpp"
#include <cassert>



void TestLimitBuyFullyFills() {
  MatchingEngine<int> Engine;

  Order<int> Sell = {.price = 100,
                     .quantity = 10,
                     .direction = Side::Sell,
                     .type = Type::Limit};

  assert(Engine.ExecuteOrder(Sell) == Status::Resting);

  Order<int> Buy = {.price = 100,
                    .quantity = 10,
                    .direction = Side::Buy,
                    .type = Type::Limit};

  assert(Engine.ExecuteOrder(Buy) == Status::Filled);
  assert(Buy.FilledQuantity == 10);
}