#ifndef SOCKET_HPP
# define SOCKET_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

class Socket {
public:
  Socket(): fd(-1), server_addr(), client_addr(), client_addrlen() {
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) {
      std::cerr << "socket() failed\n";
      throw;
    }
  }
  Socket(int fd, struct sockaddr_in addr, socklen_t addrlen): fd(fd), server_addr(), client_addr(addr), client_addrlen(addrlen) {}
  ~Socket() {
    if (::close(fd) < 0) {
      std::cerr << "close() failed\n";
    }
  }
  void reuseaddr() {
    int optval = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
      std::cerr << "setsockopt() failed\n";
      throw;
    }
  }
  void bind(int port) {
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (::bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      std::cerr << "bind() failed\n";
      throw;
    }
  }
  void listen(int backlog) {
    if (::listen(fd, backlog) < 0) {
      std::cerr << "listen() failed\n";
      throw;
    }
  }
  Socket accept() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int client_fd = ::accept(fd, (struct sockaddr *)&addr, &addrlen);
    if (client_fd < 0) {
      std::cerr << "accept() failed\n";
      throw;
    }
    return Socket(client_fd, addr, addrlen);
  }
  int send(const void *buf, size_t len) {
    static const int flags = 0;
    return ::send(fd, buf, len, flags);
  }
  // TODO: Resolve client address and name
  void resolve() {
    (void)client_addr;
    (void)client_addrlen;
  }
  int send_file(std::string filepath);
  int readline(std::string &ptr) {
    char prev = '\0', c;
    ssize_t rc;

    while (1) {
      if ((rc = read(fd, &c, 1)) == 1) {
        ptr += c;
        if (prev == '\r' && c == '\n') {
          ptr.pop_back();
          ptr.pop_back();
          return 0;
        }
        prev = c;
      } else {
        return -1;
      }
    }
  }
private:
  int fd;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  socklen_t client_addrlen;
};

#endif
