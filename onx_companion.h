#pragma once
// Room Companion integration for Home Assistant (github.com/fixtse/ha-room-companion),
// shared with ESPHome Room Companion (keep both copies identical).
// When installed, it calls the device's set_companion action with the base URL
// of its local assets (http://<ha>/api/room_companion/<token>). Icons, emoji
// and media covers are then fetched from Home Assistant over the LAN, already
// sized (no TLS, cached there); the internet sources stay as the fallback.
// The URL is kept in NVS so it works before Home Assistant connects.
#if __has_include("nvs.h")
#include "esp_memory_utils.h"
#include "esp_timer.h"
#include "nvs.h"
#define ONX_COMPANION_NVS 1
#else
#include <chrono>  // Host tests: no NVS.
#endif
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>

namespace onx_companion {
constexpr char NAMESPACE[] = "rc_comp";
constexpr char KEY[] = "url";
constexpr size_t MAX_URL = 200;
constexpr uint32_t COOLDOWN_MS = 5 * 60 * 1000;  // After a failure, use the internet sources.

inline std::mutex &lock() { static std::mutex m; return m; }
inline std::string &stored() { static std::string s; return s; }
inline uint32_t &failed_at() { static uint32_t t = 0; return t; }
inline bool &failing() { static bool f = false; return f; }
#ifdef ONX_COMPANION_NVS
inline uint32_t now_ms() { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }
#else
inline uint32_t now_ms() {
  return static_cast<uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now().time_since_epoch()).count());
}
#endif

inline bool valid(const std::string &url) {
  if (url.size() > MAX_URL || url.find("/api/room_companion/") == std::string::npos) return false;
  if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) return false;
  for (char c : url)
    if (c <= ' ' || c == '"' || c == '\'' || c == '<' || c == '>' || c == '\\' || c == '?' || c == '#') return false;
  return true;
}

inline void load_now() {
#ifdef ONX_COMPANION_NVS
  nvs_handle_t h;
  if (nvs_open(NAMESPACE, NVS_READONLY, &h) != ESP_OK) return;
  char buf[MAX_URL + 1] = {};
  size_t size = sizeof(buf);
  if (nvs_get_str(h, KEY, buf, &size) == ESP_OK && valid(buf)) {
    std::lock_guard<std::mutex> g(lock());
    stored() = buf;
  }
  nvs_close(h);
#endif
}
// NVS reads and writes touch flash, which turns the cache off: a task whose
// stack is in PSRAM (the icon and glyph workers) must not do them, or the
// device aborts (2026.11.1.5 knob boot crash). Those tasks see no URL until
// the main loop has read it, and they never write it.
inline bool stack_in_psram() {
#ifdef ONX_COMPANION_NVS
  int probe = 0;
  return esp_ptr_external_ram(&probe);
#else
  return false;
#endif
}
inline std::atomic<bool> &loaded() { static std::atomic<bool> b{false}; return b; }
// Read from NVS on first use from a task that may touch flash.
inline void ensure_loaded() {
  if (loaded() || stack_in_psram()) return;
  static std::once_flag once;
  std::call_once(once, load_now);
  loaded() = true;
}

// The base URL while usable; empty when unset or cooling down after a failure.
inline std::string base() {
  ensure_loaded();
  std::lock_guard<std::mutex> g(lock());
  if (stored().empty()) return {};
  if (failing()) {
    if (now_ms() - failed_at() < COOLDOWN_MS) return {};
    failing() = false;
  }
  return stored();
}
inline std::string configured() { ensure_loaded(); std::lock_guard<std::mutex> g(lock()); return stored(); }
inline bool active() { return !base().empty(); }
inline bool owns(const std::string &url) {
  ensure_loaded();
  std::lock_guard<std::mutex> g(lock());
  return !stored().empty() && url.rfind(stored(), 0) == 0;
}
// Home Assistant did not answer: fall back to the internet for a while.
inline void fail() {
  std::lock_guard<std::mutex> g(lock());
  if (!stored().empty()) { failed_at() = now_ms(); failing() = true; }
}

// Empty clears it. Returns false for an invalid URL.
inline bool set(std::string url) {
  ensure_loaded();
  while (!url.empty() && url.back() == '/') url.pop_back();
  if (!url.empty() && !valid(url)) return false;
  {
    std::lock_guard<std::mutex> g(lock());
    if (stored() == url && !failing()) return true;
    stored() = url;
    failing() = false;
  }
#ifdef ONX_COMPANION_NVS
  if (stack_in_psram()) return true;  // Kept in RAM only; see stack_in_psram().
  nvs_handle_t h;
  if (nvs_open(NAMESPACE, NVS_READWRITE, &h) != ESP_OK) return true;
  if (url.empty()) nvs_erase_key(h, KEY); else nvs_set_str(h, KEY, url.c_str());
  nvs_commit(h);
  nvs_close(h);
#endif
  return true;
}

inline uint32_t fnv(const std::string &s) {
  uint32_t v = 2166136261u;
  for (unsigned char c : s) v = (v ^ c) * 16777619u;
  return v;
}

// "mdi:weather-sunny" -> <base>/icon/weather-sunny.svg?size=80
inline std::string icon_url(const std::string &icon, int size) {
  if (icon.rfind("mdi:", 0) != 0 || icon.size() < 5) return {};
  const auto b = base();
  return b.empty() ? b : b + "/icon/" + icon.substr(4) + ".svg?size=" + std::to_string(size);
}
// stem: lowercase code points joined by '_', as onx_emoji builds it.
inline std::string emoji_url(const std::string &stem) {
  const auto b = base();
  return b.empty() ? b : b + "/emoji/" + stem + ".svg";
}
inline bool entity_ok(const std::string &entity) {
  if (entity.empty() || entity.size() > 96) return false;
  for (char c : entity)
    if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '.')) return false;
  return true;
}
// A media player's art cropped to w x h (JPEG). picture (entity_picture) only
// versions the URL, so a new track is a new URL. extra: e.g. "&mode=blur".
inline std::string cover_url(const std::string &entity, const std::string &picture, int w, int h,
                             const char *extra = "") {
  if (entity.rfind("media_player.", 0) != 0 || !entity_ok(entity) || picture.empty() ||
      picture == "None" || picture == "unknown" || picture == "unavailable")
    return {};
  const auto b = base();
  if (b.empty()) return b;
  char version[12];
  snprintf(version, sizeof(version), "%08x", static_cast<unsigned>(fnv(picture)));
  return b + "/cover/" + entity + "?w=" + std::to_string(w) + "&h=" + std::to_string(h) + "&fmt=jpeg" + extra +
         "&v=" + version;
}
}  // namespace onx_companion
