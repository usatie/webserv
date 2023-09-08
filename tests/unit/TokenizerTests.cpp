#include "Tokenizer.hpp"
#include "Log.hpp"

#include <iostream>
#include <string>
#include <fstream>

void test_tokenizer() {
  //Log::setLevel(Log::Debug);
  std::ifstream ifs("conf/default.conf");
  std::string s((std::istreambuf_iterator<char>(ifs)),
                std::istreambuf_iterator<char>());
  try {
    Token *tokens = tokenize(s);
    (void)tokens;
    std::cout << "Tokenize Success!" << std::endl;
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
}
