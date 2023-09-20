#ifndef CONFIG_SERVER_HPP
#define CONFIG_SERVER_HPP

#include <string>
#include <vector>

#include "config/AutoIndex.hpp"
#include "config/CgiExtensions.hpp"
#include "config/CgiHandler.hpp"
#include "config/ClientMaxBodySize.hpp"
#include "config/ErrorPage.hpp"
#include "config/Index.hpp"
#include "config/Listen.hpp"
#include "config/Location.hpp"
#include "config/RedirectReturn.hpp"
#include "config/Root.hpp"
#include "config/UploadStore.hpp"

namespace config {
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
  std::vector<CgiHandler> cgi_handlers;
  std::vector<RedirectReturn> returns;

  Server();
  Server(Command *cmd);
};
}  // namespace config

#endif
