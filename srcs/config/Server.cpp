#include "config/Server.hpp"

#include <iostream>

#include "Config.hpp"
#include "util.hpp"

namespace config {
Server::Server(Command* srv) {
  for (Command* cmd = srv->block; cmd; cmd = cmd->next) {
    switch (cmd->type) {
      case Command::CMD_LOCATION:
        locations.push_back(Location(cmd));
        break;
      case Command::CMD_LISTEN:
        listens.push_back(Listen(cmd->address, cmd->port));
        break;
      case Command::CMD_SERVER_NAME:
        server_names.insert(server_names.end(), cmd->server_names.begin(),
                            cmd->server_names.end());
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
      case Command::CMD_ERROR_PAGE: {
        ErrorPage new_error_page(cmd->error_codes, cmd->error_uri);
        error_pages.push_back(new_error_page);
      } break;
      case Command::CMD_AUTOINDEX:
        if (autoindex.configured) {
          throw std::runtime_error("Duplicate autoindex");
        }
        autoindex = AutoIndex(cmd->autoindex);
        break;
      case Command::CMD_UPLOAD_STORE:
        if (upload_store.configured) {
          throw std::runtime_error("Duplicate upload_store");
        }
        upload_store = UploadStore(cmd->upload_store);
        break;
      case Command::CMD_CLIENT_MAX_BODY_SIZE:
        if (client_max_body_size.configured) {
          throw std::runtime_error("Duplicate client_max_body_size");
        }
        client_max_body_size = ClientMaxBodySize(cmd->client_max_body_size);
        break;
      case Command::CMD_RETURN: {
        RedirectReturn new_return(cmd->return_code, cmd->return_url);
        returns.push_back(new_return);
      } break;
      case Command::CMD_CGI_EXTENSION:
        if (cgi_handlers.size() > 0) {
          throw std::runtime_error("cgi_handler is already configured");
        }
        if (!cgi_extensions.configured) {
          cgi_extensions = CgiExtensions(cmd->cgi_extensions);
        } else {
          cgi_extensions.insert(cgi_extensions.end(),
                                cmd->cgi_extensions.begin(),
                                cmd->cgi_extensions.end());
        }
        break;
      case Command::CMD_CGI_HANDLER:
        if (cgi_extensions.configured) {
          throw std::runtime_error("cgi_extensions is already configured");
        }
        cgi_handlers.push_back(
            CgiHandler(cmd->cgi_extensions, cmd->cgi_interpreter_path));
        break;
      default:
        throw std::runtime_error("Invalid command type");
    }
  }
  if (listens.empty()) {
    listens.push_back(Listen());
  }
  // TODO: Check duplicate listens (hostname must be resolved before this)
  // OK: *:8080 and localhost:8080
  // NG: *:8080 and *:8080
  // NG: *:8080 and 8080
  // NG: localhost:8080 and 127.0.0.1:8080
  // server_names duplicates are allowed
  for (unsigned int i = 0; i < server_names.size(); i++) {
    server_names[i] = util::http::normalized_host(server_names[i]);
  }
  if (server_names.empty()) {
    server_names.push_back("");
  }
  for (unsigned int i = 0; i < locations.size(); i++) {
    Location& loc = locations[i];
    // If both root and alias are not configured, inherit root from server
    if (!loc.root.configured && !loc.alias.configured) loc.root = root;
    if (!loc.index.configured) loc.index = index;
    if (!loc.autoindex.configured) loc.autoindex = autoindex;
    if (!loc.upload_store.configured) loc.upload_store = upload_store;
    if (!loc.client_max_body_size.configured)
      loc.client_max_body_size = client_max_body_size;
    // If both cgi_extensions and cgi_handlers are not configured,
    // inherit cgi_extensions or cgi_handlers from server
    if (!loc.cgi_extensions.configured && loc.cgi_handlers.empty()) {
      if (cgi_extensions.configured) {
        loc.cgi_extensions = cgi_extensions;
      } else if (!cgi_handlers.empty()) {
        loc.cgi_handlers = cgi_handlers;
      }
    }
    if (loc.error_pages.empty()) loc.error_pages = error_pages;
    // Return is always inherited if configured
    if (!returns.empty()) loc.returns = returns;
  }
}

std::ostream& operator<<(std::ostream& os, const Server& s) {
  os << "    server: " << std::endl;
  os << "      listen: " << s.listens << std::endl;
  os << "      server_name: " << s.server_names << std::endl;
  if (s.index.configured) os << "      index: " << s.index << std::endl;
  if (s.autoindex.configured)
    os << "      autoindex: " << s.autoindex << std::endl;
  if (s.client_max_body_size.configured)
    os << "      client_max_body_size: " << s.client_max_body_size << std::endl;
  if (s.root.configured) os << "      root: " << s.root << std::endl;
  if (s.error_pages.size() > 0)
    os << "      error_page: " << s.error_pages << std::endl;
  if (s.upload_store.configured)
    os << "      upload_store: " << s.upload_store << std::endl;
  if (s.cgi_extensions.size() > 0)
    os << "        cgi_extension: " << s.cgi_extensions << std::endl;
  if (s.cgi_handlers.size() > 0)
    os << "        cgi_handler: " << s.cgi_handlers << std::endl;
  if (s.returns.size() > 0) os << "      return: " << s.returns << std::endl;
  for (unsigned int j = 0; j < s.locations.size(); ++j) {
    os << s.locations[j];
  }
  return os;
}
}  // namespace config
