#include "config/Location.hpp"

#include "Config.hpp"

namespace config {
Location::Location(Command* loc) : path(loc->location) {
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
std::ostream& operator<<(std::ostream& os, const Location& l) {
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
}  // namespace config
