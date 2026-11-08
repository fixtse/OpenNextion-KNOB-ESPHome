#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <string>

namespace onx_timer {

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
