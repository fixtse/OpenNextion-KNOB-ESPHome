#include "../onx_icon_state.h"
#include "../onx_recovery_state.h"
#include <cassert>
int main() {
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
  Metadata meta; meta.bind("fan.office"); meta.configured = "mdi:ceiling-fan"; meta.registry_override = true;
  meta.runtime = "mdi:fan"; assert(meta.chosen() == "mdi:ceiling-fan");
  meta.registry_override = false; assert(meta.chosen() == "mdi:fan");
  meta.runtime = "None"; assert(meta.chosen() == "mdi:ceiling-fan");
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
