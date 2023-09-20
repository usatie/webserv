#include "Server.hpp"

#include <arpa/inet.h>
#include <netdb.h>

#include <cerrno>
#include <cstring>  // memset, strerror

#include "Config.hpp"
#include "Connection.hpp"

std::ostream& operator<<(std::ostream& os, const struct addrinfo* rp);

Server::~Server() throw() {}

Server::Server(const Config& cf)
    : maxfd(-1), listen_socks(), connections(), cf(cf) {
  FD_ZERO(&readfds);
  FD_ZERO(&writefds);
  int backlog = BACKLOG;
  for (unsigned int i = 0; i < cf.http.servers.size(); ++i) {
    Log::cdebug() << "i: " << i << std::endl;
    const Config::Server& server = cf.http.servers[i];
    for (unsigned int j = 0; j < server.listens.size(); ++j) {
      Log::cdebug() << "j: " << j << std::endl;
      const Config::Listen& listen = server.listens[j];
      const char* host =
          (listen.address == "*") ? NULL : listen.address.c_str();
      int port = listen.port;
      std::stringstream ss;
      ss << port;
      std::string port_str = ss.str();
      const char* service = port_str.c_str();
      int type = SOCK_STREAM;
      {
        struct addrinfo hints;
        struct addrinfo *result, *rp;
        int sfd, s;

        Log::debug("memset()");
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
        if (listen.address == "*") {
          hints.ai_family = AF_INET; /* Allows IPv4 only */
        }
        hints.ai_flags = AI_PASSIVE; /* Wildcard IP address */
        hints.ai_socktype = type;

        Log::debug("getaddrinfo()");
        s = getaddrinfo(host, service, &hints, &result);
        if (s != 0) {
          errno = ENOSYS;
          throw std::runtime_error("getaddrinfo() failed");
        }

        /* Walk through returned list until we find an address structure
         * that can be used to successfully connect a socket */
        for (rp = result; rp != NULL; rp = rp->ai_next) {
          sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
          if (sfd == -1) {
            Log::error("socket() failed");
            continue; /* On error, try next address */
          }
          // Don't handle exception to close the socket, because if the
          // construct of sock fails, Server can't be constructed and thus the
          // program terminates.
          util::shared_ptr<Socket> sock(new Socket(sfd));
          if (sock->reuseaddr() < 0) {
            throw std::runtime_error("sock.set_reuseaddr() failed");
          }
          Log::cdebug() << "listen : " << listen << std::endl;
          /* convert ai_addr from binary to string */
          Log::cdebug() << "ip: " << rp << std::endl;
          // IPv6 socket listening on a wildcard address [::] will accept only
          // IPv6 connections Without this option, it will accept both IPv4 and
          // IPv6 connections
          if (rp->ai_family == AF_INET6) {
            sock->ipv6only();
          }
          if (sock->bind(rp->ai_addr, rp->ai_addrlen) < 0) {
            // If wildcard address, it's possible IPv6 socket is already
            // bound to IPv4 wildcard address. In that case, bind() fails.
            // https://www.geekpage.jp/blog/?id=2017-3-8-1
            //
            // This case is not an error, so we continue to try other
            if (errno == EADDRINUSE) {
              Log::cdebug() << "EADDRINUSE" << std::endl;
              continue;
            }
            // Other case, such as invalid address, is an error
            Log::cfatal() << "bind to " << rp << ":" << port << " failed."
                          << "(" << errno << ": " << strerror(errno) << ")"
                          << std::endl;
            throw std::runtime_error("sock.bind() failed");
          }
          if (sock->listen(backlog) < 0) {
            throw std::runtime_error("sock.listen() failed");
          }
          if (sock->set_nonblock() < 0) {
            throw std::runtime_error("sock.set_nonblock() failed");
          }
          // Save the listening ip address to Config::Listen
          // Here we use const_cast to modify the const object.
          {
            memcpy(const_cast<struct sockaddr_storage*>(&listen.addr),
                   rp->ai_addr, rp->ai_addrlen);
            const_cast<socklen_t&>(listen.addrlen) = rp->ai_addrlen;
          }
          FD_SET(sock->get_fd(), &readfds);
          maxfd = std::max(maxfd, sock->get_fd());
          listen_socks.push_back(sock);
          break;  // Success, one socket per one listen directive
        }
        freeaddrinfo(result);
      }
    }
  }
}

void Server::remove_connection(
    util::shared_ptr<Connection> connection) throw() {
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
util::shared_ptr<Socket> Server::get_ready_socket() throw() {
  for (SockIterator it = listen_socks.begin(); it != listen_socks.end(); ++it) {
    if (FD_ISSET((*it)->get_fd(), &ready_rfds)) {
      return *it;
    }
  }
  return util::shared_ptr<Socket>(NULL);
}
// Logically it is not const because it returns a non-const pointer.
util::shared_ptr<Connection> Server::get_ready_connection() throw() {
  // TODO: equally distribute the processing time to each connection
  for (ConnIterator it = connections.begin(); it != connections.end(); ++it) {
    if (canResume(*it)) {
      return *it;
    }
  }
  return util::shared_ptr<Connection>(NULL);
}
void Server::process() throw() {
  if (wait() < 0) {
    return;
  }
  util::shared_ptr<Connection> conn;
  util::shared_ptr<Socket> sock;
  if ((sock = get_ready_socket()) != NULL) {
    accept(sock);
  } else if ((conn = get_ready_connection()) != NULL) {
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
