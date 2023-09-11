#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>
#include <vector>
#include <algorithm>

namespace util {
  namespace string {
    bool ends_with(const std::string& str, const std::string& suffix);
  }
  namespace vector {
    template <typename T>
    bool contains(const std::vector<T>& vec, const T& str) {
      return std::find(vec.begin(), vec.end(), str) != vec.end();
    }
  }
  namespace http {
    bool is_tchar(const char c);
    bool is_token(const std::string& str);
  }
}

#endif
