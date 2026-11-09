#pragma once
#include <mutex>
namespace esphome {using Mutex=std::mutex;using LockGuard=std::lock_guard<Mutex>;}
