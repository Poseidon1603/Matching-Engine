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
#include <vector>

enum class Side { Buy, Sell };

enum class Type { Market, Limit };

enum class Status {
  Filled,
  PartialFill,
  Resting,
  Cancelled,
  Rejected,
};

enum class CancelStatus {
  Cancelled,
  Rejected,
};

enum class GetOrderStatus { Found, NotFound };

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

struct CancelResult {
  size_t OrderID;
  std::optional<int> FilledQuantity = std::nullopt;
  CancelStatus Status;
};

template <typename T> struct GetOrder {
  T price;
  size_t OrderID;
  Side direction;
  int QuantityRemaining;
  // When the order came in
  std::chrono::milliseconds time;
};

template <typename T> struct GetOrderResult {
  GetOrderStatus Status;
  std::optional<GetOrder<T>> Order = std::nullopt;
};

template <typename T> struct Fill {
  // Order id
  size_t id;

  // Trade ID
  size_t TradeID;

  // Fill price
  T price;
  // Time of fill
  std::chrono::milliseconds time;

  // Fill Quantity
  int quantity;

  // Quantity Remaining
  int QuantityRemaining;

  Side direction;
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

  size_t OrderID = 1;

  size_t TradeID = 1;

  // IS UPDATED, BUT NO TESTS WRITTEN
  T LastPrice = T{};

  // SPREAD NOT USED YET
  T Spread = T{};

  // Vector of fills
  std::vector<Fill<T>> MatchFills;

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
  CancelStatus CancelBySide(
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

    return CancelStatus::Cancelled;
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

      // Now 2 fills but with the same tradeID, so they represent 1 trade
      std::chrono::milliseconds time =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::system_clock::now().time_since_epoch());

      Fill<T> RestingFill = {
          .id = BestBookOrder.id,
          .TradeID = TradeID,
          .price = *BestBookOrder.price,
          .time = time,
          .quantity = MinTradedQ,
          .QuantityRemaining = BestBookOrder.GetRemainingQuantity(),
      };
      MatchFills.push_back(RestingFill);

      Fill<T> ActiveFill = {
          .id = O.id,
          .TradeID = TradeID,
          .price = *BestBookOrder.price,
          .time = time,
          .quantity = MinTradedQ,
          .QuantityRemaining = O.GetRemainingQuantity(),
      };
      MatchFills.push_back(ActiveFill);
      TradeID++;

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
    // Clear Fills from previous order
    MatchFills.clear();

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

  CancelResult CancelOrderById(size_t ID) {
    // Returns std::unordered_map<size_t, ListIterator>::iterator or Index.end()
    auto IndexIt = Index.find(ID);

    // The ID doesnt exist
    if (IndexIt == Index.end()) {
      return {.OrderID = ID, .Status = CancelStatus::Rejected};
    }

    int FilledQuantity = IndexIt->second->FilledQuantity;
    // Give the iterator
    if (IndexIt->second->direction == Side::Buy) {
      return {ID, FilledQuantity, CancelBySide<Side::Buy>(IndexIt)};
    } else {
      return {ID, FilledQuantity, CancelBySide<Side::Sell>(IndexIt)};
    }
  }

  GetOrderResult<T> GetOrderById(size_t ID) {
    // unordered_map<size_t ID, std::list::Order<T>::iterator>
    auto IndexIt = Index.find(ID);

    if (IndexIt == Index.end()) {
      return {.Status = GetOrderStatus::NotFound};
    }

    auto OrderIt = IndexIt->second;

    GetOrder<T> O = {.price = *OrderIt->price,
                     .OrderID = OrderIt->id,
                     .direction = OrderIt->direction,
                     .QuantityRemaining = OrderIt->GetRemainingQuantity(),
                     .time = OrderIt->time};

    return {.Status = GetOrderStatus::Found, .Order = O};
  }

  const std::vector<Fill<T>> &LastFills() const { return MatchFills; }
};