#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
namespace onx_display_settings {
struct Settings {
  uint32_t magic{0x4F4E5844}, media_color{0x62B0FF};
  uint8_t standby_scope{0}; // 0 main carousel only, 1 all views, 2 disabled
};
inline Settings current;
struct DateTimeSettings {
  uint32_t magic{0x4F4E5854};
  uint8_t date_format{0}, hour_format{0}; // DD-MM, MM-DD, ISO; 24 / 12 hour
};
inline DateTimeSettings datetime;
inline std::string date_format_text(uint8_t value) { return value == 1 ? "mm-dd" : value == 2 ? "yyyy-mm-dd" : "dd-mm"; }
inline bool parse_date_format(const std::string &text, uint8_t &value) {
  if (text == "dd-mm") value = 0;
  else if (text == "mm-dd") value = 1;
  else if (text == "yyyy-mm-dd") value = 2;
  else return false;
  return true;
}
inline bool parse_hour_format(const std::string &text, uint8_t &value) {
  if (text == "24") value = 0;
  else if (text == "12") value = 1;
  else return false;
  return true;
}
inline const char *date_pattern() { return datetime.date_format == 1 ? "%m-%d" : datetime.date_format == 2 ? "%Y-%m-%d" : "%d-%m"; }
inline const char *time_pattern() { return datetime.hour_format == 1 ? "%I:%M %p" : "%H:%M"; }
inline std::string format_iso_date(const std::string &iso) {
  if (iso.size() < 10 || iso[4] != '-' || iso[7] != '-') return iso;
  for (size_t i : {size_t(0),size_t(1),size_t(2),size_t(3),size_t(5),size_t(6),size_t(8),size_t(9)})
    if (iso[i] < '0' || iso[i] > '9') return iso;
  return datetime.date_format == 1 ? iso.substr(5,5) : datetime.date_format == 2 ? iso.substr(0,10) : iso.substr(8,2) + "-" + iso.substr(5,2);
}
inline bool parse_color(const std::string &text, uint32_t &color) {
  if (text.size() != 7 || text[0] != '#') return false;
  uint32_t value = 0;
  for (size_t i = 1; i < 7; ++i) {
    const char c = text[i];
    int digit = c >= '0' && c <= '9' ? c-'0' : c >= 'a' && c <= 'f' ? c-'a'+10 : c >= 'A' && c <= 'F' ? c-'A'+10 : -1;
    if (digit < 0) return false;
    value = (value << 4) | digit;
  }
  color = value; return true;
}
inline std::string color_text(uint32_t color) {
  char text[8]; snprintf(text, sizeof(text), "#%06X", static_cast<unsigned>(color & 0xFFFFFF)); return text;
}
inline bool parse_scope(const std::string &text, uint8_t &scope) {
  if (text == "main") scope = 0;
  else if (text == "all") scope = 1;
  else if (text == "never") scope = 2;
  else return false;
  return true;
}
inline std::string scope_text(uint8_t scope) { return scope == 1 ? "all" : scope == 2 ? "never" : "main"; }
inline bool standby_allowed(uint8_t scope, bool detail_active) { return scope != 2 && (scope == 1 || !detail_active); }
}
