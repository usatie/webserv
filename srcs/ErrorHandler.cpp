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
    case 200:
      return http_200_status_line;
    case 201:
      return http_201_status_line;
    case 204:
      return http_204_status_line;
    case 301:
      return http_301_status_line;
    case 400:
      return http_400_status_line;
    case 403:
      return http_403_status_line;
    case 404:
      return http_404_status_line;
    case 405:
      return http_405_status_line;
    case 413:
      return http_413_status_line;
    case 500:
      return http_500_status_line;
    case 504:
      return http_504_status_line;
    case 505:
      return http_505_status_line;
    default:
      Log::cfatal() << "Unknown status code: " << status_code << std::endl;
      return http_500_status_line;
  }
}

std::string default_error_page(int status_code) throw() {
  switch (status_code) {
    case 400:
      return http_error_400_page;
    case 403:
      return http_error_403_page;
    case 404:
      return http_error_404_page;
    case 405:
      return http_error_405_page;
    case 413:
      return http_error_413_page;
    case 500:
      return http_error_500_page;
    case 504:
      return http_error_504_page;
    case 505:
      return http_error_505_page;
    default:
      Log::cfatal() << "Unknown status code: " << status_code << std::endl;
      return "";
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
  //*conn.client_socket << status_line(status_code) << CRLF;
  //*conn.client_socket << "Content-Type: text/html" << CRLF;
  //*conn.client_socket << "Content-Length: " << st.st_size << CRLF;
  //*conn.client_socket << CRLF;  // end of header
  //conn.client_socket->send_file(path.c_str());
  conn.res.status_code = status_code;
  conn.res.content_type = "text/html";
  conn.res.content_length = st.st_size;
  conn.res.content_path = path;
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

void gen_response(Connection& conn) {
  *conn.client_socket << status_line(conn.res.status_code) << CRLF;
  *conn.client_socket << "Server: " << WEBSERV_VER << CRLF;
  //*conn.client_socket << "Date: " << util::get_date() << CRLF;
  if (conn.res.keep_alive) {
    *conn.client_socket << "Connection: keep-alive" << CRLF;
    //*conn.client_socket << "Keep-Alive: timeout=" << conn.srv_cf->timeout
    //                    << CRLF;
  } else {
    *conn.client_socket << "Connection: close" << CRLF;
  }
  if (conn.res.location != "") {
    *conn.client_socket << "Location: " << conn.res.location << CRLF;
  }
  if (conn.res.content_type != "") {
    *conn.client_socket << "Content-Type: " << conn.res.content_type << CRLF;
  }
  if (conn.res.content != "") {
    *conn.client_socket << "Content-Length: " << conn.res.content.length()
                        << CRLF;
    *conn.client_socket << CRLF;  // end of header
    *conn.client_socket << conn.res.content;
  } else if (conn.res.content_path != "") {
    *conn.client_socket << "Content-Length: " << conn.res.content_length
                        << CRLF;
    *conn.client_socket << CRLF;  // end of header
    // TODO: Handle errors while reading content_path
    conn.client_socket->send_file(conn.res.content_path.c_str());
  } else {
    *conn.client_socket << "Content-Length: 0" << CRLF;
    *conn.client_socket << CRLF;  // end of header
  }
}

void ErrorHandler::handle(Connection& conn, int status_code, bool noredirect) {
  conn.client_socket->clear_sendbuf();
  if (status_code == 400) {
      conn.keep_alive = false;
      conn.res.keep_alive = false;
  }
  // Error Page by `error_page` directive
  if (!noredirect) {
    if (try_error_page(conn, status_code) == 0) {  // throwable
      gen_response(conn);
      return;
    }
  }
  // Default Error Page
  conn.res.status_code = status_code;
  conn.res.content_type = "text/html";
  conn.res.content = default_error_page(status_code);
  conn.res.content_length = conn.res.content.length();
  gen_response(conn);
}
