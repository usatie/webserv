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

static bool is_wildcard(const struct sockaddr* addr) {
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

// Define a predicate functor to use with std::remove_if
struct AddrComparePredicate {
  const struct sockaddr* rp_ai_addr;
  bool allow_wildcard;

  AddrComparePredicate(const struct sockaddr* rp_addr, bool allow_wildcard)
      : rp_ai_addr(rp_addr), allow_wildcard(allow_wildcard) {}

  bool operator()(Server::Sock sock) const {
    return util::inet::eq_addr46(
        &sock->saddr,
        reinterpret_cast<const struct sockaddr_storage*>(rp_ai_addr), true);
  }
};

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
    // Save the listening ip address to config::Listen
    {
      // Here we use const_cast to modify the const object.
      config::Listen& ll = const_cast<config::Listen&>(l);
      memcpy(&ll.addr, rp->ai_addr, rp->ai_addrlen);
    }

    // To check if the address is already listened by other server
    AddrComparePredicate pred(rp->ai_addr, false);

    // 1. Already listend (exact match) by the server
    if (std::find_if(serv_socks.begin(), serv_socks.end(), pred) !=
        serv_socks.end()) {
      Log::cfatal() << "Duplicate listen directive in a server." << std::endl;
      return -1;
    }

    // 2. Already listened (exact match) by other server
    if (std::find_if(listen_socks.begin(), listen_socks.end(), pred) !=
        listen_socks.end()) {
      Log::cdebug() << "Already listend by other server." << std::endl;
      return 0;
    }

    // 3. Already listend (wild card match)
    pred.allow_wildcard = true;
    // 3-1. Wildcard -> override the address of the other server
    if (is_wildcard(rp->ai_addr)) {
      serv_socks.erase(
          std::remove_if(serv_socks.begin(), serv_socks.end(), pred),
          serv_socks.end());
      listen_socks.erase(
          std::remove_if(listen_socks.begin(), listen_socks.end(), pred),
          listen_socks.end());
    } else {  // 3-2. Specific Address -> if already listened by other server,
              // skip
      SockIterator it;
      if ((it = std::find_if(serv_socks.begin(), serv_socks.end(), pred)) !=
              serv_socks.end() ||
          (it = std::find_if(listen_socks.begin(), listen_socks.end(), pred)) !=
              listen_socks.end()) {
        Log::cdebug() << "Already listend by other server." << std::endl;
        return 0;
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
    for (SockIterator it = socks.begin(); it != socks.end(); ++it) {
      int fd = (*it)->get_fd();
      FD_SET(fd, &readfds);
      maxfd = std::max(maxfd, fd);
      listen_socks.push_back(*it);
    }
  }
}

static int op_conn(int fd, const Server::Conn conn) {
  return std::max(fd, conn->get_fd());
}

static int op_sock(int fd, const Server::Sock sock) {
  return std::max(fd, sock->get_fd());
}

void Server::clear_connection(ConnIterator conn_it) throw() {
  int cgifd = (*conn_it)->get_cgifd();
  if (cgifd != -1) {
    FD_CLR(cgifd, &readfds);
    FD_CLR(cgifd, &writefds);
  }
  (*conn_it)->clear();
}

Server::ConnIterator Server::remove_connection(ConnIterator conn_it) throw() {
  int fd = (*conn_it)->get_fd();
  int cgifd = (*conn_it)->get_cgifd();
  // After erase, conn_it is invalidated and conn is deleted.
  ConnIterator next_it = connections.erase(conn_it);  // no throw
  FD_CLR(fd, &readfds);
  FD_CLR(fd, &writefds);
  if (cgifd != -1) {
    FD_CLR(cgifd, &readfds);
    FD_CLR(cgifd, &writefds);
  }
  if (maxfd == std::max(fd, cgifd)) {
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
      update_fdset(*it);
    } else if ((*it)->is_timeout()) {
      Log::cinfo() << "Connection timeout: connfd(" << (*it)->get_fd() << ")"
                   << " port("
                   << (*it)->client_socket->socket->get_client_port() << ")"
                   << std::endl;
      it = remove_connection(it);  // always returns the next iterator
    } else {
      ++it;
    }
  }
}

void Server::accept(Sock sock) throw() {
  try {
    Conn conn(new Connection(sock->accept(), this));  // throwable
    connections.push_back(conn);                      // throwable
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
      !conn->client_socket->isBrokenPipe) {
    Log::cdebug() << "Monitor client socket for write" << std::endl;
    FD_SET(conn->get_fd(), &writefds);
  }
  maxfd = std::max(conn->get_fd(), maxfd);
  if (conn->get_cgifd() != -1) {
    FD_CLR(conn->get_cgifd(), &readfds);
    FD_CLR(conn->get_cgifd(), &writefds);
    if (conn->cgi_pid == -1) {
      return;
    }
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
    Log::cdebug() << "remove timeout connections" << std::endl;
    remove_timeout_connections();
    return -1;
  }
  return 0;
}

void Server::resume(ConnIterator conn_it) throw() {
  int ret;
  try {
    ret = (*conn_it)->resume();  // throwable
  } catch (std::exception& e) {
    // We don't send 500 error page to keep our life simple.
    Log::cfatal() << "Server::resume() failed: " << e.what() << std::endl;
    remove_connection(conn_it);
    return;
  }
  // If the connection is aborted, remove it.
  switch (ret) {
    case WSV_REMOVE:
      remove_connection(conn_it);
      return;
    case WSV_CLEAR:
      clear_connection(conn_it);
      break;
    case WSV_AGAIN:
    case WSV_WAIT:
      break;
  }
  // Update fdset
  update_fdset(*conn_it);
}

// Define a predicate functor to check if a socket is ready to accept.
struct AcceptReadyPredicate {
  const fd_set* rfds;
  explicit AcceptReadyPredicate(const fd_set* rfds) : rfds(rfds) {}
  bool operator()(Server::Sock sock) const throw() {
    return FD_ISSET(sock->get_fd(), rfds);
  }
};

// Define a predicate functor to check if a connection is ready to resume.
struct ResumeReadyPredicate {
  const fd_set* rfds;
  const fd_set* wfds;
  ResumeReadyPredicate(const fd_set* rfds, const fd_set* wfds)
      : rfds(rfds), wfds(wfds) {}
  bool operator()(Server::Conn conn) const throw() {
    int fd = conn->get_fd(), cgifd = conn->get_cgifd();
    // 1. CLIENT_SEND
    if (FD_ISSET(fd, wfds)) {
      Log::cdebug() << "ResumeReadyPredicate: CLIENT_SEND" << std::endl;
      conn->io_status = Connection::CLIENT_SEND;
      return true;
    }
    // 2. CGI_SEND
    // 3. CGI_RECV
    if (cgifd != -1 && conn->cgi_pid != -1) {
      if (FD_ISSET(cgifd, wfds)) {
        Log::cdebug() << "ResumeReadyPredicate: CGI_SEND" << std::endl;
        conn->io_status = Connection::CGI_SEND;
        return true;
      } else if (FD_ISSET(cgifd, rfds)) {
        Log::cdebug() << "ResumeReadyPredicate: CGI_RECV" << std::endl;
        conn->io_status = Connection::CGI_RECV;
        return true;
      }
    }
    // 4. CLIENT_RECV
    if (FD_ISSET(fd, rfds)) {
      Log::cdebug() << "ResumeReadyPredicate: CLIENT_RECV" << std::endl;
      conn->io_status = Connection::CLIENT_RECV;
      return true;
    }
    // 5. NO_IO
    Log::cdebug() << "ResumeReadyPredicate: NO_IO" << std::endl;
    conn->io_status = Connection::NO_IO;
    return false;
  }
};

void Server::run() throw() {
  while (1) {
    if (wait() < 0) {
      continue;
    }
    SockIterator sock_it;
    AcceptReadyPredicate apred(&ready_rfds);
    if ((sock_it = std::find_if(listen_socks.begin(), listen_socks.end(),
                                apred)) != listen_socks.end()) {
      accept(*sock_it);  // no throw
      continue;
    }
    ConnIterator conn_it;
    ResumeReadyPredicate rpred(&ready_rfds, &ready_wfds);
    if ((conn_it = std::find_if(connections.begin(), connections.end(),
                                rpred)) != connections.end()) {
      resume(conn_it);  // no throw
      continue;
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
