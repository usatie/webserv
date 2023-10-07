#include "Socket.hpp"

#include <arpa/inet.h>  // inet_ntop

#include <sstream>

static int get_port(const struct sockaddr* addr) throw() {
  if (addr->sa_family == AF_INET6) {
    return ntohs(reinterpret_cast<const struct sockaddr_in6*>(addr)->sin6_port);
  } else if (addr->sa_family == AF_INET) {
    return ntohs(reinterpret_cast<const struct sockaddr_in*>(addr)->sin_port);
  } else {
    Log::error("Invalid socket family");
    return -1;
  }
}

int Socket::get_server_port() const throw() {
  return get_port(reinterpret_cast<const struct sockaddr*>(&saddr));
}

int Socket::get_client_port() const throw() {
  return get_port(reinterpret_cast<const struct sockaddr*>(&caddr));
}

std::string Socket::get_server_port_string() const {  // throwable
  std::stringstream ss;
  ss.exceptions(std::ios::failbit | std::ios::badbit);
  ss << get_server_port();
  return ss.str();
}

std::string Socket::get_client_ip_address() const {  // throwable
  char ipstr[INET6_ADDRSTRLEN];
  if (caddr.ss_family == AF_INET6) {
    inet_ntop(AF_INET6,
              &reinterpret_cast<const struct sockaddr_in6*>(&caddr)->sin6_addr,
              ipstr, sizeof(ipstr));
  } else if (caddr.ss_family == AF_INET) {
    inet_ntop(AF_INET,
              &reinterpret_cast<const struct sockaddr_in*>(&caddr)->sin_addr,
              ipstr, sizeof(ipstr));
  } else {
    Log::error("Invalid socket family");
    return "";
  }
  return std::string(ipstr);
}

util::shared_ptr<Socket> Socket::accept() {  // throwable
  struct sockaddr_storage new_caddr;
  socklen_t new_caddrlen = sizeof(new_caddr);
  int connfd = ::accept(fd, reinterpret_cast<struct sockaddr*>(&new_caddr),
                        &new_caddrlen);
  static unsigned int cnt = 0;
  if (connfd < 0) {
    Log::cfatal() << "accept() failed. (" << errno << ": " << strerror(errno)
                  << ")" << std::endl;
    throw std::runtime_error("accept() failed");
  }
  cnt++;
  Log::cinfo() << cnt << "th connection accepted: "
               << "connfd(" << connfd << "), port("
               << get_port(reinterpret_cast<struct sockaddr*>(&new_caddr))
               << ")" << std::endl;
  // If allocation failed, must close connfd
  util::shared_ptr<Socket> connsock;
  try {
    connsock = util::shared_ptr<Socket>(
        new Socket(connfd, reinterpret_cast<struct sockaddr*>(&saddr),
                   reinterpret_cast<struct sockaddr*>(&new_caddr), saddrlen,
                   new_caddrlen));
    // connsock->set_nolinger(0);
    return connsock;
  } catch (std::exception& e) {
    Log::fatal("new Socket() failed");
    if (::close(connfd) < 0) {
      Log::cfatal() << "close() failed. (" << errno << ": " << strerror(errno)
                    << ")" << std::endl;
    }
    throw;
  }
}

int Socket::reuseaddr() throw() {
  int optval = 1;
  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    Log::cfatal() << "setsockopt(SO_REUSEADDR) failed. (" << errno << ": "
                  << strerror(errno) << ")" << std::endl;
    return -1;
  }
  return 0;
}

// Set IPv6 only
int Socket::ipv6only() throw() {
  int optval = 1;
  if (setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &optval, sizeof(optval)) < 0) {
    Log::cfatal() << "setsockopt(IPV6_V6ONLY) failed. (" << errno << ": "
                  << strerror(errno) << ")" << std::endl;
    return -1;
  }
  return 0;
}

int Socket::bind(struct sockaddr* addr, socklen_t addrlen) throw() {
  if (::bind(fd, addr, addrlen) < 0) {
    Log::cfatal() << "bind to " << get_port(addr) << " failed."
                  << "(" << errno << ": " << strerror(errno) << ")"
                  << std::endl;
    return -1;
  }
  memcpy(&this->saddr, addr, addrlen);
  this->saddrlen = addrlen;
  return 0;
}

int Socket::listen(int backlog) throw() {
  if (::listen(fd, backlog) < 0) {
    Log::cfatal() << "listen() failed. (" << errno << ": " << strerror(errno)
                  << ")" << std::endl;
    return -1;
  }
  return 0;
}

int Socket::set_nonblock() throw() {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags < 0) {
    Log::cfatal() << "fcntl() failed. (" << errno << ": " << strerror(errno)
                  << ")" << std::endl;
    return -1;
  }
  if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    Log::cfatal() << "fcntl() failed. (" << errno << ": " << strerror(errno)
                  << ")" << std::endl;
    return -1;
  }
  return 0;
}

// cppcheck-suppress unusedFunction
int Socket::set_nolinger(int linger) throw() {
  struct linger l;
  l.l_onoff = 1;
  l.l_linger = linger;
  if (setsockopt(fd, SOL_SOCKET, SO_LINGER, &l, sizeof(l)) < 0) {
    Log::cfatal() << "setsockopt(SO_LINGER) failed. (" << errno << ": "
                  << strerror(errno) << ")" << std::endl;
    return -1;
  }
  return 0;
}
