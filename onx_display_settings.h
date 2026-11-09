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
