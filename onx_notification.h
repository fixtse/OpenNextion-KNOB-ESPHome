#pragma once
#include <algorithm>
#include <cstdint>
#include <string>

namespace onx_notification {
inline std::string clipped(const std::string &text,size_t limit) {
  size_t end=std::min(text.size(),limit);
  while(end>0 && end<text.size() && (static_cast<unsigned char>(text[end])&0xC0)==0x80)--end;
  return text.substr(0,end);
}
struct Message {
  std::string title,body,image_url;
  bool pending{false},visible{false};
  uint32_t remaining_ms{60000},shown_ms{0},revision{0};
  void dismiss() {pending=false;visible=false;}
  bool show(const std::string &heading,const std::string &text,int seconds,const std::string &image="") {
    if(text.empty())return false;
    title=clipped(heading.empty()?"Reminder":heading,96);
    for(char &ch:title)if(ch=='\r' || ch=='\n')ch=' ';
    body=clipped(text,768);
    image_url=image.size()<=1024 && (image.rfind("http://",0)==0 || image.rfind("https://",0)==0) ? image : "";
    remaining_ms=static_cast<uint32_t>(seconds<0?60:std::min(seconds,3600))*1000;
    pending=true;visible=false;++revision;return true;
  }
  void suspend(uint32_t now) {
    if(visible && remaining_ms) {
      const uint32_t elapsed=now-shown_ms;
      if(elapsed>=remaining_ms) {dismiss();return;}
      remaining_ms-=elapsed;
    }
    visible=false;
  }
  bool display(uint32_t now) {
    if(!pending)return false;
    if(!visible) {visible=true;shown_ms=now;}
    if(remaining_ms && now-shown_ms>=remaining_ms) {dismiss();return false;}
    return true;
  }
};
// Only the newest message is retained, so unattended notifications stay bounded.
inline Message current;
struct ImageState {
  uint32_t revision{0},started_ms{0};
  bool waiting{false},attempted{false},downloading{false},ready{false},failed{false};
};
inline ImageState image;
inline bool press_consumed{false};
}  // namespace onx_notification
