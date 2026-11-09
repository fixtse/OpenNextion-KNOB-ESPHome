#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace onx_playlist {
constexpr size_t MAX_ENTRIES = 8;
constexpr uint32_t OPEN_MS = 500, BACK_MS = 1500;
struct Entry { char name[49]{}, action[129]{}; };
struct Config {
  uint32_t magic{0x4F4E5851};
  char entity[129]{};
  uint8_t count{0};
  Entry entries[MAX_ENTRIES]{};
};
inline std::array<Config, 10> configs;
struct Browser {
  std::string entity;
  int selected{0};
  bool open{false};
  uint32_t sent_ms{0};
  bool sent{false};
  void clear() { entity.clear(); selected=0; open=false; sent=false; }
  void toggle(const std::string &id) {
    if (open && entity==id) { clear(); return; }
    clear(); entity=id; open=true;
  }
  void rotate(int direction, size_t count) {
    if (!open || !count) return;
    selected=(selected+(direction<0?-1:1)+static_cast<int>(count))%static_cast<int>(count);
    sent=false;
  }
};
inline Browser browser;
inline bool medium_press(uint32_t held) { return held>=OPEN_MS && held<BACK_MS; }
inline bool action_valid(const std::string &value) {
  const auto dot=value.find('.');
  if (dot==std::string::npos || (value.substr(0,dot)!="script" && value.substr(0,dot)!="automation") || dot+1==value.size() || value.size()>128) return false;
  for(size_t i=dot+1;i<value.size();++i)
    if(!((value[i]>='a' && value[i]<='z') || (value[i]>='0' && value[i]<='9') || value[i]=='_')) return false;
  return true;
}
}
