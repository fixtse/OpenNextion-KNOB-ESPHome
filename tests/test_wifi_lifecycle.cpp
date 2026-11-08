#include "../my_components/onx_mode_manager/onx_mode_manager.h"
#include "esphome/components/wifi/wifi_component.h"
#include <cassert>
#include <iostream>

using esphome::onx_app_state::OnxAppState;
using esphome::onx_mode_manager::OnxModeManager;
using namespace esphome::wifi;

struct Device {
  OnxAppState app;
  WiFiComponent wifi;
  OnxModeManager modes;
  Device(PreferenceStore &store, const std::string &ssid = "", const std::string &password = "") : wifi(store) {
    global_wifi_component = &wifi;
    app.setup();
    modes.set_app_state(&app);
    modes.set_normal_wifi_ssid(ssid);
    modes.set_normal_wifi_password(password);
    assert(modes.get_setup_priority() > esphome::setup_priority::WIFI);
    modes.setup();
    wifi.setup();
    modes.mark_boot_finished();
    modes.loop();
  }
  void check_no_restarts() const { assert(wifi.disables == 0 && wifi.enables == 0); }
};

int main() {
  PreferenceStore flash;
  {
    Device fresh(flash);
    assert(fresh.modes.is_provisioning_mode_enabled());
    assert(fresh.wifi.get_ap().get_ssid() == "ONX2424G013-eeff");
    fresh.wifi.save_wifi_sta("first-network", "first-password");
    fresh.modes.loop();
    assert(!fresh.modes.is_provisioning_mode_enabled());
    fresh.check_no_restarts();
    assert(flash.at(WiFiComponent::STABLE_KEY).get_ssid() == "first-network");
  }
  // Simulate complete power loss: new mode/Wi-Fi objects, same flash only.
  {
    Device rebooted(flash);
    assert(!rebooted.modes.is_provisioning_mode_enabled());
    assert(rebooted.wifi.has_sta());
    assert(rebooted.wifi.get_sta().get_ssid().empty()); // no selected AP yet
    assert(rebooted.wifi.saves == 0); // boot must not rewrite saved credentials
    rebooted.check_no_restarts();
    // A subsequent portal/Improv change must still persist under the same key.
    rebooted.wifi.save_wifi_sta("replacement-network", "replacement-password");
    rebooted.modes.loop();
    rebooted.check_no_restarts();
  }
  {
    Device after_second_power_loss(flash);
    after_second_power_loss.wifi.selected = true;
    assert(after_second_power_loss.wifi.get_sta().get_ssid() == "replacement-network");
    assert(after_second_power_loss.wifi.get_sta().get_password() == "replacement-password");
    // Router unavailable is not a reason to erase credentials or force setup.
    after_second_power_loss.wifi.connected = false;
    for (int i = 0; i < 100; ++i) after_second_power_loss.modes.loop();
    assert(!after_second_power_loss.modes.is_provisioning_mode_enabled());
    after_second_power_loss.check_no_restarts();
    after_second_power_loss.wifi.save_wifi_sta("open-network", "");
  }
  {
    Device open_network_reboot(flash);
    open_network_reboot.wifi.selected = true;
    assert(open_network_reboot.wifi.get_sta().get_ssid() == "open-network");
    assert(open_network_reboot.wifi.get_sta().get_password().empty());
    assert(!open_network_reboot.modes.is_provisioning_mode_enabled());
    open_network_reboot.check_no_restarts();
    open_network_reboot.modes.clear_saved_normal_wifi_prefs_();
  }
  {
    Device deliberately_reset(flash);
    assert(deliberately_reset.modes.is_provisioning_mode_enabled());
    deliberately_reset.check_no_restarts();
  }
  {
    PreferenceStore static_flash;
    Device configured(static_flash, "compiled-network", "compiled-password");
    configured.wifi.selected = true;
    assert(configured.wifi.get_sta().get_ssid() == "compiled-network");
    assert(!configured.modes.is_provisioning_mode_enabled());
    assert(configured.wifi.saves == 0);
    configured.check_no_restarts();
  }
  std::cout << "Wi-Fi lifecycle passed: first provision, cold boots, credential replacement, selected-AP delay, offline router, open networks, reset, compiled credentials\n";
}
