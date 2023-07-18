#include <iostream>
#include "Socket.hpp"
#include "DefaultHandler.hpp"
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
    DefaultHandler::handle(client_socket);
  }
  return 0;
}
