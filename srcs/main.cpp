#include <iostream>

#include "GetHandler.hpp"
#include "Header.hpp"
#include "Socket.hpp"
#include "Connection.hpp"
#include "webserv.hpp"
#define PORT 8181
#define BACKLOG 5
#define ERROR 1

int main(int argc, char *argv[]) {
  (void)argc, (void)argv;
  // If initialize server socket failed, exit.
  Socket server_socket;
  //std::vector<Socket> monitor_list;
  //std::vector<Connection> connection_list;
  try {
    server_socket.initServer(PORT, BACKLOG);
  } catch (FatalError &e) {
    std::cerr << e.what() << std::endl;
    return ERROR;
  }
  // 1 server socket
  // 10 clients socket
  //monitor_list.push_back(server_socket);
  while (1) {
    /*
    result = Select(monitor_list, connection_list);
    if (FD_SET(server_sock)) {
      Socket client_socket = server_socket.accept();
      Connection conn(client_socket);
      monitor_list.push_back(client_socket);
      connection_list.push_back(conn);
    }
    else if (FD_SET(client_sock)) {
      Connection conn = findConnection(client_sock);
      conn.resume();
    }
    */

    // TODO: If some error occured, must handle the error.
    Socket client_socket = server_socket.accept();

    client_socket.recv();
    Header header(client_socket);
    if (header.method == "GET") {
      GetHandler::handle(&client_socket, header);
    } else {
      std::cerr << "Unsupported method: " << header.method << std::endl;
      client_socket.send("HTTP/1.1 405 Method Not Allowed\r\n", 34);
    }
    client_socket.flush();
  }
  return 0;
}
