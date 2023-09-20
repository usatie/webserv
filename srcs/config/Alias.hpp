#ifndef CONFIG_ALIAS_HPP
#define CONFIG_ALIAS_HPP

#include <string>

namespace config {
class Alias : public std::string {
 public:
  bool configured;
  explicit Alias(const std::string &path)
      : std::string(path), configured(true) {}
  Alias() : std::string(), configured(false) {}
};
}  // namespace config
#endif
