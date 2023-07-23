#ifndef DEFAULT_HANDLER_HPP
# define DEFAULT_HANDLER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

# include "Socket.hpp"
const std::string RESPONSE =
"HTTP/1.1 200 OK\r\n"
"Server: nginx/1.25.1\r\n"
"Date: Tue, 11 Jul 2023 07:36:50 GMT\r\n"
"Content-Type: text/html\r\n"
"Content-Length: 615\r\n"
"Last-Modified: Tue, 13 Jun 2023 15:08:10 GMT\r\n"
"Connection: keep-alive\r\n"
"ETag: \"6488865a-267\"\r\n"
"Accept-Ranges: bytes\r\n"
"\r\n"
"<!DOCTYPE html>\n"
"<html>\n"
"<head>\n"
"<title>Welcome to nginx!</title>\n"
"<style>\n"
"html { color-scheme: light dark; }\n"
"body { width: 35em; margin: 0 auto;\n"
"font-family: Tahoma, Verdana, Arial, sans-serif; }\n"
"</style>\n"
"</head>\n"
"<body>\n"
"<h1>Welcome to nginx!</h1>\n"
"<p>If you see this page, the nginx web server is successfully installed and\n"
"working. Further configuration is required.</p>\n"
"\n"
"<p>For online documentation and support please refer to\n"
"<a href=\"http://nginx.org/\">nginx.org</a>.<br/>\n"
"Commercial support is available at\n"
"<a href=\"http://nginx.com/\">nginx.com</a>.</p>\n"
"\n"
"<p><em>Thank you for using nginx.</em></p>\n"
"</body>\n"
"</html>\n";

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

#include <fstream>
int Socket::send_file(std::string filepath) {
  std::ifstream ifs(filepath);
  std::size_t sent = 0;
  ssize_t ret = 0;
  std::string line;

  if (!ifs.is_open()) {
    std::cerr << "file open failed\n";
    return -1;
  }
  while (std::getline(ifs, line)) {
    // TODO: portable newline
    line += "\n";
    ret = send(line.c_str(), line.size());
    if (ret < 0) {
      std::cerr << "send() failed\n";
      return -1;
    }
    // TODO: handle partial send
    sent += ret;
  }
  ret = send("\r\n", 1);
  if (ret < 0) {
    std::cerr << "send() failed\n";
    return -1;
  }
  // TODO: handle partial send
  return sent + ret;
}

#include <sys/stat.h>
ssize_t get_content_length(std::string filepath) {
  struct stat st;
  if (stat(filepath.c_str(), &st) < 0) {
    std::cerr << "stat() failed\n";
    return -1;
  }
  return st.st_size;
}

class DefaultHandler {
public:
  static void handle(Socket& client_socket) {
    Header header(client_socket);
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
