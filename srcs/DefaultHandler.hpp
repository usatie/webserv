#ifndef DEFAULT_HANDLER_HPP
# define DEFAULT_HANDLER_HPP

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

class DefaultHandler {
public:
  static void handle(Socket& client_socket) {
    if (client_socket.send(RESPONSE.c_str(), RESPONSE.size()) < 0) {
      std::cerr << "send() failed\n";
    }
  }
};
#endif