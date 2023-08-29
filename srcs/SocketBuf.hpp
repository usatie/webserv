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
  std::stringstream rss, wss;

  SocketBuf() throw();  // Do not implement this
  SocketBuf& operator=(
      const SocketBuf& other) throw();  // Do not implement this
  SocketBuf(SocketBuf& other) throw();  // Do not implement this

 public:
  // Constructor/Destructor
  explicit SocketBuf(int listen_fd)
      : socket(listen_fd), rss(), wss() {
    if (socket.set_nonblock() < 0) {
      throw std::runtime_error("socket.set_nonblock() failed");
    }
  }
  ~SocketBuf() throw() {}

  // Accessors
  int get_fd() const throw() { return socket.get_fd(); }
  bool isClosed() const throw() { return socket.isClosed(); }
  // tellg() updates the internal state of the stream, so it is not const
  bool isSendBufEmpty() throw() { return (wss.str().size() - wss.tellg()) == 0; }
  bool bad() const throw() { return rss.bad() || wss.bad(); }

  // Member functions
  void setbadstate() throw() {
    rss.setstate(rss.rdstate() | std::ios::badbit);
    wss.setstate(wss.rdstate() | std::ios::badbit);
  }
  int send_file(std::string filepath) throw() {
    if (bad()) {
      return -1;
    }
    std::ifstream ifs(filepath.c_str(), std::ios::binary);

    if (!ifs.is_open()) {
      Log::fatal("file open failed");
      return -1;
    }
    wss << ifs.rdbuf() << CRLF;
    if (wss.bad()) {
      Log::fatal("wss << ifs.rdbuf() << CRLF failed");
      setbadstate();
      return -1;
    }
    return 0;
  }

  // Read line from buffer, if found, remove it from buffer and return 0
  // Otherwise, return -1
  int readline(std::string& line) throw() {
    if (bad()) {
      return -1;
    }
    // TODO: handle eofbit and failbit, they can be set by getline
    // This is temporary solution, we need unit tests!
    rss.setstate(rss.rdstate() & ~std::ios::eofbit);
    rss.setstate(rss.rdstate() & ~std::ios::failbit);
    try {
      // If there is no LF in buffer, return -1
      if (!std::getline(rss, line, LF)) {
        return -1;
      }
      // no LF found
      if (rss.eof()) {
        rss.seekg(-line.size(), std::ios::cur);
        if (!std::getline(rss, line, CR)) {
          return -1;
        }
        // If no CR nor LF found, put the line back to buffer
        if (rss.eof()) {
          Log::debug("EOF found before CR nor LF");
          rss.seekg(-line.size(), std::ios::cur);
          return -1;
        }
        // If CR found, return 0;
        return 0;
      }
      // If CRLF found, remove CR
      if (line.back() == CR) {
        line.pop_back();
      }
      // If LF found, return 0
      return 0;
    } catch (std::exception& e) {
      Log::fatal("getline() failed");
      setbadstate();
      return -1;
    }
  }

  // Actually send data on socket
  int flush() throw() {
    if (bad()) {
      return -1;
    }
    if (isSendBufEmpty()) {
      return 0;
    }
    try {
      std::string buf(wss.str());
      ssize_t ret =
          ::send(socket.get_fd(), buf.c_str(), buf.size(), SO_NOSIGPIPE);
      if (ret < 0) {
        Log::cerror() << "send() failed, errno: " << errno << "\n";
        // TODO: handle EINTR
        // ETIMEDOUT, EPIPE in any case means the connection is closed
        socket.beClosed();
        return -1;
      }
      wss.seekg(ret, std::ios::cur);
      return ret;
    } catch (std::exception& e) {
      Log::fatal("wss.str() failed");
      setbadstate();
      return -1;
    }
  }

  // Actually receive data from socket
  int fill() throw() {
    if (bad()) {
      return -1;
    }
    static char buf[MAXLINE] = {0};
    static const int flags = 0;
    ssize_t ret = ::recv(socket.get_fd(), (void *)buf, MAXLINE, flags);
    if (ret < 0) {
      Log::error("recv() failed");
      return -1;
    }
    if (ret == 0) {
      socket.beClosed();
    }
    // Fill ret bytes buf into rss
    try {
      rss << std::string(buf, ret);
      return ret;
    } catch (std::exception& e) {
      Log::fatal("rss << std::string(buf, ret) failed");
      setbadstate();
      return -1;
    }
  }

  void clear_sendbuf() throw() { wss.clear(); }

  int set_nonblock() throw() { return socket.set_nonblock(); }

  template <typename T>
  SocketBuf& operator<<(const T& t) throw() {
    if (bad()) {
      return *this;
    }
    wss << t ;
    if (wss.fail()) {
      Log::fatal("wss << msg failed");
      setbadstate();
    }
    return *this;
  }
};

#endif
