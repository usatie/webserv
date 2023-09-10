#include "GetHandler.hpp"

#include <sys/stat.h>
#include <cstring> // strerror
#include "ErrorHandler.hpp"
#include "webserv.hpp"

void GetHandler::handle(SocketBuf& client_socket, const Header& header) throw() {
  // TODO: Write response headers
  struct stat st;
  if (stat(header.fullpath.c_str(), &st) < 0) {
    Log::cerror() << "stat() failed: " << header.fullpath
                  << ", errno:" << strerror(errno) << std::endl;
    ErrorHandler::handle(client_socket, 404);
    return;
  }
  // Check if directory
  if (S_ISDIR(st.st_mode)) {
    // TODO: Check if index.html exists
    // TODO: directory listing
    Log::cdebug() << "is a directory: " << header.fullpath << std::endl;
    ErrorHandler::handle(client_socket, 404);
    return;
  }
  // Check if regular file
  if (!S_ISREG(st.st_mode)) {
    Log::cdebug() << "not a regular file: " << header.fullpath << std::endl;
    ErrorHandler::handle(client_socket, 404);
    return;
  }
  size_t content_length = st.st_size;

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
  client_socket << CRLF;  // end of header
  if (client_socket.send_file(header.fullpath) < 0) {
    Log::error("send_file() failed");
    ErrorHandler::handle(client_socket, 500);
    return;
  }
}
