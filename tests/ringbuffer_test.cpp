
#include "ringbuffer.hpp"
#include <cassert>
#include <cstddef>
#include <limits>
#include <optional>
#include <span>
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

  std::span<const int> rest = Buffer.push_some(nums);
  assert(rest.size() == 0);

  for (size_t i = 0; i < (nums.size() - rest.size()); i++) {
    assert(*Buffer.pop() == nums[i]);
  }

  assert(Buffer.empty());
}

void TestSomePushV2() {
  RingBuffer<int, 4> Buffer;

  const std::vector<int> nums = {1, 2, 3, 4, 5};

  assert(Buffer.push(10));
  assert(Buffer.push(12));
  assert(Buffer.push(13));

  assert(*Buffer.pop() == 10);

  std::span<const int> rest = Buffer.push_some(nums);

  assert(rest.size() == 3);

  assert(*Buffer.pop() == 12);
  assert(*Buffer.pop() == 13);
  assert(*Buffer.pop() == 1);
  assert(*Buffer.pop() == 2);

  assert(Buffer.empty());
}

void TestSomePushMoreThanCapacity() {
  RingBuffer<int, 8> Buffer;

  const std::vector<int> nums = {1, 2, 3, 4, 5, 6, 7, 8, 9};

  std::span<const int> rest = Buffer.push_some(nums);

  assert(rest.size() == 1);

  for (size_t i = 0; i < (nums.size() - rest.size()); i++) {
    assert(*Buffer.pop() == nums[i]);
  }

  assert(rest.size() == 1 && rest.front() == 9);

  assert(Buffer.empty());
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
  TestSomePushV2();
  TestSomePushMoreThanCapacity();

  TestRejectOnFull();

  // Overflow
  TestOverflow();
}