#pragma once
#include <string>
namespace onx_config {
inline bool valid_entity(const std::string &id) {
  const auto dot = id.find('.');
  if (id.size() > 128 || dot == std::string::npos || dot == 0 || dot + 1 == id.size() || id.find('.', dot + 1) != std::string::npos) return false;
  for (char ch : id) if (!(ch == '.' || ch == '_' || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'))) return false;
  return true;
}
inline bool valid_action(const std::string &id) {
  return id.empty() || id == "none" || (valid_entity(id) && (id.rfind("script.", 0) == 0 || id.rfind("automation.", 0) == 0));
}
inline bool binding_matches(const std::string &bound, const std::string &current) {
  return !current.empty() && bound == current;
}
}  // namespace onx_config
