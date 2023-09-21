#include <signal.h>
#include <unistd.h>

#include "Config.hpp"
#include "Server.hpp"
#include "webserv.hpp"
#define PORT 8181
#define ERROR 1

void run_server(char *filename);

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
    run_server(argv[1]);  // throwable
    return 0;
  } catch (std::exception &e) {
    Log::cfatal() << "Caught exception while starting server: " << e.what();
    return ERROR;
  }
}

// Starting servers can throw exceptions, but after the start-up process,
// the servers will not throw exceptions.
void run_server(char *filename) {
  // Construct Config from config file.
  config::Config cf;  // throwable
  if (filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
      throw std::runtime_error("Cannot open config file");
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
  server.run();  // no throw, never return
}
