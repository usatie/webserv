#ifndef ERROR_HANDLER_HPP
#define ERROR_HANDLER_HPP

#include <sstream>

#include "SocketBuf.hpp"
#include "http_special_response.hpp"
#include "webserv.hpp"

class ErrorHandler {
public:
  static void handle(std::shared_ptr<SocketBuf> client_socket, int status_code) throw() {
  client_socket->clear_sendbuf();
  std::stringstream ss;
  switch (status_code) {
    case 400:
      ss << "HTTP/1.1 400 Bad Request" << CRLF;
      ss << "Content-Type: text/html" << CRLF;
      ss << "Content-Length: " << sizeof(http_error_400_page) << CRLF;
      ss << CRLF; // end of header
      ss << http_error_400_page;
      ss << CRLF; // end of body
      break;
    case 404:
      ss << "HTTP/1.1 404 Resource Not Found" << CRLF;
      ss << "Content-Type: text/html" << CRLF;
      ss << "Content-Length: " << sizeof(http_error_404_page) << CRLF;
      ss << CRLF; // end of header
      ss << http_error_404_page;
      ss << CRLF; // end of body
      break;
    case 405:
      ss << "HTTP/1.1 405 Method Not Allowed" << CRLF;
      ss << "Content-Type: text/html" << CRLF;
      ss << "Content-Length: " << sizeof(http_error_405_page) << CRLF;
      ss << CRLF; // end of header
      ss << http_error_405_page;
      ss << CRLF; // end of body
      break;
    case 500:
      ss << "HTTP/1.1 500 Internal Server Error" << CRLF;
      ss << "Content-Type: text/html" << CRLF;
      ss << "Content-Length: " << sizeof(http_error_500_page) << CRLF;
      ss << CRLF; // end of header
      ss << http_error_500_page;
      ss << CRLF; // end of body
      break;
    default:
      Log::cerror() << "Unknown status code: " << status_code << std::endl;
      break;
  }
  client_socket->send(ss.str().c_str(), ss.str().size());
  }
};
#endif
