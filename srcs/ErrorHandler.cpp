#include "ErrorHandler.hpp"
#include "http_special_response.hpp"
#include "webserv.hpp"
#include "Connection.hpp"
#include <sstream>

void ErrorHandler::handle(Connection& conn, int status_code) throw() {
  conn.client_socket->clear_sendbuf();
  std::vector<Config::ErrorPage>::const_iterator it, end;
  if (conn.loc_cf) {
    it = conn.loc_cf->error_pages.begin();
    end = conn.loc_cf->error_pages.end();
  } else {
    it = conn.srv_cf->error_pages.begin();
    end = conn.srv_cf->error_pages.end();
  }
  for (; it != end; ++it) {
    Log::cdebug() << "Error page: " << it->uri << std::endl;
    if (std::find(it->codes.begin(), it->codes.end(), status_code) != it->codes.end()) {
      std::string error_page = it->uri;
      // TODO: local redirect
      Log::cdebug() << "Error page found: " << error_page << std::endl;
    }
  }
  switch (status_code) {
    case 400:
      *conn.client_socket << "HTTP/1.1 400 Bad Request" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: " << sizeof(http_error_400_page) - 1
                    << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_400_page;
      break;
    case 403:
      *conn.client_socket << "HTTP/1.1 403 Forbidden" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: " << sizeof(http_error_400_page) - 1 // -1 because of the null terminator
                    << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_403_page;
      break;
    case 404:
      *conn.client_socket << "HTTP/1.1 404 Resource Not Found" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: " << sizeof(http_error_404_page) - 1
                    << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_404_page;
      break;
    case 405:
      *conn.client_socket << "HTTP/1.1 405 Method Not Allowed" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: " << sizeof(http_error_405_page) - 1
                    << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_405_page;
      break;
    case 500:
      *conn.client_socket << "HTTP/1.1 500 Internal Server Error" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: " << sizeof(http_error_500_page) - 1
                    << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_500_page;
      break;
    default:
      Log::cerror() << "Unknown status code: " << status_code << std::endl;
      break;
  }
}
