#include <signal.h>
#include <unistd.h>

#include "Config.hpp"
#include "Server.hpp"
#include "webserv.hpp"
#define PORT 8181
#define ERROR 1

void sigpipe(int sig) {
  (void)sig;
  write(2, "sigpipe\n", 8);
}

// main function can throw exceptions.
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

  // Construct Config from config file.
  // Even default constructor of Config can throw exceptions.
  // But we do not handle exceptions in main function, so that
  // we can just end this program in that case.
  Config::Config cf;
  if (argc == 2) {
    std::ifstream ifs(argv[1]);
    if (!ifs.is_open()) {
      Log::fatal("Cannot open config file");
      return ERROR;
    }
    std::string s((std::istreambuf_iterator<char>(ifs)),
                  std::istreambuf_iterator<char>());
    Token *tokens = tokenize(s);
    Module *mod = parse(tokens);
    cf = Config::Config(mod);
    delete mod;
    delete tokens;
  }
  // TODO: Use config
  printConfig(cf);

  // We do not handle exceptions in constructor of Server.
  // Just end this program in that case.
  Server server(cf);

  if (signal(SIGPIPE, sigpipe) == SIG_ERR) {
    Log::fatal("signal() failed");
    return ERROR;
  }
  while (1) {
    server.process();
  }
  return 0;
}
