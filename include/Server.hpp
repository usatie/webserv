#ifndef SERVER_HPP
#define SERVER_HPP

#include <algorithm>  // std::find
#include <memory>     // std::shared_ptr
#include <vector>     // std::vector

#include "webserv.hpp"

#define BACKLOG 5
#define BUFF_SIZE 1024

class Connection;
class Socket;
class Config;

class Server {
 private:
  typedef std::vector<std::shared_ptr<Connection> > ConnVector;
  typedef ConnVector::iterator ConnIterator;
  typedef std::vector<std::shared_ptr<Socket> > SockVector;
  typedef SockVector::iterator SockIterator;
  fd_set readfds, writefds;
  fd_set ready_rfds, ready_wfds;
  int maxfd;

 public:
  // Member data
  SockVector listen_socks;
  ConnVector connections;
  const Config& cf;

  // Constructor/Destructor
  Server() throw();  // Do not implement this
  Server(const Config& cf);
  ~Server() throw() {}

  // Member functions
  void remove_connection(std::shared_ptr<Connection> connection) throw();

  void remove_all_connections() throw();

  void accept(std::shared_ptr<Socket> sock) throw();

  void update_fdset(std::shared_ptr<Connection> conn) throw();

  int wait() throw();

  bool canResume(std::shared_ptr<Connection> conn) const throw();

  std::shared_ptr<Socket> get_ready_socket() throw();

  // Logically it is not const because it returns a non-const pointer.
  std::shared_ptr<Connection> get_ready_connection() throw();

  void process() throw();
};

#endif
