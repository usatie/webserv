#ifndef SERVER_HPP
#define SERVER_HPP

#include <algorithm>  // std::find
#include <vector>     // std::vector

#include "webserv.hpp"

#define BACKLOG -1
#define BUFF_SIZE 1024

class Connection;
class Socket;
namespace config {
class Config;
class Listen;
}  // namespace config

class Server {
 public:
  typedef util::shared_ptr<Connection> Conn;
  typedef util::shared_ptr<Socket> Sock;
  typedef std::vector<Conn> ConnVector;
  typedef ConnVector::iterator ConnIterator;
  typedef std::vector<Sock> SockVector;
  typedef SockVector::iterator SockIterator;

 private:
  fd_set readfds, writefds;
  fd_set ready_rfds, ready_wfds;
  int maxfd;
  time_t last_timeout_check;

 private:
  Server() throw();  // Do not implement

 public:
  // Constructor/Destructor
  explicit Server(const config::Config& cf) throw();
  ~Server() throw();

  // Member functions
  void startup();  // throwable
  void run() throw();

 private:
  // Member data
  SockVector listen_socks;
  ConnVector connections;

 public:
  const config::Config& cf;

  void clear_fd(int fd) throw();

 private:
  // Member functions
  int listen(const config::Listen& l, SockVector& serv_socks);  // throwable

  ConnIterator remove_connection(ConnIterator conn_it) throw();
  void clear_connection(ConnIterator conn_it) throw();

  void remove_timeout_connections() throw();

  void accept(Sock sock) throw();

  void resume(ConnIterator conn_it) throw();

  void update_fdset(Conn conn) throw();

  int wait() throw();

  bool canResume(Conn conn) const throw();

  Sock get_ready_listen_socket() throw();

  // Logically it is not const because it returns a non-const pointer.
  Conn get_ready_connection() throw();
};

#endif
