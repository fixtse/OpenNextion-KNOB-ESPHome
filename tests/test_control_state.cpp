#include "../onx_control_state.h"
#include <cassert>
#include <limits>
using namespace onx_control;
int main() {
  assert(options("['off', 'heat', 'cool']") == (std::vector<std::string>{"off", "heat", "cool"}));
  assert(options("[<HVACMode.OFF: 'off'>, <HVACMode.HEAT: 'heat'>]") ==
         (std::vector<std::string>{"off", "heat"}));
  assert(options("[<HVACMode.OFF: 'off'>, \"heat\", 'cool']") ==
         (std::vector<std::string>{"off", "heat", "cool"}));
  for (const auto &raw : {"[<HVACMode.HEAT: 'heat']", "[<HVACMode.HEAT 'heat'>]",
                          "[<.HEAT: 'heat'>]", "[<HVACMode.: 'heat'>]", "[<HVACMode.HEAT: heat>]"})
    assert(options(raw).empty());
  assert(options("[\"Eco, quiet\", \"Owner's preset\"]")[1] == "Owner's preset");
  assert(options("['Owner\\'s preset', 'auto']")[0] == "Owner's preset");
  for (const auto &raw : {"None", "unknown", "[heat]", "['heat'", "['heat'] garbage"}) assert(options(raw).empty());
  assert(stepped(20, 16, 30, .5, 1) == 20.5);
  assert(stepped(30, 16, 30, .5, 1) == 30);
  assert(stepped(16, 16, 30, .5, -1) == 16);
  assert(stepped(72, 60, 85, 1, 1) == 73);
  assert(std::isnan(stepped(20, 16, 30, 0, 1)));
  assert(std::isnan(stepped(20, NAN, 30, .5, 1)));
  assert(!equal("1.0005", "1"));
  assert(equal("20.500000", "20.5"));
  Command c;
  c.request("climate.test", "climate.set_temperature", {{"temperature", "temperature", "20.5"}}, 100);
  assert(!c.due(179)); assert(c.due(180));
  c.feedback("temperature", "20.5"); assert(c.active); // A cached value cannot acknowledge an unsent command.
  c.mark_sent(180);
  c.request("climate.test", "climate.set_temperature", {{"temperature", "temperature", "21"}}, 190);
  assert(c.effective("temperature", "20") == "21");
  c.feedback("temperature", "20.5"); assert(c.active); // Intermediate feedback cannot steal the last detent.
  assert(!c.due(300)); assert(c.due(380)); c.mark_sent(380);
  c.feedback("temperature", "21.000000"); assert(!c.active);
  c.request("climate.test", "climate.set_temperature", {{"target_temp_low", "target_temp_low", "19.5"},
                                                        {"target_temp_high", "target_temp_high", "23.5"}}, 400);
  c.mark_sent(480);
  c.feedback("target_temp_low", "19.5"); assert(c.active);
  c.feedback("target_temp_high", "23.5"); assert(!c.active);
  c.request("fan.test", "fan.set_percentage", {{"percentage", "percentage", "66.67"}}, 500);
  c.mark_sent(580); c.expire(5579); assert(c.active);
  c.expire(5580); assert(!c.active && c.failed);
  assert(c.effective("percentage", "33.33") == "33.33");
  c.expire(8080); assert(!c.failed);
  // Continuous rotation is flushed at a bounded rate even without a quiet period.
  c.cancel();
  for (uint32_t t = 0; t <= 240; t += 20) c.request("number.test", "number.set_value", {{"state", "value", "5"}}, t);
  assert(!c.due(249)); assert(c.due(250));
  c.cancel(); assert(!c.active && !c.due(10000));
  c.request("climate.test", "climate.set_temperature", {{"target_temp_low", "target_temp_low", "20"},
                                                        {"target_temp_high", "target_temp_high", "24", true}}, 10000);
  c.mark_sent(10080);
  c.feedback("target_temp_low", "20"); assert(!c.active); // The unchanged endpoint need not be resent by HA.
  // Wraparound-safe timers.
  c.request("fan.test", "fan.set_percentage", {{"percentage", "percentage", "50"}}, UINT32_MAX - 50);
  assert(c.due(30)); c.mark_sent(30); c.expire(5030); assert(c.failed);
}
