#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <limits>
#include <optional>

template <typename T, std::size_t Capacity> class RingBuffer {
  static_assert(Capacity > 0, "Capacity must be greater than 0");
  static_assert((Capacity & (Capacity - 1)) == 0,
                "Capacity must be a power of 2");

private:
  static constexpr std::size_t kMask = Capacity - 1;

#ifdef __cpp_lib_hardware_interference_size
  static constexpr std::size_t kCacheLineSize =
      std::hardware_destructive_interference_size;
#else
  static constexpr std::size_t kCacheLineSize = 64;
#endif
  // without alignas head and tail would sit on the same cache line
  // so writing to one would invalidate the other (false sharing)
  alignas(kCacheLineSize) std::atomic<std::size_t> head_{0};
  alignas(kCacheLineSize) std::atomic<std::size_t> tail_{0};
  alignas(kCacheLineSize) std::array<T, Capacity> buffer_{};

public:
  RingBuffer() = default;
  explicit RingBuffer(std::size_t start) : head_(start), tail_(start) {}

  // Non copyable, non-moveable because these stop the compiler generating move
  // operations
  RingBuffer(const RingBuffer &) = delete;
  RingBuffer &operator=(const RingBuffer &) = delete;

  bool push(const T &item) {
    // used when a thread loads its own index (head_ for producer)
    const std::size_t head = head_.load(std::memory_order_relaxed);

    // memory_order_acquire is to synchronise with another thread
    if (head - tail_.load(std::memory_order_acquire) == Capacity) {
      // queue is full
      return false;
    }

    buffer_[head & kMask] = item;

    // Release ensure that writting to buffer_[head] finishes before head_
    // updates
    head_.store(head + 1, std::memory_order_release);
    return true;
  }

  std::optional<T> pop() {
    // used because we are accessing an index we (consumer thread)
    const std::size_t tail = tail_.load(std::memory_order_relaxed);

    // acquire gives us producers latest head and buffer write
    if (tail == head_.load(std::memory_order_acquire)) {
      return std::nullopt;
    }

    T item = std::move(buffer_[tail & kMask]);

    // release ensures we finish reading buffer_[tail] before freeing the slot
    tail_.store(tail + 1, std::memory_order_release);
    return item;
  }

  bool empty() const {
    const std::size_t tail = tail_.load(std::memory_order_relaxed);

    return (head_.load(std::memory_order_acquire) == tail);
  }

  bool full() const {
    const std::size_t head = head_.load(std::memory_order_relaxed);

    return (head - tail_.load(std::memory_order_acquire) == Capacity);
  }

  std::size_t size() const {
    const std::size_t tail = tail_.load(std::memory_order_acquire);

    const std::size_t head = head_.load(std::memory_order_acquire);

    return (head - tail);
  }
};