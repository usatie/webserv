#include <iostream>

#include "Connection.hpp"
#include "GetHandler.hpp"
#include "Header.hpp"
#include "Socket.hpp"
#include "webserv.hpp"
#define PORT 8181
#define BACKLOG 5
#define ERROR 1

void remove_connection(std::vector<Connection *> &connection_list,
                       Connection *connection,
                       std::vector<Socket *> &monitor_list) {
  monitor_list.erase(std::find(monitor_list.begin(), monitor_list.end(),
                               connection->get_socket()));
  connection_list.erase(
      std::find(connection_list.begin(), connection_list.end(), connection));
  delete connection;
}

int main(int argc, char *argv[]) {
  (void)argc, (void)argv;
  // If initialize server socket failed, exit.
  Socket server_socket;
  std::vector<Socket *> monitor_list;
  std::vector<Connection *> connection_list;
  try {
    server_socket.initServer(PORT, BACKLOG);
  } catch (FatalError &e) {
    std::cerr << e.what() << std::endl;
    return ERROR;
  }
  // 1 server socket
  // 10 clients socket
  monitor_list.push_back(&server_socket);
  while (1) {
    fd_set readfds;
    fd_set writefds;
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    int maxfd = 0;
    for (std::vector<Socket *>::iterator it = monitor_list.begin();
         it != monitor_list.end(); it++) {
      if ((*it)->get_fd() > maxfd) {
        maxfd = (*it)->get_fd();
      }
      FD_SET((*it)->get_fd(), &readfds);
      FD_SET((*it)->get_fd(), &writefds);
    }
    int result = select(maxfd + 1, &readfds, &writefds, NULL, NULL);
    if (result > 0) {
      if (FD_ISSET(server_socket.get_fd(), &readfds)) {
        Socket *client_socket = server_socket.accept();
        client_socket->set_nonblock();
        monitor_list.push_back(client_socket);
        connection_list.push_back(new Connection(client_socket));
      } else {
        for (std::vector<Connection *>::iterator it = connection_list.begin();
             it != connection_list.end(); it++) {
          if (((*it)->shouldRecv() && FD_ISSET((*it)->get_fd(), &readfds)) ||
              ((*it)->shouldSend() && FD_ISSET((*it)->get_fd(), &writefds))) {
            (*it)->resume();
            if ((*it)->is_done()) {
              remove_connection(connection_list, *it, monitor_list);
            }
            break;
          }
        }
      }
    }
  }
  return 0;
}
