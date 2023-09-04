#ifndef CONFIG_HPP
#define CONFIG_HPP

/*
 *
 *
 *
 * This file will be deleted soon.
 * This file was the first draft.
 * I decided to use a different approach.
 *
 *
 *
 * /

/*
#include <string>
// shared_ptr
#include <memory>

#define MB 1024 * 1024

class HTTPModule {
public:
  std::shared_ptr<HTTPModule> next;
  std::shared_ptr<Command> commands;
  std::shared_ptr<Command> servers;
  std::shared_ptr<Command> locations;
};
*/

// These flags are used in nginx.
/*
 *        AAAA  number of arguments
 *      FF      command flags
 *    TT        command type, i.e. HTTP "location" or "server" command
 */

/*
// Syntax
#define WSV_CONF_NOARGS           0x00000001
#define WSV_CONF_TAKE1            0x00000002
#define WSV_CONF_TAKE2            0x00000004
#define WSV_CONF_TAKE3            0x00000008
#define WSV_CONF_TAKE4            0x00000010
#define WSV_CONF_TAKE5            0x00000020
#define WSV_CONF_TAKE6            0x00000040
#define WSV_CONF_TAKE7            0x00000080

#define WSV_CONF_MAX_ARGS         8

#define WSV_CONF_TAKE12           (NGX_CONF_TAKE1|NGX_CONF_TAKE2)
#define WSV_CONF_TAKE13           (NGX_CONF_TAKE1|NGX_CONF_TAKE3)

#define WSV_CONF_TAKE23           (NGX_CONF_TAKE2|NGX_CONF_TAKE3)

#define WSV_CONF_TAKE123          (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3)
#define WSV_CONF_TAKE1234         (NGX_CONF_TAKE1|NGX_CONF_TAKE2|NGX_CONF_TAKE3   \
        WSV                        |NGX_CONF_TAKE4)

#define WSV_CONF_ARGS_NUMBER      0x000000ff
#define WSV_CONF_BLOCK            0x00000100
#define WSV_CONF_FLAG             0x00000200
#define WSV_CONF_ANY              0x00000400
#define WSV_CONF_1MORE            0x00000800
#define WSV_CONF_2MORE            0x00001000

// Context
#define WSV_MAIN_CONF             0x01000000
#define WSV_HTTP_MAIN_CONF        0x02000000
#define WSV_HTTP_SRV_CONF         0x04000000
#define WSV_HTTP_LOC_CONF         0x08000000
#define WSV_ANY_CONF              0xFF000000

struct CommandRule {
  std::string name;
  int syntax;
  Command defaultValue;
  int context;
};

struct CommandRule[] rules = {
  {
    "listen",
    WSV_CONF_TAKE1,
    Listen(),
    WSV_HTTP_MAIN_CONF | WSV_HTTP_SRV_CONF | WSV_CONF_1MORE,
  }
};

class Listen: public Command {
private:
  Listen(const Listen &); // Do not implement this
  Listen &operator=(const Listen &); // Do not implement this
public:
  std::string address;
  int port;
  Listen(): Command(CMD_LISTEN), address(""), port(8080) {}
  Listen(std::string address, int port): Command(Listen), address(address), port(port) {}
};


class Command {
private:
  Command(); // Do not implement this
public:
  // Generic Members
  enum Type {
    // Server
    Listen,
    ServerName,
    Root,
    Index,
    ErrorPage,
    AutoIndex,
    LimitExcept,
    UploadStore,
    ClientMaxBodySize,
    CGIExtension,
    RedirectReturn,
    // Location
    Location,
    Alias,
    // Generic
    Unknown,
  };

  enum Type type;
  std::shared_ptr<Command> next;
  Command(enum Type type): type(type) {}

  // Listen
  std::string address;
  int port;

  // ServerName
  std::vector<std::string> server_names;

  // Root
  std::string root;

  // Alias
  std::string alias;

  // Index
  std::string index;

  // ErrorPage
  std::vector<int> codes;
  // int response_code; // Do not support
  std::string uri;

  // LimitExcept
  std::vector<std::string> methods;

  // RedirectReturn
  int code;
  std::string url;

  // AutoIndex
  bool autoindex;

  // ClientMaxBodySize
  size_t client_max_body_size;

  // UploadStore
  std::string upload_store;

  // CGI Extension
  std::vector<std::string> cgi_extensions;

  // Location
  std::string location;
  
  // Server, Location
  std::shared_ptr<Command> commands;
  std::shared_ptr<Command> locations;
};

class Config {
  // Primitive Configuration Items
  class Listen {
  public:
    std::string address; // host
    int port;
  };
  class ErrorPage {
  public:
    int code;
    int response_code;
    std::string uri;
  };
  class LimitExcept {
  public:
    std::vector<std::string> methods;
  };
  class RedirectReturn {
  public:
    int code;
    std::string url;
  };
  class Root: public std::string {
    public:
      Root(const std::string &path): std::string(path) {}
      Root(): std::string("html") {}
  }
  // Composite Configuration Items
  class Location {
  public:
    Root root;
    std::string index;
    std::string alias;
    ErrorPage error_page;
    LimitExcept limit_except;
    bool autoindex;
    std::string upload_store;
    size_t client_max_body_size;
    std::string cgi_extension;
    RedirectReturn redirect_return;
    // std::shared_ptr<Location> child; // Does not support nested location.
  };
  class Server {
  public:
  public:
    std::vector<Location> location;
    std::vector<Listen> listen;
    std::vector<std::string> server_names;
    std::string root;
    std::string index;
    ErrorPage error_page;
    bool autoindex;
    std::string upload_store;
    size_t client_max_body_size;
    std::string cgi_extension;
    RedirectReturn redirect_return;
  };
  class HTTP {
  public:
    HTTP(): servers(), root("html"), index("index.html"), error_pages(), autoindex(false), client_max_body_size(1 * MB) {}
    ~HTTP() {}
    HTTP(const HTTP &rhs): servers(rhs.servers), root(rhs.root), index(rhs.index), error_pages(rhs.error_pages), autoindex(rhs.autoindex), client_max_body_size(rhs.client_max_body_size) {}
    HTTP &operator=(const HTTP &rhs) {
      if (this != &rhs) {
        servers = rhs.servers;
        root = rhs.root;
        index = rhs.index;
        error_pages = rhs.error_pages;
        autoindex = rhs.autoindex;
        client_max_body_size = rhs.client_max_body_size;
      }
      return *this;
    }
  public:
    std::vector<Server> servers;
    std::string root;
    std::string index;
    std::vector<ErrorPage> error_pages;
    bool autoindex;
    size_t client_max_body_size;
  };

  // Configuration
  public:
    HTTP http;
};
*/

#endif
