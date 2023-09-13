#include "Connection.hpp"

void send_regular_file(Connection& conn, const std::string& path,
                       size_t content_length) throw() {
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
  *conn.client_socket << "Connection: close" << CRLF;
  *conn.client_socket << "Content-Length: " << content_length << CRLF;
  *conn.client_socket << CRLF;  // end of header
  if (conn.client_socket->send_file(path) < 0) {
    Log::error("send_file() failed");
    ErrorHandler::handle(conn, 500);
    return;
  }
}
