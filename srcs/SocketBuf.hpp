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

#include "Socket.hpp"
#include "webserv.hpp"

class SocketBuf {
  // Member data
 public:
 private:
  Socket socket;
  std::vector<char> recvbuf, sendbuf;

  SocketBuf(const SocketBuf& other);

 public:
  // Constructor/Destructor
  explicit SocketBuf(int listen_fd) : socket(listen_fd) {
    if (socket.set_nonblock() < 0) {
      throw std::runtime_error("socket.set_nonblock() failed");
    }
  }
  ~SocketBuf() throw() {}
  SocketBuf() throw();  // Do not implement this
  SocketBuf& operator=(
      const SocketBuf& other) throw();  // Do not implement this
  SocketBuf(SocketBuf& other) throw();  // Do not implement this

  // Accessors
  int get_fd() const throw() { return socket.get_fd(); }
  bool isClosed() const throw() { return socket.isClosed(); }
  bool isSendBufEmpty() const throw() { return sendbuf.empty(); }

  // Member functions
  // TODO: Make this noexcept
  int send(const char msg[], size_t len) {
    // try/catch
    sendbuf.insert(sendbuf.end(), msg, msg + len);
    return 0;
  }

  // TODO: Make this noexcept
  int send_file(std::string filepath) {
    std::ifstream ifs(filepath.c_str(), std::ios::binary);

    if (!ifs.is_open()) {
      Log::error("file open failed");
      return -1;
    }
    // try/catch
    sendbuf.insert(sendbuf.end(), std::istreambuf_iterator<char>(ifs),
                   std::istreambuf_iterator<char>());
    // TODO ifstreambuf_iterator cannot handle error
    // Append CRLF to sendbuf
    // try/catch
    sendbuf.push_back('\r');
    // try/catch
    sendbuf.push_back('\n');
    return 0;
  }

  // Read line from buffer, if found, remove it from buffer and return 0
  // Otherwise, return -1
  // TODO: make this noexcept
  int readline(std::string& line) {
    char prev = '\0', c;

    for (size_t i = 0; i < recvbuf.size(); i++) {
      c = recvbuf[i];
      if (prev == '\r' && c == '\n') {
        // try/catch
        line.assign(recvbuf.begin(), recvbuf.begin() + i - 1);  // Remove "\r\n"
        // `std::vector::erase` does not throw unless an exception is thrown by
        // the assignment operator of T.
        recvbuf.erase(recvbuf.begin(), recvbuf.begin() + i + 1);
        return 0;
      }
      prev = c;
    }
    return -1;
  }

  // Actually send data on socket
  int flush() throw() {
    if (sendbuf.empty()) {
      return 0;
    }
    // ssize_t ret = ::send(fd, &sendbuf[0], sendbuf.size(), SO_NOSIGPIPE);
    ssize_t ret = ::send(socket.get_fd(), &sendbuf[0],
                         std::min(10, (int)sendbuf.size()), 0);
    if (ret < 0) {
      Log::cerror() << "send() failed, errno: " << errno << "\n";
      // TODO: handle EINTR
      // ETIMEDOUT, EPIPE in any case means the connection is closed
      socket.beClosed();
      return -1;
    }
    // `std::vector::erase` does not throw unless an exception is thrown by the
    // assignment operator of T.
    sendbuf.erase(sendbuf.begin(), sendbuf.begin() + ret);
    return ret;
  }

  void flushall() throw() {
    while (flush() > 0)
      ;
  }

  // Actually receive data from socket
  // TODO: make this noexcept
  int fill() {
    static const int flags = 0;
    int prev_size = recvbuf.size();
    // try/catch
    recvbuf.resize(prev_size + MAXLINE);
    ssize_t ret = ::recv(socket.get_fd(), &recvbuf[prev_size], MAXLINE, flags);
    if (ret < 0) {
      Log::error("recv() failed");
      // won't throw because it will shrink
      recvbuf.resize(prev_size);
      return -1;
    }
    if (ret == 0) {
      socket.beClosed();
    }
    // won't throw because it will shrink
    recvbuf.resize(prev_size + ret);
    return ret;
  }

  void clear_sendbuf() { sendbuf.clear(); }

  int set_nonblock() { return socket.set_nonblock(); }
};

#endif
