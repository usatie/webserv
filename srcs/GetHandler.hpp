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
#include "http_special_response.hpp"
#include "webserv.hpp"
#include "ErrorHandler.hpp"

class GetHandler {
 private:
  // Constructor/Destructor/Assignment Operator
  GetHandler() throw();                              // Do not implement this
  GetHandler(const GetHandler&) throw();             // Do not implement this
  GetHandler& operator=(const GetHandler&) throw();  // Do not implement this
  ~GetHandler() throw();                             // Do not implement this
 public:
  // Member functions
  static void handle(std::shared_ptr<SocketBuf> client_socket,
                     const Header& header) throw() {
    // TODO: Write response headers
    std::stringstream ss;
    ssize_t content_length;

    if ((content_length = get_content_length(header.path)) < 0) {
      Log::error("get_content_length() failed");
      ErrorHandler::handle(client_socket, 404);
      return;
    }
    ss << "HTTP/1.1 200 OK" << CRLF;
    ss << "Server: " << WEBSERV_VER << CRLF;
    // ss << "Date: Tue, 11 Jul 2023 07:36:50 GMT" << CRLF;
    if (util::string::ends_with(header.path, ".css"))
      ss << "Content-Type: text/css" << CRLF;
    else if (util::string::ends_with(header.path, ".js"))
      ss << "Content-Type: text/javascript" << CRLF;
    else if (util::string::ends_with(header.path, ".jpg"))
      ss << "Content-Type: image/jpeg" << CRLF;
    else if (util::string::ends_with(header.path, ".png"))
      ss << "Content-Type: image/png" << CRLF;
    else if (util::string::ends_with(header.path, ".gif"))
      ss << "Content-Type: image/gif" << CRLF;
    else if (util::string::ends_with(header.path, ".ico"))
      ss << "Content-Type: image/x-icon" << CRLF;
    else if (util::string::ends_with(header.path, ".svg"))
      ss << "Content-Type: image/svg+xml" << CRLF;
    else if (util::string::ends_with(header.path, ".html"))
      ss << "Content-Type: text/html" << CRLF;
    else
      ss << "Content-Type: text/plain" << CRLF;
    ss << "Content-Length: " << content_length << CRLF;
    ss << CRLF; // end of header
    client_socket->send(ss.str().c_str(), ss.str().size());
    if (client_socket->send_file(header.path) < 0) {
      Log::error("send_file() failed");
      ErrorHandler::handle(client_socket, 500);
      return;
    }
  }

 private:
  static ssize_t get_content_length(const std::string& filepath) throw() {
    struct stat st;
    if (stat(filepath.c_str(), &st) < 0) {
      Log::cerror() << "stat() failed: " << filepath << ", errno:" << strerror(errno) << std::endl;
      std::cerr << "stat() failed\n";
      return -1;
    }
    return st.st_size;
  }
};
#endif
