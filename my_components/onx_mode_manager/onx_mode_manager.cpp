#include "onx_mode_manager.h"

#include <cctype>

#include "esphome/components/logger/logger.h"
#include "esphome/components/wifi/wifi_component.h"
#ifdef USE_IMPROV_BLE
#include "esphome/components/improv_ble/improv_ble_component.h"
#endif
#include "esphome/core/helpers.h"
#include "esphome/core/log.h"
#ifdef USE_ESP32
#include "nvs.h"
#endif

namespace esphome::onx_mode_manager {

static const char *const TAG = "onx_mode_manager";
// ESPHome restored global `normal_ha_added`: 1944399030 ^ 3371913747.
static const char *const NORMAL_HA_ADDED_PREF_KEY = "3139337893";

static std::string build_provisioning_ap_ssid_() {
  std::string ssid = "ONX2424G013-";
  std::string mac = get_mac_address();
  for (char &ch : mac) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  ssid += mac.size() > 4 ? mac.substr(mac.size() - 4) : mac;
  return ssid;
}

void OnxModeManager::setup() {
  this->mode_decided_ = false;
  this->missing_binding_logged_ = false;
  this->boot_finished_ = false;
  this->provisioning_mode_enabled_ = false;
  // Configure the fallback name before WiFiComponent::setup(). Do not start,
  // stop, or restart the adapter: Wi-Fi owns credential restore and persistence.
  if (wifi::global_wifi_component != nullptr) {
    auto ap = wifi::global_wifi_component->get_ap();
    ap.set_ssid(build_provisioning_ap_ssid_());
    wifi::global_wifi_component->set_ap(ap);
    if (!this->normal_wifi_ssid_.empty()) {
      wifi::WiFiAP sta{};
      sta.set_ssid(this->normal_wifi_ssid_);
      sta.set_password(this->normal_wifi_password_);
      wifi::global_wifi_component->set_sta(sta);
    }
  }
  if (this->app_state_ != nullptr) {
    this->app_state_->set_current_mode(onx_app_state::OnxRuntimeMode::MODE_UNKNOWN);
    this->app_state_->set_mode_reason("booting");
    this->app_state_->mark_ui_dirty();
  }
}

void OnxModeManager::loop() {
  if (!this->mode_decided_) {
    this->evaluate_boot_mode_();
  } else {
    this->evaluate_runtime_mode_();
  }
}

void OnxModeManager::dump_config() {
  ESP_LOGCONFIG(TAG, "ONX Mode Manager:");
  ESP_LOGCONFIG(TAG, "  App state bound: %s", this->app_state_ != nullptr ? "YES" : "NO");
  ESP_LOGCONFIG(TAG, "  Mode decided: %s", this->mode_decided_ ? "YES" : "NO");
}

float OnxModeManager::get_setup_priority() const {
  // Prepare optional configured credentials and the fallback name before native
  // Wi-Fi starts. Mode arbitration still waits for mark_boot_finished().
  return setup_priority::WIFI + 1.0f;
}

void OnxModeManager::set_app_state(onx_app_state::OnxAppState *app_state) { this->app_state_ = app_state; }
void OnxModeManager::set_normal_wifi_ssid(const std::string &normal_wifi_ssid) {
  this->normal_wifi_ssid_ = normal_wifi_ssid;
}
void OnxModeManager::set_normal_wifi_password(const std::string &normal_wifi_password) {
  this->normal_wifi_password_ = normal_wifi_password;
}
void OnxModeManager::set_runtime_log_level(uint8_t level) { this->runtime_log_level_ = level; }
void OnxModeManager::mark_boot_finished() { this->boot_finished_ = true; }

void OnxModeManager::evaluate_boot_mode_() {
  if (this->app_state_ == nullptr) {
    if (!this->missing_binding_logged_) {
      ESP_LOGW(TAG, "Mode manager bindings are incomplete; waiting for dependencies");
      this->missing_binding_logged_ = true;
    }
    return;
  }

  if (!this->boot_finished_) {
    this->app_state_->set_current_mode(onx_app_state::OnxRuntimeMode::MODE_UNKNOWN);
    this->app_state_->set_mode_reason("booting");
    this->app_state_->mark_ui_dirty();
    return;
  }

  this->select_mode_(onx_app_state::OnxRuntimeMode::MODE_NORMAL, "normal-mode-only");
}

void OnxModeManager::evaluate_runtime_mode_() {
  if (this->app_state_ != nullptr &&
      this->app_state_->get_current_mode() == onx_app_state::OnxRuntimeMode::MODE_NORMAL) {
    this->update_provisioning_state_();
  }
}

void OnxModeManager::update_provisioning_state_() {
  // has_sta() reports configured/restored networks even before a particular AP
  // is selected or reachable. A transient disconnect is not lost credentials.
  auto *wifi_component = wifi::global_wifi_component;
  bool ble_setup_active = false;
#ifdef USE_IMPROV_BLE
  auto *ble = improv_ble::global_improv_component;
  ble_setup_active = ble != nullptr && ble->is_active() &&
                     ble->get_improv_state() != improv::STATE_PROVISIONED;
#endif
  // Improv installs candidate credentials before they have connected. Keep the
  // setup screen throughout authorization/connection, but a live connection
  // always wins over stale provisioning state.
  const bool needs_setup = wifi_component == nullptr ||
      (!wifi_component->is_connected() && (!wifi_component->has_sta() || ble_setup_active));
  if (needs_setup != this->provisioning_mode_enabled_) {
    this->provisioning_mode_enabled_ = needs_setup;
    ESP_LOGI(TAG, "%s", needs_setup ? "Waiting for native Wi-Fi provisioning"
                                   : "Wi-Fi credentials present; native Wi-Fi manages reconnection");
  }
  // WiFiComponent manages the captive portal/AP and persists both portal and
  // Improv credentials. Restarting it here would rebind pref_ from the stable
  // unconfigured key to a configuration hash, losing later saves on power-up.
}

bool OnxModeManager::clear_saved_normal_wifi_prefs_() {
  // Explicit reset only: clear the currently bound preference through Wi-Fi
  // itself, so the preferences cache cannot restore credentials after an erase.
  if (wifi::global_wifi_component != nullptr) {
    wifi::global_wifi_component->save_wifi_sta("", "");
    wifi::global_wifi_component->clear_sta();
  }
#ifdef USE_ESP32
  nvs_handle_t handle;
  esp_err_t err = nvs_open("esphome", NVS_READWRITE, &handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "normal_wifi_recover: failed to open NVS namespace for erase: %s", esp_err_to_name(err));
    return false;
  }

  bool ok = true;
  const char *const keys[] = {"88491487", "88491488", NORMAL_HA_ADDED_PREF_KEY};
  for (const char *key : keys) {
    err = nvs_erase_key(handle, key);
    if (err == ESP_OK) {
      ESP_LOGI(TAG, "normal_wifi_recover: erased NVS key '%s'", key);
    } else if (err == ESP_ERR_NVS_NOT_FOUND) {
      ESP_LOGI(TAG, "normal_wifi_recover: NVS key '%s' already absent", key);
    } else {
      ESP_LOGW(TAG, "normal_wifi_recover: failed to erase NVS key '%s': %s", key, esp_err_to_name(err));
      ok = false;
    }
  }

  err = nvs_commit(handle);
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "normal_wifi_recover: failed to commit NVS erase: %s", esp_err_to_name(err));
    ok = false;
  }
  nvs_close(handle);
  return ok;
#else
  return false;
#endif
}

void OnxModeManager::select_mode_(onx_app_state::OnxRuntimeMode mode, const std::string &reason) {
  if (this->mode_decided_ || this->app_state_ == nullptr) {
    return;
  }

  this->app_state_->set_current_mode(mode);
  this->app_state_->set_mode_reason(reason);
  this->app_state_->mark_ui_dirty();
  this->mode_decided_ = true;
  this->update_provisioning_state_();

  if (logger::global_logger != nullptr) {
    logger::global_logger->set_log_level(this->runtime_log_level_);
  }

  ESP_LOGI(TAG, "Boot mode selected: %s (%s)", this->app_state_->get_current_mode_name(), reason.c_str());
}

}  // namespace esphome::onx_mode_manager
