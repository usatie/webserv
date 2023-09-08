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
  // Member data
 public:
 private:
  int fd;
  struct sockaddr_in server_addr;
  bool closed;

  Socket(const Socket& other) throw();             // Do not implement this
  Socket& operator=(const Socket& other) throw();  // Do not implement this

 public:
  // Constructor/Destructor
  Socket() : server_addr(), closed(false) {
    if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
      Log::fatal("socket() failed");
      throw std::runtime_error("socket() failed");
    }
  }
  
  explicit Socket(int fd) : fd(fd), server_addr(), closed(false) {
    if (fd < 0) {
      Log::error("Invalid socket fd to construct Socket");
      throw std::runtime_error("Invalid socket fd");
    }
  }

  ~Socket() throw() {
    if (::close(fd) < 0) {
      Log::error("close() failed");
    }
  }

  // Accessors
  int get_fd() const throw() { return fd; }
  bool isClosed() const throw() { return closed; }
  void beClosed() throw() { closed = true; }

  // Member functions
  int reuseaddr() throw() {
    int optval = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
      Log::error("setsockopt() failed");
      return -1;
    }
    return 0;
  }

  int bind(int port) throw() {
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (::bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
      Log::error("bind() failed");
      return -1;
    }
    return 0;
  }

  int listen(int backlog) throw() {
    if (::listen(fd, backlog) < 0) {
      Log::error("listen() failed");
      return -1;
    }
    return 0;
  }

  int set_nonblock() throw() {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
      Log::error("fcntl() failed");
      return -1;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
      Log::error("fcntl() failed");
      return -1;
    }
    return 0;
  }
};

#endif
