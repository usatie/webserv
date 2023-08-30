#ifndef ERROR_HANDLER_HPP
#define ERROR_HANDLER_HPP

#include <sstream>

#include "SocketBuf.hpp"
#include "http_special_response.hpp"
#include "webserv.hpp"

class ErrorHandler {
public:
  static void handle(SocketBuf& client_socket, int status_code) throw() {
  client_socket.clear_sendbuf();
  switch (status_code) {
    case 400:
      client_socket << "temp";
      client_socket << "HTTP/1.1 400 Bad Request" << CRLF;
      client_socket << "Content-Type: text/html" << CRLF;
      client_socket << "Content-Length: " << sizeof(http_error_400_page) << CRLF;
      client_socket << CRLF; // end of header
      client_socket << http_error_400_page;
      client_socket << CRLF; // end of body
      break;
    case 404:
      client_socket << "HTTP/1.1 404 Resource Not Found" << CRLF;
      client_socket << "Content-Type: text/html" << CRLF;
      client_socket << "Content-Length: " << sizeof(http_error_404_page) << CRLF;
      client_socket << CRLF; // end of header
      client_socket << http_error_404_page;
      client_socket << CRLF; // end of body
      break;
    case 405:
      client_socket << "HTTP/1.1 405 Method Not Allowed" << CRLF;
      client_socket << "Content-Type: text/html" << CRLF;
      client_socket << "Content-Length: " << sizeof(http_error_405_page) << CRLF;
      client_socket << CRLF; // end of header
      client_socket << http_error_405_page;
      client_socket << CRLF; // end of body
      break;
    case 500:
      client_socket << "HTTP/1.1 500 Internal Server Error" << CRLF;
      client_socket << "Content-Type: text/html" << CRLF;
      client_socket << "Content-Length: " << sizeof(http_error_500_page) << CRLF;
      client_socket << CRLF; // end of header
      client_socket << http_error_500_page;
      client_socket << CRLF; // end of body
      break;
    default:
      Log::cerror() << "Unknown status code: " << status_code << std::endl;
      break;
  }
  }
};
#endif
