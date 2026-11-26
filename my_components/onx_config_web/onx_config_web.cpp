#include <algorithm>
#include <memory>
#include "onx_config_web.h"
#include "config_page.h"
#include "branding_assets.h"
#include "config_app.h"
#include "ha_browser.h"
#include "esphome/components/wifi/wifi_component.h"
#include <array>
#include <charconv>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#ifdef USE_ESP32
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#if __has_include(<mdns.h>)
#include <mdns.h>
#define ONX_HA_DISCOVERY 1
#endif
#endif

namespace esphome::onx_config_web {
static bool number(const std::string &text, unsigned &value) {
  auto result = std::from_chars(text.data(), text.data() + text.size(), value);
  return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
void ConfigWeb::setup() {
  char nonce[33];
  std::snprintf(nonce, sizeof(nonce), "%08" PRIx32 "%08" PRIx32 "%08" PRIx32 "%08" PRIx32,
                random_uint32(), random_uint32(), random_uint32(), random_uint32());
  nonce_ = nonce;
  auto *base = web_server_base::global_web_server_base;
  base->add_handler(this);
  base->init();
}
bool ConfigWeb::canHandle(AsyncWebServerRequest *request) const {
  std::array<char, AsyncWebServerRequest::URL_BUF_SIZE> buffer;
  const auto url = request->url_to(buffer);
  // Keep captive-portal routing during provisioning, and make HA Visit's root
  // address open our app after Wi-Fi connects. Do not expose the generic server
  // entity-control endpoints alongside the configuration app.
  if (wifi::global_wifi_component != nullptr && wifi::global_wifi_component->is_connected()) return true;
  return url == "/knob/favicon.svg" || url == "/knob/fixtse-logo.png" || url == "/knob" || url == "/knob/" || url == "/knob/api" || url == "/knob/result" || url == "/knob/ha.js" || url == "/knob/app.js" || url == "/knob/discover";
}
void ConfigWeb::respond_(AsyncWebServerRequest *request, int code, const char *data, size_t size) {
  auto *response = request->beginResponse(code, "application/json", reinterpret_cast<const uint8_t *>(data), size);
  response->addHeader("Cache-Control", "no-store");
  // ESPHome 2026.10's IDF wrapper defaults unsupported status codes to 500.
  // Preserve our asynchronous protocol and meaningful client error responses.
  switch (code) {
    case 202: httpd_resp_set_status(*request, "202 Accepted"); break;
    case 403: httpd_resp_set_status(*request, "403 Forbidden"); break;
    case 405: httpd_resp_set_status(*request, "405 Method Not Allowed"); break;
    case 413: httpd_resp_set_status(*request, "413 Content Too Large"); break;
    case 429: httpd_resp_set_status(*request, "429 Too Many Requests"); break;
    default: break;
  }
  request->send(response);
}
void ConfigWeb::handleRequest(AsyncWebServerRequest *request) {
  std::array<char, AsyncWebServerRequest::URL_BUF_SIZE> buffer;
  const auto url = request->url_to(buffer);
  if (request->method() == HTTP_GET && (url == "/knob/fixtse-logo.png" || url == "/knob/favicon.svg")) {
    const bool logo = url == "/knob/fixtse-logo.png";
    auto *response = request->beginResponse(200, logo ? "image/png" : "image/svg+xml",
        logo ? FIXTSE_LOGO : reinterpret_cast<const uint8_t *>(KNOB_FAVICON),
        logo ? sizeof(FIXTSE_LOGO) : sizeof(KNOB_FAVICON) - 1);
    response->addHeader("Cache-Control", "public, max-age=86400");
    request->send(response);
    return;
  }
  if ((url == "/knob/ha.js" || url == "/knob/app.js") && request->method() == HTTP_GET) {
    const char *asset = url == "/knob/ha.js" ? HA_BROWSER_JS : CONFIG_APP_JS;
    auto *response = request->beginResponse(200, "text/javascript; charset=utf-8",
        reinterpret_cast<const uint8_t *>(asset), std::strlen(asset));
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
    return;
  }
  if ((url == "/" || url == "/knob" || url == "/knob/") && request->method() == HTTP_GET) {
    const char *token = std::strstr(CONFIG_PAGE, "__ONX_TOKEN__");
    if (token == nullptr) { respond_(request, 500, "{\"error\":\"Page template unavailable\"}"); return; }
    auto *response = request->beginResponse(200, "text/html; charset=utf-8");
    response->addHeader("Cache-Control", "no-store");
    response->addHeader("X-Content-Type-Options", "nosniff");
    response->addHeader("Referrer-Policy", "no-referrer");
    // IDF sends these synchronously. Keep the large template in flash and inject
    // only the small nonce, instead of making two full internal-heap copies.
    if (httpd_resp_send_chunk(*request, CONFIG_PAGE, token - CONFIG_PAGE) != ESP_OK ||
        httpd_resp_send_chunk(*request, nonce_.data(), nonce_.size()) != ESP_OK ||
        httpd_resp_send_chunk(*request, token + 13, std::strlen(token + 13)) != ESP_OK) return;
    httpd_resp_send_chunk(*request, nullptr, 0);
    return;
  }
  if (url == "/knob/discover" && request->method() == HTTP_GET) {
    const auto token = request->get_header("X-Onx-Token");
    if (!token.has_value() || *token != nonce_) { respond_(request, 403, "{\"error\":\"Reload this page to authorize configuration\"}"); return; }
    if (request->arg("scan") == "1") start_discovery_();
    std::string body;
    {
      LockGuard lock(mutex_);
      body = std::string("{\"scanning\":") + (discovering_ ? "true" : "false") + ",\"runs\":" + std::to_string(discovery_runs_) +
#ifdef ONX_HA_DISCOVERY
             ",\"supported\":true" +
#else
             ",\"supported\":false" +
#endif
             ",\"items\":" + discovered_ + "}";
    }
    respond_(request, 200, body);
    return;
  }
  if (url == "/knob/result" && request->method() == HTTP_GET) {
    unsigned ticket;
    if (!number(request->arg("ticket"), ticket)) { respond_(request, 400, "{\"error\":\"Invalid ticket\"}"); return; }
    LockGuard lock(mutex_);
    for (const auto &result : results_)
      if (result.first == ticket) { respond_(request, 200, result.second.data(), result.second.size()); return; }
    respond_(request, 202, "{\"pending\":true}");
    return;
  }
  if (url != "/knob/api" || request->method() != HTTP_POST) { respond_(request, 405, "{\"error\":\"Method not allowed\"}"); return; }
  const auto token = request->get_header("X-Onx-Token");
  const auto origin = request->get_header("Origin");
  const auto host = request->get_header("Host");
  if (!token.has_value() || *token != nonce_ || !origin.has_value() || !host.has_value() ||
      (*origin != "http://" + *host && *origin != "https://" + *host)) {
    respond_(request, 403, "{\"error\":\"Reload this page to authorize configuration\"}"); return;
  }
  if (request->contentLength() > 8192) { respond_(request, 413, "{\"error\":\"Request too large\"}"); return; }
  // Query holds ~50 strings (~1.7 KB). Build it on the heap (PSRAM): on the
  // HTTP task's small stack it overflowed while a reopened page polled during boot.
  auto owned = std::make_unique<Query>();
  Query &query = *owned;
  query.clock_seconds_ring = request->arg("clock_seconds_ring");
  query.hold_cue = request->arg("hold_cue");
  query.media_clock = request->arg("media_clock");
  query.clock_temperature = request->arg("clock_temperature");
  query.clock_humidity = request->arg("clock_humidity");
  query.notifications_cover_timers = request->arg("notifications_cover_timers");
  query.gestures = request->arg("gestures");
  query.orientation = request->arg("orientation");
  query.media_view = request->arg("media_view");
  query.idle_view = request->arg("idle_view");
  query.timer_hidden = request->arg("timer_hidden");
  query.playlists = request->arg("playlists");
  query.data = request->arg("data"); query.total = request->arg("total");
  query.op = request->arg("op"); query.entity = request->arg("entity"); query.action = request->arg("action");
  query.name = request->arg("name"); query.area = request->arg("area");
  query.action_name = request->arg("action_name"); query.type = request->arg("type");
  query.icon = request->arg("icon"); query.icon_override = request->arg("icon_override");
  query.search = request->arg("q"); query.domain = request->arg("domain");
  query.brightness = request->arg("brightness"); query.standby = request->arg("standby");
  query.sensitivity = request->arg("sensitivity"); query.sleep_enabled = request->arg("sleep_enabled");
  query.sleep_start = request->arg("sleep_start"); query.sleep_end = request->arg("sleep_end");
  query.sleep_timeout = request->arg("sleep_timeout");
  query.secondary=request->arg("secondary");query.secondary_text=request->arg("secondary_text");query.sensor_graph=request->arg("sensor_graph");
  query.show_ampm=request->arg("show_ampm");
  query.date_format = request->arg("date_format"); query.hour_format = request->arg("hour_format");
  query.media_color = request->arg("media_color"); query.standby_scope = request->arg("standby_scope");
  unsigned slot = 0, offset = 0;
  if (query.data.size()>6144 || query.total.size()>6 || query.clock_seconds_ring.size()>1 || query.hold_cue.size()>1 || query.media_clock.size()>1 || query.clock_temperature.size()>128 || query.clock_humidity.size()>128 || query.notifications_cover_timers.size()>1 || query.orientation.size()>3 || query.media_view.size()>8 || query.gestures.size()>1600 || query.idle_view.size()>5 || query.timer_hidden.size()>1 || query.playlists.size()>2048 || query.secondary.size()>128 || query.secondary_text.size()>160 || query.sensor_graph.size()>1 || query.show_ampm.size()>1 || query.date_format.size() > 11 || query.hour_format.size() > 2 || query.media_color.size() > 7 || query.standby_scope.size() > 5 || query.icon.size() > 80 || query.icon_override.size() > 1 || query.op.size() > 16 || query.entity.size() > 128 || query.action.size() > 128 ||
      query.search.size() > 128 || query.domain.size() > 32 ||
      query.brightness.size() > 3 || query.standby.size() > 3 || query.sensitivity.size() > 1 ||
      query.sleep_enabled.size() > 1 || query.sleep_start.size() > 5 || query.sleep_end.size() > 5 || query.sleep_timeout.size() > 4 ||
      query.name.size() > 192 || query.area.size() > 192 || query.action_name.size() > 192 || query.type.size() > 32 ||
      (request->hasArg("slot") && (!number(request->arg("slot"), slot) || slot >= 20)) ||
      (request->hasArg("offset") && (!number(request->arg("offset"), offset) || offset > 100000))) {
    respond_(request, 400, "{\"error\":\"Invalid configuration request\"}"); return;
  }
  query.slot = request->hasArg("slot") ? static_cast<int>(slot) : -1; query.offset = offset;
  LockGuard lock(mutex_);
  if (pending_.size() >= 4) { respond_(request, 429, "{\"error\":\"Configuration busy; retry shortly\"}"); return; }
  query.ticket = ++next_ticket_;
  pending_.push_back(std::move(query));
  respond_(request, 202, "{\"ticket\":" + std::to_string(next_ticket_) + "}");
}
void ConfigWeb::loop() {
  Query query;
  // One device-initiated request per loop, interleaved with web requests so the
  // page can poll progress while a preset applies.
  if (!internal_.empty() && handler_) {
    Query internal = std::move(internal_.front()); internal_.pop_front();
    Body body = take_result_(handler_(internal));
    // Internal requests (preset slot saves) return short status objects.
    if (internal_result_) internal_result_(internal, std::string(body.data(), std::min<size_t>(body.size(), 512)));
  }
  {
    LockGuard lock(mutex_);
    if (pending_.empty() || !handler_) return;
    query = std::move(pending_.front()); pending_.pop_front();
  }
  // NVS, entity subscriptions, and LVGL always run in the ESPHome main task.
  Body body = take_result_(handler_(query));
  LockGuard lock(mutex_);
  results_.emplace_back(query.ticket, std::move(body));
  while (results_.size() > 4) results_.pop_front();
}
#ifdef ONX_HA_DISCOVERY
static std::string json_escape(const char *text) {
  std::string out;
  for (const char *c = text == nullptr ? "" : text; *c != '\0' && out.size() < 160; ++c) {
    if (*c == '"' || *c == '\\') { out += '\\'; out += *c; }
    else if (static_cast<unsigned char>(*c) < 0x20) out += ' ';
    else out += *c;
  }
  return out;
}
static const char *txt_value(const mdns_result_t *r, const char *key) {
  for (size_t i = 0; i < r->txt_count; i++)
    if (r->txt[i].key != nullptr && std::strcmp(r->txt[i].key, key) == 0) return r->txt[i].value;
  return nullptr;
}
void ConfigWeb::discovery_task_(void *arg) {
  auto *self = static_cast<ConfigWeb *>(arg);
  mdns_result_t *results = nullptr;
  std::string items = "[";
  std::vector<std::string> urls;
  if (mdns_query_ptr("_home-assistant", "_tcp", 3000, 8, &results) == ESP_OK) {
    size_t count = 0;
    for (mdns_result_t *r = results; r != nullptr; r = r->next) {
      char ip[16] = "";
      for (mdns_ip_addr_t *a = r->addr; a != nullptr; a = a->next)
        if (a->addr.type == ESP_IPADDR_TYPE_V4) { snprintf(ip, sizeof(ip), IPSTR, IP2STR(&a->addr.u_addr.ip4)); break; }
      const char *internal = txt_value(r, "internal_url");
      const char *base = txt_value(r, "base_url");
      std::string url = internal != nullptr && *internal ? internal : base != nullptr && *base ? base : "";
      if (url.empty() && *ip) url = std::string("http://") + ip + ":" + std::to_string(r->port);
      if (url.empty()) continue;
      const char *name = txt_value(r, "location_name");
      if (name == nullptr || !*name) name = r->instance_name;
      const char *uuid = txt_value(r, "uuid");
      const char *version = txt_value(r, "version");
      urls.push_back(url);
      if (count++) items += ',';
      items += "{\"name\":\"" + json_escape(name ? name : "Home Assistant") + "\",\"url\":\"" + json_escape(url.c_str()) +
               "\",\"ip\":\"" + ip + "\",\"port\":" + std::to_string(r->port) + ",\"uuid\":\"" + json_escape(uuid) +
               "\",\"version\":\"" + json_escape(version) + "\"}";
    }
    mdns_query_results_free(results);
  }
  items += "]";
  {
    LockGuard lock(self->mutex_);
    self->discovered_ = std::move(items);
    self->discovered_urls_ = std::move(urls);
    self->discovering_ = false;
    self->discovery_runs_++;
  }
  vTaskDelete(nullptr);
}
#endif
void ConfigWeb::start_discovery_() {
#ifdef ONX_HA_DISCOVERY
  {
    LockGuard lock(mutex_);
    if (discovering_) return;
    discovering_ = true;
  }
  if (xTaskCreate(&ConfigWeb::discovery_task_, "onx_ha_mdns", 4096, this, 1, nullptr) != pdPASS) {
    LockGuard lock(mutex_);
    discovering_ = false;
  }
#endif
}
std::string ConfigWeb::auto_ha_url(uint32_t now) {
  {
    // Read the finished result before starting another search, which would clear it.
    LockGuard lock(mutex_);
    if (discovering_) return "";
    if (discovery_runs_ > 0 && discovered_urls_.size() == 1) return discovered_urls_[0];
    if (discovery_started_ && now - last_discovery_start_ < 5000) return "";
    discovery_started_ = true;
    last_discovery_start_ = now;
  }
  start_discovery_();
  return "";
}
Body ConfigWeb::take_result_(std::string &&result) {
  if (result == LARGE_BODY_MARKER) {
    Body body;
    body.swap(large_body());
    return body;
  }
  return Body(result.data(), result.size());
}
}  // namespace esphome::onx_config_web
