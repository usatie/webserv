#include "DeleteHandler.hpp"

#include <errno.h>
#include <sys/stat.h>

#include "Connection.hpp"
#include "ErrorHandler.hpp"

void DeleteHandler::handle(Connection& conn) throw() {
  (void)conn;
  // 1. Get the path
  std::string path;
  // `root` and `alias` directive
  if (!conn.req.loc_cf) {
    // Server Root    : Append path to root
    Log::cdebug() << "Server Root" << std::endl;
    path = conn.req.srv_cf->root + conn.req.header.path;
  } else if (conn.req.loc_cf->alias.configured) {
    // TODO: Question: What if parent location has alias?
    // TODO: What if location is nested?
    // Location Alias : Replace prefix with alias
    Log::cdebug() << "Location Alias: " << conn.req.loc_cf->path << std::endl;
    path = conn.req.loc_cf->alias +
           conn.req.header.path.substr(conn.req.loc_cf->path.size());
  } else {
    // Location Root  : Append path to root
    Log::cdebug() << "Location Root: " << conn.req.loc_cf->path << std::endl;
    path = conn.req.loc_cf->root + conn.req.header.path;
  }
  // 2. Check if the path exists
  struct stat st;
  if (stat(path.c_str(), &st) < 0) {
    Log::cdebug() << "stat() failed: " << path << ", errno:" << strerror(errno)
                  << std::endl;
    if (errno == EACCES)
      return ErrorHandler::handle(conn, 403);
    else if (errno == ENOENT)
      return ErrorHandler::handle(conn, 404);
    else
      return ErrorHandler::handle(conn, 500);
  }
  if (!S_ISREG(st.st_mode)) {
    Log::cdebug() << "Not a regular file: " << path << std::endl;
    return ErrorHandler::handle(conn, 403);
  }
  // 3. Get the new path( Move to trash directory )
  std::string filename = path.substr(path.find_last_of('/') + 1);
  // Create directory if not exist
  if (mkdir("/tmp/webserv/", 0755) == -1) {
    if (errno != EEXIST) {
      Log::cfatal() << "mkdir() failed: " << strerror(errno) << std::endl;
      return ErrorHandler::handle(conn, 500);
    }
  }

  std::string trash_path;
  do {
    std::stringstream ss;
    ss << "/tmp/webserv/" << filename << "." << time(NULL);
    trash_path = ss.str();
  } while (stat(trash_path.c_str(), &st) == 0 || errno != ENOENT);
  // 4. Delete the file (Move to trash directory)
  if (std::rename(path.c_str(), trash_path.c_str()) < 0) {
    Log::cdebug() << "rename() failed: " << path
                  << ", errno:" << strerror(errno) << std::endl;
    if (errno == EACCES)
      return ErrorHandler::handle(conn, 403);
    else if (errno == ENOENT)
      return ErrorHandler::handle(conn, 404);
    else
      return ErrorHandler::handle(conn, 500);
  }

  // 5. Send the response
  conn.res.status_code = 204;
}
