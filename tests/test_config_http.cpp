#include "../my_components/onx_config_web/onx_config_web.h"
#include <cassert>
#include <iostream>

using esphome::onx_config_web::ConfigWeb;
using esphome::onx_config_web::Query;
AsyncWebServerRequest request(const std::string &token) {
  AsyncWebServerRequest req;
  req.headers={{"Origin","http://192.168.10.52"},{"Host","192.168.10.52"},{"X-Onx-Token",token}};
  req.args={{"op","refresh"}};
  return req;
}
int main() {
  ConfigWeb web;
  web.setup();
  unsigned refreshes=0;
  web.set_handler([&](const Query &query) {assert(query.op=="refresh"); ++refreshes; return std::string("{\"ok\":true}");});
  AsyncWebServerRequest page;
  page.url="/knob"; page.method_=HTTP_GET;
  web.handleRequest(&page);
  assert(page.code()==200);
  const std::string token_marker = "window.ONX_TOKEN='";
  const auto marker_start = page.body.find(token_marker);
  assert(marker_start != std::string::npos);
  const auto token_start = marker_start + token_marker.size();
  const auto token=page.body.substr(token_start,page.body.find("'",token_start)-token_start);
  assert(token.size()==32);
  assert(page.copied_response_bytes==0&&page.chunks==4);
  for(const auto &url : {"/knob/ha.js","/knob/app.js"}) {
    AsyncWebServerRequest asset;asset.url=url;asset.method_=HTTP_GET;web.handleRequest(&asset);
    assert(asset.code()==200&&!asset.body.empty()&&asset.copied_response_bytes==0);
    assert(asset.flash_response_bytes==asset.body.size());
  }
  auto refresh=request(token);
  web.handleRequest(&refresh);
  assert(refresh.code()==202); // Old wrapper incorrectly returned 500 here.
  assert(refreshes==0); // HTTP task must not perform HA refresh/NVS/UI work.
  assert(refresh.body=="{\"ticket\":1}");
  AsyncWebServerRequest result;
  result.url="/knob/result"; result.method_=HTTP_GET; result.args={{"ticket","1"}};
  web.handleRequest(&result);
  assert(result.code()==202 && result.body=="{\"pending\":true}");
  web.loop();
  assert(refreshes==1);
  web.handleRequest(&result);
  assert(result.code()==200 && result.body=="{\"ok\":true}");
  // Large responses must survive the protocol intact without another heap copy.
  const std::string large_json="{\"value\":\""+std::string(10000,'x')+"\"}";
  web.set_handler([&](const Query &){return large_json;});
  auto big=request(token);web.handleRequest(&big);web.loop();
  AsyncWebServerRequest big_result;big_result.url="/knob/result";big_result.method_=HTTP_GET;big_result.args={{"ticket","2"}};
  web.handleRequest(&big_result);assert(big_result.body==large_json&&big_result.copied_response_bytes==0);
  web.set_handler([&](const Query &query) {assert(query.op=="refresh"); ++refreshes; return std::string("{\"ok\":true}");});
  auto forbidden=request("wrong");web.handleRequest(&forbidden);assert(forbidden.code()==403);
  auto cross_origin=request(token);cross_origin.headers["Origin"]="http://another-host";web.handleRequest(&cross_origin);assert(cross_origin.code()==403);
  auto large=request(token);large.length=8193;web.handleRequest(&large);assert(large.code()==413);
  auto bad=request(token);bad.args["slot"]="20";web.handleRequest(&bad);assert(bad.code()==400);
  auto method=request(token);method.method_=HTTP_GET;web.handleRequest(&method);assert(method.code()==405);
  for(int i=0;i<4;++i) {auto queued=request(token);web.handleRequest(&queued);assert(queued.code()==202);}
  auto busy=request(token);web.handleRequest(&busy);assert(busy.code()==429);
  std::cout<<"HTTP regression passed: refresh acceptance, deferred execution, result polling, origin/token, size, method, validation, backpressure\n";
}
