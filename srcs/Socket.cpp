#include "Socket.hpp"

static int get_port(struct sockaddr_storage& addr) throw() {
  if (addr.ss_family == AF_INET6) {
    return ntohs(((struct sockaddr_in6*)&addr)->sin6_port);
  } else if (addr.ss_family == AF_INET) {
    return ntohs(((struct sockaddr_in*)&addr)->sin_port);
  } else {
    Log::error("Invalid socket family");
    return -1;
  }
}

int Socket::get_server_port() throw() { return get_port(saddr); }

int Socket::get_client_port() throw() { return get_port(caddr); }

util::shared_ptr<Socket> Socket::accept() {  // throwable
  struct sockaddr_storage caddr;
  socklen_t caddrlen = sizeof(caddr);
  int connfd = ::accept(fd, (struct sockaddr*)&caddr, &caddrlen);
  static unsigned int cnt = 0;
  cnt++;
  Log::cinfo() << cnt << "th connection accepted: "
               << "connfd(" << connfd << "), port(" << get_port(caddr) << ")"
               << std::endl;
  if (connfd < 0) {
    Log::error("accept() failed");
    throw std::runtime_error("accept() failed");
  }
  // If allocation failed, must close connfd
  util::shared_ptr<Socket> connsock;
  try {
    connsock = util::shared_ptr<Socket>(
        new Socket(connfd, (struct sockaddr*)&saddr, (struct sockaddr*)&caddr,
                   saddrlen, caddrlen));
    // connsock->set_nolinger(0);
    return connsock;
  } catch (std::exception& e) {
    Log::error("new Socket() failed");
    if (::close(connfd) < 0) {
      Log::error("close() failed");
    }
    throw e;
  }
}
