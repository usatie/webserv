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
  // Starting servers can throw exceptions, but after the start-up process,
  // the servers will not throw exceptions.
  try {
    run_server(argv[1]); // throwable
    return 0;
  } catch (std::exception &e) {
    Log::cfatal() << "Caught exception while starting server: " << e.what();
    return ERROR;
  }
}

void run_server(char *filename) {
  // Construct Config from config file.
  config::Config cf; // throwable
  if (filename) {
    std::ifstream ifs(filename);
    if (!ifs.is_open()) {
      throw std::runtime_error("Cannot open config file");
    }
    std::string s((std::istreambuf_iterator<char>(ifs)),
                  std::istreambuf_iterator<char>()); // no throw
    Token *tokens = tokenize(s); // throwable
    Module *mod = parse(tokens); // throwable
    cf = config::Config(mod); // throwable
    delete mod;
    delete tokens;
  }
  config::print(cf);

  // Start server.
  Server server(cf); // throwable
  while (1) {
    server.process(); // no throw
  }
}
