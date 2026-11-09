#pragma once
#include <cstdint>
#include <string>
namespace onx_media {
// A single click waits until the second-click window has elapsed.
struct ClickDetector {
  bool pending{false};
  uint32_t released_ms{0};
  std::string entity;
  bool release(uint32_t now, const std::string &current) {
    if (pending && entity == current && now - released_ms <= 350) {
      pending = false;
      return true;
    }
    pending = true;
    released_ms = now;
    entity = current;
    return false;
  }
  bool due(uint32_t now) const { return pending && now - released_ms > 350; }
  void cancel() { pending = false; }
};
inline ClickDetector clicks;
inline std::string requested_url, ready_url, ha_origin;
inline uint32_t requested_ms{0};
inline bool downloading{false};
inline std::string artwork_url(const std::string &origin, const std::string &picture) {
  if (picture.empty() || picture == "None" || picture == "unknown" || picture == "unavailable") return "";
  if (picture.rfind("http://", 0) == 0 || picture.rfind("https://", 0) == 0) return picture;
  if (picture[0] == '/' && picture.rfind("//", 0) != 0 && !origin.empty()) return origin + picture;
  return "";
}
inline bool artwork_needs_auth() {
  return !ha_origin.empty() && requested_url.rfind(ha_origin + "/", 0) == 0;
}
}
