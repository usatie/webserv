#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <netinet/in.h>  // sockaddr_storage, socklen_t

#include <string>
#include <vector>

#include "ConfigParser.hpp"

class Config {
 public:
  // Primitive Configuration Items
  class Listen {
   public:
    std::string address;  // ip address, wildcard, or hostname
    int port;
    bool configured;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    Listen(const std::string &address, const int &port)
        : address(address), port(port), configured(true), addr(), addrlen(0) {
      if (address.empty()) this->address = "*";
    }
    Listen() : address("*"), port(8181), configured(false) {}
  };
  class ErrorPage {
   public:
    std::vector<int> codes;
    std::string uri;
    bool configured;
    ErrorPage(const std::vector<int> &codes, const std::string &uri)
        : codes(codes), uri(uri), configured(true) {}
    ErrorPage() : codes(), uri(""), configured(false) {}
  };
  class LimitExcept {
   public:
    std::vector<std::string> methods;
    bool configured;
    LimitExcept(const std::vector<std::string> &methods)
        : methods(methods), configured(true) {}
    LimitExcept() : methods(), configured(false) {}
  };
  class RedirectReturn {
   public:
    int code;
    std::string url;
    bool configured;
    RedirectReturn(const int code, const std::string &url)
        : code(code), url(url), configured(true) {}
    RedirectReturn() : code(), url(""), configured(false) {}
  };
  class Root : public std::string {
   public:
    bool configured;
    explicit Root(const std::string &path)
        : std::string(path), configured(true) {}
    Root() : std::string("html"), configured(false) {}
  };
  class Index : public std::vector<std::string> {
   public:
    bool configured;
    explicit Index(const std::vector<std::string> &index_files)
        : std::vector<std::string>(index_files), configured(true) {}
    Index() : std::vector<std::string>(), configured(false) {
      push_back("index.html");
    }
  };
  class Alias : public std::string {
   public:
    bool configured;
    explicit Alias(const std::string &path)
        : std::string(path), configured(true) {}
    Alias() : std::string(), configured(false) {}
  };
  class ClientMaxBodySize {
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
  class AutoIndex {
   private:
    bool value;

   public:
    bool configured;
    explicit AutoIndex(const bool &value) : value(value), configured(true) {}
    AutoIndex() : value(false), configured(false) {}
    void set(const bool &value) { this->value = value; }
    operator bool() const { return value; }
  };
  class UploadStore : public std::string {
   public:
    bool configured;
    explicit UploadStore(const std::string &path)
        : std::string(path), configured(true) {}
    UploadStore() : std::string("upload"), configured(false) {}
    void set(const std::string &path) { this->assign(path); }
  };
  class CgiExtensions : public std::vector<std::string> {
   public:
    bool configured;
    explicit CgiExtensions(const std::vector<std::string> &extensions)
        : std::vector<std::string>(extensions), configured(true) {}
    CgiExtensions() : std::vector<std::string>(), configured(false) {}
  };
  // Composite Configuration Items
  class Location {
   public:
    std::string path;
    Root root;
    Index index;
    std::vector<ErrorPage> error_pages;
    AutoIndex autoindex;
    ClientMaxBodySize client_max_body_size;
    UploadStore upload_store;
    std::vector<RedirectReturn> returns;
    Alias alias;                   // Location only
    LimitExcept limit_except;      // Locatoin only
    CgiExtensions cgi_extensions;  // Location only
    std::vector<Location> locations;
    Location(Command *cmd);
  };
  class Server {
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
    std::vector<RedirectReturn> returns;

    Server();
    Server(Command *cmd);
  };
  class HTTP {
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

  // Configuration
 public:
  HTTP http;

  Config();
  Config(Module *mod);
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const std::vector<T> &v) {
  // if (v.configured) {
  //   std::cout << "[configured] ";
  // }
  for (unsigned int i = 0; i < v.size(); ++i) {
    os << v[i] << " ";
  }
  return os;
}
std::ostream &operator<<(std::ostream &os, const Config::Listen &listen);
std::ostream &operator<<(std::ostream &os, const Config::ErrorPage &error_page);
std::ostream &operator<<(std::ostream &os, const Config::RedirectReturn &ret);
std::ostream &operator<<(std::ostream &os, const Config::Location &l);
std::ostream &operator<<(std::ostream &os, const Config::Server &s);
std::ostream &operator<<(std::ostream &os, const Config::HTTP &http);
std::ostream &operator<<(std::ostream &os, const Config &cf);
void printConfig(const Config &cf);

#endif
