#ifndef HEADER_HPP
# define HEADER_HPP
# include "Socket.hpp"
# include <string>

class Header {
public:
  Header(Socket &client_socket) {
    std::string line;
    client_socket.readline(line);
    std::vector<std::string> keywords = split(line, ' ');
    // TODO: validate keywords
    method = keywords[0];
    path = keywords[1];
    version = keywords[2];
  }
  std::string method;
  std::string path;
  std::string version;

  std::vector<std::string> split(std::string str, char delim) {
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
