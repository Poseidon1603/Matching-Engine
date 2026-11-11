#pragma once

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <optional>
#include <unordered_map>

enum class Side { Buy, Sell };

enum class Type { Market, Limit };

enum class Status {
  Filled,
  PartialFill,
  Resting,
  Cancelled,
  Rejected,
};

enum class CancelResult {
  Cancelled,
  Rejected,
};

// Order Object
template <typename T> struct Order {
  size_t id = 0;
  std::optional<T> price = std::nullopt;
  int quantity;
  int FilledQuantity = 0;
  // Time in ms
  std::chrono::milliseconds time =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch());
  Side direction;
  Type type;

  // Const is a promise that calling this wont modify the object
  int GetRemainingQuantity() const { return quantity - FilledQuantity; }
};

struct Result {
  size_t OrderID;
  int FilledQuantity;
  Status OrderStatus;
};

template <typename T> class MatchingEngine {
private:
  // What buyers are willing to pay
  std::map<T, std::list<Order<T>>, std::greater<T>> Bid;

  // Ask.begin() is the lowest price a seller is asking for
  // Bid.begin() is the highest price a buyer will pay
  // Calling Ask.begin() returns [Price, std::list<Order<T>>]
  // Ask.begin()->second gets you the list of orders;

  // What sellers are willing to take
  std::map<T, std::list<Order<T>>, std::less<T>> Ask;

  // Index to Query Orders by ID, returns an iterator
  std::unordered_map<size_t, typename std::list<Order<T>>::iterator> Index;

  // NOT USED YET
  float TickSize = 0.25f;

  size_t OrderID = 0;

  T LastPrice = T{};

  T Spread = T{};

  template <Side S> bool Crosses(T Price, T PriceLimit) {
    if constexpr (S == Side::Buy) {
      return Price <= PriceLimit;
    } else
      return Price >= PriceLimit;
  }

  template <Side S> auto &GetOppositeBook() {
    if constexpr (S == Side::Buy) {
      return Ask;
    } else {
      return Bid;
    }
  }

  template <Side S> auto &GetBook() {
    if constexpr (S == Side::Buy) {
      return Bid;
    } else {
      return Ask;
    }
  }

  template <Side s>
  CancelResult CancelBySide(
      std::unordered_map<
          size_t, typename std::list<Order<T>>::iterator>::iterator IndexIt) {
    auto &Book = GetBook<s>();

    auto OrderIt = IndexIt->second;

    // Need * because price is an optional
    T price = *OrderIt->price;

    // Get pair<Price, list<Order<T>>>
    auto PriceLevel = Book.find(price);

    assert(PriceLevel != Book.end());

    // Erase Order via iterator it
    PriceLevel->second.erase(OrderIt);

    // Check whether we need to erase a price level
    if (PriceLevel->second.empty()) {
      Book.erase(PriceLevel);
    }

    // Erase from index
    Index.erase(IndexIt);

    return CancelResult::Cancelled;
  }

  template <Side S> Status Match(Order<T> &O) {
    // auto &Book = S == Side::Buy ? Ask : Bid;
    auto &Book = GetOppositeBook<S>();

    auto BestBookPrice = Book.begin();

    // Determine if we should be using lowest() or max()
    T LimitPrice;
    if (O.type == Type::Market) {
      LimitPrice = S == Side::Buy ? std::numeric_limits<T>::max()
                                  : std::numeric_limits<T>::lowest();
    } else {
      LimitPrice = *O.price;
    }

    while (!Book.empty() && O.GetRemainingQuantity() > 0 &&
           Crosses<S>(BestBookPrice->first, LimitPrice)) {
      auto &BestBookOrder = BestBookPrice->second.front();

      auto MinTradedQ = std::min(BestBookOrder.GetRemainingQuantity(),
                                 O.GetRemainingQuantity());
      BestBookOrder.FilledQuantity += MinTradedQ;
      O.FilledQuantity += MinTradedQ;

      LastPrice = *BestBookOrder.price;

      if (BestBookOrder.GetRemainingQuantity() == 0) {
        // Pops the first element in the list
        Index.erase(BestBookOrder.id);
        BestBookPrice->second.pop_front();

        if (BestBookPrice->second.empty()) {
          // if the list is empty, erase the price level
          Book.erase(BestBookPrice);

          // Get the Books best price
          BestBookPrice = Book.begin();
        }
      }
    }

    int RemainingQ = O.GetRemainingQuantity();

    if (RemainingQ == 0) {
      return Status::Filled;
    } else if (O.type == Type::Limit) {
      auto &SameSideBook = GetBook<S>();
      // Get Price level
      auto &Level = SameSideBook[*O.price];
      // Insert at back and return an iterator
      auto It = Level.insert(Level.end(), O);
      // Push into the index
      Index[O.id] = It;
      return Status::Resting;
    } else if (RemainingQ == O.quantity) {
      return Status::Cancelled;
    } else {
      return Status::PartialFill;
    }
  }

public:
  Result ExecuteOrder(Order<T> &O) {
    O.id = OrderID;
    OrderID++;

    if (!(O.quantity > 0 && O.FilledQuantity == 0)) {
      return {O.id, O.FilledQuantity, Status::Rejected};
    }

    if ((O.price.has_value()) != (O.type == Type::Limit)) {
      return {O.id, O.FilledQuantity, Status::Rejected};
    }

    if (O.direction == Side::Buy) {
      Status s = Match<Side::Buy>(O);
      return {O.id, O.FilledQuantity, s};
    } else {
      Status s = Match<Side::Sell>(O);
      return {O.id, O.FilledQuantity, s};
    }
  }

  CancelResult CancelOrderById(size_t OrderID) {
    // Returns std::unordered_map<size_t, ListIterator>::iterator or Index.end()
    auto IndexIt = Index.find(OrderID);

    // The ID doesnt exist
    if (IndexIt == Index.end()) {
      return CancelResult::Rejected;
    }

    // Give the iterator
    if (IndexIt->second->direction == Side::Buy) {
      return CancelBySide<Side::Buy>(IndexIt);
    } else {
      return CancelBySide<Side::Sell>(IndexIt);
    }
  }
};