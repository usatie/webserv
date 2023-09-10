#include "PostHandler.hpp"

#include "ErrorHandler.hpp"
#include "Connection.hpp"

void PostHandler::handle(Connection& conn) throw() {
  std::ofstream ofs(conn.header.fullpath.c_str(), std::ios::binary);
  if (!ofs.is_open()) {
    Log::fatal("file open failed");
    ErrorHandler::handle(*conn.client_socket, 500);
    return;
  }
  ofs.write(
      conn.body,
      conn.content_length);  // does not throw ref:
                             // https://en.cppreference.com/w/cpp/io/basic_ostream/write
  if (ofs.bad()) {
    Log::fatal("ofs.write failed");
    ErrorHandler::handle(*conn.client_socket, 500);
    return;
  }
  *conn.client_socket << "HTTP/1.1 201 Created" << CRLF;
  *conn.client_socket << "Location: " << conn.header.path << CRLF;
  *conn.client_socket << "Content-Type: application/json" << CRLF;
  *conn.client_socket << "Content-Length: 18" << CRLF;
  *conn.client_socket << CRLF;
  *conn.client_socket << "{\"success\":\"true\"}";
  *conn.client_socket << CRLF;
}
