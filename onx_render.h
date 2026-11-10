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
// Share the Settings list appearance with other rotary menus.
inline void settings_roller(lv_obj_t *roller,const lv_font_t *normal_font,const lv_font_t *selected_font,
                            bool reserve_arrow=true) {
  lv_obj_set_style_bg_opa(roller,LV_OPA_TRANSP,LV_PART_MAIN);
  lv_obj_set_style_border_width(roller,0,LV_PART_MAIN);
  lv_obj_set_style_text_color(roller,lv_color_hex(0x4D4D4D),LV_PART_MAIN);
  lv_obj_set_style_text_font(roller,normal_font,LV_PART_MAIN);
  lv_obj_set_style_text_line_space(roller,15,LV_PART_MAIN);
  lv_obj_set_style_bg_color(roller,lv_color_hex(0x3D3D3D),LV_PART_SELECTED);
  lv_obj_set_style_bg_opa(roller,LV_OPA_COVER,LV_PART_SELECTED);
  lv_obj_set_style_radius(roller,15,LV_PART_SELECTED);
  lv_obj_set_style_text_color(roller,lv_color_white(),LV_PART_SELECTED);
  lv_obj_set_style_text_font(roller,selected_font,LV_PART_SELECTED);
  lv_obj_set_style_pad_left(roller,20,LV_PART_SELECTED);
  lv_obj_set_style_pad_right(roller,reserve_arrow?40:20,LV_PART_SELECTED);
}

}
