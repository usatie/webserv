#ifndef SERVER_HPP
#define SERVER_HPP

#include "Connection.hpp"
#include "GetHandler.hpp"
#include "Header.hpp"
#include "Socket.hpp"
#include "webserv.hpp"
#include <algorithm> // std::find

class Server {
 public:
  Socket server_socket;
  std::vector<Socket *> monitor_list;
  std::vector<Connection *> connection_list;

  void remove_connection(Connection *connection) {
    monitor_list.erase(std::find(monitor_list.begin(), monitor_list.end(),
                                 connection->get_socket()));
    connection_list.erase(
        std::find(connection_list.begin(), connection_list.end(), connection));
    delete connection;
  }

  int init(int port, int backlog) {
    if (server_socket.initServer(port, backlog) < 0) {
      return -1;
    }
    monitor_list.push_back(&server_socket);
    return 0;
  }

  fd_set get_readfds(int &maxfd) {
    fd_set readfds;
    FD_ZERO(&readfds);
    for (std::vector<Socket *>::iterator it = monitor_list.begin();
         it != monitor_list.end(); it++) {
      int fd = (*it)->get_fd();
      maxfd = std::max(fd, maxfd);
      FD_SET(fd, &readfds);
    }
    return readfds;
  }
  fd_set get_writefds(int &maxfd) {
    fd_set writefds;
    FD_ZERO(&writefds);
    for (std::vector<Socket *>::iterator it = monitor_list.begin();
         it != monitor_list.end(); it++) {
      int fd = (*it)->get_fd();
      maxfd = std::max(fd, maxfd);
      FD_SET(fd, &writefds);
    }
    return writefds;
  }
  void accept() {
    Socket *client_socket = server_socket.accept();
    client_socket->set_nonblock();
    monitor_list.push_back(client_socket);
    connection_list.push_back(new Connection(client_socket));
  }
  bool canServerAccept(fd_set &readfds) {
    return FD_ISSET(server_socket.get_fd(), &readfds);
  }
  bool canConnectionResume(fd_set &readfds, fd_set &writefds,
                           Connection *conn) {
    if ((conn->shouldRecv() && FD_ISSET(conn->get_fd(), &readfds)) ||
        (conn->shouldSend() && FD_ISSET(conn->get_fd(), &writefds))) {
      return true;
    }
    return false;
  }
  void process() {
    int maxfd = 0;
    fd_set readfds = get_readfds(maxfd);
    fd_set writefds = get_writefds(maxfd);
    int result = ::select(maxfd + 1, &readfds, &writefds, NULL, NULL);
    if (result < 0) {
      std::cerr << "select error" << std::endl;
      return;
    }
    if (result == 0) {
      std::cerr << "select timeout" << std::endl;
      return;
    }
    if (canServerAccept(readfds)) {
      accept();
      return;
    }
    // TODO: equally distribute the processing time to each connection
    for (std::vector<Connection *>::iterator it = connection_list.begin();
         it != connection_list.end(); it++) {
      if (canConnectionResume(readfds, writefds, *it)) {
        (*it)->resume();
        if ((*it)->is_done()) {
          std::cout << "connection done" << std::endl;
          remove_connection(*it);
        }
        break;
      }
    }
  }
};

#endif
