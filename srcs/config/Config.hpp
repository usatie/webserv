#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <netinet/in.h>  // sockaddr_storage, socklen_t

#include <string>
#include <vector>

#include "ConfigParser.hpp"
#include "Listen.hpp"

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
  std::ostream &operator<<(std::ostream& os, const CgiHandler& cgi_handler);
  std::ostream &operator<<(std::ostream &os, const Location &l);
  std::ostream &operator<<(std::ostream &os, const Server &s);
  std::ostream &operator<<(std::ostream &os, const HTTP &http);
  std::ostream &operator<<(std::ostream &os, const Config &cf);
}

class config::ErrorPage {
 public:
  std::vector<int> codes;
  std::string uri;
  bool configured;
  ErrorPage(const std::vector<int> &codes, const std::string &uri)
      : codes(codes), uri(uri), configured(true) {}
  ErrorPage() : codes(), uri(""), configured(false) {}
};
class config::LimitExcept {
 public:
  std::vector<std::string> methods;
  bool configured;
  LimitExcept(const std::vector<std::string> &methods)
      : methods(methods), configured(true) {}
  LimitExcept() : methods(), configured(false) {}
};
class config::RedirectReturn {
 public:
  int code;
  std::string url;
  bool configured;
  RedirectReturn(const int code, const std::string &url)
      : code(code), url(url), configured(true) {}
  RedirectReturn() : code(), url(""), configured(false) {}
};
class config::Root : public std::string {
 public:
  bool configured;
  explicit Root(const std::string &path)
      : std::string(path), configured(true) {}
  Root() : std::string("html"), configured(false) {}
};
class config::Index : public std::vector<std::string> {
 public:
  bool configured;
  explicit Index(const std::vector<std::string> &index_files)
      : std::vector<std::string>(index_files), configured(true) {}
  Index() : std::vector<std::string>(), configured(false) {
    push_back("index.html");
  }
};
class config::Alias : public std::string {
 public:
  bool configured;
  explicit Alias(const std::string &path)
      : std::string(path), configured(true) {}
  Alias() : std::string(), configured(false) {}
};
class config::ClientMaxBodySize {
 private:
  size_t size;

 public:
  bool configured;
  explicit ClientMaxBodySize(const size_t &size)
      : size(size), configured(true) {}
  ClientMaxBodySize() : size(1 * MB), configured(false) {}
  void set(const size_t &size) { this->size = size; }
  operator size_t() const { return size; }
};
class config::AutoIndex {
 private:
  bool value;

 public:
  bool configured;
  explicit AutoIndex(const bool &value) : value(value), configured(true) {}
  AutoIndex() : value(false), configured(false) {}
  void set(const bool &value) { this->value = value; }
  operator bool() const { return value; }
};
class config::UploadStore : public std::string {
 public:
  bool configured;
  explicit UploadStore(const std::string &path)
      : std::string(path), configured(true) {}
  UploadStore() : std::string("upload"), configured(false) {}
  void set(const std::string &path) { this->assign(path); }
};
class config::CgiExtensions : public std::vector<std::string> {
 public:
  bool configured;
  explicit CgiExtensions(const std::vector<std::string> &extensions)
      : std::vector<std::string>(extensions), configured(true) {}
  CgiExtensions() : std::vector<std::string>(), configured(false) {}
};
class config::CgiHandler {
 public:
  std::vector<std::string> extensions;
  std::string interpreter_path;
  bool configured;
  CgiHandler(const std::vector<std::string> &extensions,
             const std::string &interpreter_path)
      : extensions(extensions),
        interpreter_path(interpreter_path),
        configured(true) {}
  CgiHandler() : extensions(), interpreter_path(), configured(false) {}
};
// Composite Configuration Items
class config::Location {
 public:
  std::string path;
  Root root;
  Index index;
  std::vector<ErrorPage> error_pages;
  AutoIndex autoindex;
  ClientMaxBodySize client_max_body_size;
  UploadStore upload_store;
  std::vector<RedirectReturn> returns;
  Alias alias;               // Location only
  LimitExcept limit_except;  // Locatoin only
  CgiExtensions cgi_extensions;
  std::vector<CgiHandler> cgi_handlers;
  std::vector<Location> locations;
  Location(Command *cmd);
};
class config::Server {
 public:
  std::vector<Location> locations;
  std::vector<Listen> listens;
  std::vector<std::string> server_names;
  Root root;
  Index index;
  std::vector<ErrorPage> error_pages;
  AutoIndex autoindex;
  UploadStore upload_store;
  ClientMaxBodySize client_max_body_size;
  CgiExtensions cgi_extensions;
  std::vector<CgiHandler> cgi_handlers;
  std::vector<RedirectReturn> returns;

  Server();
  Server(Command *cmd);
};
class config::HTTP {
 public:
  HTTP();
  HTTP(Module *mod);
  ~HTTP() {}

 public:
  std::vector<Server> servers;
  Root root;
  Index index;
  std::vector<ErrorPage> error_pages;
  AutoIndex autoindex;
  ClientMaxBodySize client_max_body_size;
  bool configured;
};
class config::Config {
 public:
  // Primitive Configuration Items

  // Configuration
 public:
  HTTP http;

  Config();
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
void printConfig(const config::Config &cf);

#endif
