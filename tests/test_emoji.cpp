#include "../onx_emoji.h"
#include <cassert>
#include <cstring>
static uint8_t *master(uint8_t b, uint8_t g, uint8_t r) {
  constexpr unsigned N = onx_emoji_queue::SIZE;
  auto *p = static_cast<uint8_t *>(malloc(N * N * 4));
  for (unsigned i = 0; i < N * N; ++i) { p[4*i]=b; p[4*i+1]=g; p[4*i+2]=r; p[4*i+3]=255; }
  return p;
}
static uint32_t index_of(const char *sequence) {
  const auto *found = std::lower_bound(std::begin(onx_emoji::SEQUENCES), std::end(onx_emoji::SEQUENCES), std::string_view(sequence),
    [](const char *s, std::string_view k) { return std::string_view(s) < k; });
  assert(found != std::end(onx_emoji::SEQUENCES) && std::strcmp(*found, sequence) == 0);
  return found - std::begin(onx_emoji::SEQUENCES);
}
int main() {
  assert(onx_emoji::registered && onx_emoji_queue::completed == onx_emoji::downloaded);
  // Table is sorted for prefix search.
  for (size_t i = 1; i < onx_emoji::GLYPH_COUNT; ++i)
    assert(std::string_view(onx_emoji::SEQUENCES[i - 1]) < std::string_view(onx_emoji::SEQUENCES[i]));
  // Noto file names.
  assert(onx_emoji::stem("\xF0\x9F\x98\x80") == "1f600");
  assert(onx_emoji::stem("#\xE2\x83\xA3") == "0023_20e3");
  assert(onx_emoji::stem("\xF0\x9F\x87\xB5\xF0\x9F\x87\xAA") == "1f1f5_1f1ea");
  assert(onx_emoji::stem("\xE2\x9D\xA4\xEF\xB8\x8F") == "2764");

  // Plain text is untouched and queues nothing.
  bool missing = true;
  assert(onx_emoji::encode("Beyonc\xC3\xA9", &missing) == "Beyonc\xC3\xA9" && !missing);
  assert(onx_emoji_queue::pending.empty());

  // Not downloaded yet: text keeps the codepoints (monochrome fallback) and queues once.
  const char *family = "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7\xE2\x80\x8D\xF0\x9F\x91\xA6";
  for (const char *text : {family, "\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBF", "\xF0\x9F\x87\xB5\xF0\x9F\x87\xAA", "1\xEF\xB8\x8F\xE2\x83\xA3"}) {
    auto encoded = onx_emoji::encode(text, &missing);
    assert(missing && !encoded.empty());
  }
  assert(onx_emoji_queue::pending.size() == 4);
  onx_emoji::encode(family);
  assert(onx_emoji_queue::pending.size() == 4);  // Deduplicated while in flight.
  assert(onx_emoji::encode("\xE2\x9D\xA4\xEF\xB8\x8F") == onx_emoji::encode("\xE2\x9D\xA4"));

  // A waiting label is refreshed when its emoji arrives, collapsing to one glyph.
  lv_obj_t label;
  std::string text = std::string("Hi ") + family;
  onx_emoji::set_label(&label, text.c_str());
  assert(label.text == text && label.delete_hooks == 1 && onx_emoji::waiting.count(&label));
  onx_emoji::set_label(&label, text.c_str());
  assert(label.delete_hooks == 1);
  const auto request = onx_emoji_queue::pending.front();
  assert(request.index == index_of(family) && request.stem == "1f468_200d_1f469_200d_1f467_200d_1f466");
  onx_emoji_queue::completed(request.index, master(10, 20, 30));
  assert(label.text.size() == 3 + 4 && label.text.compare(0, 3, "Hi ") == 0 && !onx_emoji::waiting.count(&label));

  // Labels changed by someone else are not overwritten.
  lv_obj_t other;
  onx_emoji::set_label(&other, "\xF0\x9F\x8E\x89 party");
  other.text = "changed";
  onx_emoji::downloaded(index_of("\xF0\x9F\x8E\x89"), master(1, 2, 3));
  assert(other.text == "changed");
  lv_event_t event{&other};
  onx_emoji::forget(&event);
  assert(!onx_emoji::waiting.count(&other) && !onx_emoji::hooked.count(&other));

  // Failures back off instead of re-queueing immediately.
  onx_emoji_queue::pending.clear(); onx_emoji::requested.clear();
  const uint32_t sun = index_of("\xE2\x98\x80");
  onx_emoji::encode("\xE2\x98\x80");
  assert(onx_emoji_queue::pending.size() == 1);
  onx_emoji_queue::pending.clear();
  onx_emoji::downloaded(sun, nullptr);
  onx_emoji::encode("\xE2\x98\x80");
  assert(onx_emoji_queue::pending.empty());
  assert(onx_emoji::failed[sun].attempts == 1);
  // A waiting label retries once the short delay passes (simulated by ageing the failure).
  lv_obj_t sunny;
  onx_emoji::set_label(&sunny, "\xE2\x98\x80 out");
  assert(onx_emoji_queue::pending.empty());
  onx_emoji::failed[sun].at -= std::chrono::seconds(3);
  onx_emoji::last_tick = {};
  onx_emoji_queue::tick();
  assert(onx_emoji_queue::pending.size() == 1 && onx_emoji_queue::pending.front().index == sun);
  onx_emoji_queue::pending.clear();
  onx_emoji::downloaded(sun, nullptr);
  assert(onx_emoji::failed[sun].attempts == 2);
  // Changed labels stop retrying.
  sunny.text = "other";
  onx_emoji::failed[sun].at -= std::chrono::seconds(6);
  onx_emoji::last_tick = {};
  onx_emoji_queue::tick();
  assert(onx_emoji_queue::pending.empty() && !onx_emoji::waiting.count(&sunny));
  assert(onx_emoji::retry_delay(4) == std::chrono::minutes(10));

  // Scaled images keep the LVGL baseline contract and the master color.
  const uint32_t code = onx_emoji::FIRST_GLYPH + index_of(family);
  for (uint16_t height : {14, 16, 24, 32, 48, 60}) {
    for (uint16_t descent : {3, 5, 7}) {
      lv_font_t text_font{height, descent, nullptr};
      for (int repeat = 0; repeat < 2; ++repeat) {
        int32_t offset = 999;
        const auto *image = static_cast<const lv_image_dsc_t *>(onx_emoji::source(&text_font, code, 0, &offset, nullptr));
        assert(image && image->header.w == height && image->data_size == height * height * 4u);
        assert(image->data[0] == 10 && image->data[1] == 20 && image->data[2] == 30 && image->data[3] == 255);
        const int top = height - descent - image->header.h - offset;
        assert(top == 0);
      }
    }
  }
  lv_font_t font{24, 0, nullptr};
  assert(!onx_emoji::source(&font, onx_emoji::FIRST_GLYPH + sun, 0, nullptr, nullptr));

  // Master cache is bounded.
  for (uint32_t i = 0; i < onx_emoji::MASTER_LIMIT + 5; ++i) onx_emoji::downloaded(i, master(0, 0, 0));
  assert(onx_emoji::masters.size() == onx_emoji::MASTER_LIMIT);
  for (uint32_t i = 0; i < 240; ++i) onx_emoji::source(&font, onx_emoji::FIRST_GLYPH + i, 0, nullptr, nullptr);
  onx_emoji::trim_cache(nullptr);
  assert(onx_emoji::cache_bytes <= onx_emoji::CACHE_LIMIT);
}
