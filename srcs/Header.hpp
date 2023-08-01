#ifndef HEADER_HPP
#define HEADER_HPP
#include <string>

#include "Socket.hpp"
#include "webserv.hpp"

class Header {
 public:
  // Member data
  std::string method;
  std::string path;
  std::string version;
  // Constructor/Destructor
  Header() {}
  ~Header() {}

  // Other member functions
  // TODO: refactor? fix?
  static std::vector<std::string> split(std::string str, char delim) {
    std::vector<std::string> ret;
    int idx = 0;
    while (str[idx]) {
      std::string line;
      while (str[idx] && str[idx] != delim) {
        line += str[idx];
        idx++;
      }
      while (str[idx] && str[idx] == delim) {
        idx++;
      }
      if (!line.empty()) {
        ret.push_back(line);
      }
    }
    return ret;
  }
};

#endif
