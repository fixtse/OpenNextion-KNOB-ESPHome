#pragma once
#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <vector>
#include <utility>

namespace onx_control {
// HA Native API attributes are Python repr strings, not necessarily JSON.
// Lists can contain StrEnum repr values, e.g. <HVACMode.HEAT: 'heat'>.
// Parse quoted values without splitting options containing commas or apostrophes.
inline std::vector<std::string> options(const std::string &raw) {
  std::vector<std::string> result;
  size_t i = raw.find_first_not_of(" \r\n\t");
  if (i == std::string::npos || raw[i++] != '[') return result;
  while (i < raw.size()) {
    i = raw.find_first_not_of(" \r\n\t", i);
    if (i == std::string::npos) return {};
    if (raw[i] == ']') return raw.find_first_not_of(" \r\n\t", i + 1) == std::string::npos ? result : std::vector<std::string>{};
    const bool enum_value = raw[i] == '<';
    if (enum_value) {
      // Accept an enum identifier followed by its quoted value, never arbitrary
      // unquoted expressions. HA forwards str(attribute), which preserves repr
      // wrappers for enums inside lists even though the state itself is "off".
      ++i;
      bool segment_start = true, qualified = false;
      for (; i < raw.size() && raw[i] != ':'; ++i) {
        const unsigned char c = raw[i];
        if (c == '.' && !segment_start) { qualified = true; segment_start = true; }
        else if (c == '_' || std::isalpha(c) || (!segment_start && std::isdigit(c))) segment_start = false;
        else return {};
      }
      if (i == raw.size() || !qualified || segment_start) return {};
      i = raw.find_first_not_of(" \r\n\t", i + 1);
      if (i == std::string::npos) return {};
    }
    const char quote = raw[i++];
    if (quote != '\'' && quote != '"') return {};
    std::string value;
    bool closed = false;
    while (i < raw.size()) {
      char c = raw[i++];
      if (c == quote) { closed = true; break; }
      if (c == '\\') {
        if (i == raw.size()) return {};
        c = raw[i++];
        if (c == 'n') c = '\n';
        else if (c == 'r') c = '\r';
        else if (c == 't') c = '\t';
      }
      value += c;
    }
    if (!closed || value.empty()) return {};
    result.push_back(value);
    i = raw.find_first_not_of(" \r\n\t", i);
    if (i == std::string::npos) return {};
    if (enum_value) {
      if (raw[i] != '>') return {};
      i = raw.find_first_not_of(" \r\n\t", i + 1);
      if (i == std::string::npos) return {};
    }
    if (raw[i] == ',') { ++i; continue; }
    if (raw[i] != ']') return {};
  }
  return {};
}
inline float number(const std::string &s) {
  char *end = nullptr;
  const float n = strtof(s.c_str(), &end);
  return end != s.c_str() && *end == '\0' && std::isfinite(n) ? n : NAN;
}
inline bool equal(const std::string &a, const std::string &b) {
  if (a == b) return true;
  const float x = number(a), y = number(b);
  return std::isfinite(x) && std::isfinite(y) && x == y;
}
struct Value {
  std::string field, parameter, requested;
  bool confirmed{false};
};
struct Command {
  std::string entity, service;
  std::vector<Value> values;
  uint32_t changed{0}, sent{0}, first_change{0};
  bool active{false}, dirty{false}, transmitted{false};
  uint32_t error_at{0};
  bool failed{false};

  void cancel() { *this = Command{}; }
  void request(const std::string &id, const std::string &action, std::vector<Value> target, uint32_t now) {
    const bool same = active && entity == id && service == action;
    if (!same) cancel();
    if (!dirty) first_change = now;
    entity = id; service = action; values = std::move(target);
    changed = now; active = dirty = true; failed = false;
  }
  std::string effective(const std::string &field, const std::string &actual) const {
    if (active) for (const auto &v : values) if (v.field == field) return v.requested;
    return actual;
  }
  bool due(uint32_t now) const {
    return active && dirty && (!transmitted || now - sent >= 200) &&
           (now - changed >= 80 || now - first_change >= 250);
  }
  void mark_sent(uint32_t now) { sent = now; transmitted = true; dirty = false; }
  void feedback(const std::string &field, const std::string &actual) {
    if (!active || dirty || !transmitted) return;
    for (auto &v : values) if (v.field == field) v.confirmed = equal(v.requested, actual);
    if (std::all_of(values.begin(), values.end(), [](const Value &v) { return v.confirmed; })) active = false;
  }
  void expire(uint32_t now) {
    if (active && transmitted && !dirty && now - sent >= 5000) {
      active = false; failed = true; error_at = now;
    }
    if (failed && now - error_at >= 2500) failed = false;
  }
};
inline float stepped(float current, float minimum, float maximum, float step, int direction) {
  if (!std::isfinite(current) || !std::isfinite(minimum) || !std::isfinite(maximum) ||
      !std::isfinite(step) || step <= 0 || minimum > maximum) return NAN;
  // Snap to the advertised grid anchored at minimum, then clamp the endpoint.
  return std::clamp(minimum + std::round((current - minimum) / step + direction) * step, minimum, maximum);
}
}  // namespace onx_control
