#pragma once
#include <cstdint>
struct lv_image_dsc_t {const uint8_t *data;};
struct lv_obj_t {const lv_image_dsc_t *source=nullptr;};
constexpr int LV_OPA_COVER=255;
inline uint32_t lv_color_hex(uint32_t color){return color;}
inline const void *lv_img_get_src(lv_obj_t *widget){return widget->source;}
inline void lv_img_set_src(lv_obj_t *widget,const lv_image_dsc_t *source){widget->source=source;}
inline void lv_obj_set_style_image_recolor(lv_obj_t *,uint32_t,int){}
inline void lv_obj_set_style_image_recolor_opa(lv_obj_t *,int,int){}
namespace esphome::image {
enum ImageType {IMAGE_TYPE_RGB};enum Transparency {TRANSPARENCY_ALPHA_CHANNEL};
class Image {
 lv_image_dsc_t descriptor_;
 public:
 Image(const uint8_t *pixels,int,int,ImageType,Transparency):descriptor_{pixels}{}
 lv_image_dsc_t *get_lv_image_dsc(){return &descriptor_;}
};
}
