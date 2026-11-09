#pragma once
#include <array>
#include <mutex>
namespace onx_registry {
// Move completed payloads from the WebSocket task to the ESPHome main loop.
// No registry, preferences, Select or UI callbacks execute on the receive task.
template<class Text, class Mutex, size_t Capacity = 4> class PayloadQueue {
 public:
  bool put(Text &text) {
    std::lock_guard<Mutex> guard(mutex_);
    if (count_ == Capacity) return false;
    entries_[(head_ + count_) % Capacity].swap(text);
    ++count_;
    return true;
  }
  bool take(Text &text) {
    std::lock_guard<Mutex> guard(mutex_);
    if (!count_) return false;
    entries_[head_].swap(text);
    entries_[head_].clear();
    head_ = (head_ + 1) % Capacity;
    --count_;
    return true;
  }
  void clear() {
    Text text;
    while (take(text)) { Text empty; text.swap(empty); }
  }
 private:
  Mutex mutex_;
  std::array<Text, Capacity> entries_{};
  size_t head_{0}, count_{0};
};
}
