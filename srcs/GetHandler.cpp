#include "GetHandler.hpp"

#include <dirent.h>  // opendir
#include <sys/stat.h>
#include <sys/types.h>

#include <cstring>  // strerror

#include "Connection.hpp"
#include "ErrorHandler.hpp"
#include "webserv.hpp"

bool validate(const char* path, size_t& content_length) {
  struct stat st;
  if (stat(path, &st) < 0) {
    Log::cdebug() << "stat() failed: " << path << ", errno:" << strerror(errno)
                  << std::endl;
    return false;
  }
  if (S_ISDIR(st.st_mode)) {
    Log::cdebug() << "is a directory: " << path << std::endl;
    return false;
  }
  if (!S_ISREG(st.st_mode)) {
    Log::cdebug() << "is not a regular file: " << path << std::endl;
    return false;
  }
  content_length = st.st_size;
  return true;
}

// 1. If fullpath is regular file, send it
// 2. Else if fullpath is directory, try to append index.html
// 3. Else if fullpath ends with '/', try to list directory
// 4. Else, return 404 or 403

#define FILE_OK 0
#define DIR_LIST_OK 1
#define STAT_ERR_403 2
#define STAT_ERR_404 3
#define STAT_ERR_500 4
int resolve(const Config::Server* srv_cf, const Config::Location* loc_cf,
            const std::string& req_path, std::string& path, struct stat& st) {
  // TODO: normalize req_path
  // 0. `root` and `alias` directive
  if (!loc_cf) {
    // Server Root    : Append path to root
    Log::cdebug() << "Server Root" << std::endl;
    path = srv_cf->root + req_path;
  } else if (loc_cf->alias.configured) {
    // Location Alias : Replace prefix with alias
    Log::cdebug() << "Location Alias: " << loc_cf->path << std::endl;
    path = loc_cf->alias + req_path.substr(loc_cf->path.size());
  } else {
    // Location Root  : Append path to root
    Log::cdebug() << "Location Root: " << loc_cf->path << std::endl;
    path = loc_cf->root + req_path;
  }

  // 1. If path is regular file, send it
  if (stat(path.c_str(), &st) < 0 && errno == ENOENT) {
    Log::cdebug() << "stat() failed: " << path << ", errno:" << strerror(errno)
                  << std::endl;
    return STAT_ERR_500;
  }
  if (S_ISREG(st.st_mode)) return FILE_OK;
  // 2. Else if fullpath is directory, try to append index.html
  const std::vector<std::string>& index =
      (!loc_cf) ? srv_cf->index : loc_cf->index;
  for (size_t i = 0; i < index.size(); ++i) {
    Log::cdebug() << "Trying index: " << path << index[i] << std::endl;
    if (stat((path + index[i]).c_str(), &st) < 0) {
      if (errno == ENOENT) continue;
      Log::cdebug() << "stat() failed: " << path << index[i]
                    << ", errno:" << strerror(errno) << std::endl;
      return STAT_ERR_500;
    }
    if (S_ISREG(st.st_mode)) {
      path += index[i];
      return FILE_OK;
    }
  }
  // 3. Else if path ends with '/', try to list directory
  if (path.back() == '/') {
    if (stat(path.c_str(), &st) < 0 && errno == ENOENT) {
      Log::cdebug() << "stat() failed: " << path
                    << ", errno:" << strerror(errno) << std::endl;
      return STAT_ERR_500;
    }
    if (loc_cf) {
      if (loc_cf->autoindex && S_ISDIR(st.st_mode)) return DIR_LIST_OK;
    } else {
      if (srv_cf->autoindex && S_ISDIR(st.st_mode)) return DIR_LIST_OK;
    }
  }
  // TODO: 301 Moved Permanently
  // TODO: 403 Forbidden
  return STAT_ERR_404;
}

void handle_regular_file(Connection& conn, const std::string& path,
                         size_t content_length) {
  *conn.client_socket << "HTTP/1.1 200 OK" << CRLF;
  *conn.client_socket << "Server: " << WEBSERV_VER << CRLF;
  // client_socket << "Date: Tue, 11 Jul 2023 07:36:50 GMT" << CRLF;
  if (util::string::ends_with(conn.header.path, ".css"))
    *conn.client_socket << "Content-Type: text/css" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".js"))
    *conn.client_socket << "Content-Type: text/javascript" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".jpg"))
    *conn.client_socket << "Content-Type: image/jpeg" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".png"))
    *conn.client_socket << "Content-Type: image/png" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".gif"))
    *conn.client_socket << "Content-Type: image/gif" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".ico"))
    *conn.client_socket << "Content-Type: image/x-icon" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".svg"))
    *conn.client_socket << "Content-Type: image/svg+xml" << CRLF;
  else if (util::string::ends_with(conn.header.path, ".html"))
    *conn.client_socket << "Content-Type: text/html" << CRLF;
  else
    *conn.client_socket << "Content-Type: text/plain" << CRLF;
  *conn.client_socket << "Content-Length: " << content_length << CRLF;
  *conn.client_socket << CRLF;  // end of header
  if (conn.client_socket->send_file(path) < 0) {
    Log::error("send_file() failed");
    ErrorHandler::handle(conn, 500);
    return;
  }
}

int compar(const struct dirent** s1, const struct dirent** s2) {
  if ((*s1)->d_type != (*s2)->d_type)
    return (*s1)->d_type > (*s2)->d_type;
  else
    return strcmp((*s1)->d_name, (*s2)->d_name);
}

void handle_directory_listing(Connection& conn,
                              const std::string& path) throw() {
  *conn.client_socket << "HTTP/1.1 200 OK" << CRLF;
  *conn.client_socket << "Server: " << WEBSERV_VER << CRLF;
  *conn.client_socket << "Content-Type: text/html" << CRLF;
  std::stringstream ss;
  try {
    ss << "<html>" << CRLF << "<head><title>Index of " << conn.header.path
       << "</title></head>" << CRLF << "<body>" << CRLF << "<h1>Index of "
       << conn.header.path << "</h1><hr><pre>";
    struct dirent **dirlist, *dp;
    int r = scandir(path.c_str(), &dirlist, NULL, compar);
    if (r < 0) {
      Log::error("scandir() failed");
      ErrorHandler::handle(conn, 500);
      return;
    }
    ss << "<a href=\"../\">../</a>" << CRLF;
    for (int i = 0; i < r; ++i) {
      dp = dirlist[i];
      struct stat st;
      if (stat((path + dp->d_name).c_str(), &st) < 0) {
        Log::error("stat() failed");
        ErrorHandler::handle(conn, 500);
        return;
      }
      if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0)
        continue;
      // Name with link
      ss << "<a href=\"" << dp->d_name;
      if (S_ISDIR(st.st_mode)) ss << "/";
      ss << "\">";
      ss << dp->d_name;
      if (S_ISDIR(st.st_mode)) ss << "/";
      ss << "</a>";

      // Date time with space aligned
      ss << std::setw(68 - strlen(dp->d_name) - (S_ISDIR(st.st_mode) ? 1 : 0));
      char buf[256];
      struct tm* tm = localtime(&st.st_mtimespec.tv_sec);
      strftime(buf, sizeof(buf), "%d-%b-%Y %H:%M", tm);
      ss << buf;

      // Size with space aligned
      ss << std::setw(20) << std::right;
      if (S_ISDIR(st.st_mode))
        ss << "-";
      else
        ss << st.st_size;
      ss << CRLF;
    }
    free(dirlist);
    ss << "</pre><hr></body>" << CRLF << "</html>" << CRLF;
    if (ss.fail()) {
      Log::error("stringstream::fail() == true");
      ErrorHandler::handle(conn, 500);
      return;
    }
    *conn.client_socket << "Content-Length: " << ss.str().length() << CRLF;
    *conn.client_socket << CRLF;  // end of header
    *conn.client_socket << ss.str();
  } catch (std::exception& e) {
    Log::cerror() << "std::exception: " << std::string(e.what());
    ErrorHandler::handle(conn, 500);
  }
}

// root/alias の解決もここでやる？
// めんどい : index fileを複数試す必要がある
// resolved_pathを使いまわしたい : connが持つ？
// statの結果を使いまわしたい : connが持つ？
// 最終的なファイルパスをどこかに保存したい
void GetHandler::handle(Connection& conn) throw() {
  // TODO: Write response headers
  struct stat st;
  std::string path;
  int response_type =
      resolve(conn.srv_cf, conn.loc_cf, conn.header.path, path, st);
  if (response_type == STAT_ERR_500) {
    ErrorHandler::handle(conn, 500);
  } else if (response_type == STAT_ERR_404) {
    ErrorHandler::handle(conn, 404);
  } else if (response_type == STAT_ERR_403) {
    ErrorHandler::handle(conn, 403);
  } else if (response_type == DIR_LIST_OK) {
    handle_directory_listing(conn, path);
  } else if (response_type == FILE_OK) {
    handle_regular_file(conn, path, st.st_size);
  } else {
    Log::cerror() << "Unknown response type: " << response_type << std::endl;
    ErrorHandler::handle(conn, 500);
  }
}
