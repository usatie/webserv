#ifndef CONFIG_REDIRECTRETURN_HPP
#define CONFIG_REDIRECTRETURN_HPP

#include <string>

namespace config {
class RedirectReturn {
 public:
  int code;
  std::string url;
  bool configured;
  RedirectReturn(const int code, const std::string &url)
      : code(code), url(url), configured(true) {}
  RedirectReturn() : code(), url(""), configured(false) {}
};
}  // namespace config

#endif
