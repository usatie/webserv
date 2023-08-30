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
  static void handle(SocketBuf& client_socket,
                     const Header& header) throw() {
    // TODO: Write response headers
    ssize_t content_length;

    if ((content_length = get_content_length(header.path)) < 0) {
      Log::error("get_content_length() failed");
      ErrorHandler::handle(client_socket, 404);
      return;
    }
    client_socket << "HTTP/1.1 200 OK" << CRLF;
    client_socket << "Server: " << WEBSERV_VER << CRLF;
    // client_socket << "Date: Tue, 11 Jul 2023 07:36:50 GMT" << CRLF;
    if (util::string::ends_with(header.path, ".css"))
      client_socket << "Content-Type: text/css" << CRLF;
    else if (util::string::ends_with(header.path, ".js"))
      client_socket << "Content-Type: text/javascript" << CRLF;
    else if (util::string::ends_with(header.path, ".jpg"))
      client_socket << "Content-Type: image/jpeg" << CRLF;
    else if (util::string::ends_with(header.path, ".png"))
      client_socket << "Content-Type: image/png" << CRLF;
    else if (util::string::ends_with(header.path, ".gif"))
      client_socket << "Content-Type: image/gif" << CRLF;
    else if (util::string::ends_with(header.path, ".ico"))
      client_socket << "Content-Type: image/x-icon" << CRLF;
    else if (util::string::ends_with(header.path, ".svg"))
      client_socket << "Content-Type: image/svg+xml" << CRLF;
    else if (util::string::ends_with(header.path, ".html"))
      client_socket << "Content-Type: text/html" << CRLF;
    else
      client_socket << "Content-Type: text/plain" << CRLF;
    client_socket << "Content-Length: " << content_length << CRLF;
    client_socket << CRLF; // end of header
    if (client_socket.send_file(header.path) < 0) {
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
