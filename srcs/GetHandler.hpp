#ifndef GET_HANDLER_HPP
#define GET_HANDLER_HPP

#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Header.hpp"
#include "Socket.hpp"
#include "Connection.hpp"
#include "webserv.hpp"
class GetHandler {
 public:
  // Member data
  // Constructor/Destructor
  // Member functions
  static void handle(std::shared_ptr<Socket> client_socket, const Header& header) {
    // TODO: Write response headers
    std::stringstream ss;
    ssize_t content_length = get_content_length(header.path);
    if (content_length < 0) {
      std::cerr << "get_content_length() failed\n";
      return;
    }
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "Server: " << SERVER_NAME << "\r\n";
    // ss << "Date: Tue, 11 Jul 2023 07:36:50 GMT\r\n";
    ss << "Content-Length: " << content_length << "\r\n";
    ss << "\r\n";
    client_socket->send(ss.str().c_str(), ss.str().size());
    client_socket->send_file(header.path);
  }

 private:
  static ssize_t get_content_length(const std::string& filepath) {
    struct stat st;
    if (stat(filepath.c_str(), &st) < 0) {
      std::cerr << "stat() failed\n";
      return -1;
    }
    return st.st_size;
  }
};
#endif
