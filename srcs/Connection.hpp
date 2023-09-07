#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "ErrorHandler.hpp"
#include "GetHandler.hpp"
#include "Header.hpp"
#include "PostHandler.hpp"
#include "CgiHandler.hpp"
#include "SocketBuf.hpp"
#include "webserv.hpp"
#include <cassert>
#include <limits.h>
#include <signal.h>

class Connection {
 public:
  // Class private enum
  typedef enum Status {
    REQ_START_LINE,     // CLIENT_RECV
    REQ_HEADER_FIELDS,  // CLIENT_RECV
    REQ_BODY,           // CLIENT_RECV
    HANDLE,             //
    HANDLE_CGI_REQ,     // CGI_SEND
    HANDLE_CGI_RES,     // CGI_RECV
    HANDLE_CGI_PARSE,   //
    RESPONSE,           // CLIENT_SEND
    DONE                // CLIENT_SEND
  } Status;

  typedef enum IOStatus {
    CLIENT_RECV, 
    CLIENT_SEND,
    CGI_RECV,
    CGI_SEND,
    NO_IO
  } IOStatus;

  // Member data
  std::shared_ptr<SocketBuf> client_socket;
  std::shared_ptr<SocketBuf> cgi_socket;
  Header header;
  Status status;
  char *body;
  size_t body_size;
  size_t content_length;
  pid_t cgi_pid;

 public:
  // Constructor/Destructor
  Connection() throw();  // Do not implement this
  explicit Connection(int listen_fd)
      : client_socket(new SocketBuf(listen_fd)),
        cgi_socket(NULL),
        header(),
        status(REQ_START_LINE),
        body(NULL),
        body_size(0),
        content_length(0),
        cgi_pid(-1){}
  ~Connection() throw() {}
  Connection(const Connection &other) throw();  // Do not implement this
  Connection &operator=(
      const Connection &other) throw();  // Do not implement this

  // Accessors
  int get_fd() const throw() { return client_socket->get_fd(); }
  int get_cgifd() const throw() {
    if (cgi_socket == NULL) return -1;
    return cgi_socket->get_fd();
  }
  bool is_done() const throw() { return status == DONE; }

  // Member functions
  // Returns negative value when an exception is thrown from STL containers
  int resume() throw() {
    // 1. Socket I/O
    IOStatus io_status = getIOStatus();
    switch (io_status) {
      case CLIENT_RECV:
        client_socket->fill();
        break;
      case CLIENT_SEND:
        client_socket->flush();
        break;
      case CGI_RECV:
        cgi_socket->fill();
        break;
      case CGI_SEND:
        cgi_socket->flush();
        break;
      case NO_IO:
        break;
    }
    // After socket i/o, check if the socket is still open
    if (io_status == CLIENT_RECV || io_status == CLIENT_SEND) {
      if (client_socket->isClosed()) {
        Log::info("client_socket->closed");
        status = DONE;
        return -1;
      }
    } else if (io_status == CGI_RECV || io_status == CGI_SEND) {
      if (cgi_socket->isClosed()) {
        Log::info("cgi_socket->closed");
        status = HANDLE_CGI_PARSE;
      }
    }
    if (io_status == CGI_SEND && cgi_socket->isSendBufEmpty()) {
      // Send EOF to CGI Script process
      Log::debug("send EOF to CGI Script process");
      shutdown(cgi_socket->get_fd(), SHUT_WR);
    }
    bool cont = true;
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
        case HANDLE_CGI_REQ:
          cont = handle_cgi_req();
          break;
        case HANDLE_CGI_RES:
          cont = handle_cgi_res();
          break;
        case HANDLE_CGI_PARSE:
          cont = handle_cgi_parse();
          status = RESPONSE;
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

  IOStatus getIOStatus() const throw() {
    switch (status) {
      case REQ_START_LINE:
      case REQ_HEADER_FIELDS:
      case REQ_BODY:
        return CLIENT_RECV;
      case HANDLE_CGI_REQ:
        return CGI_SEND;
      case HANDLE_CGI_RES:
        return CGI_RECV;
      case RESPONSE:
        return CLIENT_SEND;
      case HANDLE:
      case HANDLE_CGI_PARSE:
      case DONE:
      default:
        return NO_IO;
    }
  }

#define SPACE ' '
  // TODO: make this noexcept
  // https://datatracker.ietf.org/doc/html/rfc2616#section-5.1
  // Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
  int parse_start_line() throw() {
    std::string line;

    if (client_socket->read_telnet_line(line) < 0) {
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
      while (client_socket->read_telnet_line(line) == 0) {
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
    // if CGI 
    if (header.path.find("/cgi/") != std::string::npos) {
      // TODO: write(body, body_size) in handle()
      CgiHandler::handle(*this);
      status = HANDLE_CGI_REQ;
      return 1;
    }
    // If not CGI
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

  int handle_cgi_req() throw() {
    if (cgi_socket->isSendBufEmpty()) {
      status = HANDLE_CGI_RES;
      return 1;
    }
    return 0;
  }

  int handle_cgi_res() throw() {
    // 1. If CGI process is still running, return 0
    // Question: If CGI process exit, isClosed will be set to true?
    if (!cgi_socket->isClosed())
      return 0;

    // 2. If CGI process is not running, handle CGI response
    // To remove zombie process, wait or kill CGI process
    pid_t ret;
    int status;
    while ((ret = waitpid(cgi_pid, &status, WNOHANG)) < 0) {
      // If interrupted by signal, continue to waitpid again
      if (errno == EINTR) continue;
      // If other errors(ECHILD/EFAULT/EINVAL), kill CGI process and return 500
      // (I don't think this will happen though.)
      Log::cfatal() << "waitpid error" << std::endl;
      kill(cgi_pid, SIGKILL);
      ErrorHandler::handle(*client_socket, 500);
      status = RESPONSE;
      return 1;
    }

    // 3. ret == 0 means CGI is still running
    // (I don't think this will happen though.)
    if (ret == 0) {
      Log::cfatal() << "CGI still running" << std::endl;
      kill(cgi_pid, SIGKILL);
      ErrorHandler::handle(*client_socket, 500);
      status = RESPONSE;
      return 1;
    }
    status = HANDLE_CGI_PARSE;
    return 1;
  }

  int handle_cgi_parse() throw() {
    // TODO: parse
    char buf[1024];
    ssize_t ret;
    while ( (ret = cgi_socket->read(buf, 1024)) > 0) {
      client_socket->write(buf, ret);
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
