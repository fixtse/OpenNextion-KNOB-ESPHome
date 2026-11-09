#pragma once
#include <cstdint>
#include "onx_playlist.h"
#include <string>
#include <algorithm>
#include <cmath>
namespace onx_media {
inline std::string volume_percent(float level) {
  if (!std::isfinite(level)) return "--";
  return std::to_string(static_cast<int>(std::lround(std::clamp(level, 0.0f, 1.0f) * 100.0f))) + "%";
}
// Bitmap label fonts render the selected emoji as monochrome glyphs. Their
// optional text/emoji presentation selectors have no visible glyph of their own.
inline std::string display_text(std::string value) {
  for (const char *selector : {"\xEF\xB8\x8E", "\xEF\xB8\x8F"}) {
    size_t at = 0;
    while ((at = value.find(selector, at)) != std::string::npos) value.erase(at, 3);
  }
  return value;
}

struct ProgressView {
  bool visible{false};
  float seconds{0}, duration{0};
  int permille{0};
};
// HA timestamps anchor late feedback; monotonic time provides smooth sub-second animation.
struct Progress {
  float duration{NAN}, position{NAN}, last_drawn{NAN};
  double updated_epoch{NAN};
  uint32_t received_ms{0}, drawn_ms{0};
  std::string state;
  bool waiting_track{false};
  void new_track() {
    position = last_drawn = NAN; updated_epoch = NAN; waiting_track = true;
  }
  void set_duration(float value) { duration = value; }
  void set_position(float value, uint32_t now) {
    position = value; received_ms = now; updated_epoch = NAN;
    last_drawn = NAN; waiting_track = false;
  }
  void set_updated_at(double value) {
    updated_epoch = value;
    // A changed track may restart at the same HA position (usually zero), so its
    // unchanged position attribute may not be resent. The new timestamp anchors zero.
    if (waiting_track && std::isfinite(value)) { position = 0; waiting_track = false; }
  }
  void set_state(const std::string &value, uint32_t now) {
    if (value == state) return;
    if (state == "playing" && value == "paused" && std::isfinite(position)) {
      position = std::isfinite(last_drawn) ? last_drawn + (now - drawn_ms) / 1000.0f
                                         : position + (now - received_ms) / 1000.0f;
      received_ms = now; updated_epoch = NAN;
    } else if (state == "paused" && value == "playing") {
      received_ms = now; updated_epoch = NAN;
    } else if (value != "playing" && value != "paused") {
      position = last_drawn = NAN; updated_epoch = NAN;
    }
    state = value;
  }
  ProgressView view(uint32_t now, double epoch, bool connected) {
    ProgressView out;
    if (!connected || waiting_track || (state != "playing" && state != "paused") ||
        !std::isfinite(duration) || duration <= 0 || !std::isfinite(position) || position < 0) return out;
    double elapsed = 0;
    if (state == "playing") {
      elapsed = (now - received_ms) / 1000.0;
      if (std::isfinite(epoch) && std::isfinite(updated_epoch)) {
        const double age = epoch - updated_epoch;
        if (age < -2) return out;
        elapsed = std::max(0.0, age);
      }
    }
    out.visible = true; out.duration = duration;
    out.seconds = std::clamp(static_cast<float>(position + elapsed), 0.0f, duration);
    out.permille = std::clamp(static_cast<int>(out.seconds / duration * 1000 + 0.5f), 0, 1000);
    last_drawn = out.seconds; drawn_ms = now;
    return out;
  }
};
inline bool has_playback(const std::string &state) {
  return state == "playing" || state == "paused" || state == "buffering";
}
inline bool can_skip(const std::string &state, uint32_t features, bool previous) {
  return has_playback(state) && (features & (previous ? 16U : 32U));
}
enum class Overlay { NONE, PAUSE, NEXT, PREVIOUS };
inline const char *overlay_name(Overlay overlay) {
  return overlay == Overlay::PAUSE ? "pause" : overlay == Overlay::NEXT ? "next" : overlay == Overlay::PREVIOUS ? "previous" : "";
}
inline const char *overlay_path(Overlay overlay) {
  return overlay == Overlay::PAUSE ? "M8 6h6v20H8zM18 6h6v20h-6z" :
         overlay == Overlay::NEXT ? "M7 6v20l14-10zM23 6h3v20h-3z" :
         overlay == Overlay::PREVIOUS ? "M25 6v20L11 16zM6 6h3v20H6z" : "";
}
struct TransportFeedback {
  std::string entity;
  uint32_t sent_ms{0};
  Overlay action{Overlay::NONE};
  void clear() { entity.clear(); action = Overlay::NONE; }
  void observe(const std::string &selected, bool active) {
    if (!active || entity != selected) clear();
  }
  void show(const std::string &selected, bool previous, uint32_t now) {
    entity = selected; sent_ms = now; action = previous ? Overlay::PREVIOUS : Overlay::NEXT;
  }
  uint32_t remaining_ms(const std::string &selected, uint32_t now) const {
    const uint32_t elapsed = now - sent_ms;
    return entity == selected && action != Overlay::NONE && elapsed < 1000 ? 1000 - elapsed : 0;
  }
  Overlay view(const std::string &selected, const std::string &state, uint32_t now) const {
    if (!has_playback(state)) return Overlay::NONE;
    if (remaining_ms(selected, now)) return action;
    return state == "paused" ? Overlay::PAUSE : Overlay::NONE;
  }
};
inline TransportFeedback transport;
inline std::string requested_url, ready_url;
inline uint32_t requested_ms{0};
inline bool downloading{false}, artwork_refresh_pending{false};
inline std::string candidate_url;
inline uint32_t candidate_ms{0}, last_artwork_end_ms{0};
inline std::string artwork_url(const std::string &origin, const std::string &picture) {
  if (picture.empty() || picture == "None" || picture == "unknown" || picture == "unavailable") return "";
  if (picture.rfind("http://", 0) == 0 || picture.rfind("https://", 0) == 0) return picture;
  if (picture[0] == '/' && picture.rfind("//", 0) != 0 && !origin.empty()) return origin + picture;
  return "";
}

}
