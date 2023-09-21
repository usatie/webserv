#include <signal.h>
#include <unistd.h>

#include "Config.hpp"
#include "Server.hpp"
#include "webserv.hpp"
#define PORT 8181
#define ERROR 1

int run_server(char *filename);

int main(int argc, char *argv[]) {
#ifdef DEBUG
  Log::setLevel(Log::Debug);
#else
  Log::setLevel(Log::Warn);
#endif
  if (argc > 3) {
    Log::fatal("Usage: ./webserv [<config_file>]");
    return ERROR;
  }
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
    Log::fatal("signal() failed");
    return ERROR;
  }
  try {
    run_server(argv[1]); // throwable
    return ERROR; // if startup is successful, this line is unreachable
  } catch (std::exception &e) {
    Log::cfatal() << "Caught exception while starting server: " << e.what();
    return ERROR;
  }
}

// Starting servers can throw exceptions, but after the start-up process,
// the servers will not throw exceptions.
int run_server(char *filename) {
  // Construct Config from config file.
  config::Config cf;  // throwable
  if (filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
      return -1;
    }
    std::string s((std::istreambuf_iterator<char>(ifs)),
                  std::istreambuf_iterator<char>());  // no throw
    Token *tokens = tokenize(s);                      // throwable
    Module *mod = parse(tokens);                      // throwable
    cf = config::Config(mod);                         // throwable
    delete mod;
    delete tokens;
  }
  // Print config
  config::print(cf);

  // Start up server
  Server server(cf); // no throw
  server.startup(); // throwable

  // Run server
  server.run();  // no throw
  return -1; // unreachable, never return
}
