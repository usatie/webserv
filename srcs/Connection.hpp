#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "ErrorHandler.hpp"
#include "GetHandler.hpp"
#include "Header.hpp"
#include "PostHandler.hpp"
#include "SocketBuf.hpp"
#include "webserv.hpp"
#include <cassert>
#include <limits.h>

class Connection {
 public:
  // Class private enum
  typedef enum Status {
    REQ_START_LINE,
    REQ_HEADER_FIELDS,
    REQ_BODY,
    HANDLE,
    RESPONSE,
    DONE
  } Status;

  // Member data
  std::shared_ptr<SocketBuf> client_socket;
  Header header;
  Status status;
  char *body;
  size_t body_size;
  size_t content_length;

 public:
  // Constructor/Destructor
  Connection() throw();  // Do not implement this
  explicit Connection(int listen_fd)
      : client_socket(new SocketBuf(listen_fd)),
        header(),
        status(REQ_START_LINE),
        body(NULL),
        body_size(0),
        content_length(0) {}
  ~Connection() throw() {}
  Connection(const Connection &other) throw();  // Do not implement this
  Connection &operator=(
      const Connection &other) throw();  // Do not implement this

  // Accessors
  int get_fd() const throw() { return client_socket->get_fd(); }
  bool is_done() const throw() { return status == DONE; }

  // Member functions
  // Returns negative value when an exception is thrown from STL containers
  int resume() throw() {
    if (shouldRecv()) {
      client_socket->fill();
    } else if (shouldSend()) {
      client_socket->flush();
    }
    bool cont = true;
    // After recv/send, check if the socket is still open
    if (client_socket->isClosed()) {
      Log::info("client_socket->closed");
      status = DONE;
      cont = false;
    }
    // If the socket is closed, we don't need to do anything
    while (cont) {
      switch (status) {
        case REQ_START_LINE:
          cont = parse_start_line();
          break;
        case REQ_HEADER_FIELDS:
          cont = parse_header_fields();
          break;
        case REQ_BODY:
          cont = parse_body();
          break;
        case HANDLE:
          cont = handle();
          break;
        case RESPONSE:
          cont = response();
          break;
        case DONE:
          cont = false;
      }
    }
    // Finally, check if there is any error while handling the request
    if (client_socket->bad()) {
      Log::info("client_socket->bad()");
      status = DONE;
      return -1;
    }
    return 0;
  }

  bool shouldRecv() const throw() {
    return status == REQ_START_LINE || status == REQ_HEADER_FIELDS ||
           status == REQ_BODY;
  }

  bool shouldSend() const throw() {
    return status == HANDLE || status == RESPONSE;
  }

#define SPACE ' '
  // TODO: make this noexcept
  // https://datatracker.ietf.org/doc/html/rfc2616#section-5.1
  // Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
  int parse_start_line() throw() {
    std::string line;

    if (client_socket->readline(line) < 0) {
      return 0;
    }
    Log::cdebug() << "start line: " << line << std::endl;
    std::stringstream ss;
    ss << line;
    ss >> header.method;   // ss does not throw (cf. playground/fuga.cpp)
    ss >> header.path;     // ss does not throw
    ss >> header.version;  // ss does not throw
    // Path must be starting with /
    if (header.path[0] != '/') {
      Log::cinfo() << "Invalid path: " << header.path << std::endl;
      ErrorHandler::handle(*client_socket, 400);
      status = RESPONSE;
      return 1;
    }
    // Get cwd
    char cwd[PATH_MAX];
    if (getcwd(cwd, PATH_MAX) == 0) {
      Log::cfatal() << "getcwd failed" << std::endl;
      ErrorHandler::handle(*client_socket, 500);
      status = RESPONSE;
      return 1;
    }
    // TODO: Defense Directory traversal attack
    // TODO: Handle absoluteURI
    // TODO: Handle *
    try {
      header.fullpath = std::string(cwd) + header.path;  // throwable
    } catch (std::exception &e) {
      Log::cfatal()
          << "\"header.fullpath = std::string(cwd) + header.path\" failed"
          << std::endl;
      ErrorHandler::handle(*client_socket, 500);
      status = RESPONSE;
      return 1;
    }
    // ss.bad()  : possibly bad alloc
    if (ss.bad()) {
      Log::cfatal() << "ss bad bit is set" << line << std::endl;
      ErrorHandler::handle(*client_socket, 500);
      status = RESPONSE;
      return 1;
    }
    // ss.fail() : method or path or version is missing
    // !ss.eof()  : there are more than 3 tokens or extra white spaces
    if (ss.fail() || !ss.eof()) {
      Log::cinfo() << "Invalid start line: " << line << std::endl;
      ErrorHandler::handle(*client_socket, 400);
      status = RESPONSE;
      return 1;
    }
    status = REQ_HEADER_FIELDS;
    return 1;
  }

  int parse_header_fields() throw() {
    try {
      std::string line;
      while (client_socket->readline(line) == 0) {
        // Empty line indicates the end of header fields
        if (line == "") {
          status = REQ_BODY;
          return 1;
        }
        Log::cdebug() << "header line: " << line << std::endl;
        std::stringstream ss(
            line);  // throwable! (bad alloc) (cf. playground/hoge.cpp)
        std::string key, value;
        if (std::getline(ss, key, ':') == 0) {  // throwable
          Log::cerror() << "std::getline(ss, key, ':') failed. line: " << line
                        << std::endl;
          ErrorHandler::handle(*client_socket, 500);
          status = RESPONSE;
          return 1;
        }
        if (util::http::is_token(key) == false) {
          Log::cinfo() << "Header filed-name is not token: : " << key
                       << std::endl;
          ErrorHandler::handle(*client_socket, 400);
          status = RESPONSE;
          return 1;
        }
        // Remove leading space from value
        ss >> std::ws;
        // Extract remaining string to value
        std::getline(ss, value);     // throwable
        header.fields[key] = value;  // throwable
      }
    } catch (std::exception &e) {
      Log::cfatal() << "Exception: " << e.what() << std::endl;
      ErrorHandler::handle(*client_socket, 500);
      status = RESPONSE;
      return 1;
    }
    return 0;
  }

  int parse_body() throw() {
    if (body == NULL) {
      if (header.fields.find("Content-Length") == header.fields.end()) {
        status = HANDLE;
        return 1;
      }
      // TODO: Handle invalid Content-Length
      content_length = atoi(header.fields["Content-Length"].c_str());
      try {
        body = new char[content_length];
      } catch (std::exception &e) {
        Log::cfatal() << "Exception: " << e.what() << std::endl;
        ErrorHandler::handle(*client_socket, 500);
        status = RESPONSE;
        return 1;
      }
    }
    assert(body != NULL);  // body is guaranteed to be non null
    ssize_t ret =
        client_socket->read(body + body_size, content_length - body_size);
    if (ret < 0) {
      return 0;
    } else if (ret == 0) {
      return 0;
    } else {
      body_size += ret;
      if (body_size == content_length) {
        status = HANDLE;
        return 1;
      }
      return 0;
    }
  }

  int handle() throw() {
    if (header.method == "GET") {
      GetHandler::handle(*client_socket, header);
    } else if (header.method == "POST") {
      PostHandler::handle(*this);
    } else {
      Log::cinfo() << "Unsupported method: " << header.method << std::endl;
      ErrorHandler::handle(*client_socket, 405);
    }
    status = RESPONSE;
    return 1;
  }

  int response() throw() {
    if (client_socket->isSendBufEmpty()) {
      status = DONE;
      return 1;
    }
    return 0;
  }
};

#endif
