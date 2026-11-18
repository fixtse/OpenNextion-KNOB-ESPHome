#pragma once
#include <cstddef>
#include <cstring>
namespace esphome {
inline constexpr std::size_t MAC_ADDRESS_BUFFER_SIZE = 13;
inline void get_mac_address_into_buffer(char (&buffer)[MAC_ADDRESS_BUFFER_SIZE]) {
  std::memcpy(buffer, "aabbccddeeff", sizeof(buffer));
}
}
