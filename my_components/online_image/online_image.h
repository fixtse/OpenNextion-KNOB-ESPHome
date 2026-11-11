// SPDX-License-Identifier: MIT
// ESPHome 2026.10.2 OnlineImage API with a bounded background network fetch.
#pragma once
#include "esphome/components/http_request/http_request.h"
#include "esphome/components/runtime_image/runtime_image.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"

namespace esphome::online_image {
class OnlineImage final : public PollingComponent,
                          public runtime_image::RuntimeImage,
                          public Parented<http_request::HttpRequestComponent> {
 public:
  OnlineImage(const std::string &url, int width, int height, runtime_image::ImageFormat format,
              image::ImageType type, image::Transparency transparency, image::Image *placeholder,
              uint32_t buffer_size, bool is_big_endian = false);
  void update() override;
  void loop() override;
  void update_if(bool value) { if (value) update(); }
  void set_url(const std::string &url) { if (validate_url_(url)) url_=url; }
  void release();
  bool is_busy() const { return busy_; }
  void set_source_pixel_limit(uint32_t pixels) { source_pixel_limit_=pixels; }
  static bool memory_ready();
  const std::string &last_error() const { return last_error_; }
  template<typename V> void add_request_header(const std::string &header,V value) {
    request_headers_.emplace_back(header,value);
  }
  template<typename F> void add_on_finished_callback(F &&callback) {
    finished_.add(std::forward<F>(callback));
  }
  template<typename F> void add_on_error_callback(F &&callback) {
    failed_.add(std::forward<F>(callback));
  }
 protected:
  struct Work {
    OnlineImage *owner;
    std::string url;
    std::vector<http_request::Header> headers;
    uint32_t generation, timeout;
  };
  static void fetch_task_(void *arg);
  bool validate_url_(const std::string &url);
  bool cancelled_(uint32_t generation);
  std::string url_;
  std::vector<std::pair<std::string,TemplatableValue<std::string>>> request_headers_;
  CallbackManager<void(bool)> finished_;
  CallbackManager<void()> failed_;
  Mutex mutex_;
  bool busy_{false}, completed_{false};
  uint32_t generation_{0}, completed_generation_{0};
  uint8_t *completed_data_{nullptr};
  size_t completed_size_{0};
  uint32_t source_pixel_limit_{0};
  std::string completed_error_, last_error_;
};
}
