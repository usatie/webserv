#include "Config.hpp"

config::Config::Config() : http() {}

config::Config::Config(Module* mod) {
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

config::Location::Location(Command* loc) : path(loc->location) {
  if (path.empty()) {
    throw std::runtime_error("Empty location path");
  }
  for (Command* cmd = loc->block; cmd; cmd = cmd->next) {
    switch (cmd->type) {
      case Command::CMD_ROOT:
        if (root.configured || alias.configured) {
          throw std::runtime_error("Duplicate root");
        }
        root = Root(cmd->root);
        break;
      case Command::CMD_ALIAS:
        if (alias.configured || root.configured) {
          throw std::runtime_error("Duplicate alias");
        }
        alias = Alias(cmd->alias);
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
      case Command::CMD_UPLOAD_STORE:
        if (upload_store.configured) {
          throw std::runtime_error("Duplicate upload_store");
        }
        upload_store = UploadStore(cmd->upload_store);
        break;
      case Command::CMD_RETURN:
        returns.push_back(RedirectReturn(cmd->return_code, cmd->return_url));
        break;
      case Command::CMD_LIMIT_EXCEPT:
        if (limit_except.configured) {
          throw std::runtime_error("Duplicate limit_except");
        }
        limit_except = LimitExcept(cmd->methods);
        break;
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
      case Command::CMD_LOCATION:
        locations.push_back(Location(cmd));
        break;
      default:
        throw std::runtime_error("Invalid command");
    }
  }
}

config::Server::Server() {
  if (listens.empty()) {
    listens.push_back(Listen());
  }
  if (server_names.empty()) {
    server_names.push_back("");
  }
}

config::Server::Server(Command* srv) {
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

config::HTTP::HTTP() : configured(false) {
  servers.push_back(Server());
}

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
config::HTTP::HTTP(Module* mod) : configured(true) {
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

// Stream
std::ostream& config::operator<<(std::ostream& os, const Listen& listen) {
  os << listen.address << ":" << listen.port;
  return os;
}

std::ostream& config::operator<<(std::ostream& os,
                         const ErrorPage& error_page) {
  os << "{" << error_page.codes << " " << error_page.uri << "}";
  return os;
}

std::ostream& config::operator<<(std::ostream& os, const RedirectReturn& ret) {
  os << ret.code << " " << ret.url << " ";
  return os;
}

std::ostream& config::operator<<(std::ostream& os,
                         const CgiHandler& cgi_handler) {
  os << "{" << cgi_handler.extensions << " " << cgi_handler.interpreter_path
     << "}";
  return os;
}

std::ostream& config::operator<<(std::ostream& os, const Location& l) {
  static std::string spacing = "      ";
  os << spacing << "location: " << l.path << std::endl;
  spacing += "  ";
  if (l.root.configured) os << spacing << "root: " << l.root << std::endl;
  if (l.alias.configured) os << spacing << "alias: " << l.alias << std::endl;
  if (l.index.configured) os << spacing << "index: " << l.index << std::endl;
  if (l.limit_except.configured)
    os << spacing << "limit_except: " << l.limit_except.methods << std::endl;
  if (l.autoindex.configured)
    os << spacing << "autoindex: " << l.autoindex << std::endl;
  if (l.client_max_body_size.configured)
    os << spacing << "client_max_body_size: " << l.client_max_body_size
       << std::endl;
  if (!l.error_pages.empty())
    os << spacing << "error_page: " << l.error_pages << std::endl;
  if (!l.cgi_extensions.empty())
    os << spacing << "cgi_extension: " << l.cgi_extensions << std::endl;
  if (!l.cgi_handlers.empty())
    os << spacing << "cgi_handlers: " << l.cgi_handlers << std::endl;
  if (!l.returns.empty()) os << spacing << "return: " << l.returns << std::endl;
  if (l.upload_store.configured)
    os << spacing << "upload_store: " << l.upload_store << std::endl;
  for (unsigned int j = 0; j < l.locations.size(); ++j) {
    os << l.locations[j];
  }
  spacing.erase(spacing.size() - 2);
  return os;
}

std::ostream& config::operator<<(std::ostream& os, const Server& s) {
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

std::ostream& config::operator<<(std::ostream& os, const HTTP& http) {
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

std::ostream& config::operator<<(std::ostream& os, const Config& cf) {
  os << "Config" << std::endl;
  os << "  HTTP" << std::endl;
  os << cf.http;
  return os;
}

void printConfig(const config::Config& cf) { std::cout << cf; }