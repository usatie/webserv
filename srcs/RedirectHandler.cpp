#include "RedirectHandler.hpp"

#include "Connection.hpp"

void RedirectHandler::handle(Connection& conn, int status_code,
                             const std::string& location) throw() {
  conn.res.status_code = status_code;
  conn.res.location = location;
  conn.res.content_length = 0;
  conn.client_socket->send_response(conn.res);
}
