#include "SocketBuf.hpp"
#include <iostream>
#include <string>

#define PORT 8282

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

void T(SocketBuf &sock, const std::string &expected_line, int expected_ret, bool expected_bad, bool expected_closed) {
  static int cnt = 0;
  bool ok = true;
  std::string line;
  cnt++;
  std::cout << "Test " << cnt << ": " ;
  int ret = sock.readline(line);
  if (line != expected_line) {
    if (ok) std::cout << "NG" << std::endl;
    ok = false;
    std::cerr << "  Expected line: " << expected_line << std::endl;
    std::cerr << "  Actual line  : " << line << std::endl;
  }
  if (ret != expected_ret) {
    if (ok) std::cout << "NG" << std::endl;
    ok = false;
    std::cerr << "  Expected ret: " << expected_ret << std::endl;
    std::cerr << "  Actual ret  : " << ret << std::endl;
  }
  if (sock.bad() != expected_bad) {
    if (ok) std::cout << "NG" << std::endl;
    ok = false;
    std::cerr << "  Expected bad: " << expected_bad << std::endl;
    std::cerr << "  Actual bad  : " << sock.bad() << std::endl;
  }
  if (sock.isClosed() != expected_closed) {
    if (ok) std::cout << "NG" << std::endl;
    ok = false;
    std::cerr << "  Expected closed: " << expected_closed << std::endl;
    std::cerr << "  Actual closed  : " << sock.isClosed() << std::endl;
  }
  if (ok) {
    std::cout << "OK" << std::endl;
  }
}

void send_and_fill(int client_fd, SocketBuf &serv_sock, const std::string &msg) {
  send(client_fd, msg.c_str(), msg.size(), 0);
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(serv_sock.get_fd(), &readfds);
  select(serv_sock.get_fd() + 1, &readfds, NULL, NULL, NULL);
  serv_sock.fill();
}

void test_socketbuf() {
  int listen_fd = server(); // Server listen fd
  int client_fd = client(); // Client fd
  SocketBuf serv_sock(listen_fd); // Server conn fd wrapper
  std::string line;

  // 1. Empty
  T(serv_sock, "", -1, false, false);

  // 2. One line
  send_and_fill(client_fd, serv_sock, "hello\r\n");
  T(serv_sock, "hello", 0, false, false);
  T(serv_sock, "", -1, false, false);

  // 3. Two line
  send_and_fill(client_fd, serv_sock, "hello\r\nworld\r\n");
  T(serv_sock, "hello", 0, false, false);
  T(serv_sock, "world", 0, false, false);
  T(serv_sock, "", -1, false, false);

  // 4. LF only
  send_and_fill(client_fd, serv_sock, "hello\n");
  T(serv_sock, "hello", 0, false, false);
  T(serv_sock, "", -1, false, false);
  
  // 5. CR only
  send_and_fill(client_fd, serv_sock, "hello\r");
  T(serv_sock, "hello", 0, false, false);
  T(serv_sock, "", -1, false, false);
  
  // 6. Without CR or LF
  send_and_fill(client_fd, serv_sock, "hello");
  T(serv_sock, "", -1, false, false);
  send_and_fill(client_fd, serv_sock, " world");
  T(serv_sock, "", -1, false, false);
  send_and_fill(client_fd, serv_sock, "\r\n");
  T(serv_sock, "hello world", 0, false, false);
  T(serv_sock, "", -1, false, false);

  // x. One line + EOF
  send_and_fill(client_fd, serv_sock, "hello\r\n");
  shutdown(client_fd, SHUT_WR);
  T(serv_sock, "hello", 0, false, false);
  T(serv_sock, "", -1, false, false);
  serv_sock.fill(); // Notify socket that the peer has closed the connection
  T(serv_sock, "", -1, false, true);
}

int main() {
  Log::setLevel(Log::Warn);
  test_socketbuf();
}
