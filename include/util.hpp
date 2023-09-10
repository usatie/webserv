#ifndef UTIL_HPP
#define UTIL_HPP

#include <string>

namespace util {
  namespace string {
    bool ends_with(const std::string& str, const std::string& suffix);
  }
  namespace http {
    bool is_tchar(const char c);
    bool is_token(const std::string& str);
  }
}

#endif
