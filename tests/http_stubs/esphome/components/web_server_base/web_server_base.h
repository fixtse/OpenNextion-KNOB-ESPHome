#pragma once
#include <map>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <cstdlib>

enum {HTTP_GET, HTTP_POST, HTTP_OPTIONS};
class AsyncWebServerResponse {
 public:
  std::string body;
  explicit AsyncWebServerResponse(std::string body) : body(std::move(body)) {}
  void addHeader(const char *, const char *) {}
};
class AsyncWebServerRequest {
 public:
  static constexpr unsigned URL_BUF_SIZE = 256;
  std::string url{"/knob/api"}, body, status;
  int method_{HTTP_POST};
  std::map<std::string,std::string> args, headers;
  unsigned length{0};
  std::string_view url_to(std::span<char,URL_BUF_SIZE>) { return url; }
  int method() const { return method_; }
  unsigned contentLength() const { return length; }
  std::string arg(const char *key) const { auto it=args.find(key); return it==args.end()?"":it->second; }
  bool hasArg(const char *key) const { return args.count(key); }
  std::optional<std::string> get_header(const char *key) const { auto it=headers.find(key); return it==headers.end()?std::nullopt:std::optional<std::string>(it->second); }
  AsyncWebServerResponse *beginResponse(int code,const char *,const std::string &text) {
    // Match ESPHome 2026.10.2's actual native status-code whitelist.
    switch(code) {case 200:case 204:case 400:case 401:case 404:case 409:case 422:status=std::to_string(code);break;default:status="500";}
    return new AsyncWebServerResponse(text);
  }
  void send(AsyncWebServerResponse *response) {body=response->body; delete response;}
  int code() const {return std::atoi(status.c_str());}
};
inline void httpd_resp_set_status(AsyncWebServerRequest &request,const char *status) {request.status=status;}
class AsyncWebHandler { public: virtual ~AsyncWebHandler()=default; virtual bool canHandle(AsyncWebServerRequest *) const {return false;} virtual void handleRequest(AsyncWebServerRequest *) {} };
namespace esphome::web_server_base {
class WebServerBase { public: void add_handler(AsyncWebHandler *) {} void init() {} };
inline WebServerBase instance;
inline WebServerBase *global_web_server_base=&instance;
}
