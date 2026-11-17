#pragma once
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include <deque>
#include <functional>
#include <string>

namespace esphome::onx_config_web {
// Response bodies live in PSRAM when available: internal RAM is fragmented and a
// failed std::string allocation aborts the firmware.
using Body = std::basic_string<char, std::char_traits<char>, RAMAllocator<char>>;
// A handler that builds a large body (onx_entity_config::build_json) stores it
// here and returns LARGE_BODY_MARKER instead of copying it into a std::string.
inline Body &large_body() {
  static Body body;
  return body;
}
inline const char *const LARGE_BODY_MARKER = "\x01onx-large-body";
struct Query {
  std::string playlists;
  std::string op, entity, action, search, domain, name, area, action_name, type, icon, icon_override;
  std::string brightness, standby, sensitivity, sleep_enabled, sleep_start, sleep_end, sleep_timeout;
  std::string data, total;  // Chunked preset restore.
  std::string clock_seconds_ring, hold_cue, clock_temperature, clock_humidity, notifications_cover_timers, gestures, orientation, media_view, idle_view, timer_hidden, media_color, standby_scope, date_format, hour_format, show_ampm, secondary, secondary_text, sensor_graph;
  int slot{-1};
  unsigned offset{0};
  uint32_t ticket{0};
};
class ConfigWeb : public Component, public AsyncWebHandler {
 public:
  void setup() override;
  void loop() override;
  float get_setup_priority() const override { return setup_priority::WIFI - 0.5f; }
  void set_handler(std::function<std::string(const Query &)> handler) { handler_ = std::move(handler); }
  bool canHandle(AsyncWebServerRequest *request) const override;
  void handleRequest(AsyncWebServerRequest *request) override;
  // Device-initiated requests (presets) run through the same handler as the web
  // page, one per loop, ahead of queued web requests. Call from the main loop.
  using InternalResult = std::function<void(const Query &, const std::string &)>;
  void enqueue_internal(Query query) { query.ticket = 0; internal_.push_back(std::move(query)); }
  void clear_internal() { internal_.clear(); }
  size_t internal_pending() const { return internal_.size(); }
  void set_internal_result(InternalResult callback) { internal_result_ = std::move(callback); }
 protected:
  void respond_(AsyncWebServerRequest *request, int code, const std::string &body) {
    respond_(request, code, body.data(), body.size());
  }
  void respond_(AsyncWebServerRequest *request, int code, const char *data, size_t size);
  Body take_result_(std::string &&result);
  std::function<std::string(const Query &)> handler_;
  std::string nonce_;
  Mutex mutex_;
  std::deque<Query> pending_;
  std::deque<Query> internal_;
  InternalResult internal_result_;
  std::deque<std::pair<uint32_t, Body>> results_;
  uint32_t next_ticket_{0};
  // Home Assistant mDNS discovery (_home-assistant._tcp), run in a short-lived
  // worker task because mdns_query_ptr blocks for the scan window.
  void start_discovery_();
  static void discovery_task_(void *arg);
  bool discovering_{false};
  uint32_t discovery_runs_{0};
  std::string discovered_{"[]"};
};
}  // namespace esphome::onx_config_web
