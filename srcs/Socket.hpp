#ifndef SOCKET_HPP
#define SOCKET_HPP

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>  // memcpy
#include <fstream>
#include <iostream>
#include <string>

#include "Log.hpp"
#include "webserv.hpp"

class Socket {
  // Member data
 public:
 private:
  int fd;
  bool closed;

 public:
  // saddr is for listening socket
  // caddr is for connection socket
  struct sockaddr_storage saddr, caddr;
  socklen_t saddrlen, caddrlen;

 private:
  Socket(const Socket& other) throw();             // Do not implement this
  Socket& operator=(const Socket& other) throw();  // Do not implement this

 public:
  // Constructor/Destructor
  // Constructor for listening socket and unix domain socket
  explicit Socket(int fd)
      : fd(fd), closed(false), saddr(), caddr(), saddrlen(0), caddrlen(0) {
    if (fd < 0) {
      Log::error("Invalid socket fd to construct Socket");
      throw std::runtime_error("Invalid socket fd");
    }
  }
  // Constructor for TCP connection socket
  Socket(int fd, struct sockaddr* saddr, struct sockaddr* caddr,
         socklen_t saddrlen, socklen_t caddrlen)
      : fd(fd),
        closed(false),
        saddr(),
        caddr(),
        saddrlen(saddrlen),
        caddrlen(caddrlen) {
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
  int get_server_port() throw();
  int get_client_port() throw();
  std::string get_server_ip_address();  // throwable
  std::string get_client_ip_address();  // throwable

  // Member functions
  int reuseaddr() throw();
  int ipv6only() throw();
  int bind(struct sockaddr* addr, socklen_t addrlen) throw();
  int listen(int backlog) throw();
  int set_nonblock() throw();
  int set_nolinger(int linger) throw();
  // This is a kind of constructor, so it is THROWABLE
  // However, it has a basic guarantee that it will not leak fd
  //  std::runtime_error if accept() failed
  //  std::bad_alloc if new Socket() failed
  util::shared_ptr<Socket> accept();
};

#endif
