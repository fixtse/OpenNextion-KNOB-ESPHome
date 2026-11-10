#pragma once
#include <array>
#include <cstddef>

#ifdef USE_ESP32
#include <cstdlib>
#include <new>
#include "esp_heap_caps.h"
#include "esphome/core/log.h"
#endif

namespace onx_memory {
#ifdef USE_ESP32
// Large per-entity records belong in the board's PSRAM. Allocate on first use,
// after PSRAM initialization, rather than consuming internal .bss at boot.
// These containers are initialized and accessed by the main UI/API loop.
template<typename T, size_t N> class StateArray {
 public:
  StateArray() = default;
  StateArray(const StateArray &) = delete;
  StateArray &operator=(const StateArray &) = delete;
  ~StateArray() {
    if (storage_ != nullptr) {
      storage_->~Storage();
      heap_caps_free(storage_);
    }
  }
  constexpr size_t size() const { return N; }
  T &operator[](size_t index) { return storage()[index]; }
  const T &operator[](size_t index) const { return storage()[index]; }
  auto begin() { return storage().begin(); }
  auto end() { return storage().end(); }
  auto begin() const { return storage().cbegin(); }
  auto end() const { return storage().cend(); }

 private:
  using Storage = std::array<T, N>;
  Storage &storage() const {
    if (storage_ == nullptr) {
      void *memory = heap_caps_malloc(sizeof(Storage), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
      if (memory == nullptr) {
        ESP_LOGE("onx_memory", "Cannot allocate %zu bytes of entity state in PSRAM", sizeof(Storage));
        std::abort();
      }
      storage_ = new (memory) Storage{};
    }
    return *storage_;
  }
  mutable Storage *storage_{nullptr};
};
#else
template<typename T, size_t N> using StateArray = std::array<T, N>;
#endif
}  // namespace onx_memory
