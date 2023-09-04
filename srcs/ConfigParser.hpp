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

#define KB 1024
#define MB 1024 * 1024
#define GB 1024 * 1024 * 1024

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
    TK_SIZE,
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
    CMD_AUTOINDEX,
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
      case CMD_AUTOINDEX:
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
  std::vector<std::string> index_files;

  // ErrorPage
  std::vector<int> error_codes;
  // int response_code; // Do not support
  std::string error_uri;

  // LimitExcept
  std::vector<std::string> methods;

  // RedirectReturn
  int return_code;
  std::string return_url;

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
Command* autoindex(Token **rest, Token *tok, int context);
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
// Memo: 1. duplicate is not allowed
//       2. http block can be ommitted (but we require it)
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
  } else if (is_equal(tok, "root")) {
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
    cmd = redirect_return(rest, tok, context);
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
// Memo: 1. multiple listen is allowed
//       2. however, complete duplicate is not allowed
//          ex. OK
//          listen 80;
//          listen [::]:80;
//          ex. NG
//          listen 80;
//          listen 80;
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
// Memo: 1. duplicate is OK
//       2. name can also be duplicated
//       3. max number of server_name is ?
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

// Syntax:	root path;
// Default:	root "html";
// Context:	html, server, location
// Memo: 1. duplicate is not allowed
//       2. path can be relative like "./dir" or "dir"
//       3. path can be absolute like "/etc/nginx"
//       4. double quotes are allowed
//       5. Both directive name and args can be double quoted
Command* root(Token **rest, Token *tok, int context) {
  Log::debug("root");
  Command *cmd = new Command(Command::CMD_ROOT);
  expect_context(context, WSV_HTTP_MAIN_CONF | WSV_HTTP_SRV_CONF | WSV_HTTP_LOC_CONF);
  tok = skip(tok, "root");
  cmd->root = tok->str;
  tok = skip_kind(tok, Token::TK_IDENT);
  *rest = skip(tok, ";");
  Log::debug("root end");
  return cmd;
}

// Syntax:	index file ...;
// Default:	index index.html;
// Context:	http, server, location
// Memo: 1. duplicate is not allowed
//       2. If the index file is not found, 403 is returned
//       3. Inner index directive overwrites outer index directive
Command* index(Token **rest, Token *tok, int context) {
  Log::debug("index");
  Command *cmd = new Command(Command::CMD_INDEX);
  expect_context(context, WSV_HTTP_MAIN_CONF | WSV_HTTP_SRV_CONF | WSV_HTTP_LOC_CONF);
  tok = skip(tok, "index");
  while (tok->type == Token::TK_IDENT) {
    cmd->index_files.push_back(tok->str);
    tok = tok->next;
  }
  if (cmd->index_files.size() == 0) {
    throw std::runtime_error("invalid number of arguments in \"index\" directive");
  }
  *rest = skip(tok, ";");
  Log::debug("index end");
  return cmd;
}

// Syntax:	error_page code ... [=[response]] uri;
// Default:	—
// Context:	http, server, location, if in location
// TODO: Currently, we only do not support response
Command* error_page(Token **rest, Token *tok, int context) {
  Log::debug("error_page");
  Command *cmd = new Command(Command::CMD_ERROR_PAGE);
  expect_context(context, WSV_HTTP_MAIN_CONF | WSV_HTTP_SRV_CONF | WSV_HTTP_LOC_CONF);
  tok = skip(tok, "error_page");
  while (tok->type == Token::TK_NUM) {
    cmd->error_codes.push_back(tok->num);
    tok = tok->next;
  }
  if (cmd->error_codes.size() == 0) {
    throw std::runtime_error("invalid number of arguments in \"error_page\" directive");
  }
  cmd->error_uri = tok->str;
  tok = skip_kind(tok, Token::TK_IDENT);
  *rest = skip(tok, ";");
  Log::debug("error_page end");
  return cmd;
}

// Syntax:	autoindex on | off;
// Default:	autoindex off;
// Context:	http, server, location
Command* autoindex(Token **rest, Token *tok, int context) {
  Log::debug("autoindex");
  Command *cmd = new Command(Command::CMD_AUTOINDEX);
  expect_context(context, WSV_HTTP_MAIN_CONF | WSV_HTTP_SRV_CONF | WSV_HTTP_LOC_CONF);
  tok = skip(tok, "autoindex");
  if (consume(&tok, tok, "on")) {
    cmd->autoindex = true;
  } else if (consume(&tok, tok, "off")) {
    cmd->autoindex = false;
  } else {
    throw std::runtime_error("invalid argument in \"autoindex\" directive");
  }
  *rest = skip(tok, ";");
  Log::debug("autoindex end");
  return cmd;
}

// Syntax:	limit_except method ... { ... }
// Default:	—
// Context:	location
// TODO: Currently, we do not support block
Command* limit_except(Token **rest, Token *tok, int context) {
  Log::debug("limit_except");
  Command *cmd = new Command(Command::CMD_LIMIT_EXCEPT);
  expect_context(context, WSV_HTTP_LOC_CONF);
  tok = skip(tok, "limit_except");
  while (tok->type == Token::TK_IDENT) {
    cmd->methods.push_back(tok->str);
    tok = tok->next;
  }
  if (cmd->methods.size() == 0) {
    throw std::runtime_error("invalid number of arguments in \"limit_except\" directive");
  }
  //cmd->block = block(rest, tok, WSV_HTTP_LOC_CONF);
  *rest = skip(tok, ";"); // This is different from NGINX
  Log::debug("limit_except end");
  return cmd;
}

// Syntax:	upload_store <directory> [<level 1> [<level 2> ] … ]
// Default:	none
// Context:	server,location
// TODO: Currently, we do not support level
Command* upload_store(Token **rest, Token *tok, int context) {
  Log::debug("upload_store");
  Command *cmd = new Command(Command::CMD_UPLOAD_STORE);
  expect_context(context, WSV_HTTP_SRV_CONF | WSV_HTTP_LOC_CONF);
  tok = skip(tok, "upload_store");
  cmd->upload_store = tok->str;
  tok = skip_kind(tok, Token::TK_IDENT);
  *rest = skip(tok, ";");
  Log::debug("upload_store end");
  return cmd;
}

// Syntax:	client_max_body_size size;
// Default:	client_max_body_size 1m;
// Context:	http, server, location
Command* client_max_body_size(Token **rest, Token *tok, int context) {
  Log::debug("client_max_body_size");
  Command *cmd = new Command(Command::CMD_CLIENT_MAX_BODY_SIZE);
  expect_context(context, WSV_HTTP_MAIN_CONF | WSV_HTTP_SRV_CONF | WSV_HTTP_LOC_CONF);
  tok = skip(tok, "client_max_body_size");
  if (tok->type == Token::TK_NUM || tok->type == Token::TK_SIZE) {
    cmd->client_max_body_size = tok->num;
    tok = tok->next;
  } else {
    throw std::runtime_error("invalid argument in \"client_max_body_size\" directive");
  }
  *rest = skip(tok, ";");
  Log::debug("client_max_body_size end");
  return cmd;
}

// Syntax:	cgi_extension extension ...;
// Default:	none
// Context:	server, location
Command* cgi_extension(Token **rest, Token *tok, int context) {
  Log::debug("cgi_extension");
  Command *cmd = new Command(Command::CMD_CGI_EXTENSION);
  expect_context(context, WSV_HTTP_SRV_CONF | WSV_HTTP_LOC_CONF);
  tok = skip(tok, "cgi_extension");
  while (tok->type == Token::TK_IDENT) {
    cmd->cgi_extensions.push_back(tok->str);
    tok = tok->next;
  }
  if (cmd->cgi_extensions.size() == 0) {
    throw std::runtime_error("invalid number of arguments in \"cgi_extension\" directive");
  }
  *rest = skip(tok, ";");
  Log::debug("cgi_extension end");
  return cmd;
}

// Syntax:	return code [text];
// 			    return code URL;
// 			    return URL;
// Default:	—
// Context:	server, location, if
// Memo: 1. URL can be empty for 301, 302, 303, 307, 308
//       2. Some codes return default page
// TODO: Currently, we do not support text
Command* redirect_return(Token **rest, Token *tok, int context) {
  Log::debug("redirect_return");
  Command *cmd = new Command(Command::CMD_RETURN);
  expect_context(context, WSV_HTTP_SRV_CONF | WSV_HTTP_LOC_CONF);
  tok = skip(tok, "return");
  if (tok->type == Token::TK_NUM) {
    cmd->return_code = tok->num;
    tok = tok->next;
  }
  // URL is supported only for codes
  // 301, 302, 303, 307, 308
  if (tok->type == Token::TK_IDENT) {
    switch (cmd->return_code) {
      case 301:
      case 302:
      case 303:
      case 307:
      case 308:
        break;
      default:
        throw std::runtime_error("invalid argument in \"return\" directive");
    }
    cmd->return_url = tok->str;
    tok = tok->next;
  }
  *rest = skip(tok, ";");
  Log::debug("redirect_return end");
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
  std::cout << ident << cmd->name << ":" << std::endl;
  ident += "  ";
  if (cmd->address != "") std::cout << ident << "address: " << cmd->address << std::endl;
  if (cmd->port != 0) std::cout << ident << "port: " << cmd->port << std::endl;
  if (cmd->alias != "") std::cout << ident << cmd->alias << std::endl;
  if (cmd->location != "") std::cout << ident << cmd->location << std::endl;
  if (cmd->root != "") std::cout << ident << cmd->root << std::endl;
  if (cmd->upload_store != "") std::cout << ident << cmd->upload_store << std::endl;
  if (cmd->client_max_body_size != 0) std::cout << ident << cmd->client_max_body_size << std::endl;
  if (cmd->cgi_extensions.size() > 0) {
    std::cout << ident;
    for (unsigned long i = 0; i < cmd->cgi_extensions.size(); i++) {
      std::cout << cmd->cgi_extensions[i] << " ";
    }
    std::cout << std::endl;
  }
  if (cmd->server_names.size() > 0) {
    std::cout << ident;
    for (unsigned long i = 0; i < cmd->server_names.size(); i++) {
      std::cout << cmd->server_names[i] << " ";
    }
    std::cout << std::endl;
  }
  if (cmd->index_files.size() > 0) {
    std::cout << ident;
    for (unsigned long i = 0; i < cmd->index_files.size(); i++) {
      std::cout << cmd->index_files[i] << " ";
    }
    std::cout << std::endl;
  }
  if (cmd->error_codes.size() > 0) {
    std::cout << ident << "code: ";
    for (unsigned long i = 0; i < cmd->error_codes.size(); i++) {
      std::cout << cmd->error_codes[i] << " ";
    }
    std::cout << std::endl;
  }
  if (cmd->error_uri != "") std::cout << ident << "uri: " << cmd->error_uri << std::endl;
  if (cmd->type == Command::CMD_AUTOINDEX) {
    if (cmd->autoindex) {
      std::cout << ident << "on" << std::endl;
    } else {
      std::cout << ident << "off" << std::endl;
    }
  }
  if (cmd->type == Command::CMD_LIMIT_EXCEPT) {
    std::cout << ident;
    for (unsigned long i = 0; i < cmd->methods.size(); i++) {
      std::cout << cmd->methods[i] << " ";
    }
    std::cout << std::endl;
  }
  if (cmd->type == Command::CMD_RETURN) {
    if (cmd->return_code != 0) std::cout << ident << "code: " << cmd->return_code << std::endl;
    std::cout << ident << "url: " << cmd->return_url << std::endl;
  }

  for (Command *c = cmd->block; c; c = c->next) {
    print_cmd(c, ident);
  }
}
void print_mod(Module *mod) {
  std::cout << "Module: " << mod->name << std::endl;
  for (Command *cmd = mod->block; cmd; cmd = cmd->next) {
    print_cmd(cmd, "  ");
  }
}

#endif
