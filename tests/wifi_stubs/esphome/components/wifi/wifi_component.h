#pragma once
#include <map>
#include <string>
#include <utility>

namespace esphome::wifi {
class WiFiAP {
 public:
  void set_ssid(const std::string &ssid) { ssid_ = ssid; }
  void set_password(const std::string &password) { password_ = password; }
  const std::string &get_ssid() const { return ssid_; }
  const std::string &get_password() const { return password_; }
 private:
  std::string ssid_, password_;
};
using PreferenceStore = std::map<unsigned, WiFiAP>;
class WiFiComponent {
 public:
  static constexpr unsigned STABLE_KEY = 88491487;
  static constexpr unsigned CONFIG_KEY = 123456;
  explicit WiFiComponent(PreferenceStore &store) : store_(store) {}
  void setup() {
    // Model ESPHome's public lifecycle: start binds the preference according
    // to whether a station already exists, then loads its stored credentials.
    preference_key_ = has_sta() ? CONFIG_KEY : STABLE_KEY;
    const auto saved = store_.find(preference_key_);
    if (saved != store_.end()) set_sta(saved->second);
  }
  WiFiAP get_ap() const { return ap_; }
  void set_ap(const WiFiAP &ap) { ap_ = ap; }
  bool has_sta() const { return !sta_.get_ssid().empty(); }
  void set_sta(const WiFiAP &ap) { sta_ = ap; }
  void clear_sta() { sta_ = {}; }
  // Configured networks need not have a selected/scanned AP yet.
  WiFiAP get_sta() const { return selected ? sta_ : WiFiAP{}; }
  void save_wifi_sta(const char *ssid, const char *password) {
    WiFiAP ap; ap.set_ssid(ssid); ap.set_password(password);
    store_[preference_key_] = ap;
    set_sta(ap);
    ++saves;
  }
  void disable() { ++disables; }
  void enable() { ++enables; setup(); }
  bool selected{false};
  bool connected{false};
  unsigned saves{0}, disables{0}, enables{0};
 private:
  PreferenceStore &store_;
  WiFiAP sta_, ap_;
  unsigned preference_key_{0};
};
inline WiFiComponent *global_wifi_component = nullptr;
}
