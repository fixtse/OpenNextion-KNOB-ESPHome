#pragma once
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include <deque>
#include <functional>
#include <string>

namespace esphome::onx_config_web {
struct Query {
  std::string playlists;
  std::string op, entity, action, search, domain, name, area, action_name, type, icon, icon_override;
  std::string brightness, standby, sensitivity, sleep_enabled, sleep_start, sleep_end, sleep_timeout;
  std::string media_color, standby_scope, date_format, hour_format, show_ampm, secondary, secondary_text, sensor_graph;
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
 protected:
  void respond_(AsyncWebServerRequest *request, int code, const std::string &body);
  std::function<std::string(const Query &)> handler_;
  std::string nonce_;
  Mutex mutex_;
  std::deque<Query> pending_;
  std::deque<std::pair<uint32_t, std::string>> results_;
  uint32_t next_ticket_{0};
};
}  // namespace esphome::onx_config_web
