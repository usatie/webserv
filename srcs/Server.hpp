#ifndef SERVER_HPP
#define SERVER_HPP

#include "Connection.hpp"
#include "GetHandler.hpp"
#include "Header.hpp"
#include "SocketBuf.hpp"
#include "webserv.hpp"
#include <algorithm> // std::find

class Server {
 private:
   typedef std::vector< std::shared_ptr<Connection> > ConnVector;
   typedef ConnVector::iterator ConnIterator;
 public:
  // Member data
  Socket server_socket;
  ConnVector connections;

  // Constructor/Destructor
  Server() {}
  ~Server() {}

  // Member functions
  void remove_connection(std::shared_ptr<Connection> connection) {
    connections.erase(
        std::find(connections.begin(), connections.end(), connection));
  }

  int init(int port, int backlog) {
    if (server_socket.initServer(port, backlog) < 0) {
      return -1;
    }
    return 0;
  }

  fd_set get_readfds(int &maxfd) {
    fd_set readfds;
    FD_ZERO(&readfds);
    // Server socketBuf
    FD_SET(server_socket.get_fd(), &readfds);
    maxfd = std::max(server_socket.get_fd(), maxfd);
    // Connections' sockets
    for (ConnIterator it = connections.begin(); it != connections.end(); it++) {
      if ((*it)->shouldRecv()) {
        int fd = (*it)->get_fd();
        maxfd = std::max(fd, maxfd);
        FD_SET(fd, &readfds);
      }
    }
    return readfds;
  }
  fd_set get_writefds(int &maxfd) {
    fd_set writefds;
    FD_ZERO(&writefds);
    // Server socketBuf is unnecessary
    // Connections' sockets
    for (ConnIterator it = connections.begin(); it != connections.end(); it++) {
      if ((*it)->shouldSend()) {
        int fd = (*it)->get_fd();
        maxfd = std::max(fd, maxfd);
        FD_SET(fd, &writefds);
      }
    }
    return writefds;
  }
  void accept() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int client_fd = ::accept(server_socket.get_fd(), (struct sockaddr *)&addr, &addrlen);
    if (client_fd < 0) {
      std::cerr << "accept() failed\n";
      // TODO: handle error
      return;
    }
    std::shared_ptr<SocketBuf> client_socket = \
        std::shared_ptr<SocketBuf>(new SocketBuf(client_fd));
    client_socket->set_nonblock();
    std::shared_ptr<Connection> conn(new Connection(client_socket));
    connections.push_back(conn);
  }
  bool canServerAccept(fd_set &readfds) {
    return FD_ISSET(server_socket.get_fd(), &readfds);
  }
  bool canConnectionResume(fd_set &readfds, fd_set &writefds,
                           std::shared_ptr<Connection> conn) {
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
    for (ConnIterator it = connections.begin(); it != connections.end(); it++) {
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
