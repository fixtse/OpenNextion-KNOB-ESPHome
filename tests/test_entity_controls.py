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
        presentation = '\n'.join(line for line in (root / 'onx_detail_view.h').read_text().splitlines()
                                 if not line.startswith(('#include', '#pragma')))
        harness = r'''
#include "onx_control_state.h"
#include "onx_media_state.h"
#include "onx_icon_state.h"
#include "onx_display_settings.h"
#include "onx_sensor_view.h"
#include "onx_timer_state.h"
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
PRESENTATION
int main() {
  using namespace custom_logic_entity_sub;
  using namespace onx_controls;
  {
    auto &weather = entity_details[9]; weather.entity_id="weather.home";
    weather.state="partlycloudy"; weather.target="17.65"; weather.unit="°C";
    weather.received=weather.available=true; weather.humidity="52";
    auto view=onx_detail_view::build(weather.entity_id,"Home");
    assert(view.value.text=="17.65°C" && view.badge.text=="Partly cloudy");
    assert(view.hint.text.find("52%")!=std::string::npos);
    weather.weather_page=1;
    view=onx_detail_view::build(weather.entity_id,"Home");
    assert(view.value.text=="—" && view.badge.text=="Wind & atmosphere");
    weather.weather_page=2;
    assert(onx_detail_view::build(weather.entity_id,"Home").value.text=="Not available");
    weather.forecast_pending=true;weather.forecast_requested_ms=clock_ms;
    assert(onx_detail_view::build(weather.entity_id,"Home").value.text=="Loading…");
    clock_ms+=20000;
    assert(onx_detail_view::build(weather.entity_id,"Home").value.text=="Not available");
    weather.forecast_count=1;weather.weather_page=3;
    weather.forecast[0]={"2026-11-10T12:00:00+00:00","rainy","18","9","80"};
    view=onx_detail_view::build(weather.entity_id,"Home");
    assert(view.badge.text=="10-11" && view.value.text=="18°C" && view.context.text=="Low 9°C");
    assert(view.hint.text.find("80%")!=std::string::npos);
    view=onx_detail_view::carousel(weather.entity_id,"Home");
    assert(view.value.text=="17.65°C" && view.name.text=="Home" && !view.icon.empty());
    weather={};clock_ms=100;
  }
  {
    auto &sensor = entity_details[1];
    sensor.entity_id="sensor.madrid_piso_temperature"; sensor.state="16.63";sensor.unit="°C";sensor.received=sensor.available=true;
    onx_icon_state::metadata[1].bind(sensor.entity_id);onx_icon_state::metadata[1].device_class="temperature";
    auto view=onx_detail_view::build(sensor.entity_id,"Madrid Piso Temperature");
    assert(view.value.text=="16.63 °C"&&view.value.size==32&&view.icon_size==40);
    assert(view.badge.text=="🌡 Temperature"&&view.name.width==180);
    const auto main_view=onx_detail_view::carousel(sensor.entity_id,"Madrid Piso Temperature");
    assert(main_view.value.text=="16.63 °C"&&main_view.icon_size==40&&main_view.name.y==145);
    assert(view.badge.x==0&&view.icon_x==0);
    assert(view.icon_y>=view.value.y+view.value.size+16);
    assert(onx_detail_view::image_object_y(view.icon_y,view.icon_size)+40-view.icon_size/2==view.icon_y);
    assert(main_view.icon_y+main_view.icon_size+12<=main_view.value.y);
    assert(main_view.name.y+2*(main_view.name.size+2)+24<=212);
    assert(onx_detail_view::pagination(4,7)=="· · · · ● · ·");
    auto &climate=entity_details[2];climate.entity_id="climate.heater";climate.received=climate.available=true;
    climate.features=1;climate.state="heat";climate.target="21.5";climate.current="23.0";
    control_entity="climate.heater";control_setting="temperature";
    view=onx_detail_view::build(climate.entity_id,"Calentador");
    assert(view.badge.text=="🔥 Heat"&&view.badge.size==24&&view.badge.x==0&&view.icon.empty()&&view.value.text=="21.5°"&&view.context.text=="Now 23°");
    climate.state="cool";view=onx_detail_view::build(climate.entity_id,"Calentador");assert(view.badge.text=="❄ Cool");
    // Rendering a different slot must not change the setting being adjusted on the device.
    onx_detail_view::build(sensor.entity_id,"Temperature");assert(control_setting=="temperature");
    auto &fan=entity_details[3];fan.entity_id="fan.office";fan.received=fan.available=true;fan.state="on";
    fan.features=1;fan.percentage="40";fan.step=20;
    view=onx_detail_view::build(fan.entity_id,"Office fan");assert(view.badge.text=="🌀 Fan"&&view.value.text=="40%");
    auto &light=entity_details[4];light.entity_id="light.lamp";light.received=light.available=true;
    light.state="on";light.brightness="128";
    view=onx_detail_view::carousel(light.entity_id,"Lamp");assert(view.value.text=="On"&&view.context.text=="50% brightness");
    light.entity_id="cover.blind";light.state="opening";light.position="25";
    view=onx_detail_view::carousel(light.entity_id,"Blind");assert(view.value.text=="25%"&&view.context.text=="Opening");
    light.entity_id="media_player.bedroom";light.state="paused";light.media_title="Hometown Glory";
    view=onx_detail_view::carousel(light.entity_id,"Bedroom");assert(view.value.text=="Paused"&&view.context.text=="Hometown Glory");
    cancel_controls();
  }
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
  d.features=16|8|32;d.presets="['Eco', 'Comfort']";d.preset="Eco";
  d.fan_modes="['low', 'high']";d.fan_mode="low";d.swing_modes="['off', 'both']";d.swing_mode="off";
  for(const auto &key : {std::string("preset_mode"),std::string("fan_mode"),std::string("swing_mode")}) {
    cancel_controls();select_control_entity(d.entity_id);control_setting=key;
    press(d.entity_id);clock_ms+=100;tick(&api,d.entity_id,true);
    assert(api.sent.back().service.value=="climate.set_"+key);
    assert(api.sent.back().data[1].key.value==key);
  }
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
  cancel_controls(); d = EntityDetailState{}; d.entity_id = "media_player.test";
  d.received = d.available = true; d.features = 4; d.volume = "0.60000004";
  assert(text(d.entity_id).find("60%") != std::string::npos);
  assert(onx_detail_view::build(d.entity_id,"Bedroom").value.text == "Volume: 60%");
  d.state="playing";d.features=4|1|16384;press(d.entity_id);clock_ms+=100;tick(&api,d.entity_id,true);
  assert(api.sent.back().service=="media_player.media_pause"&&api.sent.back().data.size()==1);
  assert(effective(d,"state")=="paused");
  for(auto &c:control_commands)c.feedback("state","paused");d.state="paused";
  press(d.entity_id);clock_ms+=100;tick(&api,d.entity_id,true);
  assert(api.sent.back().service=="media_player.media_play");
  cancel_controls();d.state="idle";press(d.entity_id);assert(control_commands.empty());
  auto idle_view=onx_detail_view::build(d.entity_id,"Bedroom");assert(idle_view.context.text=="Nothing playing"&&idle_view.hint.text.empty());
  d.state="playing";d.features=4;press(d.entity_id);assert(control_commands.empty());
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
'''.replace('CACHE', cache).replace('HANDLERS', handlers).replace('PRESENTATION', presentation)
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory)
            (path / 'test.cpp').write_text(harness)
            subprocess.run(['c++', '-std=c++17', '-I' + str(root), str(path / 'test.cpp'),
                            '-o', str(path / 'test')], check=True)
            subprocess.run([str(path / 'test')], check=True)


if __name__ == '__main__':
    unittest.main()
