#include "Log.hpp"
#include "Tokenizer.hpp"
#include "ConfigParser.hpp"
#include "Config.hpp"
#include "test_util.hpp"

int test_socketbuf();
int test_configparser();
int test_tokenizer();

int test_tokenize_and_parse() {
  //Log::setLevel(Log::Debug);
  std::ifstream ifs("conf/default.conf");
  std::string s((std::istreambuf_iterator<char>(ifs)),
                std::istreambuf_iterator<char>());
  try {
    title("Tokenize");
    Token *tokens = tokenize(s);
    std::cout << "Tokenize Success!" << std::endl;
    title("Parse");
    Module *mod = parse(tokens);
    std::cout << "Parse Success!" << std::endl;
    print_mod(mod);
    title("Config");
    Config cf(mod);
    std::cout << "Config Success!" << std::endl;
    printConfig(cf);
    return 0;
  } catch (std::exception &e) {
    std::cout << e.what() << std::endl;
    return -1;
  }
}

int main() {
  Log::setLevel(Log::Warn);
  int ok = 0;
  ok |= test_socketbuf();
  ok |= test_configparser();
  ok |= test_tokenizer();
  ok |= test_tokenize_and_parse();
  return ok;
}
