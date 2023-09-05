#include "ConfigParser.hpp"
#include <iostream>
#include <string>

void test_configparser() {
  Log::setLevel(Log::Debug);

  // Generate Sample Token list
  Token dummy(Token::TK_DUMMY, "");
  {
    Token *cur = &dummy;
    cur = cur->next = new Token(Token::TK_STR, "http");
    cur = cur->next = new Token(Token::TK_PUNCT, "{");
    cur = cur->next = new Token(Token::TK_STR, "root");
    cur = cur->next = new Token(Token::TK_STR, "/var/www/html");
    cur = cur->next = new Token(Token::TK_PUNCT, ";");
    cur = cur->next = new Token(Token::TK_STR, "index");
    cur = cur->next = new Token(Token::TK_STR, "index.html");
    cur = cur->next = new Token(Token::TK_PUNCT, ";");
    cur = cur->next = new Token(Token::TK_STR, "error_page");
    cur = cur->next = new Token(Token::TK_NUM, "500");
    cur->num = 500;
    cur = cur->next = new Token(Token::TK_NUM, "501");
    cur->num = 501;
    cur = cur->next = new Token(Token::TK_STR, "/50x.html");
    cur = cur->next = new Token(Token::TK_PUNCT, ";");
    cur = cur->next = new Token(Token::TK_STR, "autoindex");
    cur = cur->next = new Token(Token::TK_STR, "on");
    cur = cur->next = new Token(Token::TK_PUNCT, ";");
    {
      cur = cur->next = new Token(Token::TK_STR, "server");
      cur = cur->next = new Token(Token::TK_PUNCT, "{");
      cur = cur->next = new Token(Token::TK_STR, "listen");
      cur = cur->next = new Token(Token::TK_NUM, "80");
      cur->num = 80;
      cur = cur->next = new Token(Token::TK_PUNCT, ";");
      cur = cur->next = new Token(Token::TK_STR, "server_name");
      cur = cur->next = new Token(Token::TK_STR, "localhost");
      cur = cur->next = new Token(Token::TK_PUNCT, ";");
      cur = cur->next = new Token(Token::TK_STR, "root");
      cur = cur->next = new Token(Token::TK_STR, "/var/www/html/localhost");
      cur = cur->next = new Token(Token::TK_PUNCT, ";");
      cur = cur->next = new Token(Token::TK_STR, "index");
      cur = cur->next = new Token(Token::TK_STR, "hello.html");
      cur = cur->next = new Token(Token::TK_PUNCT, ";");
      {
        cur = cur->next = new Token(Token::TK_STR, "location");
        cur = cur->next = new Token(Token::TK_STR, "/hello");
        cur = cur->next = new Token(Token::TK_PUNCT, "{");
        cur = cur->next = new Token(Token::TK_STR, "limit_except");
        cur = cur->next = new Token(Token::TK_STR, "GET");
        cur = cur->next = new Token(Token::TK_STR, "POST");
        cur = cur->next = new Token(Token::TK_PUNCT, ";");
        cur = cur->next = new Token(Token::TK_PUNCT, "}");
      }
      {
        cur = cur->next = new Token(Token::TK_STR, "location");
        cur = cur->next = new Token(Token::TK_STR, "/world");
        cur = cur->next = new Token(Token::TK_PUNCT, "{");
        cur = cur->next = new Token(Token::TK_STR, "upload_store");
        cur = cur->next = new Token(Token::TK_STR, "/tmp");
        cur = cur->next = new Token(Token::TK_PUNCT, ";");
        cur = cur->next = new Token(Token::TK_PUNCT, "}");
      }
      {
        cur = cur->next = new Token(Token::TK_STR, "location");
        cur = cur->next = new Token(Token::TK_STR, "/max");
        cur = cur->next = new Token(Token::TK_PUNCT, "{");
        cur = cur->next = new Token(Token::TK_STR, "client_max_body_size");
        cur = cur->next = new Token(Token::TK_NUM, "1000000");
        cur->num = 1000000;
        cur = cur->next = new Token(Token::TK_PUNCT, ";");
        cur = cur->next = new Token(Token::TK_STR, "client_max_body_size");
        cur = cur->next = new Token(Token::TK_SIZE, "1g");
        cur->num = 1 * GB;
        cur = cur->next = new Token(Token::TK_PUNCT, ";");
        cur = cur->next = new Token(Token::TK_PUNCT, "}");
      }
      // cgi_extensions
      {
        cur = cur->next = new Token(Token::TK_STR, "location");
        cur = cur->next = new Token(Token::TK_STR, "/cgi");
        cur = cur->next = new Token(Token::TK_PUNCT, "{");
        cur = cur->next = new Token(Token::TK_STR, "cgi_extension");
        cur = cur->next = new Token(Token::TK_STR, ".py");
        cur = cur->next = new Token(Token::TK_STR, ".php");
        cur = cur->next = new Token(Token::TK_PUNCT, ";");
        cur = cur->next = new Token(Token::TK_PUNCT, "}");
      }
      // return
      {
        cur = cur->next = new Token(Token::TK_STR, "location");
        cur = cur->next = new Token(Token::TK_STR, "/return");
        cur = cur->next = new Token(Token::TK_PUNCT, "{");
        cur = cur->next = new Token(Token::TK_STR, "return");
        cur = cur->next = new Token(Token::TK_NUM, "301");
        cur->num = 301;
        cur = cur->next = new Token(Token::TK_STR, "http://www.google.com");
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
