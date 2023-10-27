#include "Connection.hpp"

void send_regular_file(Connection& conn, const std::string& path,
                       size_t content_length) throw() {
  conn.res.status_code = 200;
  if (util::string::ends_with(path, ".css"))
    conn.res.content_type = "text/css";
  else if (util::string::ends_with(path, ".js"))
    conn.res.content_type = "text/javascript";
  else if (util::string::ends_with(path, ".jpg"))
    conn.res.content_type = "image/jpeg";
  else if (util::string::ends_with(path, ".png"))
    conn.res.content_type = "image/png";
  else if (util::string::ends_with(path, ".gif"))
    conn.res.content_type = "image/gif";
  else if (util::string::ends_with(path, ".ico"))
    conn.res.content_type = "image/x-icon";
  else if (util::string::ends_with(path, ".svg"))
    conn.res.content_type = "image/svg+xml";
  else if (util::string::ends_with(path, ".html"))
    conn.res.content_type = "text/html";
  else
    conn.res.content_type = "text/plain";
  conn.res.content_length = content_length;
  conn.res.content_path = path;
}
