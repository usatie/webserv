#ifndef SOCKETBUF_HPP
#define SOCKETBUF_HPP

#include <sstream>

#include "Socket.hpp"
#include "webserv.hpp"

class SocketBuf {
  class StreamCleaner {
   private:
    std::stringstream &rss, &wss;

   public:
    StreamCleaner(std::stringstream& rss, std::stringstream& wss)
        : rss(rss), wss(wss) {}
    ~StreamCleaner() {
      if (rss.bad() || wss.bad()) return;
      rss.clear();
      wss.clear();
    }
  };
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
  explicit SocketBuf(int fd) : socket(fd), rss(), wss() {
    if (socket.set_nonblock() < 0) {
      throw std::runtime_error("socket.set_nonblock() failed");
    }
  }
  ~SocketBuf() throw() {}

  // Accessors
  int get_fd() const throw() { return socket.get_fd(); }
  bool isClosed() const throw() { return socket.isClosed(); }
  // tellg() updates the internal state of the stream, so it is not const
  bool isSendBufEmpty() throw() {
    return (wss.str().size() - wss.tellg()) == 0;
  }
  bool bad() const throw() { return rss.bad() || wss.bad(); }

  // Member functions
  void setbadstate() throw() {
    Log::debug("setbadstate()");
    rss.setstate(rss.rdstate() | std::ios::badbit);
    wss.setstate(wss.rdstate() | std::ios::badbit);
  }
  int send_file(const std::string& filepath) throw();

  int readline(std::string& line) throw();

  // Read line from buffer, if found, remove it from buffer and return 0
  // Otherwise, return -1
  int read_telnet_line(std::string& line) throw();

  ssize_t read(char* buf, size_t size) throw();

  // Actually send data on socket
  int flush() throw();

  // Actually receive data from socket
  int fill() throw();

  void clear_sendbuf() throw() {
    wss.str("");
    wss.clear();
  }

  int set_nonblock() throw() { return socket.set_nonblock(); }

  template <typename T>
  SocketBuf& operator<<(const T& t) throw() {
    StreamCleaner _(rss, wss);
    if (bad()) {
      return *this;
    }
    wss << t;
    if (wss.fail()) {
      Log::fatal("wss << msg failed");
      setbadstate();
    }
    return *this;
  }

  SocketBuf& write(const char* buf, size_t size) throw();

  size_t getReadBufSize() throw() {
    StreamCleaner _(rss, wss);
    if (bad()) {
      return 0;
    }
    return rss.str().size() - rss.tellg();
  }

  SocketBuf& operator>>(SocketBuf& dst) throw() {
    StreamCleaner _(rss, wss);
    if (bad()) {
      return *this;
    }
    dst << rss.rdbuf();
    if (rss.fail()) {
      Log::fatal("dst << rss.rdbuf() failed");
      setbadstate();
    }
    return *this;
  }
};

#endif