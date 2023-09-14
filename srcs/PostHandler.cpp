#include "PostHandler.hpp"

#include "Connection.hpp"
#include "ErrorHandler.hpp"

// This is throwable
int upload_file(Connection& conn, const std::string& upload_store, std::string& filepath) {
  // Create directory
  if (mkdir(upload_store.c_str(), 0755) == -1) {
    if (errno != EEXIST) {
      Log::fatal("mkdir failed");
      ErrorHandler::handle(conn, 500);
      return -1;
    }
  }
  std::string filename;
  try {
    // Generate a unique filename in the upload directory
    do {
      // filename = "{timestamp}-{random number}"
      filename = std::to_string(time(NULL)) + "-" + std::to_string(rand());
      filepath = upload_store + "/" + filename;
    } while (access(filename.c_str(), F_OK) != -1);
  } catch (std::exception& e) {
    Log::fatal("std::to_string failed");
    filepath.clear();
    ErrorHandler::handle(conn, 500);
    return -1;
  }

  // Create file
  std::ofstream ofs(filepath.c_str(), std::ios::binary);
  if (!ofs.is_open()) {
    Log::fatal("file open failed");
    ErrorHandler::handle(conn, 500);
    return -1;
  }
  ofs.write(
      conn.body,
      conn.content_length);  // does not throw ref:
                             // https://en.cppreference.com/w/cpp/io/basic_ostream/write
  if (ofs.bad()) {
    Log::fatal("ofs.write failed");
    // Close file and remove file
    ofs.close();
    if (std::remove(filepath.c_str()) == -1) {
      Log::fatal("remove failed");
      ErrorHandler::handle(conn, 500);
      return -1;
    }
    ErrorHandler::handle(conn, 500);
    return -1;
  }
  ofs.close();
  // Send response
  *conn.client_socket << "HTTP/1.1 201 Created" << CRLF;
  // TODO: filepath is not necessarily the same as path in the request
  // TODO: what if conn.header.path is not exact match?
  // TODO: what if location is nested?
  // TODO: what if not aliased exactly the same as upload_store?
  // Question: Should we resolve the filepath location before saving the file?
  *conn.client_socket << "Location: " << conn.header.path << filename << CRLF;
  *conn.client_socket << "Content-Type: application/json" << CRLF;
  *conn.client_socket << "Content-Length: 18" << CRLF;
  *conn.client_socket << CRLF;
  *conn.client_socket << "{\"success\":\"true\"}";
  *conn.client_socket << CRLF;
  return 0;
}

void PostHandler::handle(Connection& conn) throw() {
  // `upload_store` directive
  std::string filepath;
  if (conn.loc_cf) {
    if (conn.loc_cf->upload_store.configured)
      upload_file(conn, conn.loc_cf->upload_store, filepath);
    else
      ErrorHandler::handle(conn, 405);
  } else if (conn.srv_cf->upload_store.configured) {
    upload_file(conn, conn.srv_cf->upload_store, filepath);
  } else {
    ErrorHandler::handle(conn, 405);
  }
  /*
  // Create file
  std::ofstream ofs(conn.header.fullpath.c_str(), std::ios::binary);
  if (!ofs.is_open()) {
    Log::fatal("file open failed");
    ErrorHandler::handle(conn, 500);
    return;
  }
  ofs.write(
      conn.body,
      conn.content_length);  // does not throw ref:
                             // https://en.cppreference.com/w/cpp/io/basic_ostream/write
  if (ofs.bad()) {
    Log::fatal("ofs.write failed");
    ErrorHandler::handle(conn, 500);
    return;
  }
  // Send response
  *conn.client_socket << "HTTP/1.1 201 Created" << CRLF;
  *conn.client_socket << "Location: " << conn.header.path << CRLF;
  *conn.client_socket << "Content-Type: application/json" << CRLF;
  *conn.client_socket << "Content-Length: 18" << CRLF;
  *conn.client_socket << CRLF;
  *conn.client_socket << "{\"success\":\"true\"}";
  *conn.client_socket << CRLF;
  */
}
