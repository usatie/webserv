#include "Log.hpp"
#include "Tokenizer.hpp"
#include "ConfigParser.hpp"
#include "Config.hpp"

void test_socketbuf();
void test_configparser();
void test_tokenizer();
void test_tokenize_and_parse() {
  Log::setLevel(Log::Debug);
  std::ifstream ifs("conf/default.conf");
  std::string s((std::istreambuf_iterator<char>(ifs)),
                std::istreambuf_iterator<char>());
  try {
    Token *tokens = tokenize(s);
    std::cout << "Tokenize Success!" << std::endl;
    Module *mod = parse(tokens);
    std::cout << "Parse Success!" << std::endl;
    print_mod(mod);
    Config cf(mod);
    std::cout << "Config Success!" << std::endl;
    printConfig(cf);
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
  }
}

int main() {
  Log::setLevel(Log::Warn);
  test_socketbuf();
  test_configparser();
  test_tokenizer();
  test_tokenize_and_parse();
}
