#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "GetHandler.hpp"
#include "Header.hpp"
#include "SocketBuf.hpp"
#include "webserv.hpp"

class Connection {
 private:
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

 public:
  // Constructor/Destructor
  Connection() throw();  // Do not implement this
  explicit Connection(int listen_fd)
      : client_socket(new SocketBuf(listen_fd)),
        header(),
        status(REQ_START_LINE) {}
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
    if (client_socket->get_stl_error()) {
      Log::info("client_socket->get_stl_error()");
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

 private:
  // TODO: refactor? fix?
  // TODO: make this noexcept
  static std::vector<std::string> split(std::string str, char delim) {
    std::vector<std::string> ret;
    int idx = 0;
    while (str[idx]) {
      std::string line;
      while (str[idx] && str[idx] != delim) {
        line += str[idx];
        idx++;
      }
      while (str[idx] && str[idx] == delim) {
        idx++;
      }
      if (!line.empty()) {
        ret.push_back(line);
      }
    }
    return ret;
  }

  int parse_start_line() {
    std::string line;

    if (client_socket->readline(line) < 0) {
      if (client_socket->isClosed()) {
        Log::info("client_socket->closed");
        status = DONE;
        return 1;
      }
      return 0;
    }
    std::vector<std::string> keywords = split(line, ' ');
    if (keywords.size() != 3) {
      Log::cinfo() << "Invalid start line: " << line << std::endl;
      client_socket->send("HTTP/1.1 400 Bad Request\r\n", 26);
      status = RESPONSE;
      return 1;
    }
    // TODO: validate keywords
    header.method = keywords[0];
    header.path = keywords[1];
    header.version = keywords[2];
    status = REQ_HEADER_FIELDS;
    return 1;
  }

  int parse_header_fields() {
    // TODO: implement
    status = REQ_BODY;
    return 1;
  }

  int parse_body() {
    // TODO: implement
    status = HANDLE;
    return 1;
  }

  int handle() {
    if (header.method == "GET") {
      GetHandler::handle(client_socket, header);
    } else {
      Log::cinfo() << "Unsupported method: " << header.method << std::endl;
      client_socket->send("HTTP/1.1 405 Method Not Allowed\r\n", 34);
    }
    status = RESPONSE;
    return 1;
  }

  int response() {
    if (client_socket->isSendBufEmpty()) {
      status = DONE;
      return 1;
    }
    return 0;
  }
};

#endif
