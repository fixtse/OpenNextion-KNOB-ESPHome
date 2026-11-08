#include "onx_config_web.h"
#include "config_page.h"
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
  return url == "/knob" || url == "/knob/" || url == "/knob/api" || url == "/knob/result";
}
void ConfigWeb::respond_(AsyncWebServerRequest *request, int code, const std::string &body) {
  auto *response = request->beginResponse(code, "application/json", body);
  response->addHeader("Cache-Control", "no-store");
  request->send(response);
}
void ConfigWeb::handleRequest(AsyncWebServerRequest *request) {
  std::array<char, AsyncWebServerRequest::URL_BUF_SIZE> buffer;
  const auto url = request->url_to(buffer);
  if ((url == "/knob" || url == "/knob/") && request->method() == HTTP_GET) {
    std::string page(CONFIG_PAGE);
    const auto token = page.find("__ONX_TOKEN__");
    page.replace(token, 13, nonce_);
    auto *response = request->beginResponse(200, "text/html; charset=utf-8", page);
    response->addHeader("Cache-Control", "no-store");
    response->addHeader("X-Content-Type-Options", "nosniff");
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
  if (request->contentLength() > 2048) { respond_(request, 413, "{\"error\":\"Request too large\"}"); return; }
  Query query;
  query.op = request->arg("op"); query.entity = request->arg("entity"); query.action = request->arg("action");
  query.search = request->arg("q"); query.domain = request->arg("domain");
  unsigned slot = 0, offset = 0;
  if (query.op.size() > 16 || query.entity.size() > 128 || query.action.size() > 128 ||
      query.search.size() > 128 || query.domain.size() > 32 ||
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
