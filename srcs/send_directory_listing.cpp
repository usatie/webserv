#include <dirent.h>
#include <sys/stat.h>  // stat

#include <iomanip>

#include "Connection.hpp"

static int compar(const struct dirent** s1, const struct dirent** s2) {
  if ((*s1)->d_type != (*s2)->d_type)
    return (*s1)->d_type > (*s2)->d_type;
  else
    return strcmp((*s1)->d_name, (*s2)->d_name);
}

void send_directory_listing(Connection& conn,
                            const std::string& path) {  // throwable
  // Generate HTML for directory listing
  std::stringstream ss;
  ss << "<html>" << CRLF << "<head><title>Index of " << conn.req.header.path
     << "</title></head>" << CRLF << "<body>" << CRLF << "<h1>Index of "
     << conn.req.header.path << "</h1><hr><pre>";
  struct dirent** dirlist;
  int r = scandir(path.c_str(), &dirlist, NULL, compar);
  if (r < 0) {
    Log::error("scandir() failed");
    ErrorHandler::handle(conn, 500);
    return;
  }
  ss << "<a href=\"../\">../</a>" << CRLF;
  for (int i = 0; i < r; ++i) {
    const struct dirent* dp = dirlist[i];
    struct stat st;
    if (stat((path + dp->d_name).c_str(), &st) < 0) {
      Log::error("stat() failed");
      ErrorHandler::handle(conn, 500);
      return;
    }
    if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) continue;
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
    const struct tm* tm = localtime(&st.st_mtime);
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
  // Send Response
  conn.res.status_code = 200;
  conn.res.content_type = "text/html";
  conn.res.content_length = ss.str().length();
  conn.res.content = ss.str();
}
