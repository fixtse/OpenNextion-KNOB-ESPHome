#pragma once
#include <cstdint>
#include <cstdio>
#include <string>
#include "esphome/core/hal.h"
#ifdef USE_ESP32
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#endif

namespace onx_runtime {
struct Event {
  uint32_t magic;
  uint32_t at_ms;
  uint32_t free_heap;
  char name[32];
  char entity[129];
};
#ifdef USE_ESP32
// RTC RAM survives many software/watchdog resets without writing flash. This is
// a last-event breadcrumb, not proof that the recorded operation caused a crash.
RTC_NOINIT_ATTR inline Event retained;
#else
inline Event retained{};
#endif
inline Event previous{};
inline uint32_t wifi_disconnects{0}, last_wifi_disconnect_ms{0};
inline void start() {
#ifdef USE_ESP32
  const auto reason=esp_reset_reason();
  if(reason!=ESP_RST_POWERON && reason!=ESP_RST_BROWNOUT && retained.magic==0x4F4E5852) {
    previous=retained;previous.name[31]=0;previous.entity[128]=0;
  }
#endif
  retained={};
}
inline void record(const char *name,const std::string &entity="") {
  retained.magic=0;
  retained.at_ms=esphome::millis();
#ifdef USE_ESP32
  retained.free_heap=heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#endif
  snprintf(retained.name,sizeof(retained.name),"%s",name);
  snprintf(retained.entity,sizeof(retained.entity),"%s",entity.c_str());
  retained.magic=0x4F4E5852;
}
inline const char *reset_name() {
#ifdef USE_ESP32
  switch(esp_reset_reason()) {
    case ESP_RST_POWERON:return "Power on";
    case ESP_RST_SW:return "Software restart";
    case ESP_RST_PANIC:return "Crash/panic";
    case ESP_RST_INT_WDT:return "Interrupt watchdog";
    case ESP_RST_TASK_WDT:return "Task watchdog";
    case ESP_RST_WDT:return "Watchdog";
    case ESP_RST_BROWNOUT:return "Brownout";
    default:return "Other reset";
  }
#else
  return "Unknown";
#endif
}
}
