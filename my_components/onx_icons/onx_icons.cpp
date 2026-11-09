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
#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"
#define NANOSVGRAST_IMPLEMENTATION
#include "nanosvgrast.h"

namespace esphome::onx_icons {
static constexpr size_t SIZE = 80;
static constexpr size_t MAX_SVG_BYTES = 32768;
static constexpr size_t CACHE_SIZE = 12;
static constexpr uint32_t RETRY_MS = 300000;
static const char *const TAG = "onx_icons";

// Only a validated MDI name reaches the fixed HTTPS endpoint. Never send HA credentials.
static uint8_t *fetch_bitmap(const std::string &icon) {
  const std::string url = ::onx_icon_state::download_url(icon);
  esp_http_client_config_t config{};
  config.url = url.c_str();
  config.crt_bundle_attach = esp_crt_bundle_attach;
  config.timeout_ms = 5000;
  config.disable_auto_redirect = true;
  config.buffer_size = 1024;
  auto client = esp_http_client_init(&config);
  if (client == nullptr) return nullptr;
  std::string svg;
  bool okay = false;
  const uint32_t started = millis();
  if (esp_http_client_open(client, 0) == ESP_OK) {
    const int64_t length = esp_http_client_fetch_headers(client);
    if (esp_http_client_get_status_code(client) == 200 && (length < 0 || length <= static_cast<int64_t>(MAX_SVG_BYTES))) {
      char chunk[1024];
      while (millis() - started < 10000 && svg.size() <= MAX_SVG_BYTES) {
        const int count = esp_http_client_read(client, chunk, sizeof(chunk));
        if (count <= 0) break;
        svg.append(chunk, count);
      }
      okay = esp_http_client_is_complete_data_received(client) && !svg.empty() && svg.size() <= MAX_SVG_BYTES;
    }
  }
  esp_http_client_close(client);
  esp_http_client_cleanup(client);
  if (!okay || svg.find("<svg") == std::string::npos) return nullptr;
  svg.push_back('\0');
  auto *parsed = nsvgParse(svg.data(), "px", 96);
  if (!parsed) return nullptr;
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
      for (size_t i = 0; i < SIZE * SIZE; ++i) pixels[4 * i] = pixels[4 * i + 1] = pixels[4 * i + 2] = 255;
    } else {
      heap_caps_free(pixels); pixels = nullptr;
    }
    nsvgDeleteRasterizer(rasterizer);
  }
  nsvgDelete(parsed);
  return pixels;
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
  auto *pixels = fetch_bitmap(work->icon);
  {
    LockGuard lock(work->owner->mutex_);
    work->owner->completed_icon_ = work->icon;
    work->owner->completed_pixels_ = pixels;
    work->owner->completed_ = true;
  }
  work.reset();
  vTaskDelete(nullptr);
}
void IconDownloader::request_(const std::string &icon) {
  if (busy_ || !::onx_icon_state::valid_icon(icon) || wifi::global_wifi_component == nullptr || !wifi::global_wifi_component->is_connected()) return;
  auto failed = failures_.find(icon);
  if (failed != failures_.end() && millis() - failed->second < RETRY_MS) return;
  auto *work = new (std::nothrow) Work{this, icon};
  if (!work) return;
  busy_ = true;
  if (xTaskCreate(download_task_, "onx_icon", 16384, work, 1, nullptr) != pdPASS) {
    busy_ = false; delete work; failures_[icon] = millis();
  }
}
void IconDownloader::loop() {
  std::string icon;
  uint8_t *pixels;
  {
    LockGuard lock(mutex_);
    if (!completed_) return;
    icon = std::move(completed_icon_); pixels = completed_pixels_;
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
    bitmap->used = millis(); cache_[icon] = std::move(bitmap); failures_.erase(icon);
  } else {
    if (failures_.size() >= 16) failures_.erase(failures_.begin());
    failures_[icon] = millis();
    ESP_LOGW(TAG, "Icon download unavailable; retaining entity fallback");
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
