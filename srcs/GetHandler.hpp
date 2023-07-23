#ifndef GET_HANDLER_HPP
# define GET_HANDLER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

# include "Socket.hpp"

#include <fstream>
#include <sys/stat.h>
ssize_t get_content_length(std::string filepath) {
  struct stat st;
  if (stat(filepath.c_str(), &st) < 0) {
    std::cerr << "stat() failed\n";
    return -1;
  }
  return st.st_size;
}

class GetHandler {
public:
  static void handle(Socket& client_socket, const Header &header) {
    // TODO: Write response headers
    std::stringstream ss;
    ssize_t content_length = get_content_length(header.path);
    if (content_length < 0) {
      std::cerr << "get_content_length() failed\n";
      return;
    }
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "Server: webserv/0.1\r\n";
    //ss << "Date: Tue, 11 Jul 2023 07:36:50 GMT\r\n";
    ss << "Content-Length: " << content_length << "\r\n";
    ss << "\r\n";
    client_socket.send(ss.str().c_str(), ss.str().size());
    client_socket.send_file(header.path);
    // GET /a.text HTTP/1.1
    // GET /b.text HTTP/1.1
    // GET /b.text HTTP/1.1
    // if (client_socket.send(RESPONSE.c_str(), RESPONSE.size()) < 0) {
    //   std::cerr << "send() failed\n";
    // }
  }
};
#endif
