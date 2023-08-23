#include <signal.h>
#include "webserv.hpp"
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

  Log::setLevel(Log::Debug);
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    Log::fatal("signal() failed");
    return ERROR;
  }
  while (1) {
    try {
      server.process(7);
    } catch (std::exception &e) {
      Log::cerror() << "Exception: " << e.what() << std::endl;
    }
  }
  return 0;
}
