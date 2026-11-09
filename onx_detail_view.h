#pragma once
#include "onx_entity_controls.h"
#include "onx_icon_state.h"
#include <cstdio>
namespace onx_detail_view {
struct Line { std::string text; int y, size, width; uint32_t color{0xFFFFFF}; int x{0}; };
struct View {
  std::string layout{"value"}, icon, symbol;
  Line badge{"", 36, 16, 156}, value{"--", 75, 32, 208}, context{"", 119, 16, 200},
       hint{"", 147, 14, 198}, name{"", 185, 16, 180};
  uint32_t accent{0x62B0FF};
  int icon_y{32}, icon_size{28}, icon_x{-62};
  bool available{false};
};
inline std::string compact(const std::string &value) {
  const float n = onx_control::number(value);
  if (!std::isfinite(n)) return value;
  char text[40]; snprintf(text, sizeof(text), "%.2f", n);
  std::string out(text);
  while (!out.empty() && out.back() == '0') out.pop_back();
  if (!out.empty() && out.back() == '.') out.pop_back();
  return out == "-0" ? "0" : out;
}
inline std::string sensor_class(const std::string &entity) {
  for (const auto &m : onx_icon_state::metadata) if (m.entity == entity) return m.device_class;
  return "";
}
inline std::string label(std::string key) {
  for (char &c : key) if (c == '_') c = ' ';
  if (!key.empty()) key[0] = std::toupper(static_cast<unsigned char>(key[0]));
  return key;
}
inline std::string selected_setting(const custom_logic_entity_sub::EntityDetailState &d) {
  const auto settings = onx_controls::settings(d);
  const auto &current = custom_logic_entity_sub::control_setting;
  return custom_logic_entity_sub::control_entity == d.entity_id &&
      std::find(settings.begin(), settings.end(), current) != settings.end() ? current : settings.front();
}
inline View build(const std::string &entity, const std::string &name) {
  using namespace custom_logic_entity_sub;
  View v; v.badge.x = 16; v.name.text = onx_media::display_text(name); v.icon = onx_icon_state::selected_icon(entity);
  if (v.icon.empty()) v.icon = "mdi:" + onx_icon_state::fallback(entity);
  const auto *d = entity_detail(entity);
  if (!d || !d->received || !d->available) {
    v.context.text = !d || !d->received ? "Waiting for initial data" : "Entity unavailable";
    v.hint.text = "Press to retry"; return v;
  }
  v.available = true;
  v.value.text = compact(d->state) + (d->unit.empty() ? "" : " " + d->unit);
  const auto feedback = onx_controls::feedback_text(entity);
  v.hint.text = feedback;
  if (has_prefix(entity, "climate.")) {
    const auto mode = onx_controls::effective(*d, "state");
    std::string mode_label = mode;
    if (mode == "heat") { v.symbol = "🔥"; mode_label = "Heat"; v.accent = 0xFFAA66; }
    else if (mode == "cool") { v.symbol = "❄"; mode_label = "Cool"; }
    else if (mode == "heat_cool" || mode == "auto") { v.symbol = "☀"; mode_label = "Auto"; }
    else if (mode == "dry") { v.symbol = "💧"; mode_label = "Dry"; }
    else if (mode == "fan_only") { v.symbol = "🌀"; mode_label = "Fan"; }
    else { v.symbol = "⏸"; mode_label = mode == "off" ? "Off" : mode; v.accent = 0xAAAAAA; }
    v.badge.text = v.symbol + " " + mode_label;
    const auto key = selected_setting(*d);
    const auto unit = d->unit.empty() ? "°" : d->unit;
    v.value.text = compact(onx_controls::effective(*d, key));
    if (key == "temperature" || key == "target_temp_low" || key == "target_temp_high") v.value.text += unit;
    if (key == "range") v.value.text = compact(onx_controls::effective(*d,"target_temp_low")) + "–" + compact(onx_controls::effective(*d,"target_temp_high")) + unit;
    v.context.text = "Now " + compact(d->current) + unit;
    if (key != "temperature" && key != "range") v.context.text = label(key) + " · " + v.context.text;
    if (v.hint.text.empty()) v.hint.text = "Press: mode\nHold + turn: setting";
  } else if (has_prefix(entity, "fan.")) {
    const auto state = onx_controls::effective(*d,"state");
    v.symbol = state == "off" ? "⏸" : "🌀";
    v.badge.text = v.symbol + (state == "off" ? " Off" : " Fan");
    const auto key = selected_setting(*d);
    v.value.text = key == "percentage" ? compact(onx_controls::effective(*d,key)) + "%" : onx_controls::effective(*d,key);
    v.context.text = d->preset.empty() || d->preset == "None" ? "Fan speed" : d->preset;
    if (v.hint.text.empty()) v.hint.text = "Press: power\nHold + turn: setting";
  } else if (has_prefix(entity,"sensor.") || has_prefix(entity,"binary_sensor.")) {
    const auto kind = sensor_class(entity);
    if (kind == "temperature" || d->unit == "°C" || d->unit == "°F") { v.symbol = "🌡"; v.badge.text = "🌡 Temperature"; }
    else if (kind == "humidity") { v.symbol = "💧"; v.badge.text = "💧 Humidity"; }
    else if (kind == "carbon_dioxide") { v.symbol = "☁"; v.badge.text = "☁ Carbon dioxide"; }
    else if (kind == "power" || kind == "energy") { v.symbol = "⚡"; v.badge.text = "⚡ " + std::string(kind == "power" ? "Power" : "Energy"); }
    else v.badge.text = has_prefix(entity,"binary_sensor.") ? "Status" : "Sensor";
    if (has_prefix(entity,"binary_sensor.")) v.value.text = d->state == "on" ? "On" : d->state == "off" ? "Off" : d->state;
    v.value.size = v.value.text.size() > 12 ? 24 : 32;
    v.value.y = 84; v.badge.x = 0;
    v.icon_x = 0; v.icon_y = 134; v.icon_size = 40;
  } else if (has_prefix(entity,"media_player.")) {
    v.layout = "media"; v.icon_x=0; v.icon_y=18; v.icon_size=80;
    const float volume = onx_control::number(onx_controls::effective(*d,"volume_level"));
    v.value = {"Volume: " + onx_media::volume_percent(volume),166,18,196};
    v.context = {onx_media::display_text(d->media_title.empty()?d->state:d->media_title),122,16,196};
    v.hint = {onx_media::display_text(feedback.empty()?d->media_artist:feedback),144,16,196,0xD2D2D2};
    v.badge.text.clear(); v.name.y=192; v.name.width=180;
  } else if (onx_controls::handles(entity)) {
    const auto key = selected_setting(*d);
    v.badge.text = label(key);
    v.value.text = compact(onx_controls::effective(*d,key));
    if (!d->unit.empty()) v.value.text += " " + d->unit;
    if (v.hint.text.empty() && (has_prefix(entity,"select.") || has_prefix(entity,"input_select."))) v.hint.text = "Turn to choose · Press to confirm";
  }
  if (v.value.text.size() > 14) v.value.size = std::min(v.value.size, 24);
  v.badge.color = v.accent;
  return v;
}
inline View carousel(const std::string &entity, const std::string &name) {
  using namespace custom_logic_entity_sub;
  auto v = build(entity, name); v.layout = "carousel";
  const auto badge = v.badge.text;
  v.badge.text.clear(); v.context.text.clear(); v.hint.text.clear();
  v.icon_x = 0; v.icon_y = 34; v.icon_size = 28;
  v.value.y = 84; v.value.size = 32;
  v.name.y = 145; v.name.width = 184; v.name.size = 16;
  const auto *d = entity_detail(entity);
  if (!d || !d->received || !d->available) return v;
  const auto domain = entity.substr(0,entity.find('.'));
  if (domain == "light" || domain == "switch") {
    v.value.text = d->state == "on" ? "On" : d->state == "off" ? "Off" : label(d->state);
    const float brightness = onx_control::number(d->brightness);
    if (domain == "light" && d->state == "on" && std::isfinite(brightness))
      v.context.text = onx_media::volume_percent(brightness / 255.0f) + " brightness";
  } else if (domain == "cover") {
    const float position = onx_control::number(d->position);
    v.value.text = std::isfinite(position) ? onx_media::volume_percent(position / 100.0f) : label(d->state);
    v.context.text = label(d->state);
  } else if (domain == "climate" || domain == "fan") {
    v.context.text = badge;
  } else if (domain == "media_player") {
    v.value.text = label(d->state);
    v.context.text = onx_media::display_text(d->media_title);
  } else if (domain == "automation") {
    v.value.text = d->state == "on" ? "Enabled" : d->state == "off" ? "Disabled" : label(d->state);
  } else if (domain == "script") {
    v.value.text = d->state == "on" ? "Running" : d->state == "off" ? "Idle" : label(d->state);
  } else if (domain == "scene" || domain == "button" || domain == "input_button") {
    v.value.text = "Ready";
  } else if (domain == "weather") {
    v.value.text = compact(d->target) + d->unit; v.context.text = label(d->state);
  } else if (domain == "timer") {
    v.value.text = label(d->state); v.context.text = d->timer_remaining;
  }
  if (!v.context.text.empty()) { v.context.y = 124; v.context.size = 14; v.name.y = 151; }
  if (v.value.text.size() > 14) v.value.size = 24;
  return v;
}
// LVGL keeps the unscaled object box and scales about its center.
inline int image_object_y(int visible_top, int size, int source_size = 80) {
  return visible_top - (source_size - size) / 2;
}
inline std::string pagination(int selected, int count) {
  std::string out;
  for (int i = 0; i < count; ++i) { if (i) out += " "; out += i == selected ? "●" : "·"; }
  return out;
}
}  // namespace onx_detail_view
