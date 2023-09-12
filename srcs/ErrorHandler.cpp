#include "ErrorHandler.hpp"
#include "http_special_response.hpp"
#include "webserv.hpp"
#include "Connection.hpp"
#include <sstream>

const Config::Location* select_loc_cf(const Config::Server* srv_cf, const std::string& path) throw();
bool validate(const char* path, size_t& content_length);

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
    case 500:
      return "HTTP/1.1 500 Internal Server Error";
    default:
      Log::cfatal() << "Unknown status code: " << status_code << std::endl;
      return "HTTP/1.1 500 Internal Server Error";
  }
}

template <typename ConfigItem>
const Config::ErrorPage* find_error_page(const ConfigItem* cf, int status_code) throw() {
  if (!cf) {
    return NULL;
  }
  std::vector<Config::ErrorPage>::const_iterator it, end;
  it = cf->error_pages.begin();
  end = cf->error_pages.end();
  for (; it != end; ++it) {
    if (std::find(it->codes.begin(), it->codes.end(), status_code) == it->codes.end())
      continue;
    return &(*it);
  }
  return NULL;
}

// This is kind of a constructor, so it may throw.
std::string resolve_path(const Config::Server* srv_cf, const Config::Location* loc_cf, const std::string& path) {
  std::string fullpath;
  if (!loc_cf) {
    // Server Root    : Append path to root
    Log::cdebug() << "Server Root" << std::endl;
    return srv_cf->root + path;
  } else if (!loc_cf->alias.configured) {
    // Location Root  : Append path to root
    Log::cdebug() << "Location Root: " << loc_cf->path << std::endl;
    return loc_cf->root + path;
  } else {
    // Location Alias : Replace prefix with alias
    Log::cdebug() << "Location Alias: " << loc_cf->path << std::endl;
    return loc_cf->alias + path.substr(loc_cf->path.size());
  }
}

int try_error_page(Connection& conn, int status_code) throw() {
  // 1. No context for error_page
  if (!conn.loc_cf && !conn.srv_cf) {
    return -1;
  }
  // 2. Find error_page for status_code
  const Config::ErrorPage *error_page;
  error_page = find_error_page(conn.loc_cf, status_code);
  if (!error_page)
    error_page = find_error_page(conn.srv_cf, status_code);
  if (!error_page)
    return -1;

  // 3. Generate fullpath
  std::string error_page_path = error_page->uri;
  // TODO: To implement `error_page` properly, we need to implement `internal_redirect` first.
  Log::cdebug() << "Error page found: " << error_page_path << std::endl;
  //  1. Select Location config
  const Config::Server* srv_cf = conn.srv_cf;
  const Config::Location* loc_cf = select_loc_cf(conn.srv_cf, error_page_path);
  //  2. Generate fullpath
  std::string error_page_fullpath;
  try {
    error_page_fullpath = resolve_path(srv_cf, loc_cf, error_page_path);
  } catch (std::exception &e) {
    Log::cfatal()
        << "\"header.fullpath = root or alias + header.path\" failed"
        << std::endl;
    ErrorHandler::handle(conn, 500, true);
    return 0;
  }

  //  3. Send file
  size_t content_length;
  if (!validate(error_page_fullpath.c_str(), content_length)) {
    Log::cfatal() << "Error page file not found: " << error_page_fullpath << std::endl;
    ErrorHandler::handle(conn, 404, true);
    return 0;
  }
  *conn.client_socket << status_line(status_code) << CRLF;
  *conn.client_socket << "Content-Type: text/html" << CRLF;
  *conn.client_socket << "Content-Length: " << content_length << CRLF;
  *conn.client_socket << CRLF;  // end of header
  conn.client_socket->send_file(error_page_fullpath.c_str());
  return 0;
  // This causes an internal redirect to the specified uri with the client
  // request method changed to “GET” (for all methods other than “GET” and “HEAD”).
  //
  // TODO: Handle errors while reading error_page_path
  // TODO: Handle `return` in error_page_location
  // TODO: Handle URL redirection (In this case, by default, the response code 302 is returned to the client.)
  // TODO: If an error response is processed by a proxied server or a FastCGI/uwsgi/SCGI/gRPC server, and the server may return different response codes (e.g., 200, 302, 401 or 404), it is possible to respond with the code it returns:
  // error_page 404 = /404.php;
  // 
}

void ErrorHandler::handle(Connection& conn, int status_code, bool noredirect) throw() {
  conn.client_socket->clear_sendbuf();
  // Error Page by `error_page` directive
  if (!noredirect) {
    if (try_error_page(conn, status_code) == 0) {
      return;
    }
  }
  // Default Error Page
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
