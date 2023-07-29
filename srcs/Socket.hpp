#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#define MAXLINE 1024
#include <fcntl.h>

#include "webserv.hpp"

class Socket {
 public:
  Socket() : fd(-1), server_addr(), client_addr(), client_addrlen() {}
  Socket(int fd, struct sockaddr_in addr, socklen_t addrlen)
      : fd(fd), server_addr(), client_addr(addr), client_addrlen(addrlen) {}
  ~Socket() {
    if (::close(fd) < 0) {
      std::cerr << "close() failed\n";
    }
  }

  int initServer(int port, int backlog) {
    if ( (fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      std::cerr << "socket() failed\n";
      return -1;
    }
    if (reuseaddr() < 0) {
      return -1;
    }
    if (bind(port) < 0) {
      return -1;
    }
    if (listen(backlog) < 0) {
      return -1;
    }
    if (set_nonblock() < 0) {
      return -1;
    }
    return 0;
  }

  int reuseaddr() {
    int optval = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
      std::cerr << "setsockopt() failed\n";
      return -1;
    }
    return 0;
  }

  int bind(int port) {
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (::bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
      std::cerr << "bind() failed\n";
      return -1;
    }
    return 0;
  }

  int listen(int backlog) {
    if (::listen(fd, backlog) < 0) {
      std::cerr << "listen() failed\n";
      return -1;
    }
    return 0;
  }

  Socket *accept() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int client_fd = ::accept(fd, (struct sockaddr *)&addr, &addrlen);
    if (client_fd < 0) {
      std::cerr << "accept() failed\n";
      // TODO: handle error
      exit(EXIT_FAILURE);
    }
    return new Socket(client_fd, addr, addrlen);
  }

  // TODO: Resolve client address and name
  void resolve() {
    (void)client_addr;
    (void)client_addrlen;
  }

  int send(const char *msg, size_t len) {
    sendbuf.insert(sendbuf.end(), msg, msg + len);
    return 0;
  }

  int send_file(std::string filepath) {
    std::ifstream ifs(filepath);

    if (!ifs.is_open()) {
      std::cerr << "file open failed\n";
      return -1;
    }
    sendbuf.insert(sendbuf.end(), std::istreambuf_iterator<char>(ifs),
                   std::istreambuf_iterator<char>());
    std::string line = "\r\n";
    // Append line to sendbuf
    sendbuf.insert(sendbuf.end(), line.begin(), line.end());
    return 0;
  }

  // Read line from buffer, if found, remove it from buffer and return 0
  // Otherwise, return -1
  int readline(std::string &line) {
    char prev = '\0', c;

    for (size_t i = 0; i < recvbuf.size(); i++) {
      c = recvbuf[i];
      if (prev == '\r' && c == '\n') {
        line.assign(recvbuf.begin(), recvbuf.begin() + i - 1);  // Remove "\r\n"
        recvbuf.erase(recvbuf.begin(), recvbuf.begin() + i + 1);
        return 0;
      }
      prev = c;
    }
    return -1;
  }

  // Actually send data on socket
  int flush() {
    if (sendbuf.empty()) {
      return 0;
    }
    ssize_t ret = ::send(fd, &sendbuf[0], sendbuf.size(), SO_NOSIGPIPE);
    if (ret < 0) {
      std::cerr << "send() failed\n";
      return -1;
    }
    sendbuf.erase(sendbuf.begin(), sendbuf.begin() + ret);
    return ret;
  }

  void flushall() {
    while (flush() > 0)
      ;
  }

  // Actually receive data from socket
  int recv() {
    char buf[MAXLINE];
    static const int flags = 0;
    ssize_t ret = ::recv(fd, buf, sizeof(buf) - 1, flags);
    if (ret < 0) {
      std::cerr << "recv() failed\n";
      return -1;
    }
    recvbuf.insert(recvbuf.end(), buf, buf + ret);
    if (ret == 0) {
      closed = true;
    }
    return ret;
  }

  int set_nonblock() {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
      std::cerr << "fcntl() failed\n";
      return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
      std::cerr << "fcntl() failed\n";
      return -1;
    }
    return 0;
  }

  std::vector<char> recvbuf, sendbuf;

  int get_fd() { return fd; }
  bool closed;

 private:
  int fd;
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;
  socklen_t client_addrlen;
};

#endif
