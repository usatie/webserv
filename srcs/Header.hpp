#ifndef HEADER_HPP
#define HEADER_HPP
#include <string>

#include "Socket.hpp"
#include "webserv.hpp"

class Header {
 public:
  // TODO: deprecate
  explicit Header(Socket &client_socket) {
    std::string line;
    while (client_socket.readline(line) < 0) {
      client_socket.recv();
    }
    std::vector<std::string> keywords = split(line, ' ');
    // TODO: validate keywords
    method = keywords[0];
    path = keywords[1];
    version = keywords[2];
    // TODO: parse header fields
  }
  Header() {}
  std::string method;
  std::string path;
  std::string version;

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
