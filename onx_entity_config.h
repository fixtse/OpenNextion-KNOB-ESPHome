#pragma once
#include "custom_logic.h"
#include "onx_config_rules.h"
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
inline std::string state() {
  custom_logic::load_ha_entities_if_needed();
  return esphome::json::build_json([&](JsonObject root) {
    root["count"] = custom_logic::cached_ha_entities().count;
    root["capacity"] = custom_logic::MAX_HA_ENTITY_COUNT;
    JsonArray slots = root["slots"].to<JsonArray>();
    for (size_t i = 0; i < 10; ++i) {
      custom_logic::load_device_config_if_needed(i);
      auto slot = slots.add<JsonObject>();
      slot["entity"] = custom_logic::cached_device_entity_id(i);
      slot["name"] = custom_logic::ha_entity_title_from_registry_cache(custom_logic::cached_device_entity_id(i));
      slot["action"] = action_for_slot(i);
      slot["actionName"] = custom_logic::ha_entity_title_from_registry_cache(action_for_slot(i));
    }
  });
}
inline std::string lower(std::string value) {
  for (char &ch : value) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  return value;
}
inline std::string search(const esphome::onx_config_web::Query &query) {
  custom_logic::load_ha_entities_if_needed();
  const auto &entities = custom_logic::cached_ha_entities();
  const auto needle = lower(query.search);
  return esphome::json::build_json([&](JsonObject root) {
    JsonArray items = root["items"].to<JsonArray>();
    unsigned found = 0, returned = 0;
    for (size_t i = 0; i < entities.count; ++i) {
      const auto &item = entities.entities[i];
      const std::string entity = item.entity_id;
      const std::string domain = custom_logic::ha_domain_from_entity_id(entity);
      const bool action = domain == "automation" || domain == "script";
      if (query.domain == "actions" && !action) continue;
      if (!query.domain.empty() && query.domain != "actions" && query.domain != domain) continue;
      if (!custom_logic::is_supported_normal_device_entity_id(entity.c_str())) continue;
      const auto haystack = lower(entity + " " + item.friendly_name + " " + item.area_name);
      if (!needle.empty() && haystack.find(needle) == std::string::npos) continue;
      if (found++ < query.offset || returned >= 30) continue;
      auto result = items.add<JsonObject>();
      result["entity"] = entity; result["name"] = item.friendly_name; result["area"] = item.area_name;
      ++returned;
    }
    if (query.domain.empty() || query.domain == "switch") {
      for (const char *preset : custom_logic::LOCAL_HMI_GPIO_SWITCH_PRESET_OPTIONS) {
        if (!needle.empty() && lower(preset).find(needle) == std::string::npos) continue;
        if (found++ < query.offset || returned >= 30) continue;
        auto result = items.add<JsonObject>();
        result["entity"] = preset; result["name"] = custom_logic::local_hmi_gpio_switch_display_name_from_entity_id(preset); result["area"] = "On this knob";
        ++returned;
      }
    }
    root["total"] = found;
    root["next"] = query.offset + returned < found ? static_cast<int>(query.offset + returned) : -1;
  });
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
