#ifndef SOCKET_HPP
# define SOCKET_HPP

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <string>

class FatalError: public std::exception {
public:
  FatalError(std::string msg) throw(): msg(msg) {}
  ~FatalError() throw() {}
  virtual const char *what() const throw() {
    return msg.c_str();
  }
private:
  std::string msg;
};

class Socket {
public:
  Socket(): fd(-1), server_addr(), client_addr(), client_addrlen() {}
  Socket(int fd, struct sockaddr_in addr, socklen_t addrlen): fd(fd), server_addr(), client_addr(addr), client_addrlen(addrlen) {}
  ~Socket() {
    if (::close(fd) < 0) {
      std::cerr << "close() failed\n";
    }
  }
  void initServer(int port, int backlog) {
    fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd < 0) {
      throw FatalError("socket() failed");
    }
    reuseaddr();
    bind(port);
    listen(backlog);
  }
  void reuseaddr() {
    int optval = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
      throw FatalError("setsockopt() failed");
    }
  }
  void bind(int port) {
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (::bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      throw FatalError("bind() failed");
    }
  }
  void listen(int backlog) {
    if (::listen(fd, backlog) < 0) {
      throw FatalError("listen() failed");
    }
  }
  Socket accept() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int client_fd = ::accept(fd, (struct sockaddr *)&addr, &addrlen);
    // TODO: handle error
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
  int send_file(std::string filepath) {
    std::ifstream ifs(filepath);
    std::size_t sent = 0;
    ssize_t ret = 0;
    std::string line;

    if (!ifs.is_open()) {
      std::cerr << "file open failed\n";
      return -1;
    }
    while (std::getline(ifs, line)) {
      // TODO: portable newline
      line += "\n";
      ret = send(line.c_str(), line.size());
      if (ret < 0) {
        std::cerr << "send() failed\n";
        return -1;
      }
      // TODO: handle partial send
      sent += ret;
    }
    ret = send("\r\n", 1);
    if (ret < 0) {
      std::cerr << "send() failed\n";
      return -1;
    }
    // TODO: handle partial send
    return sent + ret;
  }

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
