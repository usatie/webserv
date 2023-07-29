#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main() {
  // Send a message to the HTTP server
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  struct sockaddr_in serv_addr;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(8181);
  inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
  connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  const char *msg = "GET Makefile HTTP/1.1\r\n\r\n";
  send(sockfd, msg, strlen(msg), 0);
  char buf[1024];
  int n = recv(sockfd, buf, 1024, 0);
  buf[n] = '\0';
  printf("%s\n", buf);
  close(sockfd);
  return 0;
}
