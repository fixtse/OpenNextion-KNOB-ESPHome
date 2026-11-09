#pragma once
#include <string>
#include <algorithm>
#include <map>
#include <vector>
#include <cstring>
#include <cstdint>
constexpr int ESP_OK=0;
inline const char *esp_err_to_name(int){return "connection error";}
struct esp_http_client_config_t {
 const char *url=nullptr;int (*crt_bundle_attach)(void *)=nullptr;
 int timeout_ms=0;bool disable_auto_redirect=false;int buffer_size=0;const char *user_agent=nullptr;
};
struct FakeHttp {
 std::string url;std::map<std::string,std::string> headers;size_t offset=0;bool stalled=false;
 int status=200;
};
inline std::vector<std::string> requested_icon_urls;
inline bool fail_primary=false,fail_all=false,stall_once=false;
inline const std::string icon_svg="<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"80\" height=\"80\" viewBox=\"0 0 24 24\"><path fill=\"#fff\" d=\"M2 2H22V22H2Z\"/></svg>";
inline FakeHttp *esp_http_client_init(const esp_http_client_config_t *config){
 if(!config->crt_bundle_attach||!config->disable_auto_redirect||std::string(config->user_agent)!="ONX2424G013/MDI-icons")return nullptr;
 requested_icon_urls.emplace_back(config->url);
 auto *http=new FakeHttp;http->url=config->url;
 http->status=fail_all||(fail_primary&&http->url.find("iconify")!=std::string::npos)?403:200;
 return http;
}
inline int esp_http_client_set_header(FakeHttp *http,const char *key,const char *value){http->headers[key]=value;return 0;}
inline int esp_http_client_open(FakeHttp *,int){return ESP_OK;}
inline int64_t esp_http_client_fetch_headers(FakeHttp *){return -1;} // Chunked body with unknown length.
inline int esp_http_client_get_status_code(FakeHttp *http){return http->status;}
inline int esp_http_client_read(FakeHttp *http,char *buffer,size_t size){
 if(stall_once&&!http->stalled){http->stalled=true;return 0;}
 const auto count=std::min(size,icon_svg.size()-http->offset);
 std::memcpy(buffer,icon_svg.data()+http->offset,count);http->offset+=count;return count;
}
inline bool esp_http_client_is_complete_data_received(FakeHttp *http){return http->offset==icon_svg.size();}
inline void esp_http_client_close(FakeHttp *){}
inline void esp_http_client_cleanup(FakeHttp *http){delete http;}
