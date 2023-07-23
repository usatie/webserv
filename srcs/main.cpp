#include <iostream>

#include "GetHandler.hpp"
#include "Header.hpp"
#include "Socket.hpp"
#include "webserv.hpp"
#define PORT 8181
#define BACKLOG 5
#define ERROR 1

int main(int argc, char *argv[]) {
  (void)argc, (void)argv;
  // If initialize server socket failed, exit.
  Socket server_socket;
  try {
    server_socket.initServer(PORT, BACKLOG);
  } catch (FatalError &e) {
    std::cerr << e.what() << std::endl;
    return ERROR;
  }
  while (1) {
    // TODO: If some error occured, must handle the error.
    Socket client_socket = server_socket.accept();
    Header header(client_socket);
    if (header.method == "GET") {
      GetHandler::handle(client_socket, header);
    } else {
      std::cerr << "Unsupported method: " << header.method << std::endl;
      client_socket.send("HTTP/1.1 405 Method Not Allowed\r\n", 34);
    }
  }
  return 0;
}
