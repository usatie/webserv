#ifndef CONFIG_LOCATION_HPP
#define CONFIG_LOCATION_HPP

#include <string>
#include <vector>

#include "config/Alias.hpp"
#include "config/AutoIndex.hpp"
#include "config/CgiExtensions.hpp"
#include "config/CgiHandler.hpp"
#include "config/ClientMaxBodySize.hpp"
#include "config/ErrorPage.hpp"
#include "config/Index.hpp"
#include "config/LimitExcept.hpp"
#include "config/RedirectReturn.hpp"
#include "config/Root.hpp"
#include "config/UploadStore.hpp"

struct Command;
namespace config {
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
  Alias alias;               // Location only
  LimitExcept limit_except;  // Locatoin only
  CgiExtensions cgi_extensions;
  std::vector<CgiHandler> cgi_handlers;
  std::vector<Location> locations;
  Location(Command *cmd);
};
}  // namespace config

#endif
