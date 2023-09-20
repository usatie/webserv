#ifndef CONFIG_INDEX_HPP
#define CONFIG_INDEX_HPP

#include <string>
#include <vector>

namespace config {
class Index : public std::vector<std::string> {
 public:
  bool configured;
  explicit Index(const std::vector<std::string> &index_files)
      : std::vector<std::string>(index_files), configured(true) {}
  Index() : std::vector<std::string>(), configured(false) {
    push_back("index.html");
  }
};
}  // namespace config

#endif
