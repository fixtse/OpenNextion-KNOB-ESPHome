#include "../onx_timer_state.h"
#include <cassert>

int main() {
  using namespace onx_timer;
  assert(duration_seconds("1:02:03") == 3723);
  assert(duration_seconds("100:00:00") == 360000);
  for (auto bad : {"", "unknown", "None", "-1:00:00", "0:60:00", "0:00:99", "1:2", "999999999999999999:00:00"})
    assert(std::isnan(duration_seconds(bad)));
  auto active = view("active", 120, 120, 1100, 1050.5);
  assert(active.ready && active.seconds_left == 49.5 && active.fraction == 49.5 / 120);
  assert(countdown(active.seconds_left) == "00:50");
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
