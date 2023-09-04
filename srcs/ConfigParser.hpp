#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <string>
#include <vector>
#include <iostream>
#include "Log.hpp"

// These flags are used in nginx.
/*
 *        AAAA  number of arguments
 *      FF      command flags
 *    TT        command type, i.e. HTTP "location" or "server" command
 */

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
struct Token {
  // Generic Members
  enum Type {
    TK_DUMMY,
    TK_NUM,
    TK_IDENT,
    TK_PUNCT,
    TK_EOF
  };
  Token(enum Type type, const std::string &str): type(type), str(str), next(NULL) {}
  ~Token() {
    if (next) {
      delete next;
    }
  }

  enum Type type;
  std::string str;
  Token *next;
  int num;
private:
  Token(); // Do not implement this
};
struct Command {
  // Generic Members
  enum Type {
    CMD_DUMMY,
    CMD_LISTEN,
    CMD_SERVER_NAME,
    CMD_ROOT,
    CMD_INDEX,
    CMD_ERROR_PAGE,
    CMD_AUTO_INDEX,
    CMD_LIMIT_EXCEPT,
    CMD_UPLOAD_STORE,
    CMD_CLIENT_MAX_BODY_SIZE,
    CMD_CGI_EXTENSION,
    CMD_RETURN,
    CMD_LOCATION,
    CMD_ALIAS,
    CMD_SERVER
  };

  Command(enum Type type): type(type), next(NULL), block(NULL) {
    switch (type) {
      case CMD_DUMMY:
        name = "dummy";
        break;
      case CMD_LISTEN:
        name = "listen";
        break;
      case CMD_SERVER_NAME:
        name = "server_name";
        break;
      case CMD_ROOT:
        name = "root";
        break;
      case CMD_INDEX:
        name = "index";
        break;
      case CMD_ERROR_PAGE:
        name = "error_page";
        break;
      case CMD_AUTO_INDEX:
        name = "autoindex";
        break;
      case CMD_LIMIT_EXCEPT:
        name = "limit_except";
        break;
      case CMD_UPLOAD_STORE:
        name = "upload_store";
        break;
      case CMD_CLIENT_MAX_BODY_SIZE:
        name = "client_max_body_size";
        break;
      case CMD_CGI_EXTENSION:
        name = "cgi_extension";
        break;
      case CMD_RETURN:
        name = "return";
        break;
      case CMD_LOCATION:
        name = "location";
        break;
      case CMD_ALIAS:
        name = "alias";
        break;
      case CMD_SERVER:
        name = "server";
        break;
      default:
        name = "unknown";
        break;
    }
  }
  ~Command() {
    if (next) {
      delete next;
    }
    if (block) {
      delete block;
    }
  }

  // Generic
  enum Type type;
  std::string name;
  Command* next;

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
  Command* block;

private:
  Command(); // Do not implement this
};
struct Module {
  enum Type {
    MOD_HTTP
  };
  explicit Module(enum Type type): type(type), block(NULL), next(NULL) {
    if (type == MOD_HTTP) {
      name = "http";
    }
  }
  ~Module() {
    if (block) {
      delete block;
    }
    if (next) {
      delete next;
    }
  }

  enum Type type;
  std::string name;
  Command *block;
  Module *next;

private:
  Module(); // Do not implement this
};

// Do not handle exceptions in config parser
Module* parse(Token *token);
Module* http(Token **rest, Token *tok, int context);
Command* block(Token **rest, Token *tok, int context);
Command* command(Token **rest, Token *tok, int context);
Command* listen(Token **rest, Token *tok, int context);
Command* server_name(Token **rest, Token *tok, int context);
Command* root(Token **rest, Token *tok, int context);
Command* index(Token **rest, Token *tok, int context);
Command* error_page(Token **rest, Token *tok, int context);
Command* auto_index(Token **rest, Token *tok, int context);
Command* limit_except(Token **rest, Token *tok, int context);
Command* upload_store(Token **rest, Token *tok, int context);
Command* client_max_body_size(Token **rest, Token *tok, int context);
Command* cgi_extension(Token **rest, Token *tok, int context);
Command* redirect_return(Token **rest, Token *tok, int context);
Command* location(Token **rest, Token *tok, int context);
Command* alias(Token **rest, Token *tok, int context);
Command* server(Token **rest, Token *tok, int context);


// Utility
bool is_equal(Token *tok, const std::string &str) {
  return tok->str == str;
}

bool consume(Token **rest, Token *tok, const std::string &str) {
  if (is_equal(tok, str)) {
    *rest = tok->next;
    return true;
  }
  return false;
}

Token* skip(Token *tok, const std::string &str) {
  if (!is_equal(tok, str)) {
    Log::cfatal() << "unexpected token: " << tok->str << std::endl;
    throw std::runtime_error("unexpected token");
  }
  return tok->next;
}

Token* skip_kind(Token *tok, Token::Type kind) {
  if (tok->type != kind) {
    Log::cfatal() << "unexpected kind: " << tok->str << std::endl;
    throw std::runtime_error("unexpected kind");
  }
  return tok->next;
}

void expect_context(int context, int expected) {
  if (!(context & expected)) {
    Log::cfatal() << "unexpected context: " << context << std::endl;
    throw std::runtime_error("unexpected context");
  }
}

// config = http
Module* parse(Token *tok) {
  Log::debug("parse");
  Module *mod = http(&tok, tok, WSV_MAIN_CONF);
  if (tok->type != Token::TK_EOF) {
    Log::cfatal() << "unexpected token: " << tok->str << std::endl;
    throw std::runtime_error("unexpected token");
  }
  Log::debug("parse end");
  return mod;
}

// Syntax:	http { ... }
// Default:	—
// Context:	main
Module* http(Token **rest, Token *tok, int context) {
  Log::cdebug() << "http" << std::endl;
  Module *mod = new Module(Module::MOD_HTTP);

  expect_context(context, WSV_MAIN_CONF);
  tok = skip(tok, "http");
  mod->block = block(rest, tok, WSV_HTTP_MAIN_CONF);
  Log::debug("http end");
  return mod;
}

// block = "{" *command "}"
Command* block(Token **rest, Token *tok, int context) {
  Log::cdebug() << "block" << std::endl;
  Command *head = NULL, *cur = NULL;
  tok = skip(tok, "{");
  while (!is_equal(tok, "}")) {
    if (!head)
      head = cur = command(&tok, tok, context);
    else
      cur = cur->next = command(&tok, tok, context);
  }
  *rest = tok->next;
  Log::debug("block end");
  return head;
}

// command = listen
//         | server_name
//         | root
//         | index
//         | error_page
//         | autoindex
//         | limit_except
//         | upload_store
//         | client_max_body_size
//         | cgi_extension
//         | return
//         | location
//         | alias
//         | server
Command* command(Token **rest, Token *tok, int context) {
  Log::debug("command");
  Command *cmd = NULL;

  if (tok->type != Token::TK_IDENT) {
    Log::cfatal() << "unexpected token: " << tok->str << std::endl;
    throw std::runtime_error("unexpected token");
  }
  if (is_equal(tok, "listen")) {
    cmd = listen(rest, tok, context);
  } else if (is_equal(tok, "server_name")) {
    cmd = server_name(rest, tok, context);
  /*} else if (is_equal(tok, "root")) {
    cmd = root(rest, tok, context);
  } else if (is_equal(tok, "index")) {
    cmd = index(rest, tok, context);
  } else if (is_equal(tok, "error_page")) {
    cmd = error_page(rest, tok, context);
  } else if (is_equal(tok, "autoindex")) {
    cmd = autoindex(rest, tok, context);
  } else if (is_equal(tok, "limit_except")) {
    cmd = limit_except(rest, tok, context);
  } else if (is_equal(tok, "upload_store")) {
    cmd = upload_store(rest, tok, context);
  } else if (is_equal(tok, "client_max_body_size")) {
    cmd = client_max_body_size(rest, tok, context);
  } else if (is_equal(tok, "cgi_extension")) {
    cmd = cgi_extension(rest, tok, context);
  } else if (is_equal(tok, "return")) {
    cmd = redirect_return(rest, tok, context);*/
  } else if (is_equal(tok, "location")) {
    cmd = location(rest, tok, context);
  } else if (is_equal(tok, "alias")) {
    cmd = alias(rest, tok, context);
  } else if (is_equal(tok, "server")) {
    cmd = server(rest, tok, context);
  } else {
    Log::cfatal() << "unexpected token: " << tok->str << std::endl;
    throw std::runtime_error("unexpected token");
  }
  return cmd;
}

// Syntax:	listen address[:port] [default_server] [ssl] [http2 | quic] [proxy_protocol] [setfib=number] [fastopen=number] [backlog=number] [rcvbuf=size] [sndbuf=size] [accept_filter=filter] [deferred] [bind] [ipv6only=on|off] [reuseport] [so_keepalive=on|off|[keepidle]:[keepintvl]:[keepcnt]];
// listen port [default_server] [ssl] [http2 | quic] [proxy_protocol] [setfib=number] [fastopen=number] [backlog=number] [rcvbuf=size] [sndbuf=size] [accept_filter=filter] [deferred] [bind] [ipv6only=on|off] [reuseport] [so_keepalive=on|off|[keepidle]:[keepintvl]:[keepcnt]];
// listen unix:path [default_server] [ssl] [http2 | quic] [proxy_protocol] [backlog=number] [rcvbuf=size] [sndbuf=size] [accept_filter=filter] [deferred] [bind] [so_keepalive=on|off|[keepidle]:[keepintvl]:[keepcnt]];
// Default:	listen *:80 | *:8000;
// Context:	server
// TODO: Currently, we only support address and port
Command* listen(Token **rest, Token *tok, int context) {
  Log::debug("listen");
  Command *cmd = new Command(Command::CMD_LISTEN);
  expect_context(context, WSV_HTTP_SRV_CONF);
  tok = skip(tok, "listen");
  // TODO: support wild card
  if (tok->type == Token::TK_IDENT) { // listen address[:port]
    cmd->address = tok->str;
    tok = tok->next;
    if (consume(&tok, tok, ":")) {
      if (tok->type == Token::TK_NUM) {
        cmd->port = tok->num;
        tok = tok->next;
      } else {
        Log::cfatal() << "unexpected token: " << tok->str << std::endl;
        throw std::runtime_error("unexpected token");
      }
    }
  } else if (tok->type == Token::TK_NUM) { // listen port 
    cmd->port = tok->num;
    tok = tok->next;
  } else {
    Log::cfatal() << "unexpected token: " << tok->str << std::endl;
    throw std::runtime_error("unexpected token");
  }
  *rest = skip(tok, ";");
  Log::debug("listen end");
  return cmd;
}
// Syntax:	server_name name ...;
// Default:	server_name "";
// Context:	server
Command* server_name(Token **rest, Token *tok, int context) {
  Log::debug("server_name");
  Command *cmd = new Command(Command::CMD_SERVER_NAME);
  expect_context(context, WSV_HTTP_SRV_CONF);
  tok = skip(tok, "server_name");
  while (tok->type == Token::TK_IDENT) {
    cmd->server_names.push_back(tok->str);
    tok = tok->next;
  }
  *rest = skip(tok, ";");
  Log::debug("server_name end");
  return cmd;
}

// Syntax:	server { ... }
// Default:	—
// Context:	http
Command* server(Token **rest, Token *tok, int context) {
  Log::debug("server");
  Command *cmd = new Command(Command::CMD_SERVER);
  expect_context(context, WSV_HTTP_MAIN_CONF);
  tok = skip(tok, "server");
  cmd->block = block(rest, tok, WSV_HTTP_SRV_CONF);
  Log::debug("server end");
  return cmd;
}

// Syntax:	location [ = | ~ | ~* | ^~ ] uri { ... }
// 			    location @name { ... }
// Default:	—
// Context:	server, location
// TODO: Currently, we do not support regex location and named location
Command* location(Token **rest, Token *tok, int context)
{
  Log::debug("location");
  Command *cmd = new Command(Command::CMD_LOCATION);
  expect_context(context, WSV_HTTP_SRV_CONF | WSV_HTTP_LOC_CONF);
  tok = skip(tok, "location");
  cmd->location = tok->str;
  tok = skip_kind(tok, Token::TK_IDENT);
  cmd->block = block(rest, tok, WSV_HTTP_LOC_CONF);
  Log::debug("location end");
  return cmd;
}

// Syntax: alias path
// Default:	—
// Context:	location
Command* alias(Token **rest, Token *tok, int context)
{
  Log::debug("alias");
  Command *cmd = new Command(Command::CMD_ALIAS);
  expect_context(context, WSV_HTTP_LOC_CONF);
  tok = skip(tok, "alias");
  cmd->alias = tok->str;
  *rest = skip(tok->next, ";");
  Log::debug("alias end");
  return cmd;
}

// debug
void print_cmd(Command *cmd, std::string ident) {
  std::cout << ident << cmd->name << std::endl;
  ident += "  ";
  if (cmd->address != "") std::cout << ident << "address: " << cmd->address << std::endl;
  if (cmd->port != 0) std::cout << ident << "port: " << cmd->port << std::endl;
  if (cmd->alias != "") std::cout << ident << "alias: " << cmd->alias << std::endl;
  if (cmd->location != "") std::cout << ident << "location: " << cmd->location << std::endl;
  if (cmd->server_names.size() > 0) {
    std::cout << ident << "server_names: ";
    for (unsigned long i = 0; i < cmd->server_names.size(); i++) {
      std::cout << cmd->server_names[i] << " ";
    }
    std::cout << std::endl;
  }

  for (Command *c = cmd->block; c; c = c->next) {
    print_cmd(c, ident);
  }
}
void print_mod(Module *mod) {
  std::cout << "Module: " << mod->name << std::endl;
  for (Command *cmd = mod->block; cmd; cmd = cmd->next) {
    print_cmd(cmd, "");
  }
}

#endif
