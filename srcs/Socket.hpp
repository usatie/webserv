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

  Socket(const Socket& other);
  Socket() : fd(-1), server_addr(), closed(false) {}

 public:
  // Constructor/Destructor
  Socket(int fd): fd(fd), server_addr(), closed(false) {}
  ~Socket() {
    if (::close(fd) < 0) {
      std::cerr << "close() failed\n";
    }
  }

  // Accessors
  int get_fd() const { return fd; }
  bool isClosed() const { return closed; }
  void beClosed() { closed = true; }

  // Member functions
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

  std::shared_ptr<Socket> accept() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int client_fd = ::accept(fd, (struct sockaddr *)&addr, &addrlen);
    if (client_fd < 0) {
      std::cerr << "accept() failed\n";
      // TODO: handle error
      exit(EXIT_FAILURE);
    }
    return std::shared_ptr<Socket>(new Socket(client_fd));
  }

  // int send(const char *msg, size_t len) {
    // sendbuf.insert(sendbuf.end(), msg, msg + len);
    // return -1;
  // }

  // int send_file(std::string filepath) {
    // std::ifstream ifs(filepath.c_str(), std::ios::binary);

    // if (!ifs.is_open()) {
    //   std::cerr << "file open failed\n";
    //   return -1;
    // }
    // sendbuf.insert(sendbuf.end(), std::istreambuf_iterator<char>(ifs),
    //                std::istreambuf_iterator<char>());
    // // Append CRLF to sendbuf
    // sendbuf.push_back('\r');
    // sendbuf.push_back('\n');
    // return -1;
  // }

  // Read line from buffer, if found, remove it from buffer and return 0
  // Otherwise, return -1
  // int readline(std::string &line) {
    // char prev = '\0', c;

    // for (size_t i = 0; i < recvbuf.size(); i++) {
    //   c = recvbuf[i];
    //   if (prev == '\r' && c == '\n') {
    //     line.assign(recvbuf.begin(), recvbuf.begin() + i - 1);  // Remove "\r\n"
    //     recvbuf.erase(recvbuf.begin(), recvbuf.begin() + i + 1);
    //     return 0;
    //   }
    //   prev = c;
    // }
  //   return -1;
  // }

  // Actually send data on socket
  // int flush() {
    // if (sendbuf.empty()) {
    //   return 0;
    // }
    // //ssize_t ret = ::send(fd, &sendbuf[0], sendbuf.size(), SO_NOSIGPIPE);
    // ssize_t ret = ::send(fd, &sendbuf[0], std::min(10, (int)sendbuf.size()), 0);
    // if (ret < 0) {
    //   perror("send");
    //   std::cerr << "errno: " << errno << "\n";
    //   // TODO: handle EINTR
    //   // ETIMEDOUT, EPIPE in any case means the connection is closed
    //   closed = true;
    //   return -1;
    // }
    // sendbuf.erase(sendbuf.begin(), sendbuf.begin() + ret);
    // return ret;
	// return -1;
  // }

  // void flushall() {
    // while (flush() > 0) ;
  // }

  // Actually receive data from socket
  // int fill() {
    // char buf[MAXLINE];
    // static const int flags = 0;
    // ssize_t ret = ::recv(fd, buf, sizeof(buf) - 1, flags);
    // if (ret < 0) {
    //   std::cerr << "recv() failed\n";
    //   return -1;
    // }
    // recvbuf.insert(recvbuf.end(), buf, buf + ret);
    // if (ret == 0) {
    //   closed = true;
    // }
    // return ret;
	// return -1;
  // }

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
};

#endif
