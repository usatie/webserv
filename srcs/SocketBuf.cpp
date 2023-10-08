#include "SocketBuf.hpp"

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <fstream>
#include <iostream>
#include <string>

#include "Connection.hpp"
#include "StreamCleaner.hpp"
#include "http_special_response.hpp"

#define FILL_BUF_SIZE (1024 * 1024)
#define SEND_BUF_SIZE (1024 * 1024)

// Constructor for TCP socket
SocketBuf::SocketBuf(util::shared_ptr<Socket> socket)
    : socket(socket), rss(), wss(), hasReceivedEof(false), isBrokenPipe(false) {
  if (socket->set_nonblock() < 0) {
    throw std::runtime_error("socket->set_nonblock() failed");
  }
}

// Constructor for unix domain socket
SocketBuf::SocketBuf(int fd)
    : socket(util::shared_ptr<Socket>(new Socket(fd))),
      rss(),
      wss(),
      hasReceivedEof(false),
      isBrokenPipe(false) {
  if (socket->set_nonblock() < 0) {
    throw std::runtime_error("socket->set_nonblock() failed");
  }
}

size_t SocketBuf::getReadBufSize() throw() {
  StreamCleaner _(rss, wss);
  if (bad()) {
    return 0;
  }
  return rss.str().size() - rss.tellg();
}

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

const char* status_line(int status_code) throw() {
  switch (status_code) {
    case 200:
      return http_200_status_line;
    case 201:
      return http_201_status_line;
    case 204:
      return http_204_status_line;
    case 301:
      return http_301_status_line;
    case 400:
      return http_400_status_line;
    case 403:
      return http_403_status_line;
    case 404:
      return http_404_status_line;
    case 405:
      return http_405_status_line;
    case 413:
      return http_413_status_line;
    case 500:
      return http_500_status_line;
    case 504:
      return http_504_status_line;
    case 505:
      return http_505_status_line;
    default:
      Log::cfatal() << "Unknown status code: " << status_code << std::endl;
      return http_500_status_line;
  }
}

int SocketBuf::send_response(const Response& res) throw() {
  *this << status_line(res.status_code) << CRLF;
  *this << "Server: " << WEBSERV_VER << CRLF;
  //*conn.client_socket << "Date: " << util::get_date() << CRLF;
  if (res.keep_alive) {
    *this << "Connection: keep-alive" << CRLF;
    //*conn.client_socket << "Keep-Alive: timeout=" << conn.srv_cf->timeout
    //                    << CRLF;
  } else {
    *this << "Connection: close" << CRLF;
  }
  if (res.location != "") {
    *this << "Location: " << res.location << CRLF;
  }
  if (res.content_type != "") {
    *this << "Content-Type: " << res.content_type << CRLF;
  }
  if (res.content != "") {
    *this << "Content-Length: " << res.content.length() << CRLF;
    *this << CRLF;  // end of header
    *this << res.content;
  } else if (res.content_path != "" || res.content_length > 0) {
    *this << "Content-Length: " << res.content_length << CRLF;
    *this << CRLF;  // end of header
    // TODO: Handle errors while reading content_path
    send_file(res.content_path);
  } else {
    *this << "Content-Length: 0" << CRLF;
    *this << CRLF;  // end of header
  }

  return 0;
}

int SocketBuf::readline(std::string& line) {
  StreamCleaner _(rss, wss);
  if (bad()) {
    return -1;
  }
  // 1. getline
  // If there is an error while getline, return -1
  // i.e. str.max_size() characters have been stored.
  if (!std::getline(rss, line)) {  // throwable
    Log::debug("std::getline(LF) failed");
    line.clear();
    return -1;
  }

  // 2. EOF before LF (i.e. no LF found)
  if (rss.eof()) {
    Log::debug("no LF found");
    rss.clear(std::ios::eofbit);             // clear eofbit before seekg
    rss.seekg(-line.size(), std::ios::cur);  // rewind
    line.clear();
    return -1;
  }
  // 3. LF found
  Log::debug("LF found");
  return 0;
}
int SocketBuf::read_telnet_line(std::string& line) {  // throwable
  StreamCleaner _(rss, wss);
  if (bad()) {
    return -1;
  }
  // 1. getline
  // If there is an error while getline, return -1
  // i.e. str.max_size() characters have been stored.
  if (!std::getline(rss, line, LF)) {  // throwable
    Log::debug("std::getline(LF) failed");
    line.clear();
    return -1;
  }
  // 2. EOF before LF (i.e. no LF found)
  if (rss.eof()) {
    Log::debug("no LF found");
    rss.clear(std::ios::eofbit);             // clear eofbit before seekg
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

int SocketBuf::flush() {
  StreamCleaner _(rss, wss);
  if (bad()) {
    return -1;
  }
  if (isSendBufEmpty()) {
    Log::cerror()
        << "Something's wrong: send buffer is empty and flush() is called.\n";
    return 0;
  }
  Log::cdebug() << "wss.tellg(): " << wss.tellg() << "\n";
  // TODO: wss.fail() may be true, we need to handle it
  // in that case, tellg() returns -1
  if (wss.tellg() < 0) {
    Log::cerror() << "wss.tellg() failed\n";
    return -1;
  }
  static char buf[SEND_BUF_SIZE];
  wss.read(buf, SEND_BUF_SIZE);
  Log::cdebug() << "sent buf: "
                << std::string(buf, std::min(100, (int)wss.gcount())) << "\n";

#ifdef LINUX
  ssize_t ret =
      ::send(socket->get_fd(), static_cast<void*>(buf), wss.gcount(), 0);
#else
  ssize_t ret = ::send(socket->get_fd(), static_cast<void*>(buf), wss.gcount(),
                       SO_NOSIGPIPE);
#endif
  if (ret < 0) {
    Log::cerror() << "send() failed, errno: " << errno
                  << ", error: " << strerror(errno) << "\n";
    // EINTR will not happen because we use NONBLOCK socket
    // EAGAIN will not happen because we use select() before send()
    // So, this error is probably broken pipe
    isBrokenPipe = true;
    return -1;
  }
  // Before doing anything else, seekg clears eofbit.	(since C++11)
  // https://en.cppreference.com/w/cpp/io/basic_istream/seekg
  wss.clear(std::ios::eofbit);  // clear eofbit before seekg
  wss.seekg(ret - wss.gcount(), std::ios::cur);
  return ret;
}

int SocketBuf::fill() {  // throwable
  StreamCleaner _(rss, wss);
  if (bad()) {
    return -1;
  }
  static char buf[FILL_BUF_SIZE] = {0};
  static const int flags = 0;
  ssize_t ret =
      ::recv(socket->get_fd(), static_cast<void*>(buf), FILL_BUF_SIZE, flags);
  if (ret < 0) {
    Log::cerror() << "recv() failed, errno: " << errno
                  << ", error: " << strerror(errno) << "\n";
    return -1;
  }
  if (ret == 0) {
    Log::cdebug() << "received EOF" << std::endl;
    // Just save the EOF flag and do not close the socket in order to send
    // the remaining data.
    hasReceivedEof = true;
    return 0;
  }
  // Fill ret bytes buf into rss
  std::string s(buf, ret);  // throwable
  rss << s;
  return ret;
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
