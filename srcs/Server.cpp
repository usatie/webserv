#include "Server.hpp"

#include <arpa/inet.h>
#include <netdb.h>

#include <cerrno>
#include <numeric>  // accumulate

#include "Config.hpp"
#include "Connection.hpp"

struct AddrInfo {
  struct addrinfo* rp;
  AddrInfo() : rp(NULL) {}
  ~AddrInfo() {
    if (rp) freeaddrinfo(rp);
  }
};

std::ostream& operator<<(std::ostream& os, const struct addrinfo* rp);

Server::~Server() throw() {}

Server::Server(const config::Config& cf) throw()
    : maxfd(-1),
      last_timeout_check(time(NULL)),
      listen_socks(),
      connections(),
      cf(cf) {
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
}

static int getaddrinfo(const config::Listen& l, struct addrinfo** result) {
  const char* host = (l.address == "*") ? NULL : l.address.c_str();
  std::stringstream ss;
  ss << l.port;
  if (ss.fail()) {
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

  // If the address is bracketed, it is IPv6
  if (l.address.size() > 0 && l.address[0] == '[' &&
      l.address[l.address.size() - 1] == ']') {
    hints.ai_family = AF_INET6; /* Allows IPv6 only */
  } else {
    // Currently only IPv4 is supported for hostnames
    // TODO: IPv6 support if address is bracketed
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

static bool already_listened(const Server::SockVector& socks,
                             const struct addrinfo* rp) {
  for (unsigned int i = 0; i < socks.size(); ++i) {
    if (util::inet::eq_addr46(
            &socks[i]->saddr,
            reinterpret_cast<const struct sockaddr_storage*>(rp->ai_addr),
            false)) {
      return true;
    }
  }
  return false;
}

bool is_wildcard(const struct sockaddr* addr) {
  if (addr->sa_family == AF_INET) {
    const struct sockaddr_in* addr4 =
        reinterpret_cast<const struct sockaddr_in*>(addr);
    return addr4->sin_addr.s_addr == INADDR_ANY;
  } else if (addr->sa_family == AF_INET6) {
    const struct sockaddr_in6* addr6 =
        reinterpret_cast<const struct sockaddr_in6*>(addr);
    return IN6_IS_ADDR_UNSPECIFIED(&addr6->sin6_addr);
  } else {
    return false;
  }
}

int Server::listen(const config::Listen& l, SockVector& serv_socks) {
  struct AddrInfo info;
  if (getaddrinfo(l, &info.rp) < 0) {  // throwable
    return -1;
  }
  /* Walk through returned list until we find an address structure
   * that can be used to successfully connect a socket */
  for (struct addrinfo* rp = info.rp; rp != NULL; rp = rp->ai_next) {
    Log::cdebug() << "listen : " << l << std::endl;
    Log::cdebug() << "ip: " << rp << std::endl;
    // Already listend (exact match)
    if (already_listened(serv_socks, rp)) {
      Log::cfatal() << "Duplicate listen directive in a server." << std::endl;
      return -1;
    }
    // Already listened (exact match)
    if (already_listened(listen_socks, rp)) {
      Log::cdebug() << "Already listend by other server." << std::endl;
      {
        config::Listen& ll = const_cast<config::Listen&>(l);
        memcpy(&ll.addr, rp->ai_addr, rp->ai_addrlen);
        ll.addrlen = rp->ai_addrlen;
      }
      return 0;
    }
    // Wildcard -> override the address of the other server
    if (is_wildcard(rp->ai_addr)) {
      for (SockIterator it = listen_socks.begin(); it != listen_socks.end();) {
        if (util::inet::eq_addr46(
                &(*it)->saddr,
                reinterpret_cast<const struct sockaddr_storage*>(rp->ai_addr),
                true)) {
          it = listen_socks.erase(it);
        } else {
          ++it;
        }
      }
      for (SockIterator it = serv_socks.begin(); it != serv_socks.end();) {
        if (util::inet::eq_addr46(
                &(*it)->saddr,
                reinterpret_cast<const struct sockaddr_storage*>(rp->ai_addr),
                true)) {
          it = serv_socks.erase(it);
        } else {
          ++it;
        }
      }
    } else {  // Specific Address -> if already listened by other server, skip
      for (SockIterator it = listen_socks.begin(); it != listen_socks.end();
           ++it) {
        if (util::inet::eq_addr46(
                &(*it)->saddr,
                reinterpret_cast<const struct sockaddr_storage*>(rp->ai_addr),
                true)) {
          Log::cdebug() << "Already listend by other server." << std::endl;
          config::Listen& ll = const_cast<config::Listen&>(l);
          memcpy(&ll.addr, rp->ai_addr, rp->ai_addrlen);
          ll.addrlen = rp->ai_addrlen;
          return 0;
        }
      }
      for (SockIterator it = serv_socks.begin(); it != serv_socks.end(); ++it) {
        if (util::inet::eq_addr46(
                &(*it)->saddr,
                reinterpret_cast<const struct sockaddr_storage*>(rp->ai_addr),
                true)) {
          Log::cdebug() << "Already listend by other server." << std::endl;
          config::Listen& ll = const_cast<config::Listen&>(l);
          memcpy(&ll.addr, rp->ai_addr, rp->ai_addrlen);
          ll.addrlen = rp->ai_addrlen;
          return 0;
        }
      }
    }

    int sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    if (sfd == -1) {
      Log::cfatal() << "socket() failed. (" << errno << ": " << strerror(errno)
                    << ")" << std::endl;
      return -1;
    }
    Sock sock(new Socket(sfd));  // throwable
    if (sock->reuseaddr() < 0) {
      return -1;
    }
    if (rp->ai_family == AF_INET6) {
      if (sock->ipv6only() < 0) {
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
      return -1;
    }
    if (sock->set_nonblock() < 0) {
      return -1;
    }
    // Save the listening ip address to config::Listen
    // Here we use const_cast to modify the const object.
    {
      config::Listen& ll = const_cast<config::Listen&>(l);
      memcpy(&ll.addr, rp->ai_addr, rp->ai_addrlen);
      ll.addrlen = rp->ai_addrlen;
    }
    // Save the listening socket
    FD_SET(sock->get_fd(), &readfds);
    maxfd = std::max(maxfd, sock->get_fd());
    serv_socks.push_back(sock);  // throwable
    return 0;                    // Success, one socket per one listen directive
  }
  Log::cfatal() << "Could not bind to any address." << std::endl;
  return -1;
}

void Server::startup() {
  for (unsigned int i = 0; i < cf.http.servers.size(); ++i) {
    Log::cdebug() << "i: " << i << std::endl;
    const config::Server& server = cf.http.servers[i];
    SockVector socks;
    for (unsigned int j = 0; j < server.listens.size(); ++j) {
      Log::cdebug() << "j: " << j << std::endl;
      if (listen(server.listens[j], socks) < 0) {  // throwable
        std::exit(EXIT_FAILURE);
      }
    }
    listen_socks.insert(listen_socks.end(), socks.begin(), socks.end());
  }
}

static int op_conn(int fd, const Server::Conn conn) {
  return std::max(fd, conn->get_fd());
}

static int op_sock(int fd, const Server::Sock sock) {
  return std::max(fd, sock->get_fd());
}

Server::ConnIterator Server::remove_connection(Conn conn) throw() {
  ConnIterator next_it =
      connections.erase(std::find(connections.begin(), connections.end(),
                                  conn));  // no throw
  FD_CLR(conn->get_fd(), &readfds);
  FD_CLR(conn->get_fd(), &writefds);
  if (conn->get_cgifd() != -1) {
    FD_CLR(conn->get_cgifd(), &readfds);
    FD_CLR(conn->get_cgifd(), &writefds);
  }
  if (maxfd == std::max(conn->get_fd(), conn->get_cgifd())) {
    maxfd = std::max(
        std::accumulate(connections.begin(), connections.end(), -1, op_conn),
        std::accumulate(listen_socks.begin(), listen_socks.end(), -1, op_sock));
  }
  return next_it;
}
void Server::remove_timeout_connections() throw() {
  last_timeout_check = time(NULL);
  for (ConnIterator it = connections.begin(); it != connections.end();) {
    if ((*it)->is_cgi_timeout()) {
      Log::cinfo() << "CGI timeout: connfd(" << (*it)->get_fd() << ")"
                   << " port("
                   << (*it)->client_socket->socket->get_client_port() << ")"
                   << std::endl;
      (*it)->handle_cgi_timeout();
    } else if ((*it)->is_timeout()) {
      Log::cinfo() << "Connection timeout: connfd(" << (*it)->get_fd() << ")"
                   << " port("
                   << (*it)->client_socket->socket->get_client_port() << ")"
                   << std::endl;
      it = remove_connection(*it);  // always returns the next iterator
    } else {
      ++it;
    }
  }
}

void Server::accept(Sock sock) throw() {
  try {
    Conn conn(new Connection(sock->accept(), cf));  // throwable
    connections.push_back(conn);                    // throwable
    FD_SET(conn->get_fd(), &readfds);
    maxfd = std::max(conn->get_fd(), maxfd);
  } catch (std::exception& e) {
    Log::cerror() << "Server::accept() failed: " << e.what() << std::endl;
  }
}

void Server::update_fdset(Conn conn) throw() {
  FD_CLR(conn->get_fd(), &readfds);
  FD_CLR(conn->get_fd(), &writefds);
  if (!conn->client_socket->hasReceivedEof) {
    Log::cdebug() << "update_fdset: client_socket->hasReceivedEof == false"
                  << std::endl;
    FD_SET(conn->get_fd(), &readfds);
  }
  if (!conn->client_socket->isSendBufEmpty() &&
      !conn->client_socket->isBrokenPipe)
    FD_SET(conn->get_fd(), &writefds);
  maxfd = std::max(conn->get_fd(), maxfd);
  if (conn->get_cgifd() != -1) {
    FD_CLR(conn->get_cgifd(), &readfds);
    FD_CLR(conn->get_cgifd(), &writefds);
    if (!conn->cgi_socket->isSendBufEmpty() &&
        !conn->cgi_socket->isBrokenPipe)  // no data to send
      FD_SET(conn->get_cgifd(), &writefds);
    if (!conn->cgi_socket->hasReceivedEof)  // no data to receive
      FD_SET(conn->get_cgifd(), &readfds);
    maxfd = std::max(conn->get_cgifd(), maxfd);
  }
}
int Server::wait() throw() {
  ready_rfds = this->readfds;
  ready_wfds = this->writefds;
  // timeout 1s
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;
  int result = ::select(maxfd + 1, &ready_rfds, &ready_wfds, NULL, &timeout);
  if (result < 0) {
    Log::cerror() << "select error: " << strerror(errno) << std::endl;
    return -1;
  }
  if (result == 0 || (time(NULL) - last_timeout_check) >
                         std::min(TIMEOUT_SEC, CGI_TIMEOUT_SEC)) {
    remove_timeout_connections();
    return -1;
  }
  return 0;
}
bool Server::canResume(Conn conn) const throw() {
  // 1. CLIENT_SEND
  if (FD_ISSET(conn->get_fd(), &ready_wfds)) {
    Log::cdebug() << "canResume: CLIENT_SEND" << std::endl;
    conn->io_status = Connection::CLIENT_SEND;
    return true;
  }
  // 2. CGI_SEND
  // 3. CGI_RECV
  if (conn->get_cgifd() != -1) {
    if (FD_ISSET(conn->get_cgifd(), &ready_wfds)) {
      Log::cdebug() << "canResume: CGI_SEND" << std::endl;
      conn->io_status = Connection::CGI_SEND;
      return true;
    } else if (FD_ISSET(conn->get_cgifd(), &ready_rfds)) {
      Log::cdebug() << "canResume: CGI_RECV" << std::endl;
      conn->io_status = Connection::CGI_RECV;
      return true;
    }
  }
  // 4. CLIENT_RECV
  if (FD_ISSET(conn->get_fd(), &ready_rfds)) {
    Log::cdebug() << "canResume: CLIENT_RECV" << std::endl;
    conn->io_status = Connection::CLIENT_RECV;
    return true;
  }
  // 5. NO_IO
  conn->io_status = Connection::NO_IO;
  return false;
}

Server::Sock Server::get_ready_listen_socket() throw() {
  for (SockIterator it = listen_socks.begin(); it != listen_socks.end(); ++it) {
    if (FD_ISSET((*it)->get_fd(), &ready_rfds)) {
      return *it;
    }
  }
  return Sock();
}

// Logically it is not const because it returns a non-const pointer.
Server::Conn Server::get_ready_connection() throw() {
  // TODO: equally distribute the processing time to each connection
  for (ConnIterator it = connections.begin(); it != connections.end(); ++it) {
    if (canResume(*it)) {
      return *it;
    }
  }
  return Conn();
}

void Server::resume(Conn conn) throw() {
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
    // If the client socket has received EOF, remove the connection.
    if (conn->client_socket->hasReceivedEof &&
        conn->client_socket->isSendBufEmpty()) {
      Log::info("Client socket has received EOF, remove connection");
      remove_connection(conn);
      return;
    }
    // If the client has requested to close the connection, remove it.
    if (conn->header.fields["Connection"] == "close") {
      Log::info("Connection: close, remove connection");
      remove_connection(conn);
      return;
    }

    Log::info("Request is done, clear connection");
    if (conn->get_cgifd() != -1) {
      FD_CLR(conn->get_cgifd(), &readfds);
      FD_CLR(conn->get_cgifd(), &writefds);
    }
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
    Conn conn;  // no throw
    Sock sock;  // no throw
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
    struct sockaddr_in* ipv4 =
        reinterpret_cast<struct sockaddr_in*>(rp->ai_addr);
    addr = &(ipv4->sin_addr);
  } else if (rp->ai_family == AF_INET6) {
    struct sockaddr_in6* ipv6 =
        reinterpret_cast<struct sockaddr_in6*>(rp->ai_addr);
    addr = &(ipv6->sin6_addr);
  } else {
    os << "unknown address family: ";
    return os;
  }
  inet_ntop(rp->ai_family, addr, buf, sizeof buf);
  os << buf;
  return os;
}
