#include "util.hpp"

bool util::string::ends_with(const std::string& str,
                             const std::string& suffix) {
  return str.size() >= suffix.size() &&
         str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// Returns extension with dot
// This is kind of a constructor, so it is throwable
std::string util::path::get_extension(const std::string& filepath) {
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

bool util::http::is_token(const std::string& str) {
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
