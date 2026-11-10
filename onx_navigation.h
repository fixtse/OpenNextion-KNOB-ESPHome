#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <vector>
#include "lvgl.h"

namespace onx_navigation {
constexpr int VISIBLE_ICONS=5;
struct Point {int x,y;};
inline Point arc_point(int offset_milli) {
  const float angle=offset_milli*(17.0f*3.14159265f/180000.0f);
  return {120+static_cast<int>(std::lround(102*std::sin(angle))),
          120+static_cast<int>(std::lround(102*std::cos(angle)))};
}
inline lv_obj_t *dock=nullptr, *marker=nullptr;
inline std::array<lv_obj_t *,VISIBLE_ICONS> icons{};
inline int selected_rank=-1;
inline bool visible=false;
inline void icon_offset(void *widget,int32_t offset_milli) {
  const auto point=arc_point(offset_milli);
  lv_obj_align(static_cast<lv_obj_t *>(widget),LV_ALIGN_CENTER,point.x-120,point.y-120);
}
inline void hide() {
  if(dock)lv_obj_add_flag(dock,LV_OBJ_FLAG_HIDDEN);
  for(auto *icon:icons)if(icon)lv_anim_delete(icon,icon_offset);
  visible=false;
}
inline void create() {
  if(dock)return;
  // A transparent top layer keeps the navigator independent of sliding pages.
  dock=lv_obj_create(lv_layer_top());
  lv_obj_set_size(dock,240,240);lv_obj_set_pos(dock,0,0);
  lv_obj_set_style_pad_all(dock,0,0);lv_obj_set_style_border_width(dock,0,0);
  lv_obj_set_style_radius(dock,0,0);lv_obj_set_style_bg_opa(dock,LV_OPA_TRANSP,0);
  lv_obj_set_style_shadow_width(dock,0,0);
  lv_obj_clear_flag(dock,LV_OBJ_FLAG_SCROLLABLE);lv_obj_clear_flag(dock,LV_OBJ_FLAG_CLICKABLE);
  marker=lv_obj_create(dock);
  lv_obj_set_size(marker,24,24);lv_obj_align(marker,LV_ALIGN_CENTER,0,102);
  lv_obj_set_style_pad_all(marker,0,0);lv_obj_set_style_border_width(marker,0,0);
  lv_obj_set_style_radius(marker,12,0);
  lv_obj_set_style_bg_color(marker,lv_color_hex(0x20384E),0);
  lv_obj_set_style_bg_opa(marker,LV_OPA_COVER,0);
  lv_obj_clear_flag(marker,LV_OBJ_FLAG_SCROLLABLE);lv_obj_clear_flag(marker,LV_OBJ_FLAG_CLICKABLE);
  for(auto &icon:icons) {icon=lv_img_create(dock);lv_obj_clear_flag(icon,LV_OBJ_FLAG_CLICKABLE);}
}
template<typename Bind> void render(const std::vector<int> &entries,int selected,Bind bind) {
  auto current=std::find(entries.begin(),entries.end(),selected);
  if(current==entries.end()) {hide();return;}
  create();
  const int rank=static_cast<int>(current-entries.begin());
  const bool changed=!visible || rank!=selected_rank;
  const int shift=visible?std::clamp(rank-selected_rank,-1,1):0;
  for(int n=0;n<VISIBLE_ICONS;++n) {
    const int offset=n-2,index=rank+offset;
    if(index<0 || index>=static_cast<int>(entries.size())) {
      lv_anim_delete(icons[n],icon_offset);
      lv_obj_add_flag(icons[n],LV_OBJ_FLAG_HIDDEN);continue;
    }
    lv_obj_clear_flag(icons[n],LV_OBJ_FLAG_HIDDEN);
    bind(icons[n],entries[index],offset==0);
    lv_obj_set_style_opa(icons[n],offset==0?LV_OPA_COVER:std::abs(offset)==1?170:110,0);
    if(changed) {
      lv_anim_delete(icons[n],icon_offset);
      icon_offset(icons[n],(offset+shift)*1000);
      if(shift) {
        lv_anim_t animation;lv_anim_init(&animation);
        lv_anim_set_var(&animation,icons[n]);lv_anim_set_exec_cb(&animation,icon_offset);
        lv_anim_set_values(&animation,(offset+shift)*1000,offset*1000);
        lv_anim_set_duration(&animation,180);lv_anim_set_path_cb(&animation,lv_anim_path_ease_out);
        lv_anim_start(&animation);
      }
    }
  }
  selected_rank=rank;
  lv_obj_clear_flag(dock,LV_OBJ_FLAG_HIDDEN);visible=true;
}
}  // namespace onx_navigation
