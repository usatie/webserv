#include "GetHandler.hpp"

#include <sys/stat.h>

#include <cstring>  // strerror

#include "Connection.hpp"
#include "ErrorHandler.hpp"
#include "webserv.hpp"

bool validate(const char* path, size_t& content_length) {
  struct stat st;
  if (stat(path, &st) < 0) {
    Log::cdebug() << "stat() failed: " << path << ", errno:" << strerror(errno)
                  << std::endl;
    return false;
  }
  if (S_ISDIR(st.st_mode)) {
    Log::cdebug() << "is a directory: " << path << std::endl;
    return false;
  }
  if (!S_ISREG(st.st_mode)) {
    Log::cdebug() << "is not a regular file: " << path << std::endl;
    return false;
  }
  content_length = st.st_size;
  return true;
}

void GetHandler::handle(Connection& conn) throw() {
  // TODO: Write response headers
  std::string fullpath = conn.header.fullpath;
  size_t content_length = 0;
  bool is_valid = false;
  // Index
  if (fullpath.back() == '/') {  // Append index.html
    const std::vector<std::string>& index =
        (!conn.loc_cf) ? conn.srv_cf->index : conn.loc_cf->index;
    for (size_t i = 0; i < index.size(); ++i) {
      std::string path = fullpath + index[i];
      Log::cdebug() << "Trying index: " << path << std::endl;
      is_valid = validate(path.c_str(), content_length);
      if (is_valid) {
        fullpath = path;
        break;
      }
    }
  } else {
    is_valid = validate(fullpath.c_str(), content_length);
  }
  Log::cdebug() << "fullpath: " << fullpath << std::endl;
  if (!is_valid) {
    ErrorHandler::handle(conn, 404);
    return;
  }

  *conn.client_socket << "HTTP/1.1 200 OK" << CRLF;
  *conn.client_socket << "Server: " << WEBSERV_VER << CRLF;
  // client_socket << "Date: Tue, 11 Jul 2023 07:36:50 GMT" << CRLF;
  if (util::string::ends_with(conn.header.path, ".css"))
    *conn.client_socket << "Content-Type: text/css" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".js"))
    *conn.client_socket << "Content-Type: text/javascript" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".jpg"))
    *conn.client_socket << "Content-Type: image/jpeg" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".png"))
    *conn.client_socket << "Content-Type: image/png" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".gif"))
    *conn.client_socket << "Content-Type: image/gif" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".ico"))
    *conn.client_socket << "Content-Type: image/x-icon" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".svg"))
    *conn.client_socket << "Content-Type: image/svg+xml" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".html"))
    *conn.client_socket << "Content-Type: text/html" << CRLF;
  else
    *conn.client_socket << "Content-Type: text/plain" << CRLF;
  *conn.client_socket << "Content-Length: " << content_length << CRLF;
  *conn.client_socket << CRLF;  // end of header
  if (conn.client_socket->send_file(fullpath) < 0) {
    Log::error("send_file() failed");
    ErrorHandler::handle(conn, 500);
    return;
  }
}
