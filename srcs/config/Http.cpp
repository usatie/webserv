#include "config/Http.hpp"

#include "Config.hpp"
#include "ConfigParser.hpp"

namespace config {
// About Inheritance of directives
//
// Inheritance
// In general, a child context – one contained within another context (its
// parent) – inherits the settings of directives included at the parent level.
// Some directives can appear in multiple contexts, in which case you can
// override the setting inherited from the parent by including the directive in
// the child context. For an example, see the proxy_set_header directive. cf.
// https://docs.nginx.com/nginx/admin-guide/basic-functionality/managing-configuration-files/
//
// If you define multiple directives in different contexts then the lower
// context will replace the higher context ones. cf.
// https://blog.martinfjordvald.com/understanding-the-nginx-configuration-inheritance-model/
HTTP::HTTP(Module* mod) : configured(true) {
  for (Command* cmd = mod->block; cmd; cmd = cmd->next) {
    switch (cmd->type) {
      case Command::CMD_SERVER:
        servers.push_back(Server(cmd));
        break;
      case Command::CMD_ROOT:
        if (root.configured) {
          throw std::runtime_error("Duplicate root");
        }
        root = Root(cmd->root);
        break;
      case Command::CMD_INDEX:
        if (!index.configured) {
          index = Index(cmd->index_files);
        } else {
          index.insert(index.end(), cmd->index_files.begin(),
                       cmd->index_files.end());
        }
        break;
      case Command::CMD_ERROR_PAGE:
        error_pages.push_back(ErrorPage(cmd->error_codes, cmd->error_uri));
        break;
      case Command::CMD_AUTOINDEX:
        if (autoindex.configured) {
          throw std::runtime_error("Duplicate autoindex");
        }
        autoindex = AutoIndex(cmd->autoindex);
        break;
      case Command::CMD_CLIENT_MAX_BODY_SIZE:
        if (client_max_body_size.configured) {
          throw std::runtime_error("Duplicate client_max_body_size");
        }
        client_max_body_size = ClientMaxBodySize(cmd->client_max_body_size);
        break;
      default:
        throw std::runtime_error("Invalid command type");
    }
  }
  if (servers.empty()) {
    servers.push_back(Server());
  }
  for (unsigned int i = 0; i < servers.size(); i++) {
    Server& srv = servers[i];
    if (!srv.root.configured) srv.root = root;
    if (!srv.index.configured) srv.index = index;
    if (!srv.autoindex.configured) srv.autoindex = autoindex;
    if (!srv.client_max_body_size.configured)
      srv.client_max_body_size = client_max_body_size;
    if (srv.error_pages.empty()) srv.error_pages = error_pages;
  }
}
std::ostream& operator<<(std::ostream& os, const HTTP& http) {
  if (http.root.configured) {
    os << "    root: " << http.root << std::endl;
  } else {
    os << "    root: " << http.root << " [default]" << std::endl;
  }
  if (http.index.configured) {
    os << "    index: " << http.index << std::endl;
  } else {
    os << "    index: " << http.index << "[default]" << std::endl;
  }
  if (http.autoindex.configured) {
    os << "    autoindex: " << http.autoindex << std::endl;
  }
  if (http.client_max_body_size.configured) {
    os << "    client_max_body_size: " << http.client_max_body_size
       << std::endl;
  }
  if (http.error_pages.size() > 0) {
    os << "    error_page: " << http.error_pages << std::endl;
  }

  for (unsigned int i = 0; i < http.servers.size(); ++i) {
    os << http.servers[i];
  }
  return os;
}
}  // namespace config
