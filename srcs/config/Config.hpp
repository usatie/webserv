#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "config/Alias.hpp"
#include "config/AutoIndex.hpp"
#include "config/CgiExtensions.hpp"
#include "config/CgiHandler.hpp"
#include "config/ClientMaxBodySize.hpp"
#include "config/ConfigParser.hpp"
#include "config/ErrorPage.hpp"
#include "config/Http.hpp"
#include "config/Index.hpp"
#include "config/LimitExcept.hpp"
#include "config/Listen.hpp"
#include "config/Location.hpp"
#include "config/RedirectReturn.hpp"
#include "config/Root.hpp"
#include "config/Server.hpp"
#include "config/UploadStore.hpp"

namespace config {
class Config;
class Server;
class Location;
class Listen;
class ErrorPage;
class RedirectReturn;
class Root;
class Index;
class Alias;
class LimitExcept;
class ClientMaxBodySize;
class AutoIndex;
class UploadStore;
class CgiExtensions;
class CgiHandler;
class HTTP;
template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v);
std::ostream &operator<<(std::ostream &os, const Listen &listen);
std::ostream &operator<<(std::ostream &os, const ErrorPage &error_page);
std::ostream &operator<<(std::ostream &os, const RedirectReturn &ret);
std::ostream &operator<<(std::ostream &os, const CgiHandler &cgi_handler);
std::ostream &operator<<(std::ostream &os, const Location &l);
std::ostream &operator<<(std::ostream &os, const Server &s);
std::ostream &operator<<(std::ostream &os, const HTTP &http);
std::ostream &operator<<(std::ostream &os, const Config &cf);
void print(const Config &cf);
}  // namespace config

// Composite Configuration Items
class config::Config {
 public:
  // Primitive Configuration Items

  // Configuration
 public:
  HTTP http;

  Config() {}
  Config(Module *mod);
};

template <typename T>
std::ostream &config::operator<<(std::ostream &os, const std::vector<T> &v) {
  // if (v.configured) {
  //   std::cout << "[configured] ";
  // }
  for (unsigned int i = 0; i < v.size(); ++i) {
    os << v[i] << " ";
  }
  return os;
}

#endif
