#pragma once
#include "esphome/core/component.h"
#include "onx_icon_state.h"
#include "esphome/core/helpers.h"
#include "esphome/components/image/image.h"
#include <map>
#include <memory>
#include <string>

namespace esphome::onx_icons {
class IconDownloader : public Component {
 public:
  void loop() override;
  void retry() { failures_.clear(); errors_.clear(); }
  std::string status(const std::string &icon) const;
  std::string error(const std::string &icon) const {
    auto found = errors_.find(icon); return found == errors_.end() ? "" : found->second;
  }

  // Stable per-widget ownership keeps LVGL descriptors alive across cache eviction.
  void bind(lv_obj_t *widget, const std::string &icon, image::Image *fallback, bool active = false);
 protected:
  struct Bitmap {
    uint8_t *pixels;
    image::Image image;
    uint32_t used{0};
    explicit Bitmap(uint8_t *p);
    ~Bitmap();
  };
  struct Work { IconDownloader *owner; std::string icon; };
  static void download_task_(void *arg);
  void request_(const std::string &icon);
  std::map<std::string, std::shared_ptr<Bitmap>> cache_;
  std::map<lv_obj_t *, std::shared_ptr<Bitmap>> bindings_;
  std::map<std::string, ::onx_icon_state::Retry> failures_;
  std::map<std::string, std::string> errors_;
  Mutex mutex_;
  bool busy_{false}, completed_{false};
  std::string completed_icon_, completed_error_;
  uint8_t *completed_pixels_{nullptr};
};
}  // namespace esphome::onx_icons
