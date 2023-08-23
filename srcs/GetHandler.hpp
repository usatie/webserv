#ifndef GET_HANDLER_HPP
#define GET_HANDLER_HPP

#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Header.hpp"
#include "SocketBuf.hpp"
#include "webserv.hpp"

class GetHandler {
 private:
  // Constructor/Destructor/Assignment Operator
  GetHandler() throw();                              // Do not implement this
  GetHandler(const GetHandler&) throw();             // Do not implement this
  GetHandler& operator=(const GetHandler&) throw();  // Do not implement this
  ~GetHandler() throw();                             // Do not implement this
 public:
  // Member functions
  // TODO: Make this noexcept
  static void handle(std::shared_ptr<SocketBuf> client_socket,
                     const Header& header) throw() {
    // TODO: Write response headers
    std::stringstream ss;
    ssize_t content_length = get_content_length(header.path);
    if (content_length < 0) {
      ss << "HTTP/1.1 404 Resource Not Found\r\n";
      // TODO send some error message
      ss << "\r\n";
      client_socket->send(ss.str().c_str(), ss.str().size());
      Log::error("get_content_length() failed");
      return;
    }
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "Server: " << SERVER_NAME << "\r\n";
    // ss << "Date: Tue, 11 Jul 2023 07:36:50 GMT\r\n";
    ss << "Content-Length: " << content_length << "\r\n";
    ss << "\r\n";
    client_socket->send(ss.str().c_str(), ss.str().size());
    if (client_socket->send_file(header.path) < 0) {
      client_socket->clear_sendbuf();
      ss.str("");
      ss << "HTTP/1.1 500 Internal Server Error\r\n";
      // TODO send some error message
      ss << "\r\n";
      client_socket->send(ss.str().c_str(), ss.str().size());
      Log::error("send_file() failed");
      return;
    }
  }

 private:
  static ssize_t get_content_length(const std::string& filepath) throw() {
    struct stat st;
    if (stat(filepath.c_str(), &st) < 0) {
      std::cerr << "stat() failed\n";
      return -1;
    }
    return st.st_size;
  }
};
#endif
