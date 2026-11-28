#pragma once
// Characters the firmware fonts lack (Chinese, Japanese, Korean, Greek,
// Cyrillic, ...), drawn by the Room Companion integration for Home Assistant
// (onx_companion.h, /glyph/<codepoint>.a8h). Shared by ESPHome Room Companion
// and OpenNextion-KNOB-ESPHome: keep both copies identical.
//
// A last-resort LVGL fallback font is appended to each text font's chain. For
// a missing character it returns the downloaded bitmap, or queues a download
// and reports the glyph as missing for now. A small worker task fetches the
// glyphs over the LAN; when some arrive, every style is refreshed once so
// labels lay their text out again. Without the integration nothing changes.
// Glyphs are cached per size; the least recently drawn are dropped when the
// cache is full. prefetch() asks for a text's glyphs before it is shown (for
// example the next lyric lines in the fonts they will use).
#include "lvgl.h"
#include "onx_companion.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <deque>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#if __has_include("esp_http_client.h") && __has_include("freertos/idf_additions.h")
#include "esp_heap_caps.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_memory_utils.h"
#include "freertos/FreeRTOS.h"
#include "freertos/idf_additions.h"
#include "freertos/task.h"
#define ONX_GLYPHS_FETCH 1
#endif

namespace onx_glyphs {
constexpr size_t GLYPH_LIMIT = 1200;         // Under 1 KB of PSRAM each at panel sizes.
constexpr size_t GLYPH_KEEP = 1000;          // Trimmed to this many when full.
constexpr uint32_t IN_USE_MS = 10 * 1000;    // Drawn this recently: never dropped.
constexpr size_t QUEUE_LIMIT = 256;
constexpr size_t MAX_BYTES = 8 + 160 * 160;  // Header plus the largest bitmap accepted.
constexpr uint32_t RETRY_MS = 5 * 60 * 1000;

struct Glyph {
  uint8_t *a8{nullptr};
  uint16_t w{0}, h{0}, adv{0};
  int16_t ofs_y{0};  // Bottom of the bitmap relative to the baseline (LVGL convention).
  uint32_t used{0};  // When it was last looked up (UI task).
};
inline uint64_t key(uint32_t cp, uint16_t size) { return (uint64_t(size) << 32) | cp; }

// UI task only.
inline std::map<uint64_t, Glyph> &ready() { static std::map<uint64_t, Glyph> m; return m; }
inline std::set<uint64_t> &queued() { static std::set<uint64_t> s; return s; }
inline std::set<uint64_t> &absent() { static std::set<uint64_t> s; return s; }  // No font has it.
inline std::map<uint64_t, uint32_t> &failed() { static std::map<uint64_t, uint32_t> m; return m; }
// Bumped whenever glyphs arrive, so views that size themselves to their text can lay out again.
inline uint32_t &generation() { static uint32_t g = 0; return g; }

// Worker hand-off.
struct Result { uint64_t key; Glyph glyph; bool absent; };
inline std::mutex &mutex() { static std::mutex m; return m; }
inline std::deque<uint64_t> &requests() { static std::deque<uint64_t> d; return d; }
inline std::vector<Result> &results() { static std::vector<Result> v; return v; }

// Which characters are worth asking for: not controls, private use (color
// emoji are drawn as private-use glyphs) or emoji joiners. Symbols and emoji
// are asked for too: onx_emoji replaces the sequences it recognizes before
// LVGL looks them up, so this only draws what it leaves (dingbats such as
// ⚙ ✔ ➜, and emoji in labels without color emoji) instead of a box.
inline bool wanted(uint32_t cp) {
  if (cp < 0xA0 || (cp >= 0xD800 && cp < 0xF900) || cp >= 0xF0000) return false;
  if (cp == 0xFE0F || cp == 0xFE0E || cp == 0x200D) return false;
  return true;
}

// Parses "<w><h><adv><ascent>" (uint16 LE) + w*h alpha. Blank rows at the top
// and bottom are dropped so the bitmap hugs the glyph.
inline bool parse(const uint8_t *data, size_t size, Glyph &out) {
  if (size < 8) return false;
  const uint16_t w = data[0] | (data[1] << 8), h = data[2] | (data[3] << 8);
  const uint16_t adv = data[4] | (data[5] << 8), ascent = data[6] | (data[7] << 8);
  if (w == 0 || h == 0 || w > 160 || h > 160 || size != 8 + size_t(w) * h || ascent > h) return false;
  const uint8_t *rows = data + 8;
  auto blank = [&](int y) { for (int x = 0; x < w; ++x) if (rows[y * w + x]) return false; return true; };
  int top = 0, bottom = h;
  while (top < bottom && blank(top)) ++top;
  while (bottom > top && blank(bottom - 1)) --bottom;
  out = Glyph{};
  out.adv = adv;
  if (top == bottom) { out.w = 0; out.h = 0; out.ofs_y = 0; return true; }  // A space.
  out.w = w; out.h = uint16_t(bottom - top);
#ifdef ONX_GLYPHS_FETCH
  out.a8 = static_cast<uint8_t *>(heap_caps_malloc(size_t(out.w) * out.h, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
#else
  out.a8 = static_cast<uint8_t *>(malloc(size_t(out.w) * out.h));
#endif
  if (out.a8 == nullptr) return false;
  std::memcpy(out.a8, rows + size_t(top) * w, size_t(out.w) * out.h);
  out.ofs_y = int16_t(int(ascent) - bottom);  // Rows below the baseline are negative.
  return true;
}

#ifdef ONX_GLYPHS_FETCH
inline TaskHandle_t &worker() { static TaskHandle_t t = nullptr; return t; }
inline bool fetch(const std::string &url, std::vector<uint8_t> &body, int &status) {
  esp_http_client_config_t config{};
  config.url = url.c_str();
  config.timeout_ms = 3000;
  config.disable_auto_redirect = true;
  config.buffer_size = 1024;
  config.keep_alive_enable = true;
  config.user_agent = "RoomCompanion/glyphs";
  auto client = esp_http_client_init(&config);
  if (client == nullptr) return false;
  bool ok = false;
  status = 0;
  if (esp_http_client_open(client, 0) == ESP_OK) {
    const int64_t length = esp_http_client_fetch_headers(client);
    status = esp_http_client_get_status_code(client);
    if (status == 200 && length >= 0 && length <= int64_t(MAX_BYTES)) {
      body.resize(size_t(length));
      size_t got = 0;
      while (got < body.size()) {
        const int n = esp_http_client_read(client, reinterpret_cast<char *>(body.data()) + got, int(body.size() - got));
        if (n <= 0) break;
        got += size_t(n);
      }
      ok = got == body.size();
    } else {
      ok = status != 0;  // Home Assistant answered (404: no font has it).
    }
  }
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  return ok;
}
inline void work(void *) {
  std::vector<uint8_t> body;
  for (;;) {
    uint64_t k;
    {
      std::lock_guard<std::mutex> g(mutex());
      if (requests().empty()) k = 0;
      else { k = requests().front(); requests().pop_front(); }
    }
    if (k == 0) { ulTaskNotifyTake(pdTRUE, portMAX_DELAY); continue; }
    const std::string base = onx_companion::base();
    Result result{k, {}, false};
    bool answered = false;
    if (!base.empty()) {
      char path[48];
      snprintf(path, sizeof(path), "/glyph/%x.a8h?size=%u", unsigned(k & 0xFFFFFFFFu), unsigned(k >> 32));
      int status = 0;
      body.clear();
      answered = fetch(base + path, body, status);
      if (!answered) onx_companion::fail();
      else if (status == 200) answered = parse(body.data(), body.size(), result.glyph);
      else result.absent = status == 404;
    }
    std::lock_guard<std::mutex> g(mutex());
    results().push_back(result);
  }
}
inline void start_worker() {
  if (worker() != nullptr) return;
  // The stack lives in PSRAM: internal RAM is kept for Wi-Fi and the display.
  if (xTaskCreatePinnedToCoreWithCaps(work, "onx_glyphs", 8192, nullptr, 1, &worker(), tskNO_AFFINITY,
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) != pdPASS)
    worker() = nullptr;
}
#endif

inline uint32_t now_ms() { return onx_companion::now_ms(); }
// Glyphs on screen go to the front of the queue (urgent), prefetches to the back.
inline void request(uint64_t k, bool urgent = true) {
#ifdef ONX_GLYPHS_FETCH
  if (queued().count(k)) {
    if (!urgent) return;
    std::lock_guard<std::mutex> g(mutex());
    auto &q = requests();
    for (auto it = q.begin(); it != q.end(); ++it)
      if (*it == k) { q.erase(it); q.push_front(k); break; }
    return;
  }
  if (queued().size() >= QUEUE_LIMIT) return;
  auto f = failed().find(k);
  if (f != failed().end()) {
    if (now_ms() - f->second < RETRY_MS) return;
    failed().erase(f);
  }
  if (!onx_companion::active()) return;
  start_worker();
  if (worker() == nullptr) return;
  queued().insert(k);
  {
    std::lock_guard<std::mutex> g(mutex());
    if (urgent) requests().push_front(k); else requests().push_back(k);
  }
  xTaskNotifyGive(worker());
#else
  (void) k; (void) urgent;
#endif
}

// The glyph font of one size. dsc points at its request size.
inline bool get_glyph_dsc(const lv_font_t *font, lv_font_glyph_dsc_t *dsc, uint32_t letter, uint32_t) {
  if (!wanted(letter)) return false;
  const uint16_t size = uint16_t(reinterpret_cast<uintptr_t>(font->user_data));
  const uint64_t k = key(letter, size);
  auto found = ready().find(k);
  if (found == ready().end()) {
    if (!absent().count(k)) request(k);
    return false;
  }
  Glyph &g = found->second;
  g.used = now_ms();
  dsc->adv_w = g.adv;
  dsc->box_w = g.w;
  dsc->box_h = g.h;
  dsc->ofs_x = 0;
  dsc->ofs_y = g.ofs_y;
  dsc->stride = 0;
  dsc->format = LV_FONT_GLYPH_FORMAT_A8;
  dsc->is_placeholder = 0;
  dsc->gid.src = &g;
  return true;
}
inline const void *get_glyph_bitmap(lv_font_glyph_dsc_t *dsc, lv_draw_buf_t *buf) {
  const auto *g = static_cast<const Glyph *>(dsc->gid.src);
  if (g == nullptr || g->a8 == nullptr) return nullptr;
  if (dsc->req_raw_bitmap || buf == nullptr) return g->a8;
  const uint32_t stride = buf->header.stride ? buf->header.stride : g->w;
  auto *out = static_cast<uint8_t *>(buf->data);
  for (uint16_t y = 0; y < g->h; ++y) std::memcpy(out + size_t(y) * stride, g->a8 + size_t(y) * g->w, g->w);
  return buf;
}

inline std::map<int32_t, lv_font_t *> &fonts() { static std::map<int32_t, lv_font_t *> m; return m; }
inline bool is_glyph_font(const lv_font_t *f) {
  for (const auto &entry : fonts()) if (entry.second == f) return true;
  return false;
}
inline void poll(lv_timer_t *);
// Appends the glyph font for this line height to the end of font's fallback chain.
inline void attach(const lv_font_t *font) {
#ifdef ONX_GLYPHS_FETCH
  if (font == nullptr || font->line_height <= 0) return;
  const lv_font_t *last = font;
  for (int depth = 0; last->fallback != nullptr && depth < 8; ++depth) {
    if (is_glyph_font(last->fallback)) return;  // Already attached.
    last = last->fallback;
  }
  if (is_glyph_font(last) || last->fallback != nullptr) return;
  // Only fonts built in RAM (ESPHome's); LVGL's built-in fonts are read-only flash.
  if (!esp_ptr_internal(last) && !esp_ptr_external_ram(last)) return;
  auto &slot = fonts()[font->line_height];
  if (slot == nullptr) {
    slot = static_cast<lv_font_t *>(lv_malloc_zeroed(sizeof(lv_font_t)));
    if (slot == nullptr) { fonts().erase(font->line_height); return; }
    slot->get_glyph_dsc = get_glyph_dsc;
    slot->get_glyph_bitmap = get_glyph_bitmap;
    slot->line_height = font->line_height;
    slot->base_line = font->base_line;
    // Noto's em is about 3/4 of these fonts' line height.
    slot->user_data = reinterpret_cast<void *>(uintptr_t((font->line_height * 3 + 2) / 4));
    if (fonts().size() == 1) lv_timer_create(poll, 250, nullptr);
  }
  // Fonts are built at run time and owned by the firmware; LVGL only reads them.
  const_cast<lv_font_t *>(last)->fallback = slot;
#else
  (void) font;  // Emulator and host builds: nothing to download.
#endif
}

// Drops the least recently drawn glyphs once the cache is full, never ones
// drawn in the last IN_USE_MS (LVGL may still hold them for a pending frame).
inline void trim() {
  auto &m = ready();
  if (m.size() <= GLYPH_LIMIT) return;
  const uint32_t now = now_ms();
  std::vector<std::pair<uint32_t, uint64_t>> old;  // (age, key)
  for (const auto &entry : m) {
    const uint32_t age = now - entry.second.used;
    if (age >= IN_USE_MS) old.emplace_back(age, entry.first);
  }
  const size_t drop = std::min(old.size(), m.size() - GLYPH_KEEP);
  std::partial_sort(old.begin(), old.begin() + drop, old.end(),
                    [](const auto &a, const auto &b) { return a.first > b.first; });
  for (size_t i = 0; i < drop; ++i) {
    auto it = m.find(old[i].second);
    if (it->second.a8 != nullptr) free(it->second.a8);
    m.erase(it);
  }
}

inline uint32_t next_utf8(const std::string &s, size_t &i) {
  const uint8_t c = uint8_t(s[i++]);
  int extra = c >= 0xF0 ? 3 : c >= 0xE0 ? 2 : c >= 0xC0 ? 1 : 0;
  uint32_t cp = extra == 3 ? c & 0x07 : extra == 2 ? c & 0x0F : extra == 1 ? c & 0x1F : c;
  for (; extra > 0 && i < s.size() && (uint8_t(s[i]) & 0xC0) == 0x80; --extra) cp = (cp << 6) | (uint8_t(s[i++]) & 0x3F);
  return cp;
}
// Queues the glyphs text needs in font that its fonts lack, behind the ones on screen.
inline void prefetch(const std::string &text, const lv_font_t *font) {
#ifdef ONX_GLYPHS_FETCH
  if (font == nullptr || text.empty() || !onx_companion::active()) return;
  attach(font);
  auto slot = fonts().find(font->line_height);
  if (slot == fonts().end()) return;
  const uint16_t size = uint16_t(reinterpret_cast<uintptr_t>(slot->second->user_data));
  for (size_t i = 0; i < text.size();) {
    const uint32_t cp = next_utf8(text, i);
    if (!wanted(cp)) continue;
    const uint64_t k = key(cp, size);
    auto found = ready().find(k);
    if (found != ready().end()) { found->second.used = now_ms(); continue; }
    if (absent().count(k) || queued().count(k)) continue;
    bool covered = false;
    lv_font_glyph_dsc_t dsc;
    for (const lv_font_t *f = font; f != nullptr && !is_glyph_font(f) && !covered; f = f->fallback)
      covered = f->get_glyph_dsc != nullptr && f->get_glyph_dsc(f, &dsc, cp, 0);
    if (!covered) request(k, false);
  }
#else
  (void) text; (void) font;
#endif
}

// Takes finished downloads; when glyphs arrived, labels lay out again.
inline void poll(lv_timer_t *) {
  std::vector<Result> done;
  {
    std::lock_guard<std::mutex> g(mutex());
    done.swap(results());
  }
  bool arrived = false;
  for (auto &r : done) {
    queued().erase(r.key);
    if (r.absent) { absent().insert(r.key); continue; }
    if (r.glyph.a8 == nullptr && r.glyph.adv == 0) { failed()[r.key] = now_ms(); continue; }
    r.glyph.used = now_ms();
    auto &slot = ready()[r.key];
    if (slot.a8 != nullptr && slot.a8 != r.glyph.a8) free(slot.a8);
    slot = r.glyph;
    arrived = true;
  }
  if (!arrived) return;
  trim();
  ++generation();
  lv_obj_report_style_change(nullptr);
}
}  // namespace onx_glyphs
