#pragma once
#include <cstdint>
namespace esphome::logger {
class Logger { public: void set_log_level(uint8_t) {} };
inline Logger *global_logger = nullptr;
}
