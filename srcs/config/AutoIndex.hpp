#ifndef CONFIG_AUTOINDEX_HPP
#define CONFIG_AUTOINDEX_HPP

namespace config {
class AutoIndex {
 private:
  bool value;

 public:
  bool configured;
  explicit AutoIndex(const bool &value) : value(value), configured(true) {}
  AutoIndex() : value(false), configured(false) {}
  void set(const bool &value) { this->value = value; }
  operator bool() const { return value; }
};
}  // namespace config

#endif
