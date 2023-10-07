#include "ErrorHandler.hpp"

#include <sys/stat.h>

#include <sstream>

#include "Connection.hpp"
#include "http_special_response.hpp"
#include "webserv.hpp"

const config::Location* select_loc_cf(const config::Server* srv_cf,
                                      const std::string& path) throw();
int resolve_path(const config::Server* srv_cf, const config::Location* loc_cf,
                 const std::string& req_path, std::string& path,
                 struct stat& st);

const char* status_line(int status_code) throw() {
  switch (status_code) {
    case 400:
      return "HTTP/1.1 400 Bad Request";
    case 403:
      return "HTTP/1.1 403 Forbidden";
    case 404:
      return "HTTP/1.1 404 Not Found";
    case 405:
      return "HTTP/1.1 405 Method Not Allowed";
    case 413:
      return "HTTP/1.1 413 Request Entity Too Large";
    case 500:
      return "HTTP/1.1 500 Internal Server Error";
    case 504:
      return "HTTP/1.1 504 Gateway Timeout";
    case 505:
      return "HTTP/1.1 505 HTTP Version Not Supported";
    default:
      Log::cfatal() << "Unknown status code: " << status_code << std::endl;
      return "HTTP/1.1 500 Internal Server Error";
  }
}

template <typename ConfigItem>
const config::ErrorPage* find_error_page(const ConfigItem* cf,
                                         int status_code) throw() {
  if (!cf) {
    return NULL;
  }
  std::vector<config::ErrorPage>::const_iterator it, end;
  it = cf->error_pages.begin();
  end = cf->error_pages.end();
  for (; it != end; ++it) {
    if (util::contains(it->codes, status_code)) return &(*it);
  }
  return NULL;
}

#define OK_FILE 0
#define OK_DIR 1
#define ERR_403 2
#define ERR_404 3
#define ERR_500 4
int try_error_page(Connection& conn, int status_code) {  // throwable
  // 1. No context for error_page
  if (!conn.loc_cf && !conn.srv_cf) {
    return -1;
  }
  // 2. Find error_page for status_code
  const config::ErrorPage* error_page;
  error_page = find_error_page(conn.loc_cf, status_code);
  if (!error_page) error_page = find_error_page(conn.srv_cf, status_code);
  if (!error_page) return -1;

  // 3. Resolve path
  // TODO: To implement `error_page` properly, we need to implement
  // `internal_redirect` first.
  Log::cdebug() << "Error page found: " << error_page->uri << std::endl;
  int ret;
  struct stat st;
  std::string path;
  const config::Server* srv_cf = conn.srv_cf;
  const config::Location* loc_cf = select_loc_cf(conn.srv_cf, error_page->uri);
  ret = resolve_path(srv_cf, loc_cf, error_page->uri, path, st);

  if (ret == ERR_500) {
    ErrorHandler::handle(conn, 500, true);
    return 0;
  } else if (ret != OK_FILE) {
    Log::cfatal() << "Error page file not found: " << error_page->uri
                  << std::endl;
    ErrorHandler::handle(conn, 404, true);
    return 0;
  }
  //  4. Send error page
  *conn.client_socket << status_line(status_code) << CRLF;
  *conn.client_socket << "Content-Type: text/html" << CRLF;
  *conn.client_socket << "Content-Length: " << st.st_size << CRLF;
  *conn.client_socket << CRLF;  // end of header
  conn.client_socket->send_file(path.c_str());
  return 0;
  // This causes an internal redirect to the specified uri with the client
  // request method changed to “GET” (for all methods other than “GET” and
  // “HEAD”).
  //
  // TODO: Handle errors while reading error_page_path
  // TODO: Handle `return` in error_page_location
  // TODO: Handle URL redirection (In this case, by default, the response code
  // 302 is returned to the client.)
  // TODO: If an error response is processed by a proxied server or a
  // FastCGI/uwsgi/SCGI/gRPC server, and the server may return different
  // response codes (e.g., 200, 302, 401 or 404), it is possible to respond with
  // the code it returns: error_page 404 = /404.php;
  //
}

void ErrorHandler::handle(Connection& conn, int status_code, bool noredirect) {
  conn.client_socket->clear_sendbuf();
  // Error Page by `error_page` directive
  if (!noredirect) {
    if (try_error_page(conn, status_code) == 0) {  // throwable
      return;
    }
  }
  // Default Error Page
  switch (status_code) {
    case 400:
      conn.keep_alive = false;
      *conn.client_socket << "HTTP/1.1 400 Bad Request" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: "
                          << sizeof(http_error_400_page) - 1 << CRLF;
      *conn.client_socket << "Connection: close" << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_400_page;
      break;
    case 403:
      *conn.client_socket << "HTTP/1.1 403 Forbidden" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: "
                          << sizeof(http_error_400_page) -
                                 1  // -1 because of the null terminator
                          << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_403_page;
      break;
    case 404:
      *conn.client_socket << "HTTP/1.1 404 Resource Not Found" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: "
                          << sizeof(http_error_404_page) - 1 << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_404_page;
      break;
    case 405:
      *conn.client_socket << "HTTP/1.1 405 Method Not Allowed" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: "
                          << sizeof(http_error_405_page) - 1 << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_405_page;
      break;
    case 413:
      *conn.client_socket << "HTTP/1.1 413 Request Entity Too Large" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: "
                          << sizeof(http_error_413_page) - 1 << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_413_page;
      break;
    case 500:
      *conn.client_socket << "HTTP/1.1 500 Internal Server Error" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: "
                          << sizeof(http_error_500_page) - 1 << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_500_page;
      break;
    case 504:
      *conn.client_socket << "HTTP/1.1 504 Gateway Timeout" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: "
                          << sizeof(http_error_504_page) - 1 << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_504_page;
      break;
    case 505:
      *conn.client_socket << "HTTP/1.1 505 HTTP Version Not Supported" << CRLF;
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: "
                          << sizeof(http_error_505_page) - 1 << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << http_error_505_page;
      break;
    default:
      Log::cerror() << "Unknown status code: " << status_code << std::endl;
      break;
  }
}
