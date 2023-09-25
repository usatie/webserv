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
 private:
  typedef std::vector<util::shared_ptr<Connection> > ConnVector;
  typedef ConnVector::iterator ConnIterator;
  typedef std::vector<util::shared_ptr<Socket> > SockVector;
  typedef SockVector::iterator SockIterator;
  fd_set readfds, writefds;
  fd_set ready_rfds, ready_wfds;
  int maxfd;
  time_t last_timeout_check;

 private:
  Server() throw();  // Do not implement

 public:
  // Constructor/Destructor
  Server(const config::Config& cf) throw();
  ~Server() throw();

  // Member functions
  void startup();  // throwable
  void run() throw();

 private:
  // Member data
  SockVector listen_socks;
  ConnVector connections;
  const config::Config& cf;

  // Member functions
  int getaddrinfo(const config::Listen& l,
                  struct addrinfo** result);  // throwable
  int listen(const config::Listen& l,
             std::vector<util::shared_ptr<Socket> >& socks);  // throwable

  ConnIterator remove_connection(
      util::shared_ptr<Connection> connection) throw();

  void remove_timeout_connections() throw();

  void accept(util::shared_ptr<Socket> sock) throw();

  void resume(util::shared_ptr<Connection> conn) throw();

  void update_fdset(util::shared_ptr<Connection> conn) throw();

  int wait() throw();

  bool canResume(util::shared_ptr<Connection> conn) const throw();

  util::shared_ptr<Socket> get_ready_listen_socket() throw();

  // Logically it is not const because it returns a non-const pointer.
  util::shared_ptr<Connection> get_ready_connection() throw();
};

#endif
