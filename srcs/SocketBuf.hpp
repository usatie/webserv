#ifndef SOCKETBUF_HPP
#define SOCKETBUF_HPP

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <fstream>
#include <iostream>
#include <string>
#define MAXLINE 1024
#include <fcntl.h>

#include "webserv.hpp"
#include "Socket.hpp"

class SocketBuf {
  // Member data
 public:
 private:
  Socket socket;
  std::vector<char> recvbuf, sendbuf;

  SocketBuf(const SocketBuf& other);

 public:
  // Constructor/Destructor
  SocketBuf(): socket(-1) {}
  SocketBuf(int fd): socket(fd) {}
  ~SocketBuf() {}

  // Accessors
  int get_fd() const { return socket.get_fd(); }
  bool isClosed() const { return socket.isClosed(); }
  bool isSendBufEmpty() const { return sendbuf.empty(); }

  // Member functions
  int initServer(int port, int backlog) {
    return socket.initServer(port, backlog);
  }

  int reuseaddr() {
    return socket.reuseaddr();
  }

  int bind(int port) {
    return socket.bind(port);
  }

  int listen(int backlog) {
    return socket.listen(backlog);
  }

  std::shared_ptr<SocketBuf> accept() {
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int client_fd = ::accept(socket.get_fd(), (struct sockaddr *)&addr, &addrlen);
    if (client_fd < 0) {
      std::cerr << "accept() failed\n";
      // TODO: handle error
      exit(EXIT_FAILURE);
    }
    return std::shared_ptr<SocketBuf>(new SocketBuf(client_fd));
  }

  int send(const char msg[], size_t len) {
    sendbuf.insert(sendbuf.end(), msg, msg + len);
    return 0;
  }

  int send_file(std::string filepath) {
    std::ifstream ifs(filepath.c_str(), std::ios::binary);

    if (!ifs.is_open()) {
      std::cerr << "file open failed\n";
      return -1;
    }
    sendbuf.insert(sendbuf.end(), std::istreambuf_iterator<char>(ifs),
                   std::istreambuf_iterator<char>());
    // Append CRLF to sendbuf
    sendbuf.push_back('\r');
    sendbuf.push_back('\n');
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
    //ssize_t ret = ::send(fd, &sendbuf[0], sendbuf.size(), SO_NOSIGPIPE);
    ssize_t ret = ::send(socket.get_fd(), &sendbuf[0], std::min(10, (int)sendbuf.size()), 0);
    if (ret < 0) {
      perror("send");
      std::cerr << "errno: " << errno << "\n";
      // TODO: handle EINTR
      // ETIMEDOUT, EPIPE in any case means the connection is closed
	  socket.beClosed();
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
  int fill() {
    static const int flags = 0;
    recvbuf.reserve(recvbuf.size() + MAXLINE);
    recvbuf.resize(recvbuf.size() + MAXLINE);
    ssize_t ret = ::recv(socket.get_fd(), \
        &recvbuf[recvbuf.size() - MAXLINE], MAXLINE, flags);
    if (ret < 0) {
      std::cerr << "recv() failed\n";
      return -1;
    }
    if (ret == 0) {
      socket.beClosed();
    }
    recvbuf.resize(recvbuf.size() - MAXLINE + ret);
    return ret;
  }

  int set_nonblock() {
    return socket.set_nonblock();
  }
};

#endif

