#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <numeric>
#include <stdexcept>
#include <stdlib.h>
#include <vector>

enum Side { Buy, Sell };

// Only Market and Limit for now
// FIX has a lot more, but their expose position intentions (limit does too but
// its much more useful)
// Market Orders will eat into next price levels if the liquidity doesnt exist
// in the book at that price level
enum Type { Market, Limit };

enum Fill_Status {
  Filled,
  Partially_Filled,
  Rejected,
};

// Order Object
template <typename T> struct Order {
  // Time in ms
  T price;
  int quantity;
  std::chrono::milliseconds time =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch());
  Side direction;
  Type type;
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

  // Start with a 0.25 tick size
  // We store a price that is 4 times, whatever the user gave us
  float TickSize = 0.25f;

  // Start at order 0
  // NOTE: NOT USED YET
  size_t OrderID = 0;

  // For storing last price
  T LastPrice;

  // Storing Spread;
  T Spread;

  template <Side S> bool Crosses(T Price, T PriceLimit) {
    if constexpr (S == Side::Buy) {
      return Price <= PriceLimit;
    } else
      return Price >= PriceLimit;
  }

  template <Side S> auto &GetBook() {
    if constexpr (S == Side::Buy) {
      return &Ask;
    } else {
      return &Bid;
    }
  }

  template <Side S> Fill_Status Match(Order<T> &Order) {
    // auto &Book = S == Side::Buy ? Ask : Bid;
    auto &Book = GetBook<S>();

    auto BestBookPrice = Book.begin();
    auto &BestBookOrder = BestBookPrice->second.front();

    while (!Book.empty() && Order.quantity > 0 &&
           Crosses<S>(BestBookPrice->first, Order.price)) {
      BestBookOrder.quantity--;
      Order.quantity--;

      LastPrice = BestBookOrder.price;

      if (BestBookOrder.quantity == 0) {
        BestBookOrder.erase();
        &BestBookOrder = BestBookPrice->second.front();
      }
    }

    if (Order.quantity != 0) {
      return Partially_Filled;
    }

    return Filled;
  }

public:
  Fill_Status ExecuteOrder(Order<T> &Order) {}
};

int main(void) {

  MatchingEngine<int> Engine;

  return 0;
}