#pragma once
#include <memory>
#include <mutex>
namespace esphome {using Mutex=std::mutex;using LockGuard=std::lock_guard<Mutex>;
template<typename T> struct RAMAllocator : std::allocator<T> {
 template<typename U> struct rebind { using other = RAMAllocator<U>; };
 RAMAllocator() = default; template<typename U> RAMAllocator(const RAMAllocator<U> &) {}
};}
