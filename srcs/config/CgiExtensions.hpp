#ifndef CONFIG_CGIEXTENSIONS_HPP
#define CONFIG_CGIEXTENSIONS_HPP

#include <string>
#include <vector>

namespace config {
class CgiExtensions : public std::vector<std::string> {
 public:
  bool configured;
  explicit CgiExtensions(const std::vector<std::string> &extensions)
      : std::vector<std::string>(extensions), configured(true) {}
  CgiExtensions() : std::vector<std::string>(), configured(false) {}
};
}  // namespace config
#endif
