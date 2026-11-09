"""Run production rotary handlers against a Native API capture and HA feedback."""
from pathlib import Path
import subprocess
import tempfile
import unittest


class EntityControls(unittest.TestCase):
    def test_native_handlers(self):
        root = Path(__file__).resolve().parents[1]
        cache = (root / 'custom_logic_entity_sub.h').read_text()
        cache = cache[cache.index('struct EntityDetailState {'):cache.index('class DeviceSlotStateUpdater')]
        handlers = '\n'.join(line for line in (root / 'onx_entity_controls.h').read_text().splitlines()
                             if not line.startswith(('#include', '#pragma')))
        harness = r'''
#include "onx_control_state.h"
#include "onx_media_state.h"
#include <array>
#include <cstdio>
#include <cassert>
#include <utility>
#include <type_traits>
uint32_t clock_ms = 100;
uint32_t millis() { return clock_ms; }
namespace custom_logic { constexpr size_t CONFIGURABLE_DEVICE_SLOT_COUNT = 10; }
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
#define USE_API_HOMEASSISTANT_SERVICES
namespace esphome {
// Mirror ESPHome 2026.10's explicit construction so implicit assignments fail here too.
struct StringRef {
  std::string value;
  StringRef() = default;
  explicit StringRef(const std::string &s) : value(s) {}
  explicit StringRef(const char *s) : value(s) {}
  bool operator==(const char *s) const { return value == s; }
};
static_assert(!std::is_convertible_v<const char *, StringRef>);
namespace api {
struct HomeassistantServiceMap { StringRef key, value; };
struct Data : std::vector<HomeassistantServiceMap> { void init(size_t n) { reserve(n); } };
struct HomeassistantActionRequest { StringRef service; Data data; };
struct APIServer {
  std::vector<HomeassistantActionRequest> sent;
  void send_homeassistant_action(const HomeassistantActionRequest &call) { sent.push_back(call); }
};
}
}
namespace custom_logic_entity_sub {
inline bool has_prefix(const std::string &s, const std::string &prefix) { return s.rfind(prefix, 0) == 0; }
CACHE
}
HANDLERS
int main() {
  using namespace custom_logic_entity_sub;
  using namespace onx_controls;
  esphome::api::APIServer api;
  auto &d = entity_details[0];
  d.entity_id = "climate.test"; d.state = "off"; d.received = d.available = true;
  d.modes = "['off', 'heat', 'cool']"; d.features = 1; d.target = "20";
  d.minimum = 16; d.maximum = 30; d.step = .5;
  rotate(d.entity_id, 1); rotate(d.entity_id, 1); rotate(d.entity_id, 1);
  assert(effective(d, "temperature") == "21.500000");
  press(d.entity_id); press(d.entity_id);
  assert(effective(d, "state") == "cool");
  assert(effective(d, "temperature") == "21.500000");
  clock_ms += 100; tick(&api, d.entity_id, true);
  assert(api.sent.size() == 2);
  for (const auto &call : api.sent) {
    assert(call.data[0].key == "entity_id" && call.data[0].value == "climate.test");
    assert(call.service == "climate.set_temperature" || call.service == "climate.set_hvac_mode");
  }
  d.target = "20.5";
  for(auto &c : control_commands)c.feedback("temperature", d.target);
  assert(effective(d, "temperature") == "21.500000");
  clock_ms += 5000; tick(&api, d.entity_id, true);
  assert(effective(d, "temperature") == "20.5");
  assert(text(d.entity_id).find("No confirmation") != std::string::npos);
  // A real HA climate list can contain StrEnum repr wrappers. An off thermostat
  // must queue and transmit the mode even with no temperature capability/target.
  for (const auto &modes : {"[<HVACMode.OFF: 'off'>, <HVACMode.HEAT: 'heat'>]",
                            "['off', 'heat']", "[\"off\", \"heat\"]"}) {
    cancel_controls(); d.state = "off"; d.modes = modes; d.features = 0; d.target = "None";
    press(d.entity_id);
    assert(control_commands.size() == 1);
    assert(effective(d, "state") == "heat");
    const auto before = api.sent.size(); clock_ms += 100; tick(&api, d.entity_id, true);
    assert(api.sent.size() == before + 1);
    const auto &call = api.sent.back();
    assert(call.service == "climate.set_hvac_mode" && call.data.size() == 2);
    assert(call.data[1].key == "hvac_mode" && call.data[1].value == "heat");
    // Cached off feedback must not replace the requested heat display.
    for (auto &c : control_commands) c.feedback("state", "off");
    assert(effective(d, "state") == "heat");
    d.state = "heat";
    for (auto &c : control_commands) c.feedback("state", d.state);
    assert(!control_commands[0].active);
    press(d.entity_id); assert(effective(d, "state") == "off");
  }
  // Expired status must restore idle metadata despite retained command records.
  clock_ms += 100; tick(&api, d.entity_id, true);
  d.state = "off";
  for (auto &c : control_commands) c.feedback("state", d.state);
  clock_ms += 3000; tick(&api, d.entity_id, true);
  assert(feedback_text(d.entity_id).empty());
  cancel_controls(); d.features = 2; d.target = "None"; d.low = "19"; d.high = "23";
  rotate(d.entity_id, 1);
  assert(effective(d, "target_temp_low") == "19.500000");
  assert(effective(d, "target_temp_high") == "23.500000");
  cycle(d.entity_id, 1); rotate(d.entity_id, -1);
  assert(effective(d, "target_temp_low") == "19.000000");
  assert(effective(d, "target_temp_high") == "23.500000");
  // Once a range has been confirmed, changing low alone must acknowledge from low feedback alone.
  d.low = "19"; d.high = "23.5";
  cancel_controls(); select_control_entity(d.entity_id); control_setting = "target_temp_low";
  rotate(d.entity_id, 1); clock_ms += 100; tick(&api, d.entity_id, true);
  control_commands[0].feedback("target_temp_low", "19.5"); assert(!control_commands[0].active);
  cancel_controls(); d = EntityDetailState{}; d.entity_id = "fan.test";
  d.available = d.received = true; d.state = "on"; d.percentage = "33";
  d.features = 1; d.step = 100.0f / 3;
  rotate(d.entity_id, 1); assert(effective(d, "percentage") == "67.000000");
  rotate(d.entity_id, 1); assert(effective(d, "percentage") == "100.000000");
  assert(settings(d).size() == 1); // No unsupported oscillation/direction/presets.
  cancel_controls(); d.entity_id = "select.test"; d.state = "Eco, quiet";
  d.options = "[\"Eco, quiet\", \"Owner's preset\"]";
  rotate(d.entity_id, 1); assert(control_commands.empty());
  assert(control_preview == "Owner's preset"); press(d.entity_id);
  assert(control_commands[0].service == "select.select_option");
  assert(control_commands[0].values[0].parameter == "option");
  const auto count = api.sent.size(); tick(&api, d.entity_id, false);
  clock_ms += 1000; tick(&api, d.entity_id, true);
  assert(api.sent.size() == count && control_commands.empty());
  cancel_controls(); d.entity_id = "number.test"; d.state = "2.5";
  d.minimum = 1; d.maximum = 3; d.step = .5;
  rotate(d.entity_id, 1); assert(effective(d, "state") == "3.000000");
  rotate(d.entity_id, 1); assert(effective(d, "state") == "3.000000");
  cancel_controls(); d.entity_id = "button.test"; d.state = "unknown";
  press(d.entity_id); clock_ms += 100; tick(&api, d.entity_id, true);
  assert(api.sent.back().service == "button.press");
  assert(api.sent.back().data.size() == 1); // Never send a fictitious state parameter.
  assert(!control_commands.back().active);
  assert(feedback_text(d.entity_id) == "Action sent");
  clock_ms += 2500; tick(&api, d.entity_id, true);
  assert(feedback_text(d.entity_id).empty());
  assert(text(d.entity_id).find("Turn to adjust") == std::string::npos);

}
'''.replace('CACHE', cache).replace('HANDLERS', handlers)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            (path / 'test.cpp').write_text(harness)
            subprocess.run(['c++', '-std=c++17', '-I' + str(root), str(path / 'test.cpp'),
                            '-o', str(path / 'test')], check=True)
            subprocess.run([str(path / 'test')], check=True)


if __name__ == '__main__':
    unittest.main()
