#include <sstream>
#include <iostream>
#include <string>

int main() {
  std::stringstream ss;
  //ss << "hoge\nfuga";
  ss << "a" ;
  std::string line;
  while (std::getline(ss, line)) {
    if (ss.eof()) {
      std::cout << "EOF" << std::endl;
      ss.seekg(-line.size(), std::ios::cur);
      break;
    }
    std::cout << line << std::endl;
    std::cout << ss.eof() << std::endl;
  }
  std::cout << "----------------" << std::endl;
  std::cout << "ss.fail() = " << ss.fail() << std::endl;
  std::cout << "ss.bad() = " << ss.bad() << std::endl;
  std::cout << "ss.eof() = " << ss.eof() << std::endl;
  ss << "fuga\n";
  while (std::getline(ss, line)) {
    if (ss.eof()) {
      std::cout << "EOF" << std::endl;
      ss.seekg(-line.size(), std::ios::cur);
      break;
    }
    std::cout << line << std::endl;
    std::cout << ss.eof() << std::endl;
  }
  return 0;
}
