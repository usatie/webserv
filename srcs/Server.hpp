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
  fd_set readfds, writefds;
  int maxfd;

 public:
  // Member data
  Socket sock;
  ConnVector connections;

  // Constructor/Destructor
  Server() {
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    maxfd = 0;
  }
  ~Server() {}

  // Member functions
  void remove_connection(std::shared_ptr<Connection> connection) {
    connections.erase(
        std::find(connections.begin(), connections.end(), connection));
    FD_CLR(connection->get_fd(), &readfds);
    FD_CLR(connection->get_fd(), &writefds);
    if (connection->get_fd() == maxfd) {
      maxfd = sock.get_fd();
      for (ConnIterator it = connections.begin(); it != connections.end(); it++) {
        maxfd = std::max(maxfd, (*it)->get_fd());
      }
    }
  }

  int init(int port, int backlog) {
    if (sock.get_fd() < 0) {
      return -1;
    }
    if (sock.reuseaddr() < 0) {
      return -1;
    }
    if (sock.bind(port) < 0) {
      return -1;
    }
    if (sock.listen(backlog) < 0) {
      return -1;
    }
    if (sock.set_nonblock() < 0) {
      return -1;
    }
    FD_SET(sock.get_fd(), &readfds);
    maxfd = sock.get_fd();
    return 0;
  }

  void accept() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int client_fd = ::accept(sock.get_fd(), (struct sockaddr *)&addr, &addrlen);
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
    FD_SET(client_fd, &readfds);
    maxfd = std::max(client_fd, maxfd);
  }
  bool canServerAccept(fd_set &readfds) {
    return FD_ISSET(sock.get_fd(), &readfds);
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
    fd_set rfds = this->readfds, wfds = this->writefds;
    int result = ::select(maxfd + 1, &rfds, &wfds, NULL, NULL);
    if (result < 0) {
      std::cerr << "select error" << std::endl;
      return;
    }
    if (result == 0) {
      std::cerr << "select timeout" << std::endl;
      return;
    }
    if (canServerAccept(rfds)) {
      accept();
      return;
    }
    // TODO: equally distribute the processing time to each connection
    for (ConnIterator it = connections.begin(); it != connections.end(); it++) {
      if (canConnectionResume(rfds, wfds, *it)) {
        try {
          (*it)->resume();
        } catch (std::exception &e) {
          std::cerr << e.what() << std::endl;
          remove_connection(*it);
          return;
        }

        if ((*it)->shouldRecv()) {
          FD_SET((*it)->get_fd(), &readfds);
        } else {
          FD_CLR((*it)->get_fd(), &readfds);
        }
        if (((*it)->shouldSend())) {
          FD_SET((*it)->get_fd(), &writefds);
        } else {
          FD_CLR((*it)->get_fd(), &writefds);
        }
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
