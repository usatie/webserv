#include <iostream>
#include "Header.hpp"
#include "Socket.hpp"
#include "GetHandler.hpp"
#define PORT 8181
#define BACKLOG 5

int main(int argc, char *argv[])
{
  (void)argc, (void)argv;
  Socket server_socket = Socket();
  server_socket.reuseaddr();
  server_socket.bind(PORT);
  server_socket.listen(BACKLOG);
  while (1) {
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
