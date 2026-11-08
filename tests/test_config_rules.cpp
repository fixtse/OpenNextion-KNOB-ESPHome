#include "../onx_config_rules.h"
#include <cassert>
int main() {
  using namespace onx_config;
  assert(valid_entity("sensor.room_temperature"));
  assert(valid_action("script.goodnight"));
  assert(valid_action("automation.welcome_home"));
  assert(valid_action("none"));
  assert(valid_action(""));
  for (auto value : {"light.kitchen", "script.", "automation.foo;bar", "script.foo bar", "script.foo\n", "script.foo.bar", "SCRIPT.foo"})
    assert(!valid_action(value));
  assert(!valid_entity(std::string(130, 'a') + ".foo"));
  assert(!binding_matches("sensor.old", "sensor.new"));
  assert(!binding_matches("", ""));
  assert(binding_matches("binary_sensor.door", "binary_sensor.door"));
}
