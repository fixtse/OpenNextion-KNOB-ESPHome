#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
namespace onx_recovery {
struct Retry {
  std::string entity;
  uint32_t since{0}, delay{10000};
  unsigned attempts{0};
  bool due(const std::string &id, bool available, uint32_t now) {
    if (entity != id) { entity = id; since = now; delay = 10000; attempts = 0; }
    if (available || id.empty()) { since = now; delay = 10000; attempts = 0; return false; }
    return now - since >= delay;
  }
  void attempted(uint32_t now) {
    since = now;
    delay = 30000u << std::min(attempts, 3u);
    if (++attempts >= 5) delay = 300000;
  }
};
}  // namespace onx_recovery
