#pragma once
namespace esphome::wifi {struct WiFi {bool connected=true;bool is_connected(){return connected;}};inline WiFi instance;inline auto *global_wifi_component=&instance;}
