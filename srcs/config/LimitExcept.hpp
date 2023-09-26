#ifndef CONFIG_LIMITEXCEPT_HPP
#define CONFIG_LIMITEXCEPT_HPP

#include <string>
#include <vector>

namespace config {
class LimitExcept {
 public:
  std::vector<std::string> methods;
  bool configured;
  explicit LimitExcept(const std::vector<std::string> &methods)
      : methods(methods), configured(true) {}
  LimitExcept() : methods(), configured(false) {}
};
}  // namespace config

#endif
