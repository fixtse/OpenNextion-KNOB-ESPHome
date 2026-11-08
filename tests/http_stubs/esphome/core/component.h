#pragma once
namespace esphome {
namespace setup_priority { constexpr float WIFI = 250; }
class Component { public: virtual ~Component() = default; virtual void setup() {} virtual void loop() {} virtual float get_setup_priority() const { return 600; } };
}
