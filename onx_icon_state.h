#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace onx_icon_state {
inline bool valid_icon(const std::string &icon) {
  if (icon.size() < 5 || icon.size() > 80 || icon.rfind("mdi:", 0) != 0) return false;
  for (size_t i = 4; i < icon.size(); ++i)
    if (!((icon[i] >= 'a' && icon[i] <= 'z') || (icon[i] >= '0' && icon[i] <= '9') || icon[i] == '-')) return false;
  return true;
}
inline std::string download_url(const std::string &icon) {
  return valid_icon(icon) ? "https://api.iconify.design/mdi/" + icon.substr(4) + ".svg?color=%23ffffff&width=80&height=80" : "";
}
inline std::string backup_url(const std::string &icon) {
  return valid_icon(icon) ? "https://raw.githubusercontent.com/Templarian/MaterialDesign/master/svg/" + icon.substr(4) + ".svg" : "";
}
struct Retry {
  uint32_t failed_at{0}, delay{5000};
  unsigned attempts{0};
  bool due(uint32_t now) const { return now - failed_at >= delay; }
  void fail(uint32_t now) {
    static const uint32_t delays[] = {5000, 15000, 60000, 300000};
    failed_at = now; delay = delays[attempts < 4 ? attempts : 3];
    if (attempts < 4) ++attempts;
  }
};
inline std::string fallback(const std::string &entity, const std::string &type = "") {
  const auto domain = entity.substr(0, entity.find('.'));
  if (type == "plug") return "power-plug";
  if (domain == "light") return "lightbulb";
  if (domain == "cover") return "window-shutter";
  if (domain == "climate") return "thermostat";
  if (domain == "fan") return "fan";
  if (domain == "media_player") return "music-note";
  if (domain == "sensor") return "gauge";
  if (domain == "binary_sensor") return "checkbox-marked-circle-outline";
  if (domain == "number" || domain == "input_number") return "counter";
  if (domain == "select" || domain == "input_select") return "format-list-bulleted";
  if (domain == "scene") return "palette";
  if (domain == "button" || domain == "input_button") return "gesture-tap-button";
  if (domain == "script") return "script-text";
  if (domain == "automation") return "robot";
  if (domain == "weather") return "weather-partly-cloudy";
  if (domain == "timer") return "timer-outline";
  return "toggle-switch";
}
inline std::string weather_icon(const std::string &condition) {
  if (condition == "partlycloudy") return "mdi:weather-partly-cloudy";
  if (condition == "clear-night") return "mdi:weather-night";
  if (condition == "lightning-rainy") return "mdi:weather-lightning-rainy";
  if (condition == "snowy-rainy") return "mdi:weather-snowy-rainy";
  if (condition == "windy-variant") return "mdi:weather-windy-variant";
  for (const char *name : {"sunny", "cloudy", "fog", "hail", "lightning", "pouring", "rainy", "snowy", "windy"})
    if (condition == name) return std::string("mdi:weather-") + name;
  return "mdi:weather-partly-cloudy";
}
struct Metadata {
  std::string entity, configured, runtime, device_class, condition;
  bool registry_override{false};
  void bind(const std::string &id) {
    if (entity != id) { *this = Metadata{}; entity = id; }
  }
  std::string chosen() const {
    if (registry_override && valid_icon(configured)) return configured;
    if (valid_icon(runtime)) return runtime;
    if (entity.rfind("weather.", 0) == 0 && !condition.empty()) return weather_icon(condition);
    if (valid_icon(configured)) return configured;
    if (entity.rfind("sensor.", 0) == 0) {
      if (device_class == "temperature") return "mdi:thermometer";
      if (device_class == "humidity") return "mdi:water-percent";
      if (device_class == "illuminance") return "mdi:brightness-5";
      if (device_class == "battery") return "mdi:battery";
      if (device_class == "power") return "mdi:flash";
      if (device_class == "energy") return "mdi:lightning-bolt";
      if (device_class == "pressure" || device_class == "atmospheric_pressure") return "mdi:gauge";
    }
    return "";
  }
};
inline std::array<Metadata, 20> metadata;
inline std::string selected_icon(const std::string &entity) {
  for (const auto &item : metadata) if (item.entity == entity) return item.chosen();
  return "";
}
}  // namespace onx_icon_state
