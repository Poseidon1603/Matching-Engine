
#include "ringbuffer.hpp"
#include <cassert>
#include <cstddef>
#include <limits>
#include <optional>
#include <vector>

void TestPush() {
  RingBuffer<int, 8> Buffer;

  assert(Buffer.push(1));
}

void TestPop() {
  RingBuffer<int, 8> Buffer;

  assert(Buffer.pop() == std::nullopt);

  assert(Buffer.size() == 0);

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

  assert(Buffer.empty());
  assert(Buffer.push(1));
  assert(!Buffer.empty());
}

void TestFull() {
  RingBuffer<int, 2> Buffer;

  assert(Buffer.push(1));
  assert(Buffer.push(2));
  assert(Buffer.full());
}

void TestSomePush() {
  RingBuffer<int, 8> Buffer;

  const std::vector<int> nums = {1, 2, 3, 4, 5, 6};

  assert(Buffer.push_some(nums) == 6);

  for (size_t i = 0; i < nums.size(); i++) {
    assert(*Buffer.pop() == nums[i]);
  }
}

void TestSomePushV2() {
  RingBuffer<int, 4> Buffer;

  const std::vector<int> nums = {1, 2, 3, 4, 5};

  assert(Buffer.push(10));
  assert(Buffer.push(12));
  assert(Buffer.push(13));

  assert(*Buffer.pop() == 10);

  size_t pushed = Buffer.push_some(nums);

  assert(pushed == 2);
}

void TestSomePushMoreThanCapacity() {
  RingBuffer<int, 8> Buffer;

  const std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9};

  size_t pushed = Buffer.push_some(nums);
  assert(pushed == 8);

  for (size_t i = 0; i < pushed; i++) {
    assert(*Buffer.pop() == nums[i]);
  }
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

  for (int i = 1; i < 6; i++) {
    assert(Buffer.push(i));
  }

  assert(Buffer.size() == 5);

  for (int i = 1; i < 6; i++) {
    assert(*Buffer.pop() == i);
  }

  assert(Buffer.empty());
}

int main(void) {
  TestPop();
  TestPush();
  TestFull();
  TestEmpty();
  TestSize();
  TestSomePush();
  TestSomePushMoreThanCapacity();

  TestRejectOnFull();

  // Overflow
  TestOverflow();
}