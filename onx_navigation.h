#pragma once
#include <algorithm>
#include <array>
#include <vector>
#include "lvgl.h"

namespace onx_navigation {
constexpr int VISIBLE_ICONS=5;
inline lv_obj_t *dock=nullptr, *marker=nullptr;
inline std::array<lv_obj_t *,VISIBLE_ICONS> icons{};
inline int first=0, selected_rank=-1;
inline bool visible=false;
inline void marker_x(void *widget,int32_t x) {lv_obj_set_x(static_cast<lv_obj_t *>(widget),x);}
inline void icon_offset(void *widget,int32_t x) {lv_obj_set_style_translate_x(static_cast<lv_obj_t *>(widget),x,0);}
inline int window_start(int rank,int count,int preferred) {
  int start=std::clamp(preferred,0,std::max(0,count-VISIBLE_ICONS));
  if(rank<start)start=rank;
  else if(rank>=start+VISIBLE_ICONS)start=rank-VISIBLE_ICONS+1;
  return std::clamp(start,0,std::max(0,count-VISIBLE_ICONS));
}
inline void hide() {
  if(dock)lv_obj_add_flag(dock,LV_OBJ_FLAG_HIDDEN);
  if(marker)lv_anim_delete(marker,marker_x);
  for(auto *icon:icons)if(icon) {lv_anim_delete(icon,icon_offset);icon_offset(icon,0);}
  visible=false;
}
inline void create() {
  if(dock)return;
  // The top layer stays in place while carousel screens slide underneath it.
  dock=lv_obj_create(lv_layer_top());
  lv_obj_set_size(dock,144,30);lv_obj_set_pos(dock,48,208);
  lv_obj_set_style_pad_all(dock,0,0);lv_obj_set_style_border_width(dock,0,0);
  lv_obj_set_style_radius(dock,15,0);
  lv_obj_set_style_bg_color(dock,lv_color_hex(0x000000),0);
  lv_obj_set_style_bg_opa(dock,LV_OPA_COVER,0);
  lv_obj_clear_flag(dock,LV_OBJ_FLAG_SCROLLABLE);lv_obj_clear_flag(dock,LV_OBJ_FLAG_CLICKABLE);
  marker=lv_obj_create(dock);
  lv_obj_set_size(marker,24,24);lv_obj_set_y(marker,3);
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
  const int next_first=window_start(rank,entries.size(),first);
  const int count=std::min<int>(VISIBLE_ICONS,entries.size());
  const int offset=(VISIBLE_ICONS-count)*12;
  const int target=12+offset+(rank-next_first)*24;
  if(!visible || next_first!=first || rank!=selected_rank) {
    lv_anim_delete(marker,marker_x);
    if(visible && next_first==first) {
      lv_anim_t animation;lv_anim_init(&animation);
      lv_anim_set_var(&animation,marker);lv_anim_set_exec_cb(&animation,marker_x);
      lv_anim_set_values(&animation,lv_obj_get_x(marker),target);
      lv_anim_set_duration(&animation,160);lv_anim_set_path_cb(&animation,lv_anim_path_ease_out);
      lv_anim_start(&animation);
    } else lv_obj_set_x(marker,target);
  }
  const int shift=visible?(next_first-first)*24:0;
  first=next_first;selected_rank=rank;
  for(int n=0;n<VISIBLE_ICONS;++n) {
    if(n>=count) {lv_obj_add_flag(icons[n],LV_OBJ_FLAG_HIDDEN);continue;}
    lv_obj_clear_flag(icons[n],LV_OBJ_FLAG_HIDDEN);
    bind(icons[n],entries[first+n],first+n==rank);
    lv_obj_align(icons[n],LV_ALIGN_CENTER,offset+(n-2)*24,0);
    lv_obj_set_style_opa(icons[n],first+n==rank?LV_OPA_COVER:140,0);
    if(shift) {
      lv_anim_delete(icons[n],icon_offset);icon_offset(icons[n],shift);
      lv_anim_t animation;lv_anim_init(&animation);
      lv_anim_set_var(&animation,icons[n]);lv_anim_set_exec_cb(&animation,icon_offset);
      lv_anim_set_values(&animation,shift,0);lv_anim_set_duration(&animation,160);
      lv_anim_set_path_cb(&animation,lv_anim_path_ease_out);lv_anim_start(&animation);
    }
  }
  lv_obj_clear_flag(dock,LV_OBJ_FLAG_HIDDEN);visible=true;
}
}  // namespace onx_navigation
