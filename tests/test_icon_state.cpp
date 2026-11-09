#include "../onx_icon_state.h"
#include "../onx_recovery_state.h"
#include <cassert>
int main() {
  assert(onx_icon_state::weather_icon("partlycloudy")=="mdi:weather-partly-cloudy");
  assert(onx_icon_state::weather_icon("clear-night")=="mdi:weather-night");
  onx_icon_state::Metadata weather;weather.bind("weather.home");weather.condition="rainy";
  weather.configured="mdi:weather-partly-cloudy";
  assert(weather.chosen()=="mdi:weather-rainy");
  weather.runtime="mdi:weather-fog";assert(weather.chosen()=="mdi:weather-fog");
  weather.registry_override=true;assert(weather.chosen()=="mdi:weather-partly-cloudy");
  using namespace onx_icon_state;
  assert(valid_icon("mdi:ceiling-fan"));
  for (const auto &icon : {"", "mdi:", "mdi:../secret", "https://evil/icon.svg", "mdi:fan?token=x", "mdi:FAN", "hass:fan"}) {
    assert(!valid_icon(icon)); assert(download_url(icon).empty());
  }
  assert(download_url("mdi:fan") == "https://api.iconify.design/mdi/fan.svg?color=%23ffffff&width=80&height=80");
  assert(fallback("fan.office") == "fan"); assert(fallback("climate.bedroom") == "thermostat");
  assert(fallback("media_player.music") == "music-note"); assert(fallback("number.helper") == "counter");
  assert(fallback("input_select.mode") == "format-list-bulleted"); assert(fallback("switch.plug", "plug") == "power-plug");
  assert(fallback("unknown.new") == "toggle-switch");
  assert(backup_url("mdi:ceiling-fan") == "https://raw.githubusercontent.com/Templarian/MaterialDesign/master/svg/ceiling-fan.svg");
  assert(backup_url("mdi:../../etc/passwd").empty());
  Retry icon_retry; icon_retry.fail(100);
  assert(!icon_retry.due(5099) && icon_retry.due(5100));
  icon_retry.fail(5100); assert(icon_retry.delay == 15000);
  icon_retry.fail(20100); assert(icon_retry.delay == 60000);
  icon_retry.fail(80100); assert(icon_retry.delay == 300000);
  Metadata meta; meta.bind("fan.office"); meta.configured = "mdi:ceiling-fan"; meta.registry_override = true;
  meta.runtime = "mdi:fan"; assert(meta.chosen() == "mdi:ceiling-fan");
  meta.registry_override = false; assert(meta.chosen() == "mdi:fan");
  meta.runtime = "None"; assert(meta.chosen() == "mdi:ceiling-fan");
  meta.bind("sensor.madrid_piso_temperature"); meta.device_class = "temperature";
  assert(meta.chosen() == "mdi:thermometer");
  meta.configured = "mdi:weather-fog"; meta.registry_override = true;
  meta.runtime = "mdi:thermometer"; assert(meta.chosen() == "mdi:weather-fog");
  meta.bind("light.office"); assert(meta.chosen().empty());
  onx_recovery::Retry retry;
  assert(!retry.due("sensor.later", false, 100));
  assert(!retry.due("sensor.later", false, 10099)); assert(retry.due("sensor.later", false, 10100));
  retry.attempted(10100); assert(!retry.due("sensor.later", false, 40099)); assert(retry.due("sensor.later", false, 40100));
  retry.attempted(40100); assert(retry.delay == 60000);
  assert(!retry.due("sensor.later", true, 50000)); assert(retry.attempts == 0);
  assert(!retry.due("sensor.other", false, 60000)); // Entity changes restart the grace period.
  onx_recovery::Retry wrapped;
  wrapped.due("sensor.test", false, UINT32_MAX - 5000);
  assert(wrapped.due("sensor.test", false, 5000));
  for (int i = 0; i < 20; ++i) retry.attempted(100000 + i);
  assert(retry.delay == 300000);
}
