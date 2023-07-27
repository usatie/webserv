#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "Socket.hpp"
#include "Header.hpp"
#include "GetHandler.hpp"

class Connection
{
private:
  Socket *client_socket;
  Header header;
  typedef enum Status {
    REQ_START_LINE,
    REQ_HEADER_FIELDS,
    REQ_BODY,
    HANDLE,
    RESPONSE,
    DONE
  } Status;
public:
  Status status ;
  int resume() {
    if ( shouldRecv() ) {
      client_socket->recv();
    } else if ( shouldSend() ) {
      client_socket->flush();
    }
    switch (status) {
      case REQ_START_LINE:
        parse_start_line();
      case REQ_HEADER_FIELDS:
        parse_header_filelds();
      case REQ_BODY:
        parse_body();
      case HANDLE:
        handle();
      case RESPONSE:
        response();
      case DONE:
        return 0;
    }
  }

  bool shouldRecv() {
    return status == REQ_START_LINE || status == REQ_HEADER_FIELDS || status == REQ_BODY;
  }

  bool shouldSend() {
    return status == RESPONSE;
  }

  int parse_start_line() {
    std::string line;

    if (client_socket->readline(line) < 0) {
      return -1;
    }
    std::vector<std::string> keywords = Header::split(line, ' ');
    // TODO: validate keywords
    header.method = keywords[0];
    header.path = keywords[1];
    header.version = keywords[2];
    status = REQ_HEADER_FIELDS;
    return 0;
  }

  int parse_header_filelds() {
    // TODO: implement
    status = REQ_BODY;
    return 0;
  }

  int parse_body() {
    // TODO: implement
    status = RESPONSE;
    return 0;
  }

  int handle() {
    if (header.method == "GET") {
      GetHandler::handle(client_socket, header);
    } else {
      std::cerr << "Unsupported method: " << header.method << std::endl;
      client_socket->send("HTTP/1.1 405 Method Not Allowed\r\n", 34);
    }
    status = RESPONSE;
    return 0;
  }

  int response() {
    if (client_socket->sendbuf.empty()) {
      status = DONE;
    }
    return 0;
  }
};

#endif
