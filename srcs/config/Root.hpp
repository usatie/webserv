#ifndef CONFIG_ROOT_HPP
#define CONFIG_ROOT_HPP

#include <string>

namespace config {
class Root : public std::string {
 public:
  bool configured;
  explicit Root(const std::string &path)
      : std::string(path), configured(true) {}
  Root() : std::string("html"), configured(false) {}
};
}  // namespace config

#endif
