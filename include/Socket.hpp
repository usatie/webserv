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
  bool closed;
  // saddr is for listening socket
  // caddr is for connection socket
  struct sockaddr_storage saddr, caddr;
  socklen_t saddrlen, caddrlen;

  Socket(const Socket& other) throw();             // Do not implement this
  Socket& operator=(const Socket& other) throw();  // Do not implement this

 public:
  // Constructor/Destructor
  // Constructor for listening socket and unix domain socket
  explicit Socket(int fd) : fd(fd), closed(false), saddr(), caddr(), saddrlen(0), caddrlen(0) {
    if (fd < 0) {
      Log::error("Invalid socket fd to construct Socket");
      throw std::runtime_error("Invalid socket fd");
    }
  }
  // Constructor for TCP connection socket
  Socket(int fd, struct sockaddr* saddr, struct sockaddr* caddr, socklen_t saddrlen, socklen_t caddrlen): fd(fd), closed(false), saddr(), caddr(), saddrlen(saddrlen), caddrlen(caddrlen) {
    if (fd < 0) {
      Log::error("Invalid socket fd to construct Socket");
      throw std::runtime_error("Invalid socket fd");
    }
    memcpy(&this->saddr, saddr, saddrlen);
    memcpy(&this->caddr, caddr, caddrlen);
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

  int bind(struct sockaddr* addr, socklen_t addrlen) throw() {
    if (::bind(fd, addr, addrlen) < 0) {
      Log::error("bind() failed");
      return -1;
    }
    memcpy(&this->saddr, addr, addrlen);
    this->saddrlen = addrlen;
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

  // This is a kind of constructor, so it is THROWABLE
  //  std::runtime_error if accept() failed
  //  std::bad_alloc if new Socket() failed
  std::shared_ptr<Socket> accept() {
    struct sockaddr_storage caddr;
    socklen_t caddrlen = sizeof(caddr);
    int connfd = ::accept(fd, (struct sockaddr*)&caddr, &caddrlen);
    if (connfd < 0) {
      Log::error("accept() failed");
      throw std::runtime_error("accept() failed");
    }
    // If allocation failed, must close connfd
    std::shared_ptr<Socket> connsock;
    try {
      connsock = std::shared_ptr<Socket>(new Socket(connfd, (struct sockaddr*)&saddr, (struct sockaddr*)&caddr, saddrlen, caddrlen));
      return connsock;
    } catch (std::bad_alloc& e) {
      Log::error("new Socket() failed");
      if (::close(connfd) < 0) {
        Log::error("close() failed");
      }
      throw e;
    }
  }
};

#endif
