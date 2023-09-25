#include "util.hpp"

#include <arpa/inet.h>  // struct sockaddr_in, struct sockaddr_in6

#include <cstring>  // memcmp

bool util::string::ends_with(const std::string &str,
                             const std::string &suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Returns extension with dot
// This is kind of a constructor, so it is throwable
std::string util::path::get_extension(const std::string &filepath) {
  size_t pos = filepath.rfind('.');
  if (pos == std::string::npos) {
    return "";
  }
  return filepath.substr(pos);  // throwable
}

// https://tools.ietf.org/html/rfc7230#section-3.2.6
/*
 token          = 1*tchar

 tchar          = "!" / "#" / "$" / "%" / "&" / "'" / "*"
                / "+" / "-" / "." / "^" / "_" / "`" / "|" / "~"
                / DIGIT / ALPHA
                ; any VCHAR, except delimiters
*/
bool util::http::is_tchar(const char c) {
  return isalpha(c) || isdigit(c) || c == '!' || c == '#' || c == '$' ||
         c == '%' || c == '&' || c == '\'' || c == '*' || c == '+' ||
         c == '-' || c == '.' || c == '^' || c == '_' || c == '`' || c == '|' ||
         c == '~';
}

bool util::http::is_token(const std::string &str) {
  if (str.empty()) {
    return false;
  }
  for (size_t i = 0; i < str.size(); ++i) {
    if (!is_tchar(str[i])) {
      return false;
    }
  }
  return true;
}

// inet
static bool eq_addr(const sockaddr_in *a, const sockaddr_in *b) {
  // If port is different, return false
  if (a->sin_port != b->sin_port) {
    return false;
  }
  // If a or b is wildcard, return true
  if (a->sin_addr.s_addr == INADDR_ANY || b->sin_addr.s_addr == INADDR_ANY) {
    return true;
  }
  // Otherwise, compare address
  // sin_addr.sin_addr is just a uint32_t, so we can compare it directly
  return a->sin_addr.s_addr == b->sin_addr.s_addr;
}

static bool eq_addr6(const sockaddr_in6 *a, const sockaddr_in6 *b) {
  // If port is different, return false
  if (a->sin6_port != b->sin6_port) {
    return false;
  }
  // If a or b is wildcard, return true
  if (IN6_IS_ADDR_UNSPECIFIED(&a->sin6_addr) ||
      IN6_IS_ADDR_UNSPECIFIED(&b->sin6_addr)) {
    return true;
  }
  // Otherwise, compare address
  // sin6_addr.sin6_addr is just a uint8_t[16], so we can compare it by memcmp
  return memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(in6_addr)) == 0;
}

bool util::inet::eq_addr46(const sockaddr_storage *a,
                           const sockaddr_storage *b) {
  if (a->ss_family != b->ss_family) {
    return false;
  }
  if (a->ss_family == AF_INET) {
    return eq_addr(reinterpret_cast<const sockaddr_in *>(a),
                   reinterpret_cast<const sockaddr_in *>(b));
  } else if (a->ss_family == AF_INET6) {
    return eq_addr6(reinterpret_cast<const sockaddr_in6 *>(a),
                    reinterpret_cast<const sockaddr_in6 *>(b));
  } else {
    return false;
  }
}
