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

// pchar = unreserved / pct-encoded / sub-delims / ":" / "@"
// cppcheck-suppress unusedFunction
bool util::http::is_pchar(const std::string &str, size_t pos) {
  if (str.size() <= pos) {
    return false;
  }
  char c = str[pos];
  return is_unreserved(c) || is_pct_encoded(str, pos) || is_sub_delims(c) ||
         c == ':' || c == '@';
}

// unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
bool util::http::is_unreserved(const char c) {
  return std::isalpha(c) || std::isdigit(c) || c == '-' || c == '.' ||
         c == '_' || c == '~';
}

// sub-delims = "!" / "$" / "&" / "'" / "(" / ")"
//            / "*" / "+" / "," / ";" / "="
bool util::http::is_sub_delims(const char c) {
  return c == '!' || c == '$' || c == '&' || c == '\'' || c == '(' ||
         c == ')' || c == '*' || c == '+' || c == ',' || c == ';' || c == '=';
}

// pct-encoded = "%" HEXDIG HEXDIG
bool util::http::is_pct_encoded(const std::string &str, size_t pos) {
  if (str.size() < pos + 3) {
    return false;
  }
  return str[pos] == '%' && is_HEXDIG(str[pos + 1]) && is_HEXDIG(str[pos + 2]);
}

// HEXDIG = DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
//        / "a" / "b" / "c" / "d" / "e" / "f"
bool util::http::is_HEXDIG(const char c) {
  return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

// inet
static bool eq_addr(const sockaddr_in *a, const sockaddr_in *b,
                    bool allow_wildcard) {
  // If port is different, return false
  if (a->sin_port != b->sin_port) {
    return false;
  }
  // If a or b is wildcard, return true
  if (allow_wildcard) {
    if (a->sin_addr.s_addr == INADDR_ANY || b->sin_addr.s_addr == INADDR_ANY) {
      return true;
    }
  }
  // Otherwise, compare address
  // sin_addr.sin_addr is just a uint32_t, so we can compare it directly
  return a->sin_addr.s_addr == b->sin_addr.s_addr;
}

static bool eq_addr6(const sockaddr_in6 *a, const sockaddr_in6 *b,
                     bool allow_wildcard) {
  // If port is different, return false
  if (a->sin6_port != b->sin6_port) {
    return false;
  }
  // If a or b is wildcard, return true
  if (allow_wildcard) {
    if (IN6_IS_ADDR_UNSPECIFIED(&a->sin6_addr) ||
        IN6_IS_ADDR_UNSPECIFIED(&b->sin6_addr)) {
      return true;
    }
  }
  // Otherwise, compare address
  // sin6_addr.sin6_addr is just a uint8_t[16], so we can compare it by memcmp
  return memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(in6_addr)) == 0;
}

bool util::inet::eq_addr46(const sockaddr_storage *a, const sockaddr_storage *b,
                           bool allow_wildcard) {
  if (a->ss_family != b->ss_family) {
    return false;
  }
  if (a->ss_family == AF_INET) {
    return eq_addr(reinterpret_cast<const sockaddr_in *>(a),
                   reinterpret_cast<const sockaddr_in *>(b), allow_wildcard);
  } else if (a->ss_family == AF_INET6) {
    return eq_addr6(reinterpret_cast<const sockaddr_in6 *>(a),
                    reinterpret_cast<const sockaddr_in6 *>(b), allow_wildcard);
  } else {
    return false;
  }
}
