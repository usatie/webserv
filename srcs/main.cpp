#include <signal.h>
#include "Log.hpp"
#include "Server.hpp"
#define PORT 8181
#define BACKLOG 5
#define ERROR 1

// main function can throw exceptions.
int main(int argc, char *argv[]) {
  (void)argc, (void)argv;
  // We do not handle exceptions in constructor of Server.
  // Just end this program in that case.
  Server server(PORT, BACKLOG);

  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    std::cerr << "signal() failed\n";
    return ERROR;
  }
  while (1) {
    try {
      server.process(7);
    } catch (std::exception &e) {
      std::cerr << e.what() << std::endl;
    }
  }
  return 0;
}
