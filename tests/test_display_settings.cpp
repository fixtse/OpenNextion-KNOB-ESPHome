#include "../onx_display_settings.h"
#include <cassert>
int main() {
  using namespace onx_display_settings;
  uint32_t color=0;
  assert(parse_color("#62b0ff",color)&&color==0x62B0FF&&color_text(color)=="#62B0FF");
  for(const auto &bad : {"", "red", "#fff", "#0000000", "#12zz00", "123456"})assert(!parse_color(bad,color));
  uint8_t scope=255;
  assert(parse_scope("main",scope)&&scope==0);
  assert(standby_allowed(scope,false)&&!standby_allowed(scope,true));
  assert(parse_scope("all",scope)&&standby_allowed(scope,false)&&standby_allowed(scope,true));
  assert(parse_scope("never",scope)&&!standby_allowed(scope,false)&&!standby_allowed(scope,true));
  assert(!parse_scope("invalid",scope));
  assert(current.standby_scope==0&&current.media_color==0x62B0FF);
}
