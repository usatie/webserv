#ifndef UTIL_HPP
#define UTIL_HPP

#include <algorithm>
#include <string>
#include <vector>

namespace util {
  template <typename T>
  class shared_ptr {
    private:
      std::shared_ptr<T> ptr;
    public:
      shared_ptr(): ptr() {}
      shared_ptr(T* ptr): ptr(ptr) {}
      shared_ptr(const shared_ptr<T>& other): ptr(other.ptr) {}
      shared_ptr<T>& operator=(const shared_ptr<T>& other) {
        ptr = other.ptr;
        return *this;
      }
      T* operator->() const {
        return ptr.get();
      }
      T& operator*() const {
        return *ptr;
      }
      bool operator==(const shared_ptr<T> other) const {
        return ptr == other.ptr;
      }
      bool operator==(nullptr_t other) const {
        return ptr.get() == other;
      }
      bool operator!=(const shared_ptr<T> other) const {
        return ptr != other.ptr;
      }
      bool operator!=(nullptr_t other) const {
        return ptr.get() != other;
      }
  };

namespace string {
bool ends_with(const std::string& str, const std::string& suffix);
}
namespace path {
std::string get_extension(const std::string& str);
}

namespace vector {
template <typename T>
bool contains(const std::vector<T>& vec, const T& str) {
  return std::find(vec.begin(), vec.end(), str) != vec.end();
}
}  // namespace vector
namespace http {
bool is_tchar(const char c);
bool is_token(const std::string& str);
}  // namespace http
}  // namespace util

#endif
