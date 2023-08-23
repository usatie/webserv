#ifndef SERVER_HPP
#define SERVER_HPP

#include <algorithm>  // std::find

#include "Connection.hpp"
#include "GetHandler.hpp"
#include "Header.hpp"
#include "SocketBuf.hpp"
#include "webserv.hpp"

class Server {
 private:
  typedef std::vector<std::shared_ptr<Connection> > ConnVector;
  typedef ConnVector::iterator ConnIterator;
  fd_set readfds, writefds;
  fd_set ready_rfds, ready_wfds;
  int maxfd;

 public:
  // Member data
  Socket sock;
  ConnVector connections;

  // Constructor/Destructor
  Server() throw();  // Do not implement this
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
  ~Server() throw() {}

  // Member functions
  void remove_connection(std::shared_ptr<Connection> connection) throw() {
    connections.erase(
        std::find(connections.begin(), connections.end(), connection));
    FD_CLR(connection->get_fd(), &readfds);
    FD_CLR(connection->get_fd(), &writefds);
    if (connection->get_fd() == maxfd) {
      maxfd = sock.get_fd();
      for (ConnIterator it = connections.begin(); it != connections.end();
           ++it) {
        maxfd = std::max(maxfd, (*it)->get_fd());
      }
    }
  }

  void remove_all_connections() throw() {
    connections.clear();
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    FD_SET(sock.get_fd(), &readfds);
    maxfd = sock.get_fd();
  }

  // TODO: make this noexcept
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

  bool canServerAccept(fd_set &readfds) const throw() {
    return FD_ISSET(sock.get_fd(), &readfds);
  }

  void update_fdset(std::shared_ptr<Connection> conn) throw() {
    if (conn->shouldRecv()) {
      FD_SET(conn->get_fd(), &readfds);
    } else {
      FD_CLR(conn->get_fd(), &readfds);
    }
    if (conn->shouldSend()) {
      FD_SET(conn->get_fd(), &writefds);
    } else {
      FD_CLR(conn->get_fd(), &writefds);
    }
  }

  int wait(int timeout) throw() {
    ready_rfds = this->readfds;
    ready_wfds = this->writefds;
    struct timeval tv;
    tv.tv_sec = timeout;
    tv.tv_usec = 0;
    int result = ::select(maxfd + 1, &ready_rfds, &ready_wfds, NULL, &tv);
    if (result < 0) {
      std::cerr << "select error" << std::endl;
      return -1;
    }
    if (result == 0) {
      std::cerr << "select timeout" << std::endl;
      remove_all_connections();
      return -1;
    }
    return 0;
  }

  bool canResume(std::shared_ptr<Connection> conn) const throw() {
    return (conn->shouldRecv() && FD_ISSET(conn->get_fd(), &ready_rfds)) ||
           (conn->shouldSend() && FD_ISSET(conn->get_fd(), &ready_wfds));
  }

  // Logically it is not const because it returns a non-const pointer.
  std::shared_ptr<Connection> get_ready_connection() throw() {
    // TODO: equally distribute the processing time to each connection
    for (ConnIterator it = connections.begin(); it != connections.end(); ++it) {
      if (canResume(*it)) {
        return *it;
      }
    }
    return NULL;
  }

  // TODO: make this noexcept
  void process(int timeout) {
    if (wait(timeout) < 0) {
      return;
    }
    std::shared_ptr<Connection> conn;
    if (canServerAccept(ready_rfds)) {
      accept();
    } else if ((conn = get_ready_connection())) {
      // conn->resume() may throw an exception from STL containers
      try {
        conn->resume();
      } catch (std::exception &e) {
        std::cerr << "connection aborted: " << e.what() << std::endl;
        remove_connection(conn);
        return;
      }
      if (conn->is_done()) {
        std::cout << "connection done" << std::endl;
        remove_connection(conn);
      } else {
        update_fdset(conn);
      }
    }
  }
};

#endif
