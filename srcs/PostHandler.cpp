#include "PostHandler.hpp"

#include <sys/stat.h>  // mkdir

#include <cerrno>
#include <ctime>

#include "Connection.hpp"
#include "ErrorHandler.hpp"

#define SUCCESS 0
#define ERR_405 1
#define ERR_413 2
#define ERR_500 3

// This is throwable
template <typename ConfigItem>
static int internal_handle(Connection& conn, ConfigItem* cf) {  // throwable
  const Request& req = conn.req;
  Response& res = conn.res;
  // `upload_store` directive
  if (!cf->upload_store.configured) return ERR_405;
  // `client_max_body_size` directive
  if (cf->client_max_body_size < req.content_length) return ERR_413;
  // Create directory
  // TODO: Check if directory exists when starting the server
  if (mkdir(cf->upload_store.c_str(), 0755) == -1) {
    if (errno != EEXIST) {
      Log::fatal("mkdir failed");
      return ERR_500;
    }
  }
  std::string filename, filepath;
  std::stringstream ss;
  // Generate a unique filename in the upload directory
  do {
    ss.str("");
    ss.clear();
    std::time_t ts = std::time(NULL);
    ss << ts << "-" << rand();
    // filename = "{timestamp}-{random number}"
    if (ss.fail()) {
      Log::fatal("stringstream failed while generating filename");
      return ERR_500;
    }
    filename = ss.str();  // throwable
    filepath = cf->upload_store + "/" + filename;
  } while (access(filename.c_str(), F_OK) != -1);

  // Create file
  std::ofstream ofs(filepath.c_str(), std::ios::binary);
  if (!ofs.is_open()) {
    Log::fatal("file open failed");
    return ERR_500;
  }
  // https://en.cppreference.com/w/cpp/io/basic_ostream/write
  ofs.write(req.body.c_str(), req.body.size());  // does not throw
  if (ofs.bad()) {
    Log::fatal("ofs.write failed");
    // Close file and remove file
    ofs.close();
    if (std::remove(filepath.c_str()) == -1) {
      Log::fatal("remove failed");
      return ERR_500;
    }
    return ERR_500;
  }
  ofs.close();
  // Send response
  // TODO: filepath is not necessarily the same as path in the request
  // TODO: what if conn.header.path is not exact match?
  // TODO: what if location is nested?
  // TODO: what if not aliased exactly the same as upload_store?
  // Question: Should we resolve the filepath location before saving the file?
  res.status_code = 201;
  res.location = req.header.path + filename;
  res.content_type = "application/json";
  res.content_length = 18;
  res.content = "{\"success\":\"true\"}";
  return SUCCESS;
}

void PostHandler::handle(Connection& conn) {
  int err;
  if (conn.req.loc_cf) {
    err = internal_handle(conn, conn.req.loc_cf);  // throwable
  } else {
    err = internal_handle(conn, conn.req.srv_cf);  // throwable
  }
  switch (err) {
    case SUCCESS:
      return;
    case ERR_405:
      ErrorHandler::handle(conn, 405);
      break;
    case ERR_413:
      ErrorHandler::handle(conn, 413);
      break;
    case ERR_500:
    default:
      ErrorHandler::handle(conn, 500);
      break;
  }
}
