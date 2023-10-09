#include "GetHandler.hpp"

#include <dirent.h>  // opendir
#include <sys/stat.h>
#include <sys/types.h>

#include <cerrno>
#include <cstring>  // strerror

#include "Connection.hpp"
#include "ErrorHandler.hpp"
#include "webserv.hpp"

void send_regular_file(Connection& conn, const std::string& path,
                       size_t content_length) throw();
void send_directory_listing(Connection& conn, const std::string& path);

#define OK_FILE 0
#define OK_DIR 1
#define ERR_403 2
#define ERR_404 3
#define ERR_500 4
// 1. If fullpath is regular file, send it
// 2. Else if fullpath is directory, try to append index.html
// 3. Else if fullpath ends with '/', try to list directory
// 4. Else, return 404 or 403
// Throwable!
int resolve_path(const config::Server* srv_cf, const config::Location* loc_cf,
                 const std::string& req_path, std::string& path,
                 struct stat& st) {
  // TODO: normalize req_path
  // 0. `root` and `alias` directive
  if (!loc_cf) {
    // Server Root    : Append path to root
    Log::cdebug() << "Server Root" << std::endl;
    path = srv_cf->root + req_path;
  } else if (loc_cf->alias.configured) {
    // TODO: Question: What if parent location has alias?
    // TODO: What if location is nested?
    // Location Alias : Replace prefix with alias
    Log::cdebug() << "Location Alias: " << loc_cf->path << std::endl;
    path = loc_cf->alias + req_path.substr(loc_cf->path.size());
  } else {
    // Location Root  : Append path to root
    Log::cdebug() << "Location Root: " << loc_cf->path << std::endl;
    path = loc_cf->root + req_path;
  }

  // 1. If path is regular file, send it
  if (stat(path.c_str(), &st) < 0) {
    Log::cdebug() << "stat() failed: " << path << ", errno:" << strerror(errno)
                  << std::endl;
    switch (errno) {
      case EACCES:
        return ERR_403;
      case ENOTDIR:
        return ERR_404;
      case ENOENT:
        break;  // continue to try index
      default:
        return ERR_500;
    }
  } else if (S_ISREG(st.st_mode)) {
    return OK_FILE;
  }
  // 2. Else if fullpath is directory, try to append index.html
  const std::vector<std::string>& index =
      (!loc_cf) ? srv_cf->index : loc_cf->index;
  for (size_t i = 0; i < index.size(); ++i) {
    Log::cdebug() << "Trying index: " << path << "/" << index[i] << std::endl;
    if (stat((path + "/" + index[i]).c_str(), &st) < 0) {
      if (errno == ENOENT) continue;
      Log::cdebug() << "stat() failed: " << path << "/" << index[i]
                    << ", errno:" << strerror(errno) << std::endl;
      return ERR_500;
    }
    if (S_ISREG(st.st_mode)) {
      path += "/" + index[i];
      return OK_FILE;
    }
  }
  // 3. Else if path ends with '/', try to list directory
  if (!path.empty() && path[path.size() - 1] == '/') {
    if (stat(path.c_str(), &st) < 0 && errno != ENOENT) {
      Log::cdebug() << "stat() failed: " << path
                    << ", errno:" << strerror(errno) << std::endl;
      return ERR_500;
    }
    if (loc_cf) {
      if (loc_cf->autoindex && S_ISDIR(st.st_mode)) return OK_DIR;
    } else {
      if (srv_cf->autoindex && S_ISDIR(st.st_mode)) return OK_DIR;
    }
  }
  // TODO: 403 Forbidden
  return ERR_404;
}

void GetHandler::handle(Connection& conn) {  // throwable
  // TODO: Write response headers
  struct stat st;
  std::string path;
  int response_type = resolve_path(conn.req.srv_cf, conn.req.loc_cf,
                                   conn.req.header.path, path, st);
  if (response_type == ERR_500) {
    ErrorHandler::handle(conn, 500);
  } else if (response_type == ERR_404) {
    ErrorHandler::handle(conn, 404);
  } else if (response_type == ERR_403) {
    ErrorHandler::handle(conn, 403);
  } else if (response_type == OK_DIR) {
    send_directory_listing(conn, path);  // throwable
  } else if (response_type == OK_FILE) {
    send_regular_file(conn, path, st.st_size);
  } else {
    Log::cerror() << "Unknown response type: " << response_type << std::endl;
    ErrorHandler::handle(conn, 500);
  }
}
