#pragma once

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <limits>
#include <list>
#include <map>

enum class Side { Buy, Sell };

enum class Type { Market, Limit };

enum class Status {
  Filled,
  PartialFill,
  Resting,
  Cancelled,
  Rejected,
};

// Order Object
template <typename T> struct Order {
  // Time in ms
  T price;
  int quantity;
  int FilledQuantity = 0;
  std::chrono::milliseconds time =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch());
  Side direction;
  Type type;

  // Const is a promise that calling this wont modify the object
  int GetRemainingQuantity() const { return quantity - FilledQuantity; }
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

  float TickSize = 0.25f;

  // NOTE: NOT USED YET
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
      LimitPrice = O.price;
    }

    while (!Book.empty() && O.GetRemainingQuantity() > 0 &&
           Crosses<S>(BestBookPrice->first, LimitPrice)) {
      auto &BestBookOrder = BestBookPrice->second.front();

      auto MinTradedQ = std::min(BestBookOrder.GetRemainingQuantity(),
                                 O.GetRemainingQuantity());
      BestBookOrder.FilledQuantity += MinTradedQ;
      O.FilledQuantity += MinTradedQ;

      LastPrice = BestBookOrder.price;

      if (BestBookOrder.GetRemainingQuantity() == 0) {
        // Pops the first element in the list
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
      SameSideBook[O.price].push_back(O);
      return Status::Resting;
    } else if (RemainingQ == O.quantity) {
      return Status::Cancelled;
    } else {
      return Status::PartialFill;
    }
  }

public:
  Status ExecuteOrder(Order<T> &O) {

    if (!(O.quantity > 0 && O.GetRemainingQuantity() == O.quantity)) {
      return Status::Rejected;
    }

    if (O.direction == Side::Buy) {
      return Match<Side::Buy>(O);
    } else
      return Match<Side::Sell>(O);
  }
};