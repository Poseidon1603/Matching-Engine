
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

void TestPush() {
  RingBuffer<int, 8> Buffer;

  assert(Buffer.push(1));
}

void TestPop() {
  RingBuffer<int, 8> Buffer;

  assert(Buffer.push(1));

  assert(*Buffer.pop() == 1);
}

void TestSize() {
  RingBuffer<int, 8> Buffer;

  assert(Buffer.push(1));
  assert(Buffer.size() == 1);
}

void TestEmpty() {
  RingBuffer<int, 8> Buffer;

  assert(Buffer.push(1));
  assert(!Buffer.empty());
}

void TestFull() {
  RingBuffer<int, 2> Buffer;

  assert(Buffer.push(1));
  assert(Buffer.push(2));
  assert(Buffer.full());
}

void TestRejectOnFull() {
  RingBuffer<int, 2> Buffer;

  assert(Buffer.push(1));
  assert(Buffer.push(2));
  assert(!Buffer.push(3));
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
  TestPop();
  TestPush();
  TestFull();
  TestEmpty();
  TestSize();

  TestRejectOnFull();

  // Overflow
  TestOverflow();
}