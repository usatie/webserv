#include "RedirectHandler.hpp"

#include "Connection.hpp"

void RedirectHandler::handle(const Connection& conn, int status_code,
                             const std::string& location) throw() {
  *conn.client_socket << "HTTP/1.1 " << status_code << " Moved Permanently"
                      << CRLF;
  *conn.client_socket << "Location: " << location << CRLF;
  *conn.client_socket << CRLF;
}
