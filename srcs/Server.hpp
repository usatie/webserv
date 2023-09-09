#ifndef SERVER_HPP
#define SERVER_HPP

#include <algorithm>  // std::find

#include "Connection.hpp"
#include "GetHandler.hpp"
#include "Header.hpp"
#include "SocketBuf.hpp"
#include "Config.hpp"
#include "webserv.hpp"

#define BACKLOG 5
/*
std::vector<int> get_ports(const Config& cf) throw() {
  std::vector<int> ports;
  for (unsigned int i = 0; i < cf.http.servers.size(); ++i) {
    const Config::Server& server = cf.http.servers[i];
    for (unsigned int j = 0; j < server.listens.size(); ++j) {
      const Config::Listen& listen = server.listens[j];
      if (ports.find(listen.port) == ports.end()) {
        ports.push_back(listen.port);
      }
    }
  }
  return ports;
}
*/

class Server {
 private:
  typedef std::vector<std::shared_ptr<Connection> > ConnVector;
  typedef ConnVector::iterator ConnIterator;
  typedef std::vector<std::shared_ptr<Socket> > SockVector;
  typedef SockVector::iterator SockIterator;
  fd_set readfds, writefds;
  fd_set ready_rfds, ready_wfds;
  int maxfd;

 public:
  // Member data
  SockVector listen_socks;
  ConnVector connections;

  // Constructor/Destructor
  Server() throw();  // Do not implement this
  Server(int port, int backlog) : maxfd(-1), listen_socks(), connections() {
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    {
      std::shared_ptr<Socket> sock(new Socket());
      if (sock->reuseaddr() < 0) {
        throw std::runtime_error("sock.reuseaddr() failed");
      }
      if (sock->bind(port) < 0) {
        throw std::runtime_error("sock.bind() failed");
      }
      if (sock->listen(backlog) < 0) {
        throw std::runtime_error("sock.listen() failed");
      }
      if (sock->set_nonblock() < 0) {
        throw std::runtime_error("sock.set_nonblock() failed");
      }
      FD_SET(sock->get_fd(), &readfds);
      maxfd = sock->get_fd();
      listen_socks.push_back(sock);
    }
  }
  /*
  Server(const Config& cf): maxfd(-1), sock(), connections(), listen_socks() {
    FD_ZERO(&readfds);
    FD_ZERO(&writefds);
    int backlog = BACKLOG;
    std::vector<int> ports = get_ports(cf);
    for (unsigned int i = 0; i < ports.size(); ++i) {
      int port = ports[i];
      std::shared_ptr<Socket> s(new Socket());
      if (s->reuseaddr() < 0) {
        throw std::runtime_error("s->reuseaddr() failed");
      }
      if (s->bind(port) < 0) {
        throw std::runtime_error("s->bind() failed");
      }
      if (s->listen(backlog) < 0) {
        throw std::runtime_error("s->listen() failed");
      }
      if (s->set_nonblock() < 0) {
        throw std::runtime_error("s->set_nonblock() failed");
      }
      FD_SET(s->get_fd(), &readfds);
      maxfd = std::max(maxfd, s->get_fd());
      listen_socks.push_back(s);
    }
  }
  */
  ~Server() throw() {}

  // Member functions
  void remove_connection(std::shared_ptr<Connection> connection) throw() {
    connections.erase(
        std::find(connections.begin(), connections.end(), connection));
    FD_CLR(connection->get_fd(), &readfds);
    FD_CLR(connection->get_fd(), &writefds);
    if (connection->get_cgifd() != -1) {
      FD_CLR(connection->get_cgifd(), &readfds);
      FD_CLR(connection->get_cgifd(), &writefds);
    }
    if (connection->get_fd() == maxfd || connection->get_cgifd() == maxfd) {
      maxfd = -1;
      for (SockIterator it = listen_socks.begin(); it != listen_socks.end();
           ++it) {
        maxfd = std::max(maxfd, (*it)->get_fd());
      }
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
    maxfd = -1;
    for (SockIterator it = listen_socks.begin(); it != listen_socks.end();
         ++it) {
      FD_SET((*it)->get_fd(), &readfds);
      maxfd = std::max(maxfd, (*it)->get_fd());
    }
  }

  void accept(std::shared_ptr<Socket> sock) throw() {
    int fd = ::accept(sock->get_fd(), NULL, NULL);
    if (fd < 0) {
      Log::cerror() << "accept() failed: " << strerror(errno) << std::endl;
      return;
    }
    try {
      std::shared_ptr<Connection> conn(NULL);
      try {
        conn = std::shared_ptr< Connection >(new Connection(fd));
      } catch (std::exception &e) {
        close(fd);
        Log::cerror() << "new Connection(fd) failed: " << e.what() << std::endl;
        return;
      }
      connections.push_back(conn);
      FD_SET(conn->get_fd(), &readfds);
      maxfd = std::max(conn->get_fd(), maxfd);
    } catch (std::exception &e) {
      Log::cerror() << "Server::accept() failed: " << e.what() << std::endl;
    }
  }

  void update_fdset(std::shared_ptr<Connection> conn) throw() {
    FD_CLR(conn->get_fd(), &readfds);
    FD_CLR(conn->get_fd(), &writefds);
    if (conn->get_cgifd() != -1) {
      FD_CLR(conn->get_cgifd(), &readfds);
      FD_CLR(conn->get_cgifd(), &writefds);
    }
    switch (conn->getIOStatus()) {
      case Connection::CLIENT_RECV:
        FD_SET(conn->get_fd(), &readfds);
        break;
      case Connection::CLIENT_SEND:
        FD_SET(conn->get_fd(), &writefds);
        break;
      case Connection::CGI_SEND:
        FD_SET(conn->get_cgifd(), &writefds);
        maxfd = std::max(conn->get_cgifd(), maxfd);
        break;
      case Connection::CGI_RECV:
        FD_SET(conn->get_cgifd(), &readfds);
        maxfd = std::max(conn->get_cgifd(), maxfd);
        break;
      default:
        break;
    }
  }

  int wait() throw() {
    ready_rfds = this->readfds;
    ready_wfds = this->writefds;
    int result = ::select(maxfd + 1, &ready_rfds, &ready_wfds, NULL, NULL);
    if (result < 0) {
      Log::error("select error");
      return -1;
    }
    if (result == 0) {
      Log::info("select timeout");
      remove_all_connections();
      return -1;
    }
    return 0;
  }

  bool canResume(std::shared_ptr<Connection> conn) const throw() {
    switch (conn->getIOStatus()) {
      case Connection::CLIENT_RECV:
        return FD_ISSET(conn->get_fd(), &ready_rfds);
      case Connection::CLIENT_SEND:
        return FD_ISSET(conn->get_fd(), &ready_wfds);
      case Connection::CGI_SEND:
        return FD_ISSET(conn->get_cgifd(), &ready_wfds);
      case Connection::CGI_RECV:
        return FD_ISSET(conn->get_cgifd(), &ready_rfds);
      default:
        return false;
    }
  }

  std::shared_ptr<Socket> get_ready_socket() throw() {
    for (SockIterator it = listen_socks.begin(); it != listen_socks.end();
         ++it) {
      if (FD_ISSET((*it)->get_fd(), &ready_rfds)) {
        return *it;
      }
    }
    return std::shared_ptr<Socket>(NULL);
  }

  // Logically it is not const because it returns a non-const pointer.
  std::shared_ptr<Connection> get_ready_connection() throw() {
    // TODO: equally distribute the processing time to each connection
    for (ConnIterator it = connections.begin(); it != connections.end(); ++it) {
      if (canResume(*it)) {
        return *it;
      }
    }
    return std::shared_ptr<Connection>(NULL);
  }

  void process() throw() {
    if (wait() < 0) {
      return;
    }
    std::shared_ptr<Connection> conn;
    std::shared_ptr<Socket> sock;
    if ((sock = get_ready_socket()) != NULL) {
      accept(sock);
    } else if ((conn = get_ready_connection())) {
      if (conn->resume() < 0) {
        Log::cerror() << "connection aborted" << std::endl;
        remove_connection(conn);
        return;
      }
      if (conn->is_done()) {
        Log::info("connection done");
        remove_connection(conn);
      } else {
        update_fdset(conn);
      }
    }
  }
};

#endif
