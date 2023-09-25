#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <string>

#define KB (1024)
#define MB (1024 * 1024)
#define GB (1024 * 1024 * 1024)

struct Token {
  // Generic Members
  enum Type { TK_DUMMY, TK_NUM, TK_SIZE, TK_STR, TK_PUNCT, TK_EOF };
  Token(enum Type type, const std::string &str)
      : type(type), str(str), next(NULL), num(0) {}
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
  Token();  // Do not implement this
};

Token *tokenize(const std::string &s);

#endif
