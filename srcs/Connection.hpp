#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "GetHandler.hpp"
#include "Header.hpp"
#include "Socket.hpp"

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
  std::shared_ptr<Socket> client_socket;
  Header header;
  Status status;
 public:
  // Constructor/Destructor
  Connection() {}
  Connection(std::shared_ptr<Socket> client_socket) : client_socket(client_socket) {}
  ~Connection() {}
  Connection(const Connection &other) { *this = other; }
  Connection &operator=(const Connection &other) {
    if (this != &other) {
      client_socket = other.client_socket;
      header = other.header;
    }
    return *this;
  }

  // Accessors
  int get_fd() { return client_socket->get_fd(); }
  bool is_done() { return status == DONE; }

  // Member functions
  int resume() {
    if (shouldRecv()) {
      client_socket->recv();
    } else if (shouldSend()) {
      if (client_socket->flush() < 0) {
        if (client_socket->isClosed()) {
          std::cerr << "client_socket->closed\n";
          status = DONE;
        }
      }
    }
    while (1) {
      switch (status) {
        case REQ_START_LINE:
          if (parse_start_line() <= 0) {
            return 0;
          }
          break;
        case REQ_HEADER_FIELDS:
          if (parse_header_fields() <= 0) {
            return 0;
          }
          break;
        case REQ_BODY:
          if (parse_body() <= 0) {
            return 0;
          }
          break;
        case HANDLE:
          if (handle() <= 0) {
            return 0;
          }
          break;
        case RESPONSE:
          if (response() <= 0) {
            return 0;
          }
          break;
        case DONE:
          return 0;
      }
    }
    return 0;
  }

  bool shouldRecv() {
    return status == REQ_START_LINE || status == REQ_HEADER_FIELDS ||
           status == REQ_BODY;
  }

  bool shouldSend() { return status == HANDLE || status == RESPONSE; }

private:
  // TODO: refactor? fix?
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
        std::cerr << "client_socket->closed\n";
        status = DONE;
        return 1;
      }
      return 0;
    }
    std::vector<std::string> keywords = split(line, ' ');
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
      std::cerr << "Unsupported method: " << header.method << std::endl;
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
