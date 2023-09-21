#include "Server.hpp"

#include <arpa/inet.h>
#include <netdb.h>

#include <cerrno>
#include <cstring>  // memset, strerror

#include "Config.hpp"
#include "Connection.hpp"

std::ostream& operator<<(std::ostream& os, const struct addrinfo* rp);

Server::~Server() throw() {}

Server::Server(const config::Config& cf) throw()
    : maxfd(-1), listen_socks(), connections(), cf(cf) {
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
}

int Server::getaddrinfo(const config::Listen& l, struct addrinfo** result) {
  const char* host = (l.address == "*") ? NULL : l.address.c_str();
  std::stringstream ss;
  ss << l.port;
  if (ss.bad()) {
    return -1;
  }
  std::string port(ss.str());  // throwable
  struct addrinfo hints;

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
  // TODO: AF_INET or AF_INET6 depends on the address format
  // i.e. "192.168.1.1"                             -> AF_INET
  //      "*"                                       -> AF_INET
  //      "localhost"                               -> AF_INET
  //      "google.com"                              -> AF_INET
  //      "[::]"                                    -> AF_INET6
  //      "[::1]"                                   -> AF_INET6
  //      "2001:0db8:85a3:0000:0000:8a2e:0370:7334" -> AF_INET6

  hints.ai_family = AF_UNSPEC; /* Allows IPv4 or IPv6 */
  if (l.address == "*") {
    hints.ai_family = AF_INET; /* Allows IPv4 only */
  }
  hints.ai_flags = AI_PASSIVE; /* Wildcard IP address */
  hints.ai_socktype = SOCK_STREAM;

  int error = ::getaddrinfo(host, port.c_str(), &hints, result);
  if (error) {
    return -1;
  }
  return 0;
}

int Server::listen(const config::Listen& l) {
  struct addrinfo* result;
  if (getaddrinfo(l, &result) < 0) {  // throwable
    return -1;
  }
  /* Walk through returned list until we find an address structure
   * that can be used to successfully connect a socket */
  for (struct addrinfo* rp = result; rp != NULL; rp = rp->ai_next) {
    Log::cdebug() << "listen : " << l << std::endl;
    Log::cdebug() << "ip: " << rp << std::endl;
    int sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) {
      Log::cfatal() << "socket() failed. (" << errno << ": " << strerror(errno)
                    << ")" << std::endl;
      return -1;
    }
    util::shared_ptr<Socket> sock(new Socket(sfd));  // throwable
    if (sock->reuseaddr() < 0) {
      Log::cfatal() << "sock.set_reuseaddr() failed. (" << errno << ": "
                    << strerror(errno) << ")" << std::endl;
      return -1;
    }
    if (rp->ai_family == AF_INET6) {
      if (sock->ipv6only() < 0) {
        Log::cfatal() << "sock.set_ipv6only() failed. (" << errno << ": "
                      << strerror(errno) << ")" << std::endl;
        return -1;
      }
    }
    if (sock->bind(rp->ai_addr, rp->ai_addrlen) < 0) {
      Log::cfatal() << "bind to " << rp << ":" << l.port << " failed."
                    << "(" << errno << ": " << strerror(errno) << ")"
                    << std::endl;
      return -1;
    }
    if (sock->listen(BACKLOG) < 0) {
      Log::cfatal() << "sock.listen() failed. (" << errno << ": "
                    << strerror(errno) << ")" << std::endl;
      return -1;
    }
    if (sock->set_nonblock() < 0) {
      Log::cfatal() << "sock.set_nonblock() failed. (" << errno << ": "
                    << strerror(errno) << ")" << std::endl;
      return -1;
    }
    // Save the listening ip address to config::Listen
    // Here we use const_cast to modify the const object.
    {
      config::Listen& ll = const_cast<config::Listen&>(l);
      memcpy(&ll.addr, rp->ai_addr, rp->ai_addrlen);
      ll.addrlen = rp->ai_addrlen;
    }
    FD_SET(sock->get_fd(), &readfds);
    maxfd = std::max(maxfd, sock->get_fd());
    listen_socks.push_back(sock);  // throwable
    break;  // Success, one socket per one listen directive
  }
  freeaddrinfo(result);
  return 0;
}

void Server::startup() {
  for (unsigned int i = 0; i < cf.http.servers.size(); ++i) {
    Log::cdebug() << "i: " << i << std::endl;
    const config::Server& server = cf.http.servers[i];
    for (unsigned int j = 0; j < server.listens.size(); ++j) {
      Log::cdebug() << "j: " << j << std::endl;
      if (listen(server.listens[j]) < 0) {  // throwable
        std::exit(EXIT_FAILURE);
      }
    }
  }
}

void Server::remove_connection(
    util::shared_ptr<Connection> connection) throw() {
  connections.erase(std::find(connections.begin(), connections.end(),
                              connection));  // no throw
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
    for (ConnIterator it = connections.begin(); it != connections.end(); ++it) {
      maxfd = std::max(maxfd, (*it)->get_fd());
    }
  }
}
void Server::remove_all_connections() throw() {
  connections.clear();
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  maxfd = -1;
  for (SockIterator it = listen_socks.begin(); it != listen_socks.end(); ++it) {
    FD_SET((*it)->get_fd(), &readfds);
    maxfd = std::max(maxfd, (*it)->get_fd());
  }
}

void Server::accept(util::shared_ptr<Socket> sock) throw() {
  try {
    util::shared_ptr<Connection> conn(
        new Connection(sock->accept(), cf));  // throwable
    connections.push_back(conn);              // throwable
    FD_SET(conn->get_fd(), &readfds);
    maxfd = std::max(conn->get_fd(), maxfd);
  } catch (std::exception& e) {
    Log::cerror() << "Server::accept() failed: " << e.what() << std::endl;
  }
}

void Server::update_fdset(util::shared_ptr<Connection> conn) throw() {
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
int Server::wait() throw() {
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
bool Server::canResume(util::shared_ptr<Connection> conn) const throw() {
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

util::shared_ptr<Socket> Server::get_ready_listen_socket() throw() {
  for (SockIterator it = listen_socks.begin(); it != listen_socks.end(); ++it) {
    if (FD_ISSET((*it)->get_fd(), &ready_rfds)) {
      return *it;
    }
  }
  return util::shared_ptr<Socket>();
}

// Logically it is not const because it returns a non-const pointer.
util::shared_ptr<Connection> Server::get_ready_connection() throw() {
  // TODO: equally distribute the processing time to each connection
  for (ConnIterator it = connections.begin(); it != connections.end(); ++it) {
    if (canResume(*it)) {
      return *it;
    }
  }
  return util::shared_ptr<Connection>();
}

void Server::resume(util::shared_ptr<Connection> conn) throw() {
  try {
    conn->resume();  // throwable
  } catch (std::exception& e) {
    // We don't send 500 error page to keep our life simple.
    remove_connection(conn);
    return;
  }
  // If the connection is aborted, remove it.
  if (conn->is_remove()) {
    Log::info("connection aborted");
    remove_connection(conn);
    return;
  }
  // If the request is done, clear it.
  if (conn->is_clear()) {
    Log::info("Request is done, clear connection");
    conn->clear();
  }
  // Update fdset
  update_fdset(conn);
}

void Server::run() throw() {
  while (1) {
    if (wait() < 0) {
      continue;
    }
    util::shared_ptr<Connection> conn;  // no throw
    util::shared_ptr<Socket> sock;      // no throw
    if ((sock = get_ready_listen_socket()) != NULL) {
      accept(sock);  // no throw
    } else if ((conn = get_ready_connection()) != NULL) {
      resume(conn);  // no throw
    }
  }
}

// Stream
std::ostream& operator<<(std::ostream& os, const struct addrinfo* rp) {
  char buf[INET6_ADDRSTRLEN];
  void* addr;
  if (rp->ai_family == AF_INET) {
    struct sockaddr_in* ipv4 = (struct sockaddr_in*)rp->ai_addr;
    addr = &(ipv4->sin_addr);
  } else if (rp->ai_family == AF_INET6) {
    struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)rp->ai_addr;
    addr = &(ipv6->sin6_addr);
  } else {
    os << "unknown address family: ";
    return os;
  }
  inet_ntop(rp->ai_family, addr, buf, sizeof buf);
  os << buf;
  return os;
}
