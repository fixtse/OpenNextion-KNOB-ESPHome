#pragma once
#include <cstring>
#include "lvgl.h"

// Avoid reallocating label buffers or invalidating image caches for unchanged UI.
namespace onx_render {
inline void label(lv_obj_t *widget, const char *text) {
  if (std::strcmp(lv_label_get_text(widget), text) != 0) lv_label_set_text(widget, text);
}
inline void image(lv_obj_t *widget, const void *source) {
  if (lv_img_get_src(widget) != source) lv_img_set_src(widget, source);
}
}
