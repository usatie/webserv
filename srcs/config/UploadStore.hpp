#ifndef CONFIG_UPLOADSTORE_HPP
#define CONFIG_UPLOADSTORE_HPP

#include <string>

namespace config {
class UploadStore : public std::string {
 public:
  bool configured;
  explicit UploadStore(const std::string &path)
      : std::string(path), configured(true) {}
  UploadStore() : std::string("upload"), configured(false) {}
  void set(const std::string &path) { this->assign(path); }
};
}  // namespace config

#endif
