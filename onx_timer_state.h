#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>

namespace onx_timer {

// Use the original duration for unnamed Assist timers, never the countdown.
// HA resolves duplicate names/durations and sends the authoritative update.
enum class Action { PAUSE, RESUME, STOP };
inline constexpr const char *conversation_agent = "conversation.home_assistant";
// HA's English sentences accept hours/minutes/seconds of 1-100 each, so say
// "5 minute", "1 hour and 30 minute", never "300 second".
inline std::string spoken_duration(uint32_t total) {
  const uint32_t h = total / 3600, m = (total / 60) % 60, s = total % 60;
  std::string out;
  auto add = [&](uint32_t v, const char *unit) {
    if (!v) return;
    if (!out.empty()) out += " and ";
    out += std::to_string(v) + ' ' + unit;
  };
  add(h, "hour"); add(m, "minute"); add(s, "second");
  return out.empty() ? "0 second" : out;
}
inline std::string control_sentence(const std::string &name, uint32_t total, Action action) {
  return std::string(action == Action::STOP ? "cancel the " : action == Action::RESUME ? "resume the " : "pause the ") +
         (name.empty() ? spoken_duration(total) : name) + " timer";
}

inline std::string control_sentence(const std::string &name, uint32_t total, bool resume) {
  return control_sentence(name, total, resume ? Action::RESUME : Action::PAUSE);
}
// One timer from a voice satellite's onx.timer.v1 "timers" list
// ([name, duration, remaining, active]), soonest first.
struct VoiceTimer { std::string name; uint32_t duration{0}, remaining{0}; bool active{false}; };
// The satellite's "Stop timer alarm" button for its "Voice timer" sensor, using
// Home Assistant's default entity ids ("" when the sensor id does not match).
inline std::string stop_alarm_button(const std::string &sensor) {
  const std::string prefix = "sensor.", suffix = "_voice_timer";
  if (sensor.size() <= prefix.size() + suffix.size() || sensor.compare(0, prefix.size(), prefix) != 0 ||
      sensor.compare(sensor.size() - suffix.size(), suffix.size(), suffix) != 0) return "";
  return "button." + sensor.substr(prefix.size(), sensor.size() - prefix.size() - suffix.size()) + "_stop_timer_alarm";
}

struct Popup { bool visible{false}, dismissed{false}; int previous_index{0}; std::string entity; };
inline Popup popup;
inline bool popup_due(uint32_t idle_ms, bool detail_active, bool dismissed=false) {
  return idle_ms >= 10000 || (!dismissed && !detail_active && idle_ms >= 1500);
}
// Native API serializes list attributes as Python literals. Convert only the
// JSON-compatible subset; never evaluate input or accept unbounded payloads.
inline std::string attribute_json(const std::string &input) {
  if (input.size() > 8192) return "";
  std::string out; out.reserve(input.size()); char quote=0;
  for (size_t i=0;i<input.size();++i) {
    char c=input[i];
    if (quote) {
      if (c==quote) {out+='"';quote=0;}
      else if (c=='\\' && i+1<input.size()) {
        char next=input[++i];
        if (next==39) out+=39;
        else {out+='\\';out+=next;}
      } else {if(c=='"')out+='\\';out+=c;}
    } else if (c==39 || c=='"') {quote=c;out+='"';}
    else if (input.compare(i,4,"None")==0) {out+="null";i+=3;}
    else if (input.compare(i,4,"True")==0) {out+="true";i+=3;}
    else if (input.compare(i,5,"False")==0) {out+="false";i+=4;}
    else out+=c;
  }
  return quote ? "" : out;
}

// HA timers use H:MM:SS, including durations longer than 24 hours.
inline double duration_seconds(const std::string &value) {
  uint64_t parts[3]{};
  size_t part = 0;
  bool digit = false;
  for (char ch : value) {
    if (ch == ':' && digit && part < 2) {
      ++part;
      digit = false;
    } else if (ch >= '0' && ch <= '9') {
      parts[part] = parts[part] * 10 + static_cast<unsigned>(ch - '0');
      if (parts[part] > 315360000) return NAN;
      digit = true;
    } else {
      return NAN;
    }
  }
  if (part != 2 || !digit || parts[1] >= 60 || parts[2] >= 60) return NAN;
  return static_cast<double>(parts[0] * 3600 + parts[1] * 60 + parts[2]);
}

struct View {
  bool ready{false};
  double seconds_left{0};
  double fraction{0};
};

// NAN is a float. A conditional mixing it with an integer Unix timestamp
// rounds modern deadlines to 128-second steps before conversion to double.
inline double deadline_epoch(bool available, int64_t epoch) {
  return available ? static_cast<double>(epoch) : std::numeric_limits<double>::quiet_NaN();
}

inline View view(const std::string &state, double duration, double remaining,
                 double finishes_at, double now) {
  if (state == "idle" || state == "ringing") return {true, 0, 0};
  double left = NAN;
  if (state == "paused") left = remaining;
  // remaining is a snapshot attribute, NOT a ticking value while active.
  if (state == "active" && std::isfinite(finishes_at) && finishes_at > 0 && now > 0 && std::isfinite(now))
    left = finishes_at - now;
  if (!std::isfinite(left)) return {};
  left = std::max(0.0, left);
  const double fraction = std::isfinite(duration) && duration > 0
                              ? std::clamp(left / duration, 0.0, 1.0) : 0;
  return {true, left, fraction};
}

inline std::string countdown(double seconds) {
  if (!std::isfinite(seconds) || seconds < 0 || seconds > UINT32_MAX) return "--:--";
  const auto total = static_cast<uint32_t>(std::ceil(seconds));
  char text[32];
  if (total >= 3600)
    std::snprintf(text, sizeof(text), "%lu:%02lu:%02lu", static_cast<unsigned long>(total / 3600),
                  static_cast<unsigned long>((total / 60) % 60), static_cast<unsigned long>(total % 60));
  else
    std::snprintf(text, sizeof(text), "%02lu:%02lu", static_cast<unsigned long>(total / 60),
                  static_cast<unsigned long>(total % 60));
  return text;
}

}  // namespace onx_timer
