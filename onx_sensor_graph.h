#pragma once
#include "onx_sensor_view.h"
#ifdef USE_LVGL
namespace onx_sensor_graph {
inline lv_obj_t *canvas=nullptr,*minimum_label=nullptr,*maximum_label=nullptr;
inline int plot_left=42;
struct Pixel {int16_t x,y;uint32_t epoch;};
inline std::array<Pixel,73> pixels;
inline size_t count=0;
inline uint32_t ink=0x62B0FF;
inline void draw(lv_event_t *event) {
  lv_area_t area;lv_obj_get_coords(canvas,&area);
  auto *layer=lv_event_get_layer(event);
  lv_draw_line_dsc_t line;lv_draw_line_dsc_init(&line);line.width=1;line.color=lv_color_hex(0x344653);
  for(int y : {0,32,64}) {
    line.p1.x=area.x1+plot_left;line.p1.y=area.y1+y;line.p2.x=area.x1+185;line.p2.y=area.y1+y;
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
inline void render(lv_obj_t *parent,const std::string &entity,uint32_t now,uint32_t color,const lv_font_t *font) {
  const int i=onx_sensor_view::index(entity);
  if(i<0||!onx_sensor_view::configs[i].graph){hide();return;}
  if(!canvas) {
    canvas=lv_obj_create(parent);lv_obj_set_size(canvas,186,65);lv_obj_align(canvas,LV_ALIGN_TOP_MID,0,104);
    lv_obj_set_style_bg_opa(canvas,LV_OPA_TRANSP,0);lv_obj_set_style_border_width(canvas,0,0);
    lv_obj_set_style_pad_all(canvas,0,0);lv_obj_clear_flag(canvas,LV_OBJ_FLAG_SCROLLABLE);lv_obj_clear_flag(canvas,LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(canvas,draw,LV_EVENT_DRAW_MAIN,nullptr);
    minimum_label=lv_label_create(canvas);maximum_label=lv_label_create(canvas);
    for(auto *label:{minimum_label,maximum_label}) {
      lv_obj_set_style_text_font(label,font,0);
      lv_obj_set_style_text_color(label,lv_color_hex(0xD2D2D2),0);
      lv_obj_set_style_text_align(label,LV_TEXT_ALIGN_RIGHT,0);
      lv_label_set_long_mode(label,LV_LABEL_LONG_DOT);
    }
  }
  lv_obj_clear_flag(canvas,LV_OBJ_FLAG_HIDDEN);ink=color;count=0;
  const auto history=onx_sensor_view::states[i].history(now);
  lv_obj_add_flag(minimum_label,LV_OBJ_FLAG_HIDDEN);
  lv_obj_add_flag(maximum_label,LV_OBJ_FLAG_HIDDEN);
  if(!history.empty()) {
    float low=history.front().value,high=low;
    for(const auto &p:history){low=std::min(low,p.value);high=std::max(high,p.value);}
    const auto labels=onx_sensor_view::axis_labels(low,high);
    lv_label_set_text(minimum_label,labels.first.c_str());
    lv_label_set_text(maximum_label,labels.second.c_str());
    lv_point_t lower_size,upper_size;
    lv_text_get_size(&lower_size,labels.first.c_str(),font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    lv_text_get_size(&upper_size,labels.second.c_str(),font,0,0,LV_COORD_MAX,LV_TEXT_FLAG_NONE);
    plot_left=std::min(66,std::max(lower_size.x,upper_size.x)+5);
    for(auto *label:{minimum_label,maximum_label}) {
      lv_obj_set_width(label,plot_left-5);
      lv_obj_clear_flag(label,LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_align(maximum_label,LV_ALIGN_TOP_LEFT,0,0);
    lv_obj_align(minimum_label,LV_ALIGN_BOTTOM_LEFT,0,0);
    for(const auto &p:history) {
      if(count>=pixels.size())break;
      pixels[count++]={int16_t(185-(now-p.epoch)*(185-plot_left)/43200),int16_t(high>low?64-(p.value-low)*64/(high-low):32),p.epoch};
    }
  }
  lv_obj_invalidate(canvas);
}
}
#endif
