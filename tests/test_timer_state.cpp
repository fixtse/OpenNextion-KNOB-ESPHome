#include "../onx_timer_state.h"
#include <cassert>

int main() {
  using namespace onx_timer;
  assert(!popup_due(9999,true,true));assert(popup_due(10000,true,true));
  assert(!popup_due(1500,false,true));assert(popup_due(1500,false,false));
  assert(attribute_json("[{'status': 'set', 'remaining': None}]")=="[{\"status\": \"set\", \"remaining\": null}]");
  assert(attribute_json("{'name': 'bad").empty());
  assert(attribute_json(std::string(8193,'x')).empty());
  assert(duration_seconds("1:02:03") == 3723);
  assert(duration_seconds("100:00:00") == 360000);
  for (auto bad : {"", "unknown", "None", "-1:00:00", "0:60:00", "0:00:99", "1:2", "999999999999999999:00:00"})
    assert(std::isnan(duration_seconds(bad)));
  auto active = view("active", 120, 120, 1100, 1050.5);
  assert(active.ready && active.seconds_left == 49.5 && active.fraction == 49.5 / 120);
  assert(countdown(active.seconds_left) == "00:50");
  // Real Nest deadline: float promotion used to round this down by 16s.
  const int64_t nest_finish = 1794403600;
  assert(deadline_epoch(true, nest_finish) == 1794403600.0);
  assert(std::isnan(deadline_epoch(false, nest_finish)));
  assert(countdown(view("active", 900, NAN, deadline_epoch(true, nest_finish),
                        1794402733.0).seconds_left) == "14:27");
  // Neighboring deadlines can round up too; preserve every second.
  for (int64_t finish = nest_finish - 128; finish <= nest_finish + 128; ++finish)
    assert(view("active", 900, NAN, deadline_epoch(true, finish),
                static_cast<double>(finish - 60)).seconds_left == 60);
  assert(countdown(3601) == "1:00:01");
  assert(countdown(NAN) == "--:--");
  assert(!view("active", 60, 60, NAN, 10).ready);
  assert(!view("active", 60, 60, 100, 0).ready);
  assert(view("paused", 60, 30, NAN, 0).seconds_left == 30);
  assert(view("paused", 60, 30, 100, 1000).seconds_left == 30);
  assert(view("idle", NAN, NAN, NAN, 0).ready);
  assert(view("ringing", 0, 0, 0, 0).ready);
  assert(!view("unavailable", 60, 30, 100, 10).ready);
  assert(view("active", 60, 60, 100, 101).seconds_left == 0);
  assert(view("active", 60, 60, 200, 100).fraction == 1);
}
