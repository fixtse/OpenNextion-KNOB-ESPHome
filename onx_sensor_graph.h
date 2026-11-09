#pragma once
#include "onx_sensor_view.h"
#ifdef USE_LVGL
namespace onx_sensor_graph {
inline lv_obj_t *canvas=nullptr;
struct Pixel {int16_t x,y;uint32_t epoch;};
inline std::array<Pixel,73> pixels;
inline size_t count=0;
inline uint32_t ink=0x62B0FF;
inline void draw(lv_event_t *event) {
  lv_area_t area;lv_obj_get_coords(canvas,&area);
  auto *layer=lv_event_get_layer(event);
  lv_draw_line_dsc_t line;lv_draw_line_dsc_init(&line);line.width=1;line.color=lv_color_hex(0x344653);
  for(int y : {0,32,64}) {
    line.p1.x=area.x1;line.p1.y=area.y1+y;line.p2.x=area.x1+185;line.p2.y=area.y1+y;
    lv_draw_line(layer,&line);
  }
  line.width=2;line.color=lv_color_hex(ink);
  for(size_t i=1;i<count;++i) {
    if(pixels[i].epoch-pixels[i-1].epoch>1200)continue;
    line.p1.x=area.x1+pixels[i-1].x;line.p1.y=area.y1+pixels[i-1].y;
    line.p2.x=area.x1+pixels[i].x;line.p2.y=area.y1+pixels[i].y;
    lv_draw_line(layer,&line);
  }
}
inline void hide() {if(canvas)lv_obj_add_flag(canvas,LV_OBJ_FLAG_HIDDEN);}
inline void render(lv_obj_t *parent,const std::string &entity,uint32_t now,uint32_t color) {
  const int i=onx_sensor_view::index(entity);
  if(i<0||!onx_sensor_view::configs[i].graph){hide();return;}
  if(!canvas) {
    canvas=lv_obj_create(parent);lv_obj_set_size(canvas,186,65);lv_obj_align(canvas,LV_ALIGN_TOP_MID,0,104);
    lv_obj_set_style_bg_opa(canvas,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(canvas,0,0);
    lv_obj_set_style_pad_all(canvas,0,0);lv_obj_clear_flag(canvas,LV_OBJ_FLAG_SCROLLABLE);lv_obj_clear_flag(canvas,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(canvas,draw,LV_EVENT_DRAW_MAIN,nullptr);
  }
  lv_obj_clear_flag(canvas,LV_OBJ_FLAG_HIDDEN);ink=color;count=0;
  const auto history=onx_sensor_view::states[i].history(now);
  if(!history.empty()) {
    float low=history.front().value,high=low;
    for(const auto &p:history){low=std::min(low,p.value);high=std::max(high,p.value);}
    for(const auto &p:history) {
      if(count>=pixels.size())break;
      pixels[count++]={int16_t(185-(now-p.epoch)*185/43200),int16_t(high>low?64-(p.value-low)*64/(high-low):32),p.epoch};
    }
  }
  lv_obj_invalidate(canvas);
}
}
#endif
