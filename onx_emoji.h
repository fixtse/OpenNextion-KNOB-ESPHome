#pragma once
#include "lvgl.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include "esp_heap_caps.h"
#include "onx_emoji_table.h"
#include "onx_emoji_queue.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

// Color emoji are not stored in flash. A recognized sequence is downloaded once
// (Noto Color Emoji SVG via the icon worker), kept in PSRAM and drawn through an
// LVGL image font. Until then the text keeps the original codepoints, so the
// small monochrome NotoEmoji subset in the text fonts still shows common emoji.
namespace onx_emoji {
inline constexpr uint32_t FIRST_GLYPH = 0xF0000;
inline constexpr size_t GLYPH_COUNT = sizeof(SEQUENCES) / sizeof(SEQUENCES[0]);
inline constexpr size_t MASTER_LIMIT = 48;   // ~9 KB of PSRAM each.
inline constexpr size_t PENDING_LIMIT = 24;
// Retry quickly while a label still shows the emoji (a notification is short-lived),
// then back off so an offline knob does not keep hitting the network.
inline std::chrono::steady_clock::duration retry_delay(unsigned attempts) {
  switch (attempts) {
    case 1: return std::chrono::seconds(2);
    case 2: return std::chrono::seconds(5);
    case 3: return std::chrono::seconds(15);
    default: return std::chrono::minutes(10);
  }
}

struct CachedGlyph { uint32_t code; uint16_t size; lv_image_dsc_t image; };
inline std::deque<CachedGlyph> cache;  // Per-font-size scaled copies.
inline size_t cache_bytes = 0;
inline constexpr size_t CACHE_LIMIT = 512 * 1024;
struct Master { uint8_t *pixels; uint32_t used; };
inline std::map<uint32_t, Master> masters;  // Downloaded SIZE x SIZE BGRA images by glyph index.
inline uint32_t use_counter = 0;
inline std::set<uint32_t> requested;
struct Failure { std::chrono::steady_clock::time_point at; unsigned attempts; };
inline std::map<uint32_t, Failure> failed;

// Noto names files by lowercase codepoints joined with '_', without U+FE0F.
inline std::string stem(const char *sequence) {
  std::string out;
  const auto *s = reinterpret_cast<const unsigned char *>(sequence);
  while (*s) {
    uint32_t cp; int extra;
    if (*s < 0x80) { cp = *s; extra = 0; }
    else if ((*s >> 5) == 6) { cp = *s & 0x1F; extra = 1; }
    else if ((*s >> 4) == 14) { cp = *s & 0x0F; extra = 2; }
    else { cp = *s & 0x07; extra = 3; }
    ++s;
    for (int i = 0; i < extra && (*s & 0xC0) == 0x80; ++i) cp = (cp << 6) | (*s++ & 0x3F);
    if (cp == 0xFE0F) continue;
    char hex[12];
    std::snprintf(hex, sizeof(hex), "%s%04x", out.empty() ? "" : "_", static_cast<unsigned>(cp));
    out += hex;
  }
  return out;
}
inline void request(uint32_t index) {
  if (requested.count(index) || onx_emoji_queue::pending.size() >= PENDING_LIMIT) return;
  auto failure = failed.find(index);
  // Keep the attempt count until a download succeeds, so backoff grows.
  if (failure != failed.end() &&
      std::chrono::steady_clock::now() - failure->second.at < retry_delay(failure->second.attempts)) return;
  requested.insert(index);
  onx_emoji_queue::pending.push_back({index, stem(SEQUENCES[index])});
}

// LVGL shapes one codepoint at a time: collapse a downloaded sequence to a
// supplementary private-use glyph before layout, preserving one advance width.
// Sequences not downloaded yet stay as text and are queued; *missing reports them.
inline std::string encode(const char *text, bool *missing = nullptr) {
  if (missing) *missing = false;
  std::string normalized;
  for (size_t at = 0; text[at];) {
    if (std::strncmp(text + at, "\xEF\xB8\x8F", 3) == 0) { at += 3; continue; }
    normalized += text[at++];
  }
  std::string result;
  for (size_t at = 0; at < normalized.size();) {
    size_t matched_length = 0, matched_index = 0;
    const auto *begin = std::begin(SEQUENCES);
    const auto *end = std::end(SEQUENCES);
    // Narrow the candidate range as UTF-8 bytes arrive; stop as soon as no
    // sequence has this prefix. Ordinary text usually needs only one lookup.
    for (size_t length = 1; at + length <= normalized.size(); ++length) {
      const std::string_view prefix(normalized.data() + at, length);
      const auto *candidate = std::lower_bound(begin, end, prefix,
        [](const char *sequence, std::string_view key) { return std::string_view(sequence) < key; });
      if (candidate == end) break;
      const std::string_view sequence(*candidate);
      if (sequence.substr(0, length) != prefix) break;
      begin = candidate;
      if (sequence.size() == length) {
        matched_length = length;
        matched_index = candidate - std::begin(SEQUENCES);
      }
    }
    if (!matched_length) { result += normalized[at++]; continue; }
    if (masters.count(matched_index)) {
      const uint32_t code = FIRST_GLYPH + matched_index;
      result += static_cast<char>(0xF0 | (code >> 18));
      result += static_cast<char>(0x80 | ((code >> 12) & 63));
      result += static_cast<char>(0x80 | ((code >> 6) & 63));
      result += static_cast<char>(0x80 | (code & 63));
    } else {
      result.append(normalized, at, matched_length);
      request(matched_index);
      if (missing) *missing = true;
    }
    at += matched_length;
  }
  return result;
}
inline const void *source(const lv_font_t *font, uint32_t code, uint32_t, int32_t *offset_y, void *) {
  // LVGL: top = line_top + line_height - base_line - image_height - offset_y.
  // Full-line images need to include the descent to stay inside the label.
  if (offset_y) *offset_y = -static_cast<int32_t>(font->base_line);
  const uint16_t size = font->line_height;
  for (const auto &entry : cache)
    if (entry.code == code && entry.size == size) return &entry.image;
  if (code < FIRST_GLYPH) return nullptr;
  auto master = masters.find(code - FIRST_GLYPH);
  if (master == masters.end() || size == 0) return nullptr;
  master->second.used = ++use_counter;
  auto *pixels = static_cast<uint8_t *>(heap_caps_malloc(size * size * 4, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!pixels) return nullptr;
  // Box filter from the downloaded master, averaging premultiplied color.
  constexpr unsigned N = onx_emoji_queue::SIZE;
  const uint8_t *src = master->second.pixels;
  for (uint16_t y = 0; y < size; ++y) {
    const unsigned y0 = y * N / size, y1 = std::max<unsigned>(y0 + 1, (y + 1) * N / size);
    for (uint16_t x = 0; x < size; ++x) {
      const unsigned x0 = x * N / size, x1 = std::max<unsigned>(x0 + 1, (x + 1) * N / size);
      uint32_t b = 0, g = 0, r = 0, a = 0, n = 0;
      for (unsigned sy = y0; sy < y1; ++sy)
        for (unsigned sx = x0; sx < x1; ++sx, ++n) {
          const uint8_t *p = src + (sy * N + sx) * 4;
          b += p[0] * p[3]; g += p[1] * p[3]; r += p[2] * p[3]; a += p[3];
        }
      auto *dst = pixels + (y * size + x) * 4;
      dst[0] = a ? b / a : 0; dst[1] = a ? g / a : 0; dst[2] = a ? r / a : 0; dst[3] = a / n;
    }
  }
  lv_image_dsc_t image{};
  image.header.magic = LV_IMAGE_HEADER_MAGIC;
  image.header.cf = LV_COLOR_FORMAT_ARGB8888;
  image.header.stride = size * 4;
  image.header.w = size;
  image.header.h = size;
  image.data_size = size * size * 4;
  image.data = pixels;
  cache.push_back({code, size, image});
  cache_bytes += size * size * 4;
  return &cache.back().image;
}
// Drawing is deferred: retire cached descriptors only after the frame completes.
inline void trim_cache(lv_event_t *) {
  if (cache_bytes <= CACHE_LIMIT) return;
  for (const auto &entry : cache) {
    lv_image_cache_drop(&entry.image);
    heap_caps_free(const_cast<uint8_t *>(entry.image.data));
  }
  cache.clear();
  cache_bytes = 0;
}
struct Font { const lv_font_t *original; lv_font_t *color; };
inline std::vector<Font> fonts;
inline void enable(lv_obj_t *label) {
  const auto *original = lv_obj_get_style_text_font(label, LV_PART_MAIN);
  if (!original) return;
  for (const auto &entry : fonts) {
    if (entry.color == original) return;
    if (entry.original == original) {
      lv_obj_set_style_text_font(label, entry.color, LV_PART_MAIN);
      return;
    }
  }
  if (fonts.empty()) lv_display_add_event_cb(lv_display_get_default(), trim_cache, LV_EVENT_REFR_READY, nullptr);
  auto *color = lv_imgfont_create(original->line_height, source, nullptr);
  if (!color) return;
  color->base_line = original->base_line;
  color->fallback = original;
  fonts.push_back({original, color});
  lv_obj_set_style_text_font(label, color, LV_PART_MAIN);
}

// Labels showing a not-yet-downloaded emoji, re-rendered when it arrives.
struct Waiting { std::string raw, shown; };
inline std::map<lv_obj_t *, Waiting> waiting;
inline std::set<lv_obj_t *> hooked;
inline void forget(lv_event_t *event) {
  auto *obj = static_cast<lv_obj_t *>(lv_event_get_target(event));
  waiting.erase(obj);
  hooked.erase(obj);
}
inline void set_label(lv_obj_t *widget, const char *text) {
  enable(widget);
  bool missing = false;
  auto encoded = encode(text, &missing);
  if (std::strcmp(lv_label_get_text(widget), encoded.c_str()) != 0) lv_label_set_text(widget, encoded.c_str());
  if (!missing) { waiting.erase(widget); return; }
  if (hooked.insert(widget).second) lv_obj_add_event_cb(widget, forget, LV_EVENT_DELETE, nullptr);
  waiting[widget] = {text, std::move(encoded)};
}
inline void downloaded(uint32_t index, uint8_t *pixels) {
  requested.erase(index);
  if (!pixels) {
    auto &failure = failed[index];
    failure.at = std::chrono::steady_clock::now();
    failure.attempts++;
    return;
  }
  failed.erase(index);
  if (masters.size() >= MASTER_LIMIT) {
    // Scaled copies in `cache` stay valid; only the master is released.
    auto oldest = std::min_element(masters.begin(), masters.end(),
      [](const auto &a, const auto &b) { return a.second.used < b.second.used; });
    heap_caps_free(oldest->second.pixels);
    masters.erase(oldest);
  }
  masters[index] = {pixels, ++use_counter};
  // Only refresh labels nobody has changed since; re-encoding may queue more.
  std::vector<std::pair<lv_obj_t *, std::string>> refresh;
  for (const auto &entry : waiting)
    if (std::strcmp(lv_label_get_text(entry.first), entry.second.shown.c_str()) == 0)
      refresh.emplace_back(entry.first, entry.second.raw);
  for (const auto &entry : refresh) set_label(entry.first, entry.second.c_str());
}
// Re-request emoji for labels still waiting on screen once their retry is due.
// Deleted or changed labels drop out of `waiting`, which ends their retries.
inline std::chrono::steady_clock::time_point last_tick{};
inline void tick() {
  const auto now = std::chrono::steady_clock::now();
  if (waiting.empty() || now - last_tick < std::chrono::seconds(1)) return;
  last_tick = now;
  for (auto it = waiting.begin(); it != waiting.end();) {
    if (std::strcmp(lv_label_get_text(it->first), it->second.shown.c_str()) != 0) { it = waiting.erase(it); continue; }
    encode(it->second.raw.c_str());  // Queues any missing emoji whose retry is due.
    ++it;
  }
}
inline const bool registered = (onx_emoji_queue::completed = downloaded, onx_emoji_queue::tick = tick, true);
}
