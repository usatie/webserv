#include "SocketBuf.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <fstream>
#include <iostream>
#include <string>

#define MAXLINE 1024
#include <fcntl.h>

int SocketBuf::send_file(const std::string& filepath) throw() {
  StreamCleaner _(rss, wss);
  if (bad()) {
    return -1;
  }
  std::ifstream ifs(filepath.c_str(), std::ios::binary);

  if (!ifs.is_open()) {
    Log::fatal("file open failed");
    return -1;
  }
  wss << ifs.rdbuf();
  if (wss.bad()) {
    Log::fatal("wss << ifs.rdbuf() failed");
    setbadstate();
    return -1;
  }
  return 0;
}
int SocketBuf::readline(std::string& line) throw() {
  StreamCleaner _(rss, wss);
  if (bad()) {
    return -1;
  }
  // 1. getline
  try {
    // If there is an error while getline, return -1
    // i.e. str.max_size() characters have been stored.
    if (!std::getline(rss, line)) {  // throwable
      Log::debug("std::getline(LF) failed");
      line.clear();
      return -1;
    }
  } catch (std::exception& e) {
    Log::fatal("getline() failed");
    line.clear();
    setbadstate();
    return -1;
  }

  // 2. EOF before LF (i.e. no LF found)
  if (rss.eof()) {
    Log::debug("no LF found");
    rss.seekg(-line.size(), std::ios::cur);  // rewind
    line.clear();
    return -1;
  }
  // 3. LF found
  Log::debug("LF found");
  return 0;
}
int SocketBuf::read_telnet_line(std::string& line) throw() {
  StreamCleaner _(rss, wss);
  if (bad()) {
    return -1;
  }
  // 1. getline
  try {
    // If there is an error while getline, return -1
    // i.e. str.max_size() characters have been stored.
    if (!std::getline(rss, line, LF)) {  // throwable
      Log::debug("std::getline(LF) failed");
      line.clear();
      return -1;
    }
  } catch (std::exception& e) {
    Log::fatal("getline() failed");
    line.clear();
    setbadstate();
    return -1;
  }
  // 2. EOF before LF (i.e. no LF found)
  if (rss.eof()) {
    Log::debug("no LF found");
    rss.seekg(-line.size(), std::ios::cur);  // rewind
    line.clear();
    return -1;
  }
  // 3. CR not found (i.e. only LF found)
  // The line terminator for message-header fields is the sequence CRLF.
  // However, we recommend that applications, when parsing such headers,
  // recognize a single LF as a line terminator and ignore the leading CR.
  // https://www.rfc-editor.org/rfc/rfc2616#section-19.3
  if (line.empty() || line[line.size() - 1] != CR) {
    Log::debug("only LF found");
    return 0;
  }
  // 4. CRLF found
  Log::debug("CRLF found");
  line.resize(line.size() - 1);  // line.pop_back();
  return 0;
}
ssize_t SocketBuf::read(char* buf, size_t size) throw() {
  StreamCleaner _(rss, wss);
  if (bad()) {
    return -1;
  }
  rss.read(buf,
           size);  // does not throw ref:
                   // https://en.cppreference.com/w/cpp/io/basic_istream/read
  if (rss.bad()) return -1;
  return rss.gcount();
}
int SocketBuf::flush() throw() {
  StreamCleaner _(rss, wss);
  if (bad()) {
    return -1;
  }
  if (isSendBufEmpty()) {
    return 0;
  }
  try {
    Log::cdebug() << "wss.tellg(): " << wss.tellg() << "\n";
    // TODO: wss.fail() may be true, we need to handle it
    // in that case, tellg() returns -1
    if (wss.tellg() < 0) {
      Log::cerror() << "wss.tellg() failed\n";
      return -1;
    }
    std::string buf(wss.str());
    // TODO: buf may contain unnecessary leading data, we need to remove them

#ifdef LINUX
    ssize_t ret = ::send(socket->get_fd(), &buf.c_str()[wss.tellg()],
                         buf.size() - wss.tellg(), 0);
#else
    ssize_t ret = ::send(socket->get_fd(), &buf.c_str()[wss.tellg()],
                         buf.size() - wss.tellg(), SO_NOSIGPIPE);
#endif
    if (ret < 0) {
      Log::cerror() << "send() failed, errno: " << errno
                    << ", error: " << strerror(errno) << "\n";
      // TODO: handle EINTR
      // ETIMEDOUT, EPIPE in any case means the connection is closed
      socket->beClosed();
      return -1;
    }
    wss.seekg(ret, std::ios::cur);
    return ret;
  } catch (std::exception& e) {
    Log::cfatal() << "wss.str() failed: " << e.what() << "\n";
    setbadstate();
    return -1;
  }
}
int SocketBuf::fill() throw() {
  StreamCleaner _(rss, wss);
  if (bad()) {
    return -1;
  }
  static char buf[MAXLINE] = {0};
  static const int flags = 0;
  ssize_t ret =
      ::recv(socket->get_fd(), static_cast<void*>(buf), MAXLINE, flags);
  if (ret < 0) {
    Log::error("recv() failed");
    return -1;
  }
  if (ret == 0) {
    socket->beClosed();
  }
  // Fill ret bytes buf into rss
  try {
    std::string s(buf, ret);
    rss << s;
    return ret;
  } catch (std::exception& e) {
    Log::fatal("std::string(buf, ret) failed");
    setbadstate();
    return -1;
  }
}
SocketBuf& SocketBuf::write(const char* buf, size_t size) throw() {
  StreamCleaner _(rss, wss);
  if (bad()) {
    return *this;
  }
  wss.write(buf, size);
  if (wss.fail()) {
    Log::fatal("wss.write(buf, size) failed");
    setbadstate();
  }
  return *this;
}
