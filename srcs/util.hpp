#ifndef UTIL_HPP
#define UTIL_HPP

#include <algorithm>
#include <string>
#include <vector>

struct sockaddr_storage;
namespace util {
template <typename U>
class Deleter;

template <typename T, class D>
class shared_ptr;

namespace string {
bool ends_with(const std::string& str, const std::string& suffix);
}
namespace path {
std::string get_extension(const std::string& filepath);
}

namespace vector {
template <typename T>
bool contains(const std::vector<T>& vec, const T& str) {
  return std::find(vec.begin(), vec.end(), str) != vec.end();
}
}  // namespace vector
namespace http {
bool is_tchar(const char c);
bool is_pchar(const std::string& str, size_t pos);
bool is_unreserved(const char c);
bool is_sub_delims(const char c);
bool is_HEXDIG(const char c);
bool is_pct_encoded(const std::string& str, size_t pos);
bool is_token(const std::string& str);
}  // namespace http
namespace inet {
bool eq_addr46(const sockaddr_storage* a, const sockaddr_storage* b,
               bool allow_wildcard);
}  // namespace inet
}  // namespace util

#include "util.tpp"

#endif
