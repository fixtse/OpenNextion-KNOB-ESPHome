#pragma once
// Bluetooth is used only for Improv provisioning. Starting Bluedroid needs a
// large contiguous block of internal RAM, and a failed start asserts and
// reboots the panel. Start it only when Wi-Fi needs provisioning or has been
// down for 30 s, and only with enough internal headroom. An RTC flag limits a
// failed start to one reboot: the next boot skips Bluetooth.
#include <cstddef>
#include <cstdint>
#include "esphome/core/defines.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#ifdef USE_ESP32
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#endif
#ifdef USE_ESP32_BLE
#include "esphome/components/esp32_ble/ble.h"
#include "esphome/components/wifi/wifi_component.h"
#endif

namespace onx_ble_gate {
inline constexpr uint32_t WIFI_DOWN_MS = 30000;
inline constexpr size_t MIN_FREE_INTERNAL = 56 * 1024;
inline constexpr size_t MIN_LARGEST_BLOCK = 36 * 1024;
inline constexpr uint32_t START_SETTLE_MS = 15000;
inline constexpr uint32_t STARTING = 0x424C4553;  // "BLES"

// Pure decision, kept separate from the hardware for tests.
inline bool should_start(bool provisioned, bool wifi_connected, uint32_t wifi_down_ms, size_t free_internal,
                         size_t largest_block) {
  if (wifi_connected) return false;
  if (provisioned && wifi_down_ms < WIFI_DOWN_MS) return false;
  return free_internal >= MIN_FREE_INTERNAL && largest_block >= MIN_LARGEST_BLOCK;
}

#ifdef USE_ESP32
RTC_NOINIT_ATTR inline uint32_t start_flag;
#else
inline uint32_t start_flag{0};
#endif
inline bool checked{false}, blocked{false}, requested{false};
inline uint32_t wifi_down_since{0}, requested_at{0}, last_skip_log{0};

#ifdef USE_ESP32_BLE
inline void tick(esphome::esp32_ble::ESP32BLE *ble) {
  static const char *const TAG = "onx_ble_gate";
  if (ble == nullptr) return;
  const uint32_t now = esphome::millis();
  if (!checked) {
    checked = true;
    const auto reason = esp_reset_reason();
    if (start_flag == STARTING && reason != ESP_RST_POWERON && reason != ESP_RST_BROWNOUT) {
      blocked = true;
      ESP_LOGW(TAG, "Bluetooth did not finish starting before the last reset; Bluetooth stays off until reboot");
    }
    start_flag = 0;
  }
  if (requested && start_flag == STARTING && (ble->is_active() || now - requested_at > START_SETTLE_MS))
    start_flag = 0;

  auto *wifi = esphome::wifi::global_wifi_component;
  const bool connected = wifi != nullptr && wifi->is_connected();
  if (connected) {
    // Wi-Fi's on_connect handler disables Bluetooth; allow a later restart.
    wifi_down_since = 0;
    requested = false;
    return;
  }
  if (wifi_down_since == 0) wifi_down_since = now != 0 ? now : 1;
  if (blocked || requested) return;

  const bool provisioned = wifi != nullptr && wifi->has_sta();
  const size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  const size_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!should_start(provisioned, false, now - wifi_down_since, free_internal, largest)) {
    if ((!provisioned || now - wifi_down_since >= WIFI_DOWN_MS) && now - last_skip_log > 60000) {
      last_skip_log = now;
      ESP_LOGW(TAG, "Bluetooth start deferred: free internal %u, largest block %u", static_cast<unsigned>(free_internal),
               static_cast<unsigned>(largest));
    }
    return;
  }
  ESP_LOGI(TAG, "Starting Bluetooth for Improv: free internal %u, largest block %u",
           static_cast<unsigned>(free_internal), static_cast<unsigned>(largest));
  start_flag = STARTING;
  requested = true;
  requested_at = now;
  ble->enable();
}
#endif
}  // namespace onx_ble_gate
