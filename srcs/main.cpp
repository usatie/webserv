#include "Server.hpp"
#define PORT 8181
#define BACKLOG 5
#define ERROR 1

int main(int argc, char *argv[]) {
  (void)argc, (void)argv;
  // If initialize server socket failed, exit.
  Server server;

  if (server.init(PORT, BACKLOG) < 0) {
    return ERROR;
  }
  while (1) {
    server.process();
  }
  return 0;
}
