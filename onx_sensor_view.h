#pragma once
#include "onx_memory.h"
#include "onx_display_settings.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>
#include <utility>
namespace onx_sensor_view {
// Keep axis labels compact without rounding distinct bounds to the same text.
inline std::pair<std::string,std::string> axis_labels(float low,float high) {
  char bottom[24],top[24];
  for(int precision=4;precision<=7;++precision) {
    snprintf(bottom,sizeof(bottom),"%.*g",precision,static_cast<double>(low==0?0:low));
    snprintf(top,sizeof(top),"%.*g",precision,static_cast<double>(high==0?0:high));
    if(low==high || std::string(bottom)!=top)break;
  }
  return {bottom,top};
}
struct Config {
  char primary[129]{}, secondary[129]{}, text[161]{"{value}"};
  uint8_t graph{0};
};
inline onx_memory::StateArray<Config,20> configs;
struct Point { uint32_t epoch{0}; float value{NAN}; };
struct State {
  std::string primary, secondary, value, unit;
  time_t secondary_epoch{0};
  bool received{false}, available{false}, pending{false}, requested{false};
  uint32_t requested_ms{0};
  std::array<Point,73> points{};
  void bind(const Config &c) {
    if(primary!=c.primary || secondary!=c.secondary) { *this=State{};primary=c.primary;secondary=c.secondary; }
  }
  void sample(uint32_t epoch, float value) {
    if(epoch<1577836800 || !std::isfinite(value)) return;
    const uint32_t bucket=epoch/600;
    auto &p=points[bucket%points.size()];
    if(p.epoch/600==bucket && p.epoch>epoch)return;
    p={epoch,value};
  }
  std::vector<Point> history(uint32_t now) const {
    std::vector<Point> out;
    for(const auto &p:points) if(p.epoch && p.epoch<=now && now-p.epoch<=43200 && std::isfinite(p.value))out.push_back(p);
    std::sort(out.begin(),out.end(),[](const Point&a,const Point&b){return a.epoch<b.epoch;});
    return out;
  }
};
inline onx_memory::StateArray<State,20> states;
inline int index(const std::string &entity) {
  for(size_t i=0;i<configs.size();++i)if(entity==configs[i].primary)return int(i);
  return -1;
}
inline std::string history_entity(const std::string &primary) {
  const int i=index(primary);
  return primary.rfind("switch.",0)==0 && i>=0 && configs[i].secondary[0] ? std::string(configs[i].secondary) : primary;
}
inline void replace(std::string &text,const std::string &key,const std::string &value) {
  size_t at=0;while((at=text.find(key,at))!=std::string::npos){text.replace(at,key.size(),value);at+=value.size();}
}
inline std::string relative(time_t target,time_t now) {
  if(!target || now<1577836800)return "—";
  const int64_t delta=int64_t(target)-int64_t(now), seconds=delta<0?-delta:delta;
  if(seconds<60)return delta<0?"just now":"in less than a minute";
  const auto count=seconds>=86400?(seconds+1800)/86400:seconds>=3600?(seconds+120)/3600:(seconds+30)/60;
  std::string unit=seconds>=86400?"day":seconds>=3600?"hour":"minute";
  if(count!=1)unit+='s';
  const auto amount=std::to_string(std::max<int64_t>(1,count))+" "+unit;
  return delta<0?amount+" ago":"in "+amount;
}
inline std::string format(const Config &config,const State &state,time_t now) {
  if(!config.secondary[0])return "";
  if(!state.received)return "Waiting for secondary data";
  if(!state.available)return "Secondary unavailable";
  std::string text=config.text;
  std::string date="—",time="—";
  if(state.secondary_epoch) {
    struct tm local{};localtime_r(&state.secondary_epoch,&local);
    char buffer[48];strftime(buffer,sizeof(buffer),onx_display_settings::date_pattern(),&local);date=buffer;
    strftime(buffer,sizeof(buffer),onx_display_settings::time_pattern(),&local);time=buffer;
    if(onx_display_settings::datetime.hour_format==1 && time.size()>1 && time[0]=='0')time.erase(0,1);
  }
  // Replace computed placeholders first, leaving raw values uninterpreted.
  replace(text,"{relative}",relative(state.secondary_epoch,now));
  replace(text,"{datetime}",date+" "+time);replace(text,"{date}",date);replace(text,"{time}",time);
  replace(text,"{unit}",state.unit);replace(text,"{value}",state.value);
  return text.substr(0,240);
}
}
