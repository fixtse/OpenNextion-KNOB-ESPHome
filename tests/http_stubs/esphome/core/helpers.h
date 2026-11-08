#pragma once
#include <cstdint>
#include <cstdio>
#include <mutex>
#include <string>
namespace esphome {
using Mutex = std::mutex;
using LockGuard = std::lock_guard<Mutex>;
inline uint32_t random_uint32() { return 123; }
template<typename... Args> std::string str_sprintf(const char *format, Args... args) { char text[128]; std::snprintf(text,sizeof(text),format,args...); return text; }
}
