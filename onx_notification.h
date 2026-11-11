#pragma once
#include <algorithm>
#include <cstdint>
#include <cctype>
#include <string>

namespace onx_notification {
inline std::string clipped(const std::string &text,size_t limit) {
  size_t end=std::min(text.size(),limit);
  while(end>0 && end<text.size() && (static_cast<unsigned char>(text[end])&0xC0)==0x80)--end;
  return text.substr(0,end);
}
inline std::string plain_text(const std::string &source,size_t limit) {
  // Android/Telegram notification bodies can contain HTML, including escaped
  // markup. Decode first so &lt;br/&gt; and <br/> behave identically.
  const auto input=clipped(source,8192);
  std::string decoded;decoded.reserve(input.size());
  auto utf8=[&](uint32_t cp) {
    if(cp<0x80)decoded+=char(cp);
    else if(cp<0x800) {decoded+=char(0xC0|(cp>>6));decoded+=char(0x80|(cp&63));}
    else if(cp<0x10000) {decoded+=char(0xE0|(cp>>12));decoded+=char(0x80|((cp>>6)&63));decoded+=char(0x80|(cp&63));}
    else {decoded+=char(0xF0|(cp>>18));decoded+=char(0x80|((cp>>12)&63));decoded+=char(0x80|((cp>>6)&63));decoded+=char(0x80|(cp&63));}
  };
  for(size_t i=0;i<input.size();) {
    if(input[i]=='&') {
      const auto end=input.find(';',i+1);
      if(end!=std::string::npos && end-i<=12) {
        const auto entity=input.substr(i+1,end-i-1);
        uint32_t cp=0;
        if(entity=="amp")cp='&';else if(entity=="lt")cp='<';else if(entity=="gt")cp='>';
        else if(entity=="quot")cp='"';else if(entity=="apos" || entity=="#39")cp='\'';else if(entity=="nbsp")cp=' ';
        else if(!entity.empty() && entity[0]=='#') {
          size_t n=1;unsigned base=10;
          if(n<entity.size() && (entity[n]=='x' || entity[n]=='X')) {base=16;++n;}
          bool valid=n<entity.size();
          for(;n<entity.size();++n) {
            const unsigned char c=entity[n];
            const unsigned digit=c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:16;
            if(digit>=base || cp>(0x10FFFF-digit)/base) {valid=false;break;}
            cp=cp*base+digit;
          }
          if(!valid || cp>0x10FFFF || (cp>=0xD800 && cp<=0xDFFF))cp=0;
        }
        if(cp) {utf8(cp);i=end+1;continue;}
      }
    }
    decoded+=input[i++];
  }
  std::string out;out.reserve(std::min(decoded.size(),limit));
  auto newline=[&]() {
    while(!out.empty() && out.back()==' ')out.pop_back();
    if(!out.empty() && (out.back()!='\n' || (out.size()>1 && out[out.size()-2]!='\n')))out+='\n';
  };
  for(size_t i=0;i<decoded.size() && (out.size()<limit || (static_cast<unsigned char>(decoded[i])&0xC0)==0x80);) {
    if(decoded[i]=='<') {
      const auto end=decoded.find('>',i+1);
      size_t start=i+1;
      if(start<decoded.size() && decoded[start]=='/')++start;
      if(end!=std::string::npos && end-i<=128 && start<end && std::isalpha(static_cast<unsigned char>(decoded[start]))) {
        size_t stop=start;
        while(stop<end && std::isalnum(static_cast<unsigned char>(decoded[stop])))++stop;
        std::string tag=decoded.substr(start,stop-start);
        for(char &c:tag)c=std::tolower(static_cast<unsigned char>(c));
        if(tag=="br" || tag=="p" || tag=="div" || tag=="li")newline();
        i=end+1;continue;
      }
    }
    const char c=decoded[i++];
    if(c=='\r') {if(i<decoded.size() && decoded[i]=='\n')++i;newline();}
    else if(c=='\n')newline();
    else out+=c=='\t'?' ':c;
  }
  while(!out.empty() && (out.back()=='\n' || out.back()==' '))out.pop_back();
  return clipped(out,limit);
}
struct Message {
  std::string title,body,image_url;
  bool pending{false},visible{false};
  uint32_t remaining_ms{60000},total_ms{60000},shown_ms{0},revision{0};
  void dismiss() {pending=false;visible=false;}
  bool show(const std::string &heading,const std::string &text,int seconds,const std::string &image="") {
    if(text.empty())return false;
    title=plain_text(heading.empty()?"Reminder":heading,96);
    for(char &ch:title)if(ch=='\r' || ch=='\n')ch=' ';
    body=plain_text(text,768);
    image_url=image.size()<=1024 && (image.rfind("http://",0)==0 || image.rfind("https://",0)==0) ? image : "";
    remaining_ms=static_cast<uint32_t>(seconds<0?60:std::min(seconds,3600))*1000;
    total_ms=remaining_ms;
    pending=true;visible=false;++revision;return true;
  }
  int countdown(uint32_t now) const {
    if(!total_ms)return -1;
    const uint32_t elapsed=visible?now-shown_ms:0;
    const uint32_t left=elapsed>=remaining_ms?0:remaining_ms-elapsed;
    return static_cast<int>(static_cast<uint64_t>(left)*1000/total_ms);
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
