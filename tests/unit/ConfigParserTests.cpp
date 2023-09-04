#include "ConfigParser.hpp"
#include <iostream>
#include <string>

void test_configparser() {
  // Generate Sample Token list
  Token dummy(Token::TK_DUMMY, "");
  {
    Token *cur = &dummy;
    cur = cur->next = new Token(Token::TK_IDENT, "http");
    cur = cur->next = new Token(Token::TK_PUNCT, "{");
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
      cur = cur->next = new Token(Token::TK_PUNCT, "}");
    }
    cur = cur->next = new Token(Token::TK_PUNCT, "}");
    cur = cur->next = new Token(Token::TK_EOF, "");
  }

  // Parse configuration file
  Module *config = parse(dummy.next);
  print_mod(config);
}
