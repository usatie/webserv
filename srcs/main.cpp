#include <signal.h>

#include "Server.hpp"
#include "webserv.hpp"
#define PORT 8181
#define BACKLOG 5
#define ERROR 1

// main function can throw exceptions.
int main(int argc, char *argv[]) {
#ifdef DEBUG
  Log::setLevel(Log::Debug);
#else
  Log::setLevel(Log::Warn);
#endif

  (void)argc, (void)argv;
  // We do not handle exceptions in constructor of Server.
  // Just end this program in that case.
  Server server(PORT, BACKLOG);

  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    Log::fatal("signal() failed");
    return ERROR;
  }
  while (1) {
    server.process(7);
  }
  return 0;
}
