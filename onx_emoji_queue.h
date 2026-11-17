#pragma once
// Hand-off between the emoji renderer (onx_emoji.h, LVGL side) and the icon
// download worker (onx_icons). Kept free of LVGL so the downloader stays testable.
#include <cstdint>
#include <deque>
#include <string>

namespace onx_emoji_queue {
// Downloaded emoji are rasterized once at this size (BGRA, PSRAM) and scaled per font.
inline constexpr unsigned SIZE = 48;
struct Request { uint32_t index; std::string stem; };  // stem: Noto file name, e.g. "1f468_200d_1f469".
inline std::deque<Request> pending;
// Called on the UI task. pixels is SIZE*SIZE*4 BGRA owned by the receiver, or null on failure.
inline void (*completed)(uint32_t index, uint8_t *pixels) = nullptr;
// Called on the UI task while the worker is idle, so failed emoji can be retried.
inline void (*tick)() = nullptr;
}
