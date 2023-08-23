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
  static void handle(std::shared_ptr<SocketBuf> client_socket,
                     const Header& header) throw() {
    // TODO: Write response headers
    std::stringstream ss;
    ssize_t content_length;

    if ((content_length = get_content_length(header.path)) < 0) {
      Log::error("get_content_length() failed");
      ss << "HTTP/1.1 404 Resource Not Found\r\n";
      // TODO send some error message
      ss << "\r\n";
      client_socket->send(ss.str().c_str(), ss.str().size());
      return;
    }
    ss << "HTTP/1.1 200 OK\r\n";
    ss << "Server: " << SERVER_NAME << "\r\n";
    // ss << "Date: Tue, 11 Jul 2023 07:36:50 GMT\r\n";
    if (header.path.find(".css") != std::string::npos)
      ss << "Content-Type: text/css\r\n";
    else if (header.path.find(".js") != std::string::npos)
      ss << "Content-Type: text/javascript\r\n";
    else if (header.path.find(".jpg") != std::string::npos)
      ss << "Content-Type: image/jpeg\r\n";
    else if (header.path.find(".png") != std::string::npos)
      ss << "Content-Type: image/png\r\n";
    else if (header.path.find(".gif") != std::string::npos)
      ss << "Content-Type: image/gif\r\n";
    else if (header.path.find(".ico") != std::string::npos)
      ss << "Content-Type: image/x-icon\r\n";
    else if (header.path.find(".html") != std::string::npos) 
      ss << "Content-Type: text/html\r\n";
    else
      ss << "Content-Type: text/plain\r\n";
    ss << "Content-Length: " << content_length << "\r\n";
    ss << "\r\n";
    client_socket->send(ss.str().c_str(), ss.str().size());
    if (client_socket->send_file(header.path) < 0) {
      Log::error("send_file() failed");
      client_socket->clear_sendbuf();
      ss.str("");
      ss << "HTTP/1.1 500 Internal Server Error\r\n";
      // TODO send some error message
      ss << "\r\n";
      client_socket->send(ss.str().c_str(), ss.str().size());
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
