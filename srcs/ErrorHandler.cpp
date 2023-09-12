#include "ErrorHandler.hpp"
#include "http_special_response.hpp"
#include "webserv.hpp"
#include "Connection.hpp"
#include <sstream>

const Config::Location* select_loc_cf(const Config::Server* srv_cf, const std::string& path) throw();
void ErrorHandler::handle(Connection& conn, int status_code, bool noredirect) throw() {
  conn.client_socket->clear_sendbuf();
  if (!noredirect) {
    std::vector<Config::ErrorPage>::const_iterator it, end;
    if (conn.loc_cf) {
      it = conn.loc_cf->error_pages.begin();
      end = conn.loc_cf->error_pages.end();
    } else if (conn.srv_cf) {
      it = conn.srv_cf->error_pages.begin();
      end = conn.srv_cf->error_pages.end();
    } else {
      goto default_error_page;
    }
    for (; it != end; ++it) {
      Log::cdebug() << "Error page: " << it->uri << std::endl;
      if (std::find(it->codes.begin(), it->codes.end(), status_code) != it->codes.end()) {
        std::string error_page_path = it->uri;
        // TODO: To implement `error_page` properly, we need to implement `internal_redirect` first.
        Log::cdebug() << "Error page found: " << error_page_path << std::endl;
        //  1. Select Location config
        const Config::Server* srv_cf = conn.srv_cf;
        const Config::Location* loc_cf = select_loc_cf(conn.srv_cf, error_page_path);
        //  2. Generate fullpath
        std::string error_page_fullpath;
        try {
          if (!loc_cf) {
            // Server Root    : Append path to root
            Log::cdebug() << "Server Root" << std::endl;
            error_page_fullpath = srv_cf->root + error_page_path;
          } else if (!loc_cf->alias.configured) {
            // Location Root  : Append path to root
            Log::cdebug() << "Location Root: " << loc_cf->path << std::endl;
            error_page_fullpath = loc_cf->root + error_page_path;
          } else {
            // Location Alias : Replace prefix with alias
            Log::cdebug() << "Location Alias: " << loc_cf->path << std::endl;
            error_page_fullpath = loc_cf->alias + error_page_path.substr(loc_cf->path.size());
          }
        } catch (std::exception &e) {
          Log::cfatal()
              << "\"header.fullpath = root or alias + header.path\" failed"
              << std::endl;
          ErrorHandler::handle(conn, 500, true);
          return;
        }

        //  3. Send file
        std::string error_page_content;
        try {
          std::ifstream ifs(error_page_fullpath.c_str());
          // TODO: Handle errors while reading error_page_path, directory, regular file, etc.
          if (!ifs.is_open()) {
            Log::cfatal() << "Error page file not found: " << error_page_fullpath << std::endl;
            ErrorHandler::handle(conn, 404, true);
            return;
          }
          std::stringstream ss;
          ss << ifs.rdbuf();
          error_page_content = ss.str();
        } catch (std::exception &e) {
          Log::cfatal() << "Error page file read failed: " << error_page_fullpath << std::endl;
          ErrorHandler::handle(conn, 500, true);
          return;
        }
  switch (status_code) {
    case 400:
      *conn.client_socket << "HTTP/1.1 400 Bad Request" << CRLF;
      break;
    case 403:
      *conn.client_socket << "HTTP/1.1 403 Forbidden" << CRLF;
      break;
    case 404:
      *conn.client_socket << "HTTP/1.1 404 Not Found" << CRLF;
      break;
    case 405:
      *conn.client_socket << "HTTP/1.1 405 Method Not Allowed" << CRLF;
      break;
    case 500:
      *conn.client_socket << "HTTP/1.1 500 Internal Server Error" << CRLF;
      break;
    default:
      Log::cfatal() << "Unknown status code: " << status_code << std::endl;
      return;
  }
      *conn.client_socket << "Content-Type: text/html" << CRLF;
      *conn.client_socket << "Content-Length: " << error_page_content.size() << CRLF;
      *conn.client_socket << CRLF;  // end of header
      *conn.client_socket << error_page_content;
      return;
        // This causes an internal redirect to the specified uri with the client request method changed to “GET” (for all methods other than “GET” and “HEAD”).
        //
        // TODO: Handle errors while reading error_page_path
        // TODO: Handle `return` in error_page_location
        // TODO: Handle URL redirection (In this case, by default, the response code 302 is returned to the client.)
        // TODO: If an error response is processed by a proxied server or a FastCGI/uwsgi/SCGI/gRPC server, and the server may return different response codes (e.g., 200, 302, 401 or 404), it is possible to respond with the code it returns:
        // error_page 404 = /404.php;
        // 
      }
    }
  }
default_error_page:
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
