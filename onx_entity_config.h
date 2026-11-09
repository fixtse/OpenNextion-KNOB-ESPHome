#pragma once
#include "custom_logic.h"
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
inline esphome::ESPPreferenceObject datetime_preference;
inline bool display_settings_loaded{false};
inline void load_display_settings() {
  if (display_settings_loaded || esphome::global_preferences == nullptr) return;
  display_preference = esphome::global_preferences->make_preference<onx_display_settings::Settings>(esphome::fnv1_hash("onx.display.v1"));
  onx_display_settings::Settings saved{};
  if (display_preference.load(&saved) && saved.magic == 0x4F4E5844 && saved.media_color <= 0xFFFFFF && saved.standby_scope <= 2)
    onx_display_settings::current = saved;
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
  display_settings_loaded = true;
}
inline bool save_display_settings(const std::string &color, const std::string &scope, const std::string &date_format, const std::string &hour_format, bool show_ampm) {
  load_display_settings();
  onx_display_settings::Settings next{};
  if (!display_settings_loaded || !onx_display_settings::parse_color(color, next.media_color) ||
      !onx_display_settings::parse_scope(scope, next.standby_scope)) return false;
  onx_display_settings::DateTimeSettings date_next{};
  if (!onx_display_settings::parse_date_format(date_format, date_next.date_format) ||
      !onx_display_settings::parse_hour_format(hour_format, date_next.hour_format)) return false;
  date_next.show_ampm = show_ampm ? 1 : 0;
  if (next.media_color == onx_display_settings::current.media_color && next.standby_scope == onx_display_settings::current.standby_scope &&
      date_next.date_format == onx_display_settings::datetime.date_format && date_next.hour_format == onx_display_settings::datetime.hour_format && date_next.show_ampm == onx_display_settings::datetime.show_ampm) return true;
  if (!datetime_preference.save(&date_next) || !display_preference.save(&next) || !esphome::global_preferences->sync()) return false;
  onx_display_settings::current = next; onx_display_settings::datetime = date_next; return true;
}

inline bool sensor_configs_loaded=false;
inline std::array<esphome::ESPPreferenceObject,10> sensor_preferences;
inline void load_sensor_configs() {
  if(sensor_configs_loaded || esphome::global_preferences==nullptr)return;
  for(size_t i=0;i<10;++i) {
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
  load_sensor_configs();if(!sensor_configs_loaded||i>=10)return false;
  onx_sensor_view::Config next{};
  snprintf(next.primary,sizeof(next.primary),"%s",primary.c_str());snprintf(next.secondary,sizeof(next.secondary),"%s",secondary.c_str());
  snprintf(next.text,sizeof(next.text),"%s",text.empty()?"{value}":text.c_str());next.graph=graph;
  if(!sensor_preferences[i].save(&next)||!esphome::global_preferences->sync())return false;
  onx_sensor_view::configs[i]=next;onx_sensor_view::states[i].bind(next);return true;
}

inline bool playlists_loaded=false;
inline esphome::ESPPreferenceObject playlist_preferences[10];
inline void load_playlists() {
  if(playlists_loaded || esphome::global_preferences==nullptr) return;
  for(size_t i=0;i<10;++i) {
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
  if(!playlists_loaded || slot>=10) return false;
  if(!playlist_preferences[slot].save(&next) || !esphome::global_preferences->sync()) return false;
  onx_playlist::configs[slot]=next;
  onx_playlist::browser.clear();
  return true;
}
inline const onx_playlist::Config *playlists_for_ui(int ui_slot,const std::string &entity) {
  load_playlists();
  if(entity.rfind("media_player.",0)!=0) return nullptr;
  for(size_t i=0;i<10;++i) {
    const auto &config=onx_playlist::configs[i];
    if(custom_logic::normal_ui_slot_for_device_config_slot(i)==static_cast<size_t>(ui_slot) && entity==config.entity && config.count) return &config;
  }
  return nullptr;
}
struct PressBinding {
  char source[MAX_HA_ENTITY_ID_LENGTH + 1]{};
  char action[MAX_HA_ENTITY_ID_LENGTH + 1]{};
};
struct StoredBindings {
  uint32_t magic{0x4F4E5850};
  PressBinding slots[10]{};
};
static StoredBindings bindings;
static esphome::ESPPreferenceObject preference;
static bool loaded = false;
inline void load() {
  if (loaded || esphome::global_preferences == nullptr) return;
  preference = esphome::global_preferences->make_preference<StoredBindings>(esphome::fnv1_hash("onx.press_bindings.v1"));
  StoredBindings stored{};
  if (preference.load(&stored) && stored.magic == bindings.magic) {
    for (auto &slot : stored.slots) {
      slot.source[MAX_HA_ENTITY_ID_LENGTH] = '\0'; slot.action[MAX_HA_ENTITY_ID_LENGTH] = '\0';
      if ((!std::string(slot.source).empty() && !onx_config::valid_entity(slot.source) && !custom_logic::is_local_hmi_gpio_switch_preset_option(slot.source)) || !onx_config::valid_action(slot.action)) slot = {};
    }
    bindings = stored;
  }
  loaded = true;
}
inline std::string action_for_slot(size_t slot) {
  load();
  if (slot >= 10) return "";
  custom_logic::load_device_config_if_needed(slot);
  return onx_config::binding_matches(bindings.slots[slot].source, custom_logic::cached_device_entity_id(slot))
             ? bindings.slots[slot].action : "";
}
inline std::string action_for_ui(int ui_slot) {
  if (ui_slot < 0 || ui_slot >= 10) return "";
  for (size_t slot = 0; slot < 10; ++slot)
    if (custom_logic::normal_ui_slot_for_device_config_slot(slot) == static_cast<size_t>(ui_slot)) return action_for_slot(slot);
  return "";
}
inline bool save_action(size_t slot, const std::string &entity, const std::string &action) {
  load();
  if (!loaded || slot >= 10 || !onx_config::valid_action(action)) return false;
  auto next = bindings;
  custom_logic::copy_string_to_fixed_buffer(entity, next.slots[slot].source);
  custom_logic::copy_string_to_fixed_buffer(action, next.slots[slot].action);
  if (!preference.save(&next) || !esphome::global_preferences->sync()) return false;
  bindings = next;
  return true;
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
inline esphome::ESPPreferenceObject icon_preferences[10];
inline void load_icons() {
  if (icons_loaded || esphome::global_preferences == nullptr) return;
  for (size_t i = 0; i < 10; ++i) {
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
  if (!icons_loaded || slot >= 10 || (!icon.empty() && !onx_icon_state::valid_icon(icon))) return false;
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
    for (size_t i = 0; i < 10; ++i) {
      custom_logic::load_device_config_if_needed(i);
      auto slot = slots.add<JsonObject>();
      slot["entity"] = custom_logic::cached_device_entity_id(i);
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
  for (size_t i = 0; i < 10; ++i) {
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
