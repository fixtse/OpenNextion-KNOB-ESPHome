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
struct Metadata {
  std::string entity, configured, runtime;
  bool registry_override{false};
  void bind(const std::string &id) {
    if (entity != id) { *this = Metadata{}; entity = id; }
  }
  std::string chosen() const {
    if (registry_override && valid_icon(configured)) return configured;
    return valid_icon(runtime) ? runtime : valid_icon(configured) ? configured : "";
  }
};
inline std::array<Metadata, 10> metadata;
inline std::string selected_icon(const std::string &entity) {
  for (const auto &item : metadata) if (item.entity == entity) return item.chosen();
  return "";
}
}  // namespace onx_icon_state
