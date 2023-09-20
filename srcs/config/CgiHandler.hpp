#ifndef CONFIG_CGIHANDLER_HPP
#define CONFIG_CGIHANDLER_HPP

#include <string>
#include <vector>

namespace config {
class CgiHandler {
 public:
  std::vector<std::string> extensions;
  std::string interpreter_path;
  bool configured;
  CgiHandler(const std::vector<std::string> &extensions,
             const std::string &interpreter_path)
      : extensions(extensions),
        interpreter_path(interpreter_path),
        configured(true) {}
  CgiHandler() : extensions(), interpreter_path(), configured(false) {}
};
}  // namespace config
#endif
