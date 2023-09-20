#ifndef CONFIG_CLIENTMAXBODYSIZE_HPP
#define CONFIG_CLIENTMAXBODYSIZE_HPP

#define MB (1024 * 1024)

namespace config {
class ClientMaxBodySize {
 private:
  size_t size;

 public:
  bool configured;
  explicit ClientMaxBodySize(const size_t &size)
      : size(size), configured(true) {}
  ClientMaxBodySize() : size(1 * MB), configured(false) {}
  void set(const size_t &size) { this->size = size; }
  operator size_t() const { return size; }
};
}  // namespace config

#endif
