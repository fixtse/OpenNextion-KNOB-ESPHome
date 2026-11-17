#pragma once
#include <cstdint>
#include <cstdlib>
#define LV_COLOR_16_SWAP 0
#define LV_COLOR_FORMAT_ARGB8888 1
struct lv_image_dsc_t { struct { unsigned magic, cf, w, h, stride; } header; unsigned data_size; const uint8_t *data; };
struct lv_font_t { uint16_t line_height, base_line; const lv_font_t *fallback; };
#include <string>
struct lv_obj_t { std::string text; int delete_hooks=0; };
struct lv_event_t { lv_obj_t *target=nullptr; };
inline const char *lv_label_get_text(lv_obj_t *o){return o->text.c_str();}
inline int label_sets=0;
inline void lv_label_set_text(lv_obj_t *o,const char *t){o->text=t;label_sets++;}
#define LV_EVENT_DELETE 1
inline void lv_obj_add_event_cb(lv_obj_t *o,void (*)(lv_event_t*),int,void*){o->delete_hooks++;}
inline void *lv_event_get_target(lv_event_t *e){return e->target;}
inline lv_font_t *lv_imgfont_create(uint16_t, const void *(*)(const lv_font_t*,uint32_t,uint32_t,int32_t*,void*), void*) {return nullptr;}
inline const lv_font_t *lv_obj_get_style_text_font(lv_obj_t*,int) {return nullptr;}
inline void lv_obj_set_style_text_font(lv_obj_t*,const lv_font_t*,int) {}
#define LV_PART_MAIN 0
#define LV_IMAGE_HEADER_MAGIC 0x19
#define LV_EVENT_REFR_READY 0
inline void lv_image_cache_drop(const void*) {}
inline void *lv_display_get_default() { return nullptr; }
inline void lv_display_add_event_cb(void*, void (*)(lv_event_t*), int, void*) {}
