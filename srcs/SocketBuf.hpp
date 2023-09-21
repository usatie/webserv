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
  util::shared_ptr<Socket> socket;
  std::stringstream rss, wss;

  SocketBuf() throw();  // Do not implement this
  SocketBuf& operator=(
      const SocketBuf& other) throw();  // Do not implement this
  SocketBuf(SocketBuf& other) throw();  // Do not implement this

 public:
  bool hasReceivedEof;
  // Constructor/Destructor
  // Constructor for TCP socket
  explicit SocketBuf(util::shared_ptr<Socket> socket)
      : socket(socket), rss(), wss(), hasReceivedEof(false) {
    if (socket->set_nonblock() < 0) {
      throw std::runtime_error("socket->set_nonblock() failed");
    }
  }
  // Constructor for unix domain socket
  explicit SocketBuf(int fd)
      : socket(util::shared_ptr<Socket>(new Socket(fd))),
        rss(),
        wss(),
        hasReceivedEof(false) {
    if (socket->set_nonblock() < 0) {
      throw std::runtime_error("socket->set_nonblock() failed");
    }
  }
  ~SocketBuf() throw() {}

  // Accessors
  int get_fd() const throw() { return socket->get_fd(); }
  bool isClosed() const throw() { return socket->isClosed(); }
  //  tellg() updates the internal state of the stream, so it is not const
  bool isSendBufEmpty() throw() {
    return (wss.str().size() - wss.tellg()) == 0;
  }
  bool isRecvBufEmpty() throw() {
    return (rss.str().size() - rss.tellg()) == 0;
  }
  bool bad() const throw() { return rss.bad() || wss.bad(); }

  // Member functions
  void setbadstate() throw() {
    Log::debug("setbadstate()");
    rss.setstate(rss.rdstate() | std::ios::badbit);
    wss.setstate(wss.rdstate() | std::ios::badbit);
  }
  int send_file(const std::string& filepath) throw();

  int readline(std::string& line);  // throwable

  // Read line from buffer, if found, remove it from buffer and return 0
  // Otherwise, return -1
  int read_telnet_line(std::string& line);

  ssize_t read(char* buf, size_t size) throw();

  // Actually send data on socket
  int flush();  // throwable

  // Actually receive data from socket
  int fill();  // throwable

  void clear_sendbuf() throw() {
    wss.str("");
    wss.clear();
  }

  SocketBuf& write(const char* buf, size_t size) throw();

  size_t getReadBufSize() throw() {
    StreamCleaner _(rss, wss);
    if (bad()) {
      return 0;
    }
    return rss.str().size() - rss.tellg();
  }

  // Operators
  // member of pointer operators
  util::shared_ptr<Socket> operator->() throw() { return socket; }
  const util::shared_ptr<Socket> operator->() const throw() { return socket; }
  // indirection operators
  Socket& operator*() throw() { return *socket; }
  const Socket& operator*() const throw() { return *socket; }

  // stream insertion operator
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

  // stream extraction operator
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
