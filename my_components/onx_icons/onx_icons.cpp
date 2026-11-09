#include "onx_icons.h"
#include "onx_icon_state.h"
#include "esphome/components/wifi/wifi_component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include "src/misc/cache/instance/lv_image_header_cache.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <new>
#include <ctime>
// NanoSVG's parser alone requires a large contiguous allocation. Keep all SVG
// working memory in external RAM rather than the constrained internal/DMA heap.
static void *onx_svg_malloc(size_t size) { return heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
static void *onx_svg_realloc(void *p, size_t size) { return heap_caps_realloc(p, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); }
#define NSVG_MALLOC onx_svg_malloc
#define NSVG_REALLOC onx_svg_realloc
#define NSVG_FREE heap_caps_free
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"
#undef NSVG_MALLOC
#undef NSVG_REALLOC
#undef NSVG_FREE

namespace esphome::onx_icons {
static constexpr size_t SIZE = 80;
static constexpr size_t MAX_SVG_BYTES = 32768;
static constexpr size_t CACHE_SIZE = 12;
static const char *const TAG = "onx_icons";

// Both endpoints are fixed HTTPS hosts; redirects and device credentials are never used.
static bool fetch_svg(const std::string &url, std::string &svg, std::string &error) {
  esp_http_client_config_t config{};
  config.url = url.c_str();
  config.crt_bundle_attach = esp_crt_bundle_attach;
  config.timeout_ms = 5000;
  config.disable_auto_redirect = true;
  config.buffer_size = 1024;
  config.user_agent = "ONX2424G013/MDI-icons";
  auto client = esp_http_client_init(&config);
  if (client == nullptr) { error = "HTTP client allocation failed"; return false; }
  esp_http_client_set_header(client, "Accept", "image/svg+xml");
  esp_http_client_set_header(client, "Accept-Encoding", "identity");
  svg.clear();
  bool okay = false;
  const uint32_t started = millis();
  const auto opened = esp_http_client_open(client, 0);
  if (opened == ESP_OK) {
    const int64_t length = esp_http_client_fetch_headers(client);
    const int status = esp_http_client_get_status_code(client);
    if (status == 200 && (length < 0 || length <= static_cast<int64_t>(MAX_SVG_BYTES))) {
      char chunk[1024];
      while (millis() - started < 10000 && svg.size() <= MAX_SVG_BYTES) {
        const int count = esp_http_client_read(client, chunk, sizeof(chunk));
        if (count < 0) break;
        if (count > 0) svg.append(chunk, count);
        if (esp_http_client_is_complete_data_received(client)) { okay = true; break; }
        // A zero-byte read is not necessarily EOF for a chunked response.
        if (count == 0) vTaskDelay(1);
      }
      okay &= !svg.empty() && svg.size() <= MAX_SVG_BYTES && svg.find("<svg") != std::string::npos;
      if (!okay) error = "incomplete, oversized, or invalid SVG response";
    } else error = "HTTP " + std::to_string(status) + " or oversized icon";
  } else error = std::string("HTTPS connect: ") + esp_err_to_name(opened);
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  return okay;
}
static uint8_t *raster_bitmap(std::string svg, std::string &error) {
  svg.push_back('\0');
  auto *parsed = nsvgParse(svg.data(), "px", 96);
  if (!parsed) { error = "SVG parser PSRAM allocation failed"; return nullptr; }
  uint8_t *pixels = nullptr;
  if (parsed->shapes && std::isfinite(parsed->width) && std::isfinite(parsed->height) &&
      parsed->width > 0 && parsed->height > 0 && parsed->width <= 4096 && parsed->height <= 4096) {
    pixels = static_cast<uint8_t *>(heap_caps_calloc(SIZE * SIZE, 4, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    auto *rasterizer = nsvgCreateRasterizer();
    if (pixels && rasterizer) {
      const float scale = std::min(SIZE / parsed->width, SIZE / parsed->height);
      nsvgRasterize(rasterizer, parsed, (SIZE - parsed->width * scale) / 2,
                    (SIZE - parsed->height * scale) / 2, scale, pixels, SIZE, SIZE, SIZE * 4);
      // Monochrome icons: BGRA and RGBA have identical white channels for LVGL.
      bool visible = false;
      for (size_t i = 0; i < SIZE * SIZE; ++i) {
        visible |= pixels[4 * i + 3] != 0;
        pixels[4 * i] = pixels[4 * i + 1] = pixels[4 * i + 2] = 255;
      }
      if (!visible) { heap_caps_free(pixels); pixels = nullptr; }
    } else {
      heap_caps_free(pixels); pixels = nullptr;
    }
    nsvgDeleteRasterizer(rasterizer);
  }
  nsvgDelete(parsed);
  if (pixels == nullptr) error = "SVG dimensions invalid or raster allocation failed";
  return pixels;
}
static uint8_t *fetch_bitmap(const std::string &icon, std::string &error) {
  for (const auto &url : {::onx_icon_state::download_url(icon), ::onx_icon_state::backup_url(icon)}) {
    std::string svg;
    if (fetch_svg(url, svg, error)) {
      if (auto *pixels = raster_bitmap(std::move(svg), error)) return pixels;
    }
    ESP_LOGW(TAG, "%s icon source failed (%s)", icon.c_str(), error.c_str());
  }
  return nullptr;
}
IconDownloader::Bitmap::Bitmap(uint8_t *p)
    : pixels(p), image(p, SIZE, SIZE, esphome::image::IMAGE_TYPE_RGB, esphome::image::TRANSPARENCY_ALPHA_CHANNEL) {}
IconDownloader::Bitmap::~Bitmap() {
  // Bitmap destruction only occurs in loop()/bind(), on the UI task.
  lv_image_cache_drop(image.get_lv_image_dsc());
  lv_image_header_cache_drop(image.get_lv_image_dsc());
  heap_caps_free(pixels);
}
void IconDownloader::download_task_(void *arg) {
  std::unique_ptr<Work> work(static_cast<Work *>(arg));
  std::string error;
  auto *pixels = fetch_bitmap(work->icon, error);
  {
    LockGuard lock(work->owner->mutex_);
    work->owner->completed_icon_ = work->icon;
    work->owner->completed_error_ = std::move(error);
    work->owner->completed_pixels_ = pixels;
    work->owner->completed_ = true;
  }
  work.reset();
  vTaskDelete(nullptr);
}
std::string IconDownloader::status(const std::string &icon) const {
  if (!::onx_icon_state::valid_icon(icon)) return "missing_icon_metadata";
  if (cache_.find(icon) != cache_.end()) return "ready";
  if (failures_.find(icon) != failures_.end()) return "retry_wait";
  if (wifi::global_wifi_component == nullptr || !wifi::global_wifi_component->is_connected()) return "waiting_for_wifi";
  if (std::time(nullptr) < 1577836800) return "waiting_for_clock";
  return busy_ ? "download_busy" : "awaiting_display";
}
void IconDownloader::request_(const std::string &icon) {
  if (busy_ || !::onx_icon_state::valid_icon(icon) || wifi::global_wifi_component == nullptr || !wifi::global_wifi_component->is_connected()) return;
  // Starting before wall-clock synchronization can make certificate validation fail.
  // Wait instead of poisoning the icon retry cache during normal boot.
  if (std::time(nullptr) < 1577836800) return;
  auto failed = failures_.find(icon);
  if (failed != failures_.end() && !failed->second.due(millis())) return;
  auto *work = new (std::nothrow) Work{this, icon};
  if (!work) return;
  busy_ = true;
  ESP_LOGI(TAG, "Downloading %s", icon.c_str());
  if (xTaskCreate(download_task_, "onx_icon", 16384, work, 1, nullptr) != pdPASS) {
    busy_ = false; delete work; failures_[icon].fail(millis());
  }
}
void IconDownloader::loop() {
  std::string icon, error;
  uint8_t *pixels;
  {
    LockGuard lock(mutex_);
    if (!completed_) return;
    icon = std::move(completed_icon_); pixels = completed_pixels_;
    error = std::move(completed_error_);
    completed_pixels_ = nullptr; completed_ = false;
  }
  busy_ = false;
  if (pixels) {
    if (cache_.size() >= CACHE_SIZE) {
      const uint32_t now = millis();
      auto oldest = std::max_element(cache_.begin(), cache_.end(), [&](const auto &a, const auto &b) {
        return now - a.second->used < now - b.second->used;
      });
      cache_.erase(oldest); // Existing widgets retain their shared ownership.
    }
    auto bitmap = std::make_shared<Bitmap>(pixels);
    bitmap->used = millis(); cache_[icon] = std::move(bitmap); failures_.erase(icon); errors_.erase(icon);
    ESP_LOGI(TAG, "Downloaded and rendered %s", icon.c_str());
  } else {
    if (failures_.size() >= 16) { errors_.erase(failures_.begin()->first); failures_.erase(failures_.begin()); }
    errors_[icon] = error;
    failures_[icon].fail(millis());
    ESP_LOGW(TAG, "%s unavailable (%s); retaining fallback, retry in %ums",
             icon.c_str(), error.c_str(), static_cast<unsigned>(failures_[icon].delay));
  }
}
void IconDownloader::bind(lv_obj_t *widget, const std::string &icon, image::Image *fallback, bool active) {
  if (!widget || !fallback) return;
  auto found = cache_.find(icon);
  auto bitmap = found == cache_.end() ? std::shared_ptr<Bitmap>{} : found->second;
  if (bitmap) bitmap->used = millis();
  // Assign the descriptor before releasing the previous widget's backing bitmap.
  lv_img_set_src(widget, bitmap ? bitmap->image.get_lv_image_dsc() : fallback->get_lv_image_dsc());
  bindings_[widget] = bitmap;
  lv_obj_set_style_image_recolor(widget, lv_color_hex(active ? 0x62B0FF : 0xFFFFFF), 0);
  lv_obj_set_style_image_recolor_opa(widget, LV_OPA_COVER, 0);
  if (!bitmap) request_(icon);
}
}  // namespace esphome::onx_icons
