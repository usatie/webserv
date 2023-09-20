#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <iostream>
#include <string>
#include <vector>

#include "Log.hpp"
#include "Tokenizer.hpp"

// These flags are used in nginx.
/*
 *        AAAA  number of arguments
 *      FF      command flags
 *    TT        command type, i.e. HTTP "location" or "server" command
 */

// Syntax
#define WSV_CONF_NOARGS 0x00000001
#define WSV_CONF_TAKE1 0x00000002
#define WSV_CONF_TAKE2 0x00000004
#define WSV_CONF_TAKE3 0x00000008
#define WSV_CONF_TAKE4 0x00000010
#define WSV_CONF_TAKE5 0x00000020
#define WSV_CONF_TAKE6 0x00000040
#define WSV_CONF_TAKE7 0x00000080

#define WSV_CONF_MAX_ARGS 8

#define WSV_CONF_TAKE12 (NGX_CONF_TAKE1 | NGX_CONF_TAKE2)
#define WSV_CONF_TAKE13 (NGX_CONF_TAKE1 | NGX_CONF_TAKE3)

#define WSV_CONF_TAKE23 (NGX_CONF_TAKE2 | NGX_CONF_TAKE3)

#define WSV_CONF_TAKE123 (NGX_CONF_TAKE1 | NGX_CONF_TAKE2 | NGX_CONF_TAKE3)
#define WSV_CONF_TAKE1234 \
  (NGX_CONF_TAKE1 | NGX_CONF_TAKE2 | NGX_CONF_TAKE3 WSV | NGX_CONF_TAKE4)

#define WSV_CONF_ARGS_NUMBER 0x000000ff
#define WSV_CONF_BLOCK 0x00000100
#define WSV_CONF_FLAG 0x00000200
#define WSV_CONF_ANY 0x00000400
#define WSV_CONF_1MORE 0x00000800
#define WSV_CONF_2MORE 0x00001000

// Context
#define WSV_MAIN_CONF 0x01000000
#define WSV_HTTP_MAIN_CONF 0x02000000
#define WSV_HTTP_SRV_CONF 0x04000000
#define WSV_HTTP_LOC_CONF 0x08000000
#define WSV_ANY_CONF 0xFF000000
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
    CMD_CGI_HANDLER,
    CMD_RETURN,
    CMD_LOCATION,
    CMD_ALIAS,
    CMD_SERVER
  };

  Command(enum Type type) : type(type), next(NULL), block(NULL) {
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
      case CMD_CGI_HANDLER:
        name = "cgi_handler";
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
  Command *next;

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

  // CGI Extension / CGI Handler
  std::vector<std::string> cgi_extensions;

  // CGI Handler
  std::string cgi_interpreter_path;

  // Location
  std::string location;

  // Server, Location
  Command *block;

 private:
  Command();  // Do not implement this
};
struct Module {
  enum Type { MOD_HTTP };
  explicit Module(enum Type type) : type(type), block(NULL), next(NULL) {
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
  Module();  // Do not implement this
};

Module *parse(Token *token);
void print_mod(Module *mod);

#endif
