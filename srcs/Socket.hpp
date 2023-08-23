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
      std::cerr << "socket() failed\n";
      throw std::runtime_error("socket() failed");
    }
  }
  explicit Socket(int listen_fd) : server_addr(), closed(false) {
    if ((fd = accept(listen_fd, NULL, NULL)) < 0) {
      std::cerr << "accept() failed\n";
      throw std::runtime_error("accept() failed");
    }
  }
  ~Socket() throw() {
    if (::close(fd) < 0) {
      std::cerr << "close() failed\n";
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
      std::cerr << "setsockopt() failed\n";
      return -1;
    }
    return 0;
  }

  int bind(int port) throw() {
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if (::bind(fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
      std::cerr << "bind() failed\n";
      return -1;
    }
    return 0;
  }

  int listen(int backlog) throw() {
    if (::listen(fd, backlog) < 0) {
      std::cerr << "listen() failed\n";
      return -1;
    }
    return 0;
  }

  int set_nonblock() throw() {
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
};

#endif
