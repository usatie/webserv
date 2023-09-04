#include "ConfigParser.hpp"
#include <iostream>
#include <string>

void test_configparser() {
  Log::setLevel(Log::Debug);

  // Generate Sample Token list
  Token dummy(Token::TK_DUMMY, "");
  {
    Token *cur = &dummy;
    cur = cur->next = new Token(Token::TK_IDENT, "http");
    cur = cur->next = new Token(Token::TK_PUNCT, "{");
    cur = cur->next = new Token(Token::TK_IDENT, "root");
    cur = cur->next = new Token(Token::TK_IDENT, "/var/www/html");
    cur = cur->next = new Token(Token::TK_PUNCT, ";");
    cur = cur->next = new Token(Token::TK_IDENT, "index");
    cur = cur->next = new Token(Token::TK_IDENT, "index.html");
    cur = cur->next = new Token(Token::TK_PUNCT, ";");
    cur = cur->next = new Token(Token::TK_IDENT, "error_page");
    cur = cur->next = new Token(Token::TK_NUM, "500");
    cur->num = 500;
    cur = cur->next = new Token(Token::TK_NUM, "501");
    cur->num = 501;
    cur = cur->next = new Token(Token::TK_IDENT, "/50x.html");
    cur = cur->next = new Token(Token::TK_PUNCT, ";");
    cur = cur->next = new Token(Token::TK_IDENT, "autoindex");
    cur = cur->next = new Token(Token::TK_IDENT, "on");
    cur = cur->next = new Token(Token::TK_PUNCT, ";");
    {
      cur = cur->next = new Token(Token::TK_IDENT, "server");
      cur = cur->next = new Token(Token::TK_PUNCT, "{");
      cur = cur->next = new Token(Token::TK_IDENT, "listen");
      cur = cur->next = new Token(Token::TK_NUM, "80");
      cur->num = 80;
      cur = cur->next = new Token(Token::TK_PUNCT, ";");
      cur = cur->next = new Token(Token::TK_IDENT, "server_name");
      cur = cur->next = new Token(Token::TK_IDENT, "localhost");
      cur = cur->next = new Token(Token::TK_PUNCT, ";");
      cur = cur->next = new Token(Token::TK_IDENT, "root");
      cur = cur->next = new Token(Token::TK_IDENT, "/var/www/html/localhost");
      cur = cur->next = new Token(Token::TK_PUNCT, ";");
      cur = cur->next = new Token(Token::TK_IDENT, "index");
      cur = cur->next = new Token(Token::TK_IDENT, "hello.html");
      cur = cur->next = new Token(Token::TK_PUNCT, ";");
      {
        cur = cur->next = new Token(Token::TK_IDENT, "location");
        cur = cur->next = new Token(Token::TK_IDENT, "/hello");
        cur = cur->next = new Token(Token::TK_PUNCT, "{");
        cur = cur->next = new Token(Token::TK_IDENT, "limit_except");
        cur = cur->next = new Token(Token::TK_IDENT, "GET");
        cur = cur->next = new Token(Token::TK_IDENT, "POST");
        cur = cur->next = new Token(Token::TK_PUNCT, ";");
        cur = cur->next = new Token(Token::TK_PUNCT, "}");
      }
      cur = cur->next = new Token(Token::TK_PUNCT, "}");
    }
    cur = cur->next = new Token(Token::TK_PUNCT, "}");
    cur = cur->next = new Token(Token::TK_EOF, "");
  }

  // Parse configuration file
  Module *config = parse(dummy.next);
  print_mod(config);
}
