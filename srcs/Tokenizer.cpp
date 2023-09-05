#include "Tokenizer.hpp"

bool ispunct(char c) {
  return c == '{' || c == '}' || c == ';';
}

bool isnumber(const std::string &s) {
  std::string::const_iterator it = s.begin();
  std::string::const_iterator end = s.end();
  for (; it != end; ++it) {
    if (!isdigit(*it)) {
      return false;
    }
  }
  return true;
}

bool issize(const std::string &s) {
  std::string::const_iterator it = s.begin();
  std::string::const_iterator end = s.end();
  for (; it != end; ++it) {
    if (!isdigit(*it)) {
      break ;
    }
  }
  if (it == end) {
    return false;
  }
  switch (*it) {
    case 'k':
      ++it;
      break;
    case 'm':
      ++it;
      break;
    case 'g':
      ++it;
      break;
    default:
      return false;
  }
  return it == end;
}

// TODO: Currently, NGINX doesn't support comment blocks.
// TODO: Currently, we don't support escape characters.
// TODO: Currently, we don't support string literals(quoted strings).
Token* tokenize(const std::string &s) {
  // Dummy head technique can't be used here because dummy's destructor will be called and delete the whole list.
  // Token dummy, *cur = &dummy;
  Token *head, *cur;
  head = cur = NULL;
  std::string::const_iterator it = s.begin();
  std::string::const_iterator end = s.end();
  while (it != end) {
    if (isspace(*it)) {
      ++it;
      continue;
    }
    // Punctuation
    if (ispunct(*it)) {
      if (cur) cur = cur->next = new Token(Token::TK_PUNCT, std::string(1, *it));
      else head = cur = new Token(Token::TK_PUNCT, std::string(1, *it));
      ++it;
      continue;
    }
    // Comment
    if (*it == '#') {
      while (it != end && *it != '\n') {
        ++it;
      }
      continue;
    }

    // String or Number or Size
    // ex.
    //    location
    //    /path/to/file
    //    80
    //    1k 1m 1g
    //    192.168.1.1
    std::string str;
    // Escape characters are roughly supported for escaping space and punctuations.
    // Currently the next character of a backslash is always appended to the string
    // i.e. \t \n \r \v \f \a \b \e \0 characters like these are not supported
    while (it != end && !isspace(*it) && !ispunct(*it)) {
      if (*it == '\\') {
        ++it;
        if (it == end) {
          throw std::runtime_error("Unexpected end of string");
        }
      }
      str += *it;
      ++it;
    }
    if (cur) cur = cur->next = new Token(Token::TK_STR, str);
    else head = cur = new Token(Token::TK_STR, str);
    if (isnumber(str)) {
      cur->type = Token::TK_NUM;
      cur->num = atoi(str.c_str());
    } else if (issize(str)) {
      cur->type = Token::TK_SIZE;
      cur->num = atoi(str.c_str());
      if (str.back() == 'k') {
        cur->num *= KB;
      } else if (str.back() == 'm') {
        cur->num *= MB;
      } else if (str.back() == 'g') {
        cur->num *= GB;
      }
    }
  }
  if (cur) cur = cur->next = new Token(Token::TK_EOF, "");
  else head = cur = new Token(Token::TK_EOF, "");
  return head;
}
