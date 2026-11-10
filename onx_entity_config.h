#pragma once
#include <vector>
#include "custom_logic.h"
#include "onx_gesture.h"
#include "onx_detail_view.h"
#include "onx_config_rules.h"
#include "onx_icon_state.h"
#include "onx_display_settings.h"
#include "onx_sensor_view.h"
#include "esphome/components/onx_config_web/onx_config_web.h"

namespace onx_entity_config {
using custom_logic::MAX_HA_ENTITY_ID_LENGTH;
// Web configuration can legitimately exceed ESPHome's generic 5 KB serializer
// limit. Allocate its document in PSRAM, measure first, and return complete JSON
// or an explicit error, never a truncated successful response.
template<typename F> inline std::string build_json(F &&fill) {
  auto doc = custom_logic::make_psram_json_document();
  fill(doc.template to<JsonObject>());
  if (doc.overflowed()) return "{\"error\":\"Not enough memory to build configuration response\"}";
  const size_t size = measureJson(doc);
  if (size > 65536) return "{\"error\":\"Configuration response exceeds size limit\"}";
  std::string result; result.reserve(size);
  serializeJson(doc, result);
  return result;
}

inline esphome::ESPPreferenceObject display_preference;
inline esphome::ESPPreferenceObject idle_preference;
inline esphome::ESPPreferenceObject media_style_preference;
inline esphome::ESPPreferenceObject datetime_preference;
inline bool display_settings_loaded{false};
inline void load_display_settings() {
  if (display_settings_loaded || esphome::global_preferences == nullptr) return;
  display_preference = esphome::global_preferences->make_preference<onx_display_settings::Settings>(esphome::fnv1_hash("onx.display.v1"));
  onx_display_settings::Settings saved{};
  if (display_preference.load(&saved) && saved.magic == 0x4F4E5844 && saved.media_color <= 0xFFFFFF && saved.standby_scope <= 2)
    onx_display_settings::current = saved;
  idle_preference=esphome::global_preferences->make_preference<onx_display_settings::IdleSettings>(esphome::fnv1_hash("onx.idle_screen.v1"));
  onx_display_settings::IdleSettings idle_saved{};
  if(idle_preference.load(&idle_saved) && idle_saved.magic==0x4F4E5849 && idle_saved.media<=1)onx_display_settings::idle=idle_saved;
  datetime_preference = esphome::global_preferences->make_preference<onx_display_settings::DateTimeSettings>(esphome::fnv1_hash("onx.datetime.v2"));
  onx_display_settings::DateTimeSettings date_saved{};
  if (datetime_preference.load(&date_saved) && date_saved.magic == 0x4F4E5854 && date_saved.date_format <= 5 && date_saved.hour_format <= 1 && date_saved.show_ampm <= 1)
    onx_display_settings::datetime = date_saved;
  else {
    struct LegacyDate {uint32_t magic;uint8_t date_format,hour_format;};
    auto old=esphome::global_preferences->make_preference<LegacyDate>(esphome::fnv1_hash("onx.datetime.v1"));
    LegacyDate legacy{};
    if(old.load(&legacy) && legacy.magic==0x4F4E5854 && legacy.date_format<=2 && legacy.hour_format<=1) {
      onx_display_settings::datetime.date_format=legacy.date_format;onx_display_settings::datetime.hour_format=legacy.hour_format;
    }
  }
  media_style_preference=esphome::global_preferences->make_preference<onx_display_settings::MediaStyle>(esphome::fnv1_hash("onx.media_style.v1"));
  onx_display_settings::MediaStyle media_saved{};
  if(media_style_preference.load(&media_saved) && media_saved.magic==0x4F4E584D && media_saved.material<=1)
    onx_display_settings::media_style=media_saved;
  display_settings_loaded = true;
}
inline bool save_display_settings(const std::string &color, const std::string &scope, const std::string &date_format, const std::string &hour_format, bool show_ampm,const std::string &idle_view="",const std::string &media_view="") {
  load_display_settings();
  auto media_next=onx_display_settings::media_style;
  if(!media_view.empty()) {
    if(media_view!="classic" && media_view!="material")return false;
    media_next.material=media_view=="material";
  }
  auto idle_next=onx_display_settings::idle;
  if(!idle_view.empty()) {
    if(idle_view!="clock" && idle_view!="media")return false;
    idle_next.media=idle_view=="media";
  }
  onx_display_settings::Settings next{};
  if (!display_settings_loaded || !onx_display_settings::parse_color(color, next.media_color) ||
      !onx_display_settings::parse_scope(scope, next.standby_scope)) return false;
  onx_display_settings::DateTimeSettings date_next{};
  if (!onx_display_settings::parse_date_format(date_format, date_next.date_format) ||
      !onx_display_settings::parse_hour_format(hour_format, date_next.hour_format)) return false;
  date_next.show_ampm = show_ampm ? 1 : 0;
  if (next.media_color == onx_display_settings::current.media_color && next.standby_scope == onx_display_settings::current.standby_scope &&
      date_next.date_format == onx_display_settings::datetime.date_format && date_next.hour_format == onx_display_settings::datetime.hour_format && date_next.show_ampm == onx_display_settings::datetime.show_ampm && idle_next.media==onx_display_settings::idle.media && media_next.material==onx_display_settings::media_style.material) return true;
  if (!media_style_preference.save(&media_next) || !idle_preference.save(&idle_next) || !datetime_preference.save(&date_next) || !display_preference.save(&next) || !esphome::global_preferences->sync()) return false;
  onx_display_settings::current = next; onx_display_settings::datetime = date_next; onx_display_settings::idle=idle_next; onx_display_settings::media_style=media_next; return true;
}

// Keep subscriptions and compact runtime indices intact; hide only carousel entries.
struct TimerVisibility { char entity[129]{}; bool hidden{false}; };
inline std::array<TimerVisibility,20> timer_visibility;
inline std::array<esphome::ESPPreferenceObject,20> timer_visibility_preferences;
inline bool timer_visibility_loaded=false;
inline void load_timer_visibility() {
  if(timer_visibility_loaded || esphome::global_preferences==nullptr)return;
  for(size_t i=0;i<20;++i) {
    timer_visibility_preferences[i]=esphome::global_preferences->make_preference<TimerVisibility>(
        esphome::fnv1_hash("onx.timer.visibility.v1."+std::to_string(i)));
    TimerVisibility saved{};
    if(timer_visibility_preferences[i].load(&saved)) {
      saved.entity[128]=0;
      if(onx_config::valid_entity(saved.entity))timer_visibility[i]=saved;
    }
  }
  timer_visibility_loaded=true;
}
inline bool timer_hidden(size_t slot,const std::string &entity) {
  load_timer_visibility();
  return slot<20 && !entity.empty() && timer_visibility[slot].hidden && entity==timer_visibility[slot].entity;
}
inline bool save_timer_visibility(size_t slot,const std::string &entity,bool hidden) {
  load_timer_visibility();
  if(!timer_visibility_loaded || slot>=20)return false;
  TimerVisibility next{};
  snprintf(next.entity,sizeof(next.entity),"%s",entity.c_str());next.hidden=hidden;
  if(!timer_visibility_preferences[slot].save(&next)||!esphome::global_preferences->sync())return false;
  timer_visibility[slot]=next;return true;
}
inline bool carousel_hidden(int ui_slot) {
  for(size_t slot=0;slot<20;++slot)
    if(custom_logic::normal_ui_slot_for_device_config_slot(slot)==static_cast<size_t>(ui_slot))
      return timer_hidden(slot,custom_logic::cached_device_entity_id(slot));
  return false;
}
// Settings remains at 'total', including when every configured timer is hidden.
inline int carousel_selection(int selected,int total,int direction) {
  while(selected>=0 && selected<total && carousel_hidden(selected))selected+=direction<0?-1:1;
  if(selected<0) {
    selected=0;
    while(selected<total && carousel_hidden(selected))++selected;
  }
  return selected;
}
inline std::vector<int> carousel_entries(int total) {
  std::vector<int> entries;entries.reserve(total+1);
  for(int i=0;i<total;++i)if(!carousel_hidden(i))entries.push_back(i);
  entries.push_back(total);  // Settings is always reachable.
  return entries;
}

// A missing neighbour is -1; the settings gear remains the final landmark.
inline int carousel_neighbour(int selected,int total,int direction) {
  int index=selected+direction;
  while(index>=0 && index<total && carousel_hidden(index))index+=direction;
  return index<0 || index>total ? -1 : index;
}

inline int carousel_position(int index) {
  int visible=0;
  for(int i=0;i<index;++i)if(!carousel_hidden(i))++visible;
  return visible;
}

inline bool sensor_configs_loaded=false;
inline std::array<esphome::ESPPreferenceObject,20> sensor_preferences;
inline void load_sensor_configs() {
  if(sensor_configs_loaded || esphome::global_preferences==nullptr)return;
  for(size_t i=0;i<20;++i) {
    const auto key="onx.sensor.v1."+std::to_string(i);
    sensor_preferences[i]=esphome::global_preferences->make_preference<onx_sensor_view::Config>(esphome::fnv1_hash(key.c_str()));
    onx_sensor_view::Config saved{};
    if(sensor_preferences[i].load(&saved)) {
      saved.primary[128]=saved.secondary[128]=saved.text[160]=0;
      if((!saved.primary[0]||onx_config::valid_entity(saved.primary)) && (!saved.secondary[0]||onx_config::valid_entity(saved.secondary)) && saved.graph<=1)
        onx_sensor_view::configs[i]=saved;
    }
    onx_sensor_view::states[i].bind(onx_sensor_view::configs[i]);
  }
  sensor_configs_loaded=true;
}
inline bool save_sensor_config(size_t i,const std::string &primary,const std::string &secondary,const std::string &text,bool graph) {
  load_sensor_configs();if(!sensor_configs_loaded||i>=20)return false;
  onx_sensor_view::Config next{};
  snprintf(next.primary,sizeof(next.primary),"%s",primary.c_str());snprintf(next.secondary,sizeof(next.secondary),"%s",secondary.c_str());
  snprintf(next.text,sizeof(next.text),"%s",text.empty()?"{value}":text.c_str());next.graph=graph;
  if(!sensor_preferences[i].save(&next)||!esphome::global_preferences->sync())return false;
  onx_sensor_view::configs[i]=next;onx_sensor_view::states[i].bind(next);return true;
}

inline bool playlists_loaded=false;
inline esphome::ESPPreferenceObject playlist_preferences[20];
inline void load_playlists() {
  if(playlists_loaded || esphome::global_preferences==nullptr) return;
  for(size_t i=0;i<20;++i) {
    playlist_preferences[i]=esphome::global_preferences->make_preference<onx_playlist::Config>(esphome::fnv1_hash("onx.playlists.v1."+std::to_string(i)));
    onx_playlist::Config saved{};
    if(!playlist_preferences[i].load(&saved) || saved.magic!=0x4F4E5851 || saved.count>onx_playlist::MAX_ENTRIES) continue;
    saved.entity[128]=0;
    bool valid=std::string(saved.entity).rfind("media_player.",0)==0 && onx_config::valid_entity(saved.entity);
    for(auto &entry:saved.entries) {
      entry.name[48]=entry.action[128]=0;
    }
    for(size_t j=0;j<saved.count;++j) valid=valid && saved.entries[j].name[0] && onx_playlist::action_valid(saved.entries[j].action);
    if(valid) onx_playlist::configs[i]=saved;
  }
  playlists_loaded=true;
}
inline bool parse_playlists(const std::string &entity,const std::string &raw,onx_playlist::Config &next) {
  if(raw.empty()) return true; // Old clients clear the list when saving a slot.
  auto doc=custom_logic::make_psram_json_document();
  if(deserializeJson(doc,raw) || !doc.is<JsonArray>() || doc.size()>onx_playlist::MAX_ENTRIES) return false;
  if(doc.size() && entity.rfind("media_player.",0)!=0) return false;
  custom_logic::copy_string_to_fixed_buffer(entity,next.entity);
  for(JsonObject entry:doc.as<JsonArray>()) {
    if(!entry["name"].is<std::string>() || !entry["action"].is<std::string>()) return false;
    const auto name=entry["name"].as<std::string>(),action=entry["action"].as<std::string>();
    if(name.empty() || name.size()>48 || name.find_first_of("\r\n")!=std::string::npos || !onx_playlist::action_valid(action)) return false;
    auto &target=next.entries[next.count++];
    custom_logic::copy_string_to_fixed_buffer(name,target.name);
    custom_logic::copy_string_to_fixed_buffer(action,target.action);
  }
  return true;
}
inline bool save_playlists(size_t slot,const onx_playlist::Config &next) {
  load_playlists();
  if(!playlists_loaded || slot>=20) return false;
  if(!playlist_preferences[slot].save(&next) || !esphome::global_preferences->sync()) return false;
  onx_playlist::configs[slot]=next;
  onx_playlist::browser.clear();
  return true;
}
inline const onx_playlist::Config *playlists_for_ui(int ui_slot,const std::string &entity) {
  load_playlists();
  if(entity.rfind("media_player.",0)!=0) return nullptr;
  for(size_t i=0;i<20;++i) {
    const auto &config=onx_playlist::configs[i];
    if(custom_logic::normal_ui_slot_for_device_config_slot(i)==static_cast<size_t>(ui_slot) && entity==config.entity && config.count) return &config;
  }
  return nullptr;
}
inline bool gestures_loaded=false;
inline std::array<esphome::ESPPreferenceObject,20> gesture_preferences;
inline void load_gestures() {
  if(gestures_loaded || esphome::global_preferences==nullptr)return;
  for(size_t i=0;i<20;++i) {
    gesture_preferences[i]=esphome::global_preferences->make_preference<onx_gesture::Config>(esphome::fnv1_hash("onx.gestures.v1."+std::to_string(i)));
    onx_gesture::Config saved{};
    if(!gesture_preferences[i].load(&saved) || saved.magic!=0x4F4E5847)continue;
    saved.entity[128]=saved.feedback[128]=0;
    bool valid=(onx_config::valid_entity(saved.entity) || custom_logic::is_local_hmi_gpio_switch_preset_option(saved.entity)) && (!saved.feedback[0] || onx_config::valid_entity(saved.feedback)) &&
        saved.popup<=1 && saved.responses<=1 && saved.events<=1 && saved.step>=1 && saved.step<=100;
    for(auto &action:saved.actions) {action[128]=0;valid=valid && onx_config::valid_action(action);}
    if(valid)onx_gesture::configs[i]=saved;
  }
  gestures_loaded=true;
}
inline bool parse_gestures(const std::string &entity,const std::string &raw,onx_gesture::Config &next) {
  snprintf(next.entity,sizeof(next.entity),"%s",entity.c_str());
  if(raw.empty())return true;
  auto doc=custom_logic::make_psram_json_document();
  if(deserializeJson(doc,raw) || !doc.is<JsonObject>())return false;
  const char *keys[]={"cw","ccw","holdCw","holdCcw"};
  for(size_t i=0;i<4;++i) {
    const std::string action=doc[keys[i]] | "";
    if(!onx_config::valid_action(action))return false;
    snprintf(next.actions[i],sizeof(next.actions[i]),"%s",action.c_str());
  }
  const std::string feedback=doc["feedback"] | "";
  if(!feedback.empty() && !onx_config::valid_entity(feedback))return false;
  snprintf(next.feedback,sizeof(next.feedback),"%s",feedback.c_str());
  for(const auto key:{"popup","responses","events"}) {
    const std::string value=doc[key] | (std::string(key)=="popup"?"1":"0");
    if(value!="0" && value!="1")return false;
    if(std::string(key)=="popup")next.popup=value=="1";
    if(std::string(key)=="responses")next.responses=value=="1";
    if(std::string(key)=="events")next.events=value=="1";
  }
  unsigned step=1;
  const std::string step_text=doc["step"] | "1";
  if(!onx_config::integer_in_range(step_text,1,100,step))return false;
  next.step=step;return true;
}
inline bool save_gestures(size_t slot,const onx_gesture::Config &next) {
  load_gestures();if(!gestures_loaded || slot>=20)return false;
  if(!gesture_preferences[slot].save(&next) || !esphome::global_preferences->sync())return false;
  const bool same=std::string(next.entity)==onx_gesture::configs[slot].entity && std::string(next.feedback)==onx_gesture::configs[slot].feedback;
  const uint32_t next_token=onx_gesture::states[slot].token+1;
  onx_gesture::configs[slot]=next;
  if(!same)onx_gesture::states[slot]={};
  onx_gesture::states[slot].token=next_token;onx_gesture::states[slot].feedback_ms=0;
  onx_gesture::clear_pending();return true;
}
inline int gesture_slot_for_ui(int ui_slot) {
  load_gestures();
  for(size_t i=0;i<20;++i)
    if(custom_logic::normal_ui_slot_for_device_config_slot(i)==static_cast<size_t>(ui_slot) &&
        !custom_logic::cached_device_entity_id(i).empty() && custom_logic::cached_device_entity_id(i)==onx_gesture::configs[i].entity)return i;
  return -1;
}

struct PressBinding {
  char source[MAX_HA_ENTITY_ID_LENGTH + 1]{};
  char action[MAX_HA_ENTITY_ID_LENGTH + 1]{};
};
struct StoredBindings {
  uint32_t magic{0x4F4E5850};
  PressBinding slots[20]{};
};
static StoredBindings bindings;
static esphome::ESPPreferenceObject preference;
static bool loaded = false;
inline void load() {
  if (loaded || esphome::global_preferences == nullptr) return;
  preference = esphome::global_preferences->make_preference<StoredBindings>(esphome::fnv1_hash("onx.press_bindings.v2"));
  auto stored_buffer=custom_logic::make_ram_allocated<StoredBindings>();
  if(!stored_buffer)return;
  auto &stored=*stored_buffer;
  bool restored=preference.load(&stored) && stored.magic==bindings.magic;
  if(!restored) {
    struct LegacyBindings {uint32_t magic;PressBinding slots[10];};
    auto legacy_pref=esphome::global_preferences->make_preference<LegacyBindings>(esphome::fnv1_hash("onx.press_bindings.v1"));
    auto legacy_buffer=custom_logic::make_ram_allocated<LegacyBindings>();
    if(!legacy_buffer)return;
    auto &legacy=*legacy_buffer;
    if(legacy_pref.load(&legacy) && legacy.magic==bindings.magic) {
      std::copy(std::begin(legacy.slots),std::end(legacy.slots),std::begin(stored.slots));
      restored=true;
    }
  }
  if (restored) {
    for (auto &slot : stored.slots) {
      slot.source[MAX_HA_ENTITY_ID_LENGTH] = '\0'; slot.action[MAX_HA_ENTITY_ID_LENGTH] = '\0';
      if ((!std::string(slot.source).empty() && !onx_config::valid_entity(slot.source) && !custom_logic::is_local_hmi_gpio_switch_preset_option(slot.source)) || !onx_config::valid_press_action(slot.source,slot.action)) slot = {};
    }
    bindings = stored;
  }
  loaded = true;
}
inline std::string action_for_slot(size_t slot) {
  load();
  if (slot >= 20) return "";
  custom_logic::load_device_config_if_needed(slot);
  return onx_config::binding_matches(bindings.slots[slot].source, custom_logic::cached_device_entity_id(slot))
             ? bindings.slots[slot].action : "";
}
inline std::string action_for_ui(int ui_slot) {
  if (ui_slot < 0 || ui_slot >= 20) return "";
  for (size_t slot = 0; slot < 20; ++slot)
    if (custom_logic::normal_ui_slot_for_device_config_slot(slot) == static_cast<size_t>(ui_slot)) return action_for_slot(slot);
  return "";
}
inline bool save_action(size_t slot, const std::string &entity, const std::string &action) {
  load();
  if (!loaded || slot >= 20 || !onx_config::valid_press_action(entity,action)) return false;
  auto next_buffer=custom_logic::make_ram_allocated<StoredBindings>();
  if(!next_buffer)return false;
  auto &next=*next_buffer;next=bindings;
  custom_logic::copy_string_to_fixed_buffer(entity, next.slots[slot].source);
  custom_logic::copy_string_to_fixed_buffer(action, next.slots[slot].action);
  if (!preference.save(&next) || !esphome::global_preferences->sync()) return false;
  bindings = next;
  return true;
}
inline void customize_control_view(int ui_slot,const std::string &entity,onx_detail_view::View &view) {
  const auto press=action_for_ui(ui_slot);
  if(entity.rfind("climate.",0)==0 && !press.empty())
    view.hint.text=press=="none"?"Press: no action":"Press: custom action";
  const int slot=gesture_slot_for_ui(ui_slot);
  if(slot<0)return;
  const auto &config=onx_gesture::configs[slot];
  if(config.actions[0][0] || config.actions[1][0] || config.events)view.hint.text="Turn: custom control";
  if(config.actions[2][0] || config.actions[3][0])view.hint.text+="\nHold + turn: custom control";
  if(entity.rfind("sensor.",0)==0 && config.feedback[0] && onx_gesture::states[slot].received) {
    const auto &state=onx_gesture::states[slot];
    view.value.text=state.value+(state.unit.empty()?"":" "+state.unit);
  }
}
inline std::string error(const char *message) {
  return build_json([&](JsonObject root) { root["error"] = message; });
}
struct StoredIcon {
  uint32_t magic{0x4F4E5849};
  char entity[129]{}, icon[81]{};
  bool registry_override{false};
};
inline bool icons_loaded{false};
inline esphome::ESPPreferenceObject icon_preferences[20];
inline void load_icons() {
  if (icons_loaded || esphome::global_preferences == nullptr) return;
  for (size_t i = 0; i < 20; ++i) {
    icon_preferences[i] = esphome::global_preferences->make_preference<StoredIcon>(
        esphome::fnv1_hash("onx.entity_icon.v1." + std::to_string(i)));
    StoredIcon stored;
    if (icon_preferences[i].load(&stored) && stored.magic == 0x4F4E5849) {
      stored.entity[128] = 0; stored.icon[80] = 0;
      if (onx_config::valid_entity(stored.entity) && (stored.icon[0] == 0 || onx_icon_state::valid_icon(stored.icon))) {
        auto &meta = onx_icon_state::metadata[i];
        meta.bind(stored.entity); meta.configured = stored.icon; meta.registry_override = stored.registry_override;
      }
    }
  }
  icons_loaded = true;
}
inline bool save_icon(size_t slot, const std::string &entity, const std::string &icon, bool override) {
  load_icons();
  if (!icons_loaded || slot >= 20 || (!icon.empty() && !onx_icon_state::valid_icon(icon))) return false;
  const auto &existing = onx_icon_state::metadata[slot];
  if (existing.entity == entity && existing.configured == icon && existing.registry_override == override) return true;
  StoredIcon stored;
  custom_logic::copy_string_to_fixed_buffer(entity, stored.entity);
  custom_logic::copy_string_to_fixed_buffer(icon, stored.icon);
  stored.registry_override = override;
  if (!icon_preferences[slot].save(&stored) || !esphome::global_preferences->sync()) return false;
  auto &meta = onx_icon_state::metadata[slot];
  meta.bind(entity); meta.configured = icon; meta.registry_override = override;
  return true;
}
inline std::string state(const std::string &host = "", bool wifi_connected = false, bool ha_connected = false, float wifi_rssi = NAN, const std::string &artwork_error = "") {
  custom_logic::load_ha_entities_if_needed();
  load_icons();
  load_playlists();
  load_gestures();
  return build_json([&](JsonObject root) {
    root["wifi_connected"] = wifi_connected;
    root["ha_connected"] = ha_connected;
    root["wifi_rssi_dbm"] = wifi_rssi;
    root["wifi_disconnects_since_boot"] = onx_runtime::wifi_disconnects;
    root["last_wifi_disconnect_ms"] = onx_runtime::last_wifi_disconnect_ms;
    root["artwork_error"] = artwork_error;
    root["count"] = custom_logic::cached_ha_entities().count;
    std::string url = custom_logic::build_ha_http_state_url(host, "sensor.placeholder");
    const auto path = url.find("/api/states/");
    if (path != std::string::npos) url.resize(path);
    root["ha_url"] = url;
    custom_logic::load_standby_weather_config_if_needed();
    root["weather"] = custom_logic::cached_standby_weather_entity_id;
    root["capacity"] = custom_logic::MAX_HA_ENTITY_COUNT;
#ifdef ESPHOME_PROJECT_VERSION
    root["firmware"] = ESPHOME_PROJECT_VERSION;
#endif
    root["uptime_ms"] = millis();
    root["reset_reason_text"] = onx_runtime::reset_name();
    auto previous=root["last_event_before_boot"].to<JsonObject>();
    previous["available"]=onx_runtime::previous.magic==0x4F4E5852;
    previous["name"]=onx_runtime::previous.name;
    previous["entity"]=onx_runtime::previous.entity;
    previous["uptime_ms"]=onx_runtime::previous.at_ms;
    previous["internal_heap_free"]=onx_runtime::previous.free_heap;
    root["artwork_status"]=onx_media::downloading?"downloading":!onx_media::ready_url.empty()?"ready":onx_media::artwork_wait_reason.empty()?"not_loaded":onx_media::artwork_wait_reason.c_str();
#ifdef USE_ESP32
    root["internal_heap_free"]=heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    root["internal_heap_minimum"]=heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    root["internal_heap_largest_block"]=heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    root["psram_free"]=heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif
#ifdef USE_ESP32
    root["reset_reason"] = static_cast<int>(esp_reset_reason());
#endif
    JsonArray slots = root["slots"].to<JsonArray>();
    for (size_t i = 0; i < 20; ++i) {
      custom_logic::load_device_config_if_needed(i);
      auto slot = slots.add<JsonObject>();
      slot["entity"] = custom_logic::cached_device_entity_id(i);
      auto gestures=slot["gestures"].to<JsonObject>();
      const auto &control=onx_gesture::configs[i];
      const bool bound=custom_logic::cached_device_entity_id(i)==control.entity;
      const char *gesture_keys[]={"cw","ccw","holdCw","holdCcw"};
      for(size_t j=0;j<4;++j)gestures[gesture_keys[j]]=bound?control.actions[j]:"";
      gestures["feedback"]=bound?control.feedback:"";
      gestures["popup"]=!bound || control.popup?"1":"0";
      gestures["responses"]=bound && control.responses?"1":"0";
      gestures["events"]=bound && control.events?"1":"0";
      gestures["step"]=std::to_string(bound?control.step:1);
      slot["timerHidden"] = timer_hidden(i,custom_logic::cached_device_entity_id(i))?"1":"0";
      JsonArray playlists=slot["playlists"].to<JsonArray>();
      const auto &playlist=onx_playlist::configs[i];
      if(custom_logic::cached_device_entity_id(i)==playlist.entity)
        for(size_t j=0;j<playlist.count;++j) {
          auto entry=playlists.add<JsonObject>();entry["name"]=playlist.entries[j].name;entry["action"]=playlist.entries[j].action;
        }
      const auto &sensor=onx_sensor_view::configs[i];
      const bool same_sensor=custom_logic::cached_device_entity_id(i)==sensor.primary;
      slot["secondary"] = same_sensor?sensor.secondary:"";
      slot["secondaryText"] = same_sensor?sensor.text:"{value}";
      slot["sensorGraph"] = same_sensor&&sensor.graph?"1":"0";
      const auto &icon = onx_icon_state::metadata[i];
      const bool same_icon_entity = icon.entity == custom_logic::cached_device_entity_id(i);
      slot["icon"] = same_icon_entity ? icon.configured : "";
      slot["iconOverride"] = same_icon_entity && icon.registry_override ? "1" : "0";
      slot["name"] = custom_logic::ha_entity_title_from_registry_cache(custom_logic::cached_device_entity_id(i));
      for (size_t j = 0; j < custom_logic::cached_ha_entities().count; ++j) {
        const auto &entry = custom_logic::cached_ha_entities().entities[j];
        if (custom_logic::cached_device_entity_id(i) == entry.entity_id) slot["area"] = entry.area_name;
      }
      slot["action"] = action_for_slot(i);
      slot["type"] = custom_logic::cached_device_custom_type(i);
      slot["typeOption"] = custom_logic::ha_domain_from_entity_id(custom_logic::cached_device_entity_id(i)) == "light" ? custom_logic::light_type_option_from_custom_mask(custom_logic::cached_device_custom_light_capability_mask(i)) : custom_logic::ha_domain_from_entity_id(custom_logic::cached_device_entity_id(i)) == "cover" ? (custom_logic::cached_device_custom_light_capability_mask(i) == 0 ? "Cover (Preset Off)" : "Cover (Preset On)") : custom_logic::cached_device_custom_type(i) == "plug" ? "Plug" : "";
      slot["actionName"] = custom_logic::ha_entity_title_from_registry_cache(action_for_slot(i));
    }
  });
}
inline bool save_selected_metadata(const esphome::onx_config_web::Query &query) {
  custom_logic::load_ha_entities_if_needed();
  auto next = custom_logic::make_ram_allocated<custom_logic::StoredHaEntities>();
  if (!next) return false;
  next->magic = custom_logic::HA_ENTITIES_MAGIC;
  const auto &old = custom_logic::cached_ha_entities();
  auto add = [&](const std::string &entity) {
    if (!onx_config::valid_entity(entity)) return;
    for (size_t i = 0; i < next->count; ++i) if (entity == next->entities[i].entity_id) return;
    if (next->count >= custom_logic::MAX_HA_ENTITY_COUNT) return;
    auto &item = next->entities[next->count++];
    for (size_t i = 0; i < old.count; ++i) if (entity == old.entities[i].entity_id) item = old.entities[i];
    custom_logic::copy_string_to_fixed_buffer(entity, item.entity_id);
    if (entity == query.entity) {
      custom_logic::copy_string_to_fixed_buffer(query.name, item.friendly_name);
      custom_logic::copy_string_to_fixed_buffer(query.area, item.area_name);
    }
    if (entity == query.action) custom_logic::copy_string_to_fixed_buffer(query.action_name, item.friendly_name);
  };
  for (size_t i = 0; i < 20; ++i) {
    custom_logic::load_device_config_if_needed(i);
    add(custom_logic::cached_device_entity_id(i));
    add(action_for_slot(i));
  }
  custom_logic::load_standby_weather_config_if_needed();
  add(custom_logic::cached_standby_weather_entity_id);
  return custom_logic::save_ha_entities(*next);
}
}  // namespace onx_entity_config

namespace onx_entity_config {
inline void sync_select(size_t slot, esphome::select::Select *select) {
  custom_logic::apply_device_config_options(slot, select);
  const auto &entity = custom_logic::cached_device_entity_id(slot);
  const auto &options = select->traits.get_options();
  for (size_t i = 0; i < options.size(); ++i) {
    if (custom_logic::extract_entity_id_from_device1_option(options[i]) == entity) { select->publish_state(i); return; }
  }
  // A manually entered entity can be used even if HA's cached list omitted it.
  esphome::FixedVector<const char *> updated;
  updated.init(options.size() + 1);
  for (const auto *option : options) updated.push_back(option);
  updated.push_back(entity.c_str());
  select->traits.set_options(updated);
  select->publish_state(updated.size() - 1);
}
}  // namespace onx_entity_config
