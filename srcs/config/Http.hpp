#ifndef CONFIG_HTTP_HPP
#define CONFIG_HTTP_HPP

#include <vector>

#include "config/AutoIndex.hpp"
#include "config/ClientMaxBodySize.hpp"
#include "config/ErrorPage.hpp"
#include "config/Index.hpp"
#include "config/Root.hpp"
#include "config/Server.hpp"

struct Module;

namespace config {
class HTTP {
 public:
  HTTP() : configured(false) { servers.push_back(Server()); }
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
}  // namespace config

#endif
