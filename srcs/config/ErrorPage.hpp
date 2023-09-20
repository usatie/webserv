#ifndef CONFIG_ERRORPAGE_HPP
#define CONFIG_ERRORPAGE_HPP

#include <string>
#include <vector>

namespace config {
class ErrorPage {
 public:
  std::vector<int> codes;
  std::string uri;
  bool configured;
  ErrorPage(const std::vector<int> &codes, const std::string &uri)
      : codes(codes), uri(uri), configured(true) {}
  ErrorPage() : codes(), uri(""), configured(false) {}
};
}  // namespace config
#endif
