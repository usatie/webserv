#include "SocketBuf.hpp"
#include "test_util.hpp"
#include <iostream>
#include <string>

#define PORT 8282
#define GREEN "\033[32m"
#define RESET "\033[0m"
#define RED "\033[31m"
#define OK GREEN "OK" RESET
#define NG RED "NG" RESET

static int err = 0;

int server() {
  // Server socket
  int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd < 0) {
    std::cerr << "socket error" << std::endl;
    exit(1);
  }
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  int optval = 1;
  if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    std::cerr << "setsockopt error" << std::endl;
    exit(1);
  }
  if (bind(listen_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    std::cerr << "bind error" << std::endl;
    exit(1);
  }
  if (listen(listen_fd, 5) < 0) {
    std::cerr << "listen error" << std::endl;
    exit(1);
  }
  return listen_fd;
}

int client() {
  sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  // Client Socket
  int client_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (client_fd < 0) {
    std::cerr << "socket error" << std::endl;
    exit(1);
  }
  if (connect(client_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
    std::cerr << "connect error" << std::endl;
    exit(1);
  }
  return client_fd;
}

void T(SocketBuf &sock, const std::string &expected_line, int expected_ret, bool expected_bad, bool expected_brokenpipe, bool expected_eof) {
  static int cnt = 0;
  bool ok = true;
  std::string line;
  cnt++;
  std::cout << "  " << cnt << ": " ;
  int ret = sock.read_telnet_line(line);
  if (line != expected_line) {
    if (ok) std::cout << NG << std::endl;
    ok = false;
    std::cerr << "    Expected line: " << expected_line << std::endl;
    std::cerr << "    Actual line  : " << line << std::endl;
  }
  if (ret != expected_ret) {
    if (ok) std::cout << NG << std::endl;
    ok = false;
    std::cerr << "    Expected ret: " << expected_ret << std::endl;
    std::cerr << "    Actual ret  : " << ret << std::endl;
  }
  if (sock.bad() != expected_bad) {
    if (ok) std::cout << NG << std::endl;
    ok = false;
    std::cerr << "    Expected bad: " << expected_bad << std::endl;
    std::cerr << "    Actual bad  : " << sock.bad() << std::endl;
  }
  if (sock.isBrokenPipe != expected_brokenpipe) {
    if (ok) std::cout << NG << std::endl;
    ok = false;
    std::cerr << "    Expected isBrokenPipe: " << expected_brokenpipe << std::endl;
    std::cerr << "    Actual isBrokenPipe  : " << sock.isBrokenPipe << std::endl;
  }
  if (sock.hasReceivedEof != expected_eof) {
    if (ok) std::cout << NG << std::endl;
    ok = false;
    std::cerr << "    Expected eof: " << expected_eof << std::endl;
    std::cerr << "    Actual eof  : " << sock.hasReceivedEof << std::endl;
  }
  if (ok) {
    std::cout << OK << " (\"" << expected_line << "\", "
      << expected_ret << ", " << expected_bad << ", " << expected_brokenpipe << ", " << expected_eof
      << ")" << std::endl;
  } else {
    err = -1;
  }
}

void send_and_fill(int client_fd, SocketBuf &serv_sock, const std::string &msg) {
  send(client_fd, msg.c_str(), msg.size(), 0);
  fd_set readfds;
  FD_ZERO(&readfds);
  while (FD_ISSET(serv_sock.get_fd(), &readfds) == false) {
    FD_SET(serv_sock.get_fd(), &readfds);
    select(serv_sock.get_fd() + 1, &readfds, NULL, NULL, NULL);
  }
  serv_sock.fill();
}

void select_and_fill(SocketBuf &serv_sock) {
  fd_set writefds;
  FD_ZERO(&writefds);
  while (FD_ISSET(serv_sock.get_fd(), &writefds) == false) {
    FD_SET(serv_sock.get_fd(), &writefds);
    Log::debug("select");
    int ret = select(serv_sock.get_fd() + 1, &writefds, NULL, NULL, NULL);
    Log::cdebug() << "select ret = " << ret << std::endl;
  }
  serv_sock.fill();
}

int test_socketbuf() {
  int listen_fd = server(); // Server listen fd
  int client_fd = client(); // Client fd
  int srv_conn_fd = accept(listen_fd, NULL, NULL); // Server conn fd
  SocketBuf serv_sock(srv_conn_fd); // Server conn fd wrapper
  std::string line;

  // 1. Empty Buffer
  title("Empty Buffer");
  T(serv_sock, "", -1, false, false, false);

  // 2. One line
  title("One line");
  send_and_fill(client_fd, serv_sock, "hello\r\n");
  T(serv_sock, "hello", 0, false, false, false);
  T(serv_sock, "", -1, false, false, false);

  // 3. Two line
  title("Two line");
  send_and_fill(client_fd, serv_sock, "hello\r\nworld\r\n");
  T(serv_sock, "hello", 0, false, false, false);
  T(serv_sock, "world", 0, false, false, false);
  T(serv_sock, "", -1, false, false, false);

  // 4. LF only
  title("LF only");
  send_and_fill(client_fd, serv_sock, "hello\n");
  T(serv_sock, "hello", 0, false, false, false);
  T(serv_sock, "", -1, false, false, false);
  
  // 5. CR only
  title("CR only");
  send_and_fill(client_fd, serv_sock, "hello\r");
  T(serv_sock, "", -1, false, false, false);
  // 5.1 LF
  title("LF");
  send_and_fill(client_fd, serv_sock, "\n");
  T(serv_sock, "hello", 0, false, false, false);
  
  // 6. Without CR or LF
  title("Without CR or LF");
  send_and_fill(client_fd, serv_sock, "hello");
  T(serv_sock, "", -1, false, false, false);
  send_and_fill(client_fd, serv_sock, " world");
  T(serv_sock, "", -1, false, false, false);
  send_and_fill(client_fd, serv_sock, "\r\n");
  T(serv_sock, "hello world", 0, false, false, false);
  T(serv_sock, "", -1, false, false, false);

  // 7. Empty line
  title("Empty line");
  send_and_fill(client_fd, serv_sock, "\r\n");
  T(serv_sock, "", 0, false, false, false);
  T(serv_sock, "", -1, false, false, false);

  // x. One line + EOF
  title("One line + EOF");
  send_and_fill(client_fd, serv_sock, "hello\r\n");
  shutdown(client_fd, SHUT_WR);
  T(serv_sock, "hello", 0, false, false, false);
  T(serv_sock, "", -1, false, false, false);
  select_and_fill(serv_sock); // Notify socket that the peer has shut down the writing side of this socket
  T(serv_sock, "", -1, false, false, true);
  return err;
}
