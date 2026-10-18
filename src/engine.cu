#include <chrono>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <numeric>
#include <stdexcept>
#include <stdlib.h>
#include <vector>

enum Type { Buy, Sell };

// Only Market and Limit for now
// FIX has a lot more, but their expose position intentions (limit does too but
// its )
enum Order_Type { Market, Limit };

enum Fill_Status {
  Filled,
  Partially_Filled,
  Rejected,
};

// Order Object (Do we assign the order id)
template <typename T> struct Order {
  // Time in ms
  T price;
  int quantity;
  std::chrono::milliseconds time =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch());
  Type direction;
  Order_Type type;
};

// We want to take in something like:
// Price
// Quantity
// Type
// Direction
// TimeStamped Arrival time?

// When we match we only want to return:
// A list of all the orders where you matched
// We give back a direction
// We dont need to give back the type

template <typename T> struct MatchedOrder {
  // Do we keep an id for each matching?
  T price;
  int quantity;
  Type direction;
};

// We basically want to give back every singal filled price
template <typename T> struct MatchedOrders {
  // Order ID
  size_t id;

  // Matched Orders
  std::vector<Order<T>> Orders;
};

template <typename T> class MatchingEngine {
private:
  // Bid (used for buyers)
  std::map<T, std::list<Order<T>>, std::less<T>> Bid;

  // btw Ask.begin() is the lowest price, and Bid.begin() is the highest price
  // Calling Ask.begin() or Bid.begin() will give you a pair of <price, list of
  // orders at that price> Ask.begin()->first gets you the price itself
  // Ask.begin()->second gets you the list of orders at that price (sorted by
  // time)

  // Ask (used for sellers)
  std::map<T, std::list<Order<T>>, std::greater<T>> Ask;

  // Start with a 0.25 tick size
  float tickSize = 0.25f;

  // Start at order 0
  size_t orderID = 0;

public:
  // Indexing Reminder
  T func(T a) { return Ask[a].front().price; }

  // Returns Order ID
  MatchedOrders<T> ExecuteOrder(Order<T> a) {
    //a.id = ++orderID;
    if (a.direction == Buy) {
      if (a.type == Market) {
        // Need to match and execute
        while (!Ask.empty() && a.quantity > 0) {
          auto BestAsk = Ask.begin();
          auto BestAskOrder = BestAsk->second.front();

          while (BestAskOrder.quantity > 0 && a.quantity > 0) {
            // now we want to match
            BestAskOrder.quantity--;
            a.quantity--;

            // Do we need a list of fill prices?

            // if BestAskOrder quantity = 0, we need to remove the entry
            if (BestAskOrder.quantity == 0) {
              BestAsk->second.
            }
          }
        }
      } else {
        // Its a limit order (buy)
        Bid[a.price].push_back(a);

        if (Bid.begin()->first >= Ask.begin()->first) {
          // Match and Execute
          auto BestBid = Bid.begin();
          auto BestAsk = Ask.begin();
        }
      }
    } else {
      if (a.type == Market) {
        // Need to match and execute

      } else {
        // Its a limit order (sell)
        Ask[a.price].push_back(a);
      }
    }

    // Return Generated ID (Or a rejection)
    return a.id;
  }
};

template <typename T> struct SimpleOrder {
  // Price
  T price;

  // quantity
  int quantity;

  // direction
  Type direction;

  // Order Type
  Order_Type type;
};

template <typename T> class SimpleMatchingEngine {
private:
  // Bid ask
  std::map<T, std::list<SimpleOrder<T>>, std::greater<T>> Ask;
  std::map<T, std::list<SimpleOrder<T>>, std::less<T>> Bid;

  T last_price;
  T spread;

public:
  size_t ExecuteOrder(SimpleOrder<T> &Order) {
    if (Order.type == Limit) {

    } else if (Order.type == Market) {
      if (Order.direction == Buy) {
        // This should fill right now at the best possible price
        auto BestAskPrice = Ask.begin();
        auto &BestAskOrder = BestAskPrice->second.front();

        // This represents a matching
        while (Order.quantity > 0 && !Ask.empty()) {
          BestAskOrder.quantity--;
          // This call might be a waste of mem + cpu
          last_price = BestAskOrder.price;
          if (BestAskOrder.quantity == 0) {
            // If we depleted this order, we need to get a new one in
            // NOTE: Might not be the best way to do this
            BestAskPrice->second.front().erase();
            BestAskOrder = BestAskPrice->second.front();
          }
        }

        // What if we cant actually match all orders?
        if (Order.quantity != 0) {
          // We couldnt do all the matching
          // For this simple version, we are going to just disregard the rest of
          // the order
          return Partially_Filled;
        }

        return Filled;

      } else if (Order.direction == Sell) {

      } else {
        throw std::runtime_error("Erm Chat, help pls");
      }
    } else {
      throw std::runtime_error("Chat What the fuck how is this possible");
    }
  }
};

int main(void) {

  MatchingEngine<int> Engine;

  Engine.ExecuteOrder(
      Order<int>{.id = 1, .price = 100, .direction = Buy, .type = Limit});

  return 0;
}