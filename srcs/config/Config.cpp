#include "Config.hpp"

namespace config {

Config::Config(Module* mod) {
  for (; mod; mod = mod->next) {
    if (mod->type != Module::MOD_HTTP) {
      throw std::runtime_error("Invalid module type");
    }
    if (http.configured) {
      throw std::runtime_error("Duplicate http block");
    }
    http = HTTP(mod);
  }
}

// Stream
std::ostream& operator<<(std::ostream& os, const Listen& listen) {
  os << listen.address << ":" << listen.port;
  return os;
}

std::ostream& operator<<(std::ostream& os, const ErrorPage& error_page) {
  os << "{" << error_page.codes << " " << error_page.uri << "}";
  return os;
}

std::ostream& operator<<(std::ostream& os, const RedirectReturn& ret) {
  os << ret.code << " " << ret.url << " ";
  return os;
}

std::ostream& operator<<(std::ostream& os, const CgiHandler& cgi_handler) {
  os << "{" << cgi_handler.extensions << " " << cgi_handler.interpreter_path
     << "}";
  return os;
}

std::ostream& operator<<(std::ostream& os, const Config& cf) {
  os << "Config" << std::endl;
  os << "  HTTP" << std::endl;
  os << cf.http;
  return os;
}

void print(const Config& cf) { std::cout << cf; }
}  // namespace config
