#pragma once
#include <algorithm>
#include <cstdint>
#include <deque>
#include <string>
#include "onx_memory.h"
#include "onx_notification.h"

namespace onx_gesture {
struct LegacyConfig {
  uint32_t magic{0x4F4E5847};
  char entity[129]{},actions[4][129]{},feedback[129]{};
  uint8_t popup{1},responses{0},events{0},step{1};
};
// A new preference key preserves existing v1 gesture settings on upgrade.
struct Config : LegacyConfig { uint8_t press_responses{0}; };
struct State {
  std::string value,unit,notice;
  bool received{false},press_request{false};
  uint32_t requested_ms{0},feedback_ms{0},token{0};
};
inline onx_memory::StateArray<Config,20> configs;
inline onx_memory::StateArray<State,20> states;
inline const char *names[]={"clockwise","counterclockwise","held_clockwise","held_counterclockwise"};
inline int gesture(int direction,bool pressed) {return (pressed?2:0)+(direction<0?1:0);}
struct Request {int slot,kind,delta;};
inline std::deque<Request> pending;
inline int accumulator{0},accumulator_kind{-1},accumulator_slot{-1};
inline void clear_pending() {pending.clear();accumulator=0;accumulator_slot=accumulator_kind=-1;}
inline bool routes(int slot,int kind) {
  return slot>=0 && slot<20 && kind>=0 && kind<4 && (configs[slot].actions[kind][0] || configs[slot].events);
}
inline void queue(int slot,int kind,int threshold) {
  if(std::string(configs[slot].actions[kind])=="none" && !configs[slot].events)return;
  if(accumulator_slot!=slot || accumulator_kind!=kind) {
    if(accumulator_slot!=slot)pending.clear();
    accumulator=0;accumulator_slot=slot;accumulator_kind=kind;
  }
  if(++accumulator<std::clamp(threshold,1,4))return;
  accumulator=0;
  const int delta=(kind%2?-1:1)*configs[slot].step;
  if(!pending.empty() && pending.back().slot==slot && pending.back().kind==kind) {
    pending.back().delta=std::clamp(pending.back().delta+delta,-1000,1000);return;
  }
  if(pending.size()<8)pending.push_back({slot,kind,delta});
}
inline void feedback(int slot,const std::string &value,const std::string &unit,uint32_t now,bool response=false) {
  if(slot<0 || slot>=20)return;
  auto &state=states[slot];
  const auto text=onx_notification::clipped(value,128);
  const bool changed=state.received && (text!=state.value || unit!=state.unit);
  state.notice.clear();state.value=text;state.unit=onx_notification::clipped(unit,24);state.received=true;
  const bool popup=state.press_request ? configs[slot].press_responses : configs[slot].popup;
  if(popup && (response || (changed && state.token && now-state.requested_ms<5000)))state.feedback_ms=now;
}
}  // namespace onx_gesture
