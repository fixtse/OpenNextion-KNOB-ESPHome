#pragma once
#include <cstdio>
#include "custom_logic.h"
#include "custom_logic_entity_sub.h"

namespace onx_controls {
using namespace custom_logic_entity_sub;
inline bool handles(const std::string &id) {
  return knob_control_domain(id) || has_prefix(id, "climate.") || has_prefix(id, "media_player.");
}
inline std::string domain(const std::string &id) { return id.substr(0, id.find('.')); }
inline std::string actual(const EntityDetailState &d, const std::string &key) {
  if (key == "temperature") return d.target;
  if (key == "target_temp_low") return d.low;
  if (key == "target_temp_high") return d.high;
  if (key == "percentage") return d.percentage;
  if (key == "preset_mode") return d.preset;
  if (key == "fan_mode") return d.fan_mode;
  if (key == "swing_mode") return d.swing_mode;
  if (key == "humidity") return d.humidity;
  if (key == "volume_level") return d.volume;
  if (key == "source") return d.source;
  if (key == "oscillating") return d.oscillating == "True" || d.oscillating == "true" ? "true" : "false";
  if (key == "is_volume_muted") return d.muted == "True" || d.muted == "true" ? "true" : "false";
  if (key == "direction") return d.direction;
  return d.state;
}
inline std::string effective(const EntityDetailState &d, const std::string &key) {
  return control_entity == d.entity_id ? requested_value(key, actual(d, key)) : actual(d, key);
}
inline std::vector<std::string> settings(const EntityDetailState &d) {
  std::vector<std::string> out;
  auto list = [&](const std::string &key, const std::string &raw, uint32_t bit) {
    if ((d.features & bit) && !onx_control::options(raw).empty()) out.push_back(key);
  };
  if (has_prefix(d.entity_id, "climate.")) {
    if ((d.features & 1) && std::isfinite(detail_number(d.target))) out.push_back("temperature");
    if ((d.features & 2) && std::isfinite(detail_number(d.low)) && std::isfinite(detail_number(d.high))) {
      out.push_back("range"); out.push_back("target_temp_low"); out.push_back("target_temp_high");
    }
    list("preset_mode", d.presets, 16); list("fan_mode", d.fan_modes, 8); list("swing_mode", d.swing_modes, 32);
    if ((d.features & 4) && std::isfinite(detail_number(d.humidity))) out.push_back("humidity");
    if (out.empty()) out.push_back("state");
  } else if (has_prefix(d.entity_id, "fan.")) {
    if ((d.features & 1) && std::isfinite(detail_number(d.percentage)) && std::isfinite(d.step) && d.step > 0) out.push_back("percentage");
    list("preset_mode", d.presets, 8);
    if (d.features & 2) out.push_back("oscillating");
    if (d.features & 4) out.push_back("direction");
    if (out.empty()) out.push_back("state");
  } else if (has_prefix(d.entity_id, "media_player.")) {
    if (d.features & 4) out.push_back("volume_level");
    list("source", d.sources, 2048);
    if (d.features & 8) out.push_back("is_volume_muted");
    if (out.empty()) out.push_back("state");
  } else out.push_back("state");
  return out;
}
inline std::string setting(const EntityDetailState &d) {
  const auto list = settings(d);
  if (std::find(list.begin(), list.end(), control_setting) == list.end()) control_setting = list.front();
  return control_setting;
}
inline void cycle(const std::string &id, int direction) {
  select_control_entity(id);
  const auto *d = entity_detail(id);
  if (!d) return;
  const auto list = settings(*d);
  const auto key = setting(*d);
  auto found = std::find(list.begin(), list.end(), key);
  const int index = found - list.begin(), count = list.size();
  control_setting = list[(index + direction + count) % count];
  control_preview.clear();
}
inline void request(const EntityDetailState &d, const std::string &service, const std::string &field,
                    const std::string &parameter, const std::string &value) {
  if (!value.empty() && !onx_control::equal(value, effective(d, field))) request_control(d.entity_id, service, {{field, parameter, value}});
}
inline std::string choices(const EntityDetailState &d, const std::string &key) {
  if (key == "preset_mode") return d.presets;
  if (key == "fan_mode") return d.fan_modes;
  if (key == "swing_mode") return d.swing_modes;
  if (key == "source") return d.sources;
  if (key == "direction") return "['forward', 'reverse']";
  if (key == "oscillating" || key == "is_volume_muted") return "['false', 'true']";
  if (has_prefix(d.entity_id, "select.") || has_prefix(d.entity_id, "input_select.")) return d.options;
  return "";
}
inline std::string action(const EntityDetailState &d, const std::string &key) {
  const auto type = domain(d.entity_id);
  if (key == "percentage") return "fan.set_percentage";
  if (key == "oscillating") return "fan.oscillate";
  if (key == "is_volume_muted") return "media_player.volume_mute";
  if (key == "source") return "media_player.select_source";
  if (key == "volume_level") return "media_player.volume_set";
  if (key == "state") return type + (type == "select" || type == "input_select" ? ".select_option" : ".set_value");
  return type + ".set_" + key;
}
inline void rotate(const std::string &id, int direction) {
  select_control_entity(id);
  const auto *d = entity_detail(id);
  if (!d || !d->available) return;
  if (has_prefix(id, "media_player.")) {
    if (!(d->features & 4)) return;
    control_setting = "volume_level"; control_preview.clear();
  }
  const auto key = setting(*d), raw = choices(*d, key);
  if (!raw.empty()) {
    const auto value = next_option(raw, control_preview.empty() ? effective(*d, key) : control_preview, direction);
    if (!value.empty()) control_preview = value; // Explicit press to commit a choice.
    return;
  }
  if (has_prefix(id, "scene.") || has_prefix(id, "button.") || has_prefix(id, "input_button.")) return;
  float minimum = d->minimum, maximum = d->maximum, step = d->step;
  if (key == "percentage") { minimum = 0; maximum = 100; }
  if (key == "volume_level") { minimum = 0; maximum = 1; step = 0.02f; }
  if (key == "humidity") { minimum = d->humidity_min; maximum = d->humidity_max; step = 1; }
  if (has_prefix(id, "climate.") && (!std::isfinite(step) || step <= 0)) step = 0.5f;
  if (key == "range") {
    const float lo = detail_number(effective(*d, "target_temp_low")), hi = detail_number(effective(*d, "target_temp_high"));
    if (!std::isfinite(lo) || !std::isfinite(hi) || lo > hi || !std::isfinite(minimum) || !std::isfinite(maximum) || lo < minimum || hi > maximum) return;
    const float delta = std::clamp(direction * step, minimum - lo, maximum - hi);
    if (std::fabs(delta) < 0.0001f) return;
    request_control(id, "climate.set_temperature", {{"target_temp_low", "target_temp_low", std::to_string(lo + delta)},
                            {"target_temp_high", "target_temp_high", std::to_string(hi + delta)}});
    return;
  }
  if (key == "target_temp_low") maximum = std::min(maximum, detail_number(effective(*d, "target_temp_high")));
  if (key == "target_temp_high") minimum = std::max(minimum, detail_number(effective(*d, "target_temp_low")));
  float value = onx_control::stepped(detail_number(effective(*d, key)), minimum, maximum, step, direction);
  if (!std::isfinite(value)) return;
  if (key == "percentage") value = std::round(value);
  if (onx_control::equal(std::to_string(value), effective(*d, key))) return;
  const auto service = has_prefix(id, "climate.") && key != "humidity" ? "climate.set_temperature" : action(*d, key);
  // Range thermostats require both endpoints in the same service call.
  if (key == "target_temp_low" || key == "target_temp_high") {
    const auto other = key == "target_temp_low" ? "target_temp_high" : "target_temp_low";
    request_control(id, service, {{key, key, std::to_string(value)}, {other, other, effective(*d, other), onx_control::equal(effective(*d, other), actual(*d, other))}});
  } else request(*d, service, key, key == "state" ? "value" : key, std::to_string(value));
}
inline void press(const std::string &id) {
  select_control_entity(id);
  const auto *d = entity_detail(id);
  if (!d || !d->available) return;
  const auto key = setting(*d);
  if (!control_preview.empty()) {
    const auto list = onx_control::options(choices(*d, key));
    if (std::find(list.begin(), list.end(), control_preview) != list.end())
      request(*d, action(*d, key), key, key == "state" ? "option" : key == "is_volume_muted" ? "is_volume_muted" : key, control_preview);
    control_preview.clear();
  } else if (has_prefix(id, "climate.")) {
    const auto mode = next_climate_mode(id);
    if (mode.empty()) {
      ESP_LOGW("onx_controls", "Cannot cycle HVAC mode: entity=%s state=%s hvac_modes=%s",
               id.c_str(), d->state.c_str(), d->modes.c_str());
      return;
    }
    ESP_LOGI("onx_controls", "HVAC mode requested: entity=%s from=%s to=%s",
             id.c_str(), effective(*d, "state").c_str(), mode.c_str());
    request(*d, "climate.set_hvac_mode", "state", "hvac_mode", mode);
  } else if (has_prefix(id, "media_player.")) {
    const auto state = effective(*d, "state");
    if (!onx_media::has_playback(state)) return;
    const bool pause = state == "playing" || state == "buffering";
    if (!(d->features & (pause ? 1U : 16384U))) return;
    request(*d, pause ? "media_player.media_pause" : "media_player.media_play", "state", "", pause ? "paused" : "playing");
  } else if (has_prefix(id, "fan.")) {
    const bool on = effective(*d, "state") == "on";
    // Fans predating TURN_ON/TURN_OFF flags still support these standard actions.
    request(*d, on ? "fan.turn_off" : "fan.turn_on", "state", "", on ? "off" : "on");
  } else if (has_prefix(id, "scene.") || has_prefix(id, "button.") || has_prefix(id, "input_button.")) {
    request(*d, domain(id) + (has_prefix(id, "scene.") ? ".turn_on" : ".press"), "state", "", "activated");
  }
}
inline void tick(esphome::api::APIServer *api, const std::string &selected, bool connected) {
  if (!connected || selected.empty() || selected != control_entity) { cancel_controls(); return; }
  auto *d = entity_detail(selected);
  if (!d || !d->available) { cancel_controls(); return; }
  const uint32_t now = millis();
  for (auto &command : control_commands) {
    command.expire(now);
    if (!api || !command.due(now)) continue;
#ifdef USE_API_HOMEASSISTANT_SERVICES
    esphome::api::HomeassistantActionRequest call;
    call.service = esphome::StringRef(command.service);
    call.data.init(command.values.size() + 1);
    esphome::api::HomeassistantServiceMap target;
    target.key = esphome::StringRef("entity_id"); target.value = esphome::StringRef(selected); call.data.push_back(target);
    for (const auto &value : command.values) {
      if (value.parameter.empty()) continue;
      esphome::api::HomeassistantServiceMap item;
      item.key = esphome::StringRef(value.parameter); item.value = esphome::StringRef(value.requested); call.data.push_back(item);
    }
    api->send_homeassistant_action(call);
    ESP_LOGI("onx_controls", "HA action sent: service=%s entity=%s", command.service.c_str(), selected.c_str());
    command.mark_sent(now);
    // Stateless actions have no predictable target state to acknowledge.
    if (has_prefix(selected, "scene.") || has_prefix(selected, "button.") || has_prefix(selected, "input_button."))
      command.active = false;
#endif
  }
}
// Only actual transient feedback belongs over normal metadata. Historical command
// entries remain for confirmed state bookkeeping, but never keep idle hints visible.
inline std::string feedback_text(const std::string &id) {
  if (control_entity != id) return "";
  if (!control_preview.empty()) return "Press to confirm";
  bool failed = false, pending = false, dirty = false, sent = false;
  for (const auto &command : control_commands) {
    failed |= command.failed && millis() - command.error_at < 2500;
    pending |= command.active;
    dirty |= command.active && command.dirty;
    sent |= command.transmitted && millis() - command.sent < 2500;
  }
  if (failed) return "No confirmation";
  if (pending) return dirty ? "Requested..." : "Waiting for HA...";
  return sent ? "Action sent" : "";
}
inline std::string text(const std::string &id) {
  const auto *d = entity_detail(id);
  if (!d || !d->received) return "Waiting for initial data";
  if (!d->available) return "Entity unavailable";
  select_control_entity(id);
  const auto key = setting(*d);
  std::string label = key;
  for (char &c : label) if (c == '_') c = ' ';
  auto display = [](const std::string &s) {
    float n = detail_number(s);
    if (!std::isfinite(n)) return s;
    char buf[24]; snprintf(buf, sizeof(buf), "%.6f", n);
    std::string out = buf;
    while (!out.empty() && out.back() == '0') out.pop_back();
    if (!out.empty() && out.back() == '.') out.pop_back();
    return out;
  };
  std::string value = control_preview.empty() ? effective(*d, key) : control_preview;
  if (key == "range") value = display(effective(*d, "target_temp_low")) + " - " + display(effective(*d, "target_temp_high"));
  else if (key == "volume_level") value = onx_media::volume_percent(detail_number(value));
  else value = display(value);
  if (key == "state" && (has_prefix(id, "scene.") || has_prefix(id, "button.") || has_prefix(id, "input_button."))) label = "Press to activate";
  std::string out = label + "\n" + value;
  if (has_prefix(id, "climate.")) out += "\nNow: " + d->current + " / " + effective(*d, "state");
  const auto feedback = feedback_text(id);
  if (!feedback.empty()) out += "\n" + feedback;
  else if (has_prefix(id, "climate.")) out += "\nPress: HVAC mode";
  else if (has_prefix(id, "fan.")) out += "\nPress: power";
  if (settings(*d).size() > 1) out += "\nHold + turn: setting";
  return out;
}
}  // namespace onx_controls
