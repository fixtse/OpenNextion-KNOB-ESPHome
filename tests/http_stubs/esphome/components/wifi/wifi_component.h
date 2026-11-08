#pragma once
namespace esphome::wifi {
class WiFiComponent { public: bool is_connected() const { return true; } };
inline WiFiComponent *global_wifi_component = nullptr;
}
