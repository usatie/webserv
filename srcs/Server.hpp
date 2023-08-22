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
  Server(); // Do not implement this
  Server(int port, int backlog) : sock(), connections() {
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    if (sock.reuseaddr() < 0) {
      throw std::runtime_error("sock.reuseaddr() failed");
    }
    if (sock.bind(port) < 0) {
      throw std::runtime_error("sock.bind() failed");
    }
    if (sock.listen(backlog) < 0) {
      throw std::runtime_error("sock.listen() failed");
    }
    if (sock.set_nonblock() < 0) {
      throw std::runtime_error("sock.set_nonblock() failed");
    }
    FD_SET(sock.get_fd(), &readfds);
    maxfd = sock.get_fd();
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

  void remove_all_connections() {
    connections.clear();
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(sock.get_fd(), &readfds);
    maxfd = sock.get_fd();
  }

  void accept() {
    // The call to the allocation function (operator new) is indeterminately
    // sequenced with respect to (until C++17) the evaluation of the constructor
    // arguments in a new-expression.
    //
    // So, ::accept() must be called inside the constructor of Connection.
    // Actually, it is called inside the constructor of Socket class.
    std::shared_ptr<Connection> conn(new Connection(sock.get_fd()));
    connections.push_back(conn);
    FD_SET(conn->get_fd(), &readfds);
    maxfd = std::max(conn->get_fd(), maxfd);
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
  void process(int timeout) {
    fd_set rfds = this->readfds, wfds = this->writefds;
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    int result = ::select(maxfd + 1, &rfds, &wfds, NULL, &tv);
    if (result < 0) {
      std::cerr << "select error" << std::endl;
      return;
    }
    if (result == 0) {
      std::cerr << "select timeout" << std::endl;
      remove_all_connections();
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
