#include "onx_config_web.h"
#include "config_page.h"
#include "config_app.h"
#include "ha_browser.h"
#include "esphome/components/wifi/wifi_component.h"
#include <array>
#include <charconv>

namespace esphome::onx_config_web {
static bool number(const std::string &text, unsigned &value) {
  auto result = std::from_chars(text.data(), text.data() + text.size(), value);
  return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
void ConfigWeb::setup() {
  nonce_ = str_sprintf("%08x%08x%08x%08x", random_uint32(), random_uint32(), random_uint32(), random_uint32());
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
  return url == "/knob" || url == "/knob/" || url == "/knob/api" || url == "/knob/result" || url == "/knob/ha.js" || url == "/knob/app.js";
}
void ConfigWeb::respond_(AsyncWebServerRequest *request, int code, const std::string &body) {
  auto *response = request->beginResponse(code, "application/json", body);
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
  if ((url == "/knob/ha.js" || url == "/knob/app.js") && request->method() == HTTP_GET) {
    auto *response = request->beginResponse(200, "text/javascript; charset=utf-8",
        url == "/knob/ha.js" ? HA_BROWSER_JS : CONFIG_APP_JS);
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
    return;
  }
  if ((url == "/" || url == "/knob" || url == "/knob/") && request->method() == HTTP_GET) {
    std::string page(CONFIG_PAGE);
    const auto token = page.find("__ONX_TOKEN__");
    page.replace(token, 13, nonce_);
    auto *response = request->beginResponse(200, "text/html; charset=utf-8", page);
    response->addHeader("Cache-Control", "no-store");
    response->addHeader("X-Content-Type-Options", "nosniff");
    response->addHeader("Referrer-Policy", "no-referrer");
    request->send(response);
    return;
  }
  if (url == "/knob/result" && request->method() == HTTP_GET) {
    unsigned ticket;
    if (!number(request->arg("ticket"), ticket)) { respond_(request, 400, "{\"error\":\"Invalid ticket\"}"); return; }
    LockGuard lock(mutex_);
    for (const auto &result : results_)
      if (result.first == ticket) { respond_(request, 200, result.second); return; }
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
  if (request->contentLength() > 4096) { respond_(request, 413, "{\"error\":\"Request too large\"}"); return; }
  Query query;
  query.op = request->arg("op"); query.entity = request->arg("entity"); query.action = request->arg("action");
  query.name = request->arg("name"); query.area = request->arg("area");
  query.action_name = request->arg("action_name"); query.type = request->arg("type");
  query.icon = request->arg("icon"); query.icon_override = request->arg("icon_override");
  query.search = request->arg("q"); query.domain = request->arg("domain");
  query.brightness = request->arg("brightness"); query.standby = request->arg("standby");
  query.sensitivity = request->arg("sensitivity"); query.sleep_enabled = request->arg("sleep_enabled");
  query.sleep_start = request->arg("sleep_start"); query.sleep_end = request->arg("sleep_end");
  query.sleep_timeout = request->arg("sleep_timeout");
  unsigned slot = 0, offset = 0;
  if (query.icon.size() > 80 || query.icon_override.size() > 1 || query.op.size() > 16 || query.entity.size() > 128 || query.action.size() > 128 ||
      query.search.size() > 128 || query.domain.size() > 32 ||
      query.brightness.size() > 3 || query.standby.size() > 3 || query.sensitivity.size() > 1 ||
      query.sleep_enabled.size() > 1 || query.sleep_start.size() > 5 || query.sleep_end.size() > 5 || query.sleep_timeout.size() > 4 ||
      query.name.size() > 192 || query.area.size() > 192 || query.action_name.size() > 192 || query.type.size() > 32 ||
      (request->hasArg("slot") && (!number(request->arg("slot"), slot) || slot >= 10)) ||
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
  {
    LockGuard lock(mutex_);
    if (pending_.empty() || !handler_) return;
    query = std::move(pending_.front()); pending_.pop_front();
  }
  // NVS, entity subscriptions, and LVGL always run in the ESPHome main task.
  std::string result = handler_(query);
  LockGuard lock(mutex_);
  results_.emplace_back(query.ticket, std::move(result));
  while (results_.size() > 4) results_.pop_front();
}
}  // namespace esphome::onx_config_web
