#pragma once
#include "custom_logic.h"
#include "onx_config_rules.h"
#include "onx_icon_state.h"
#include "esphome/components/onx_config_web/onx_config_web.h"

namespace onx_entity_config {
using custom_logic::MAX_HA_ENTITY_ID_LENGTH;
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
  return esphome::json::build_json([&](JsonObject root) { root["error"] = message; });
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
inline std::string state(const std::string &host = "", bool wifi_connected = false, bool ha_connected = false) {
  custom_logic::load_ha_entities_if_needed();
  load_icons();
  return esphome::json::build_json([&](JsonObject root) {
    root["wifi_connected"] = wifi_connected;
    root["ha_connected"] = ha_connected;
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
#ifdef USE_ESP32
    root["reset_reason"] = static_cast<int>(esp_reset_reason());
#endif
    JsonArray slots = root["slots"].to<JsonArray>();
    for (size_t i = 0; i < 10; ++i) {
      custom_logic::load_device_config_if_needed(i);
      auto slot = slots.add<JsonObject>();
      slot["entity"] = custom_logic::cached_device_entity_id(i);
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
