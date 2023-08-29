#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <string>

int main() {
  /*
  std::cout << "[" << ss.str() << "]" << std::endl;
  ss.clear();
  ss.str("hoge\nfuga\n");
  while (std::getline(ss, line)) {
    std::cout << line << std::endl;
    std::cout << ss.eof() << std::endl;
  }
  */
  std::stringstream ss("a b c ");
  //std::cout << ss.bad() << std::endl;
  //std::cout << ss.fail() << std::endl;
  //std::cout << ss.str() << std::endl;
  std::string line;
  std::getline(ss, line);
  if (line.back() == '\r')
    line.pop_back();
  std::stringstream tmp(line);
  std::string method, path, version;
  tmp >> method >> path >> version;
  std::cout << "METHOD: " << method << std::endl;
  std::cout << "PATH: " << path << std::endl;
  std::cout << "VERSION: " << version << std::endl;
  std::cout << "fail: " << tmp.fail() << std::endl;
  std::cout << "bad: " << tmp.bad() << std::endl;
  std::cout << "eof: " << tmp.eof() << std::endl;

  /*
  std::cout << "<<<Header Fields>>>" << std::endl;
  while (std::getline(ss, line)) {
    if (line.back() == '\r')
      line.pop_back();
    if (line == "") {
      break;
    }
    std::stringstream tmp(line);
    std::string key, value;
    std::getline(tmp, key, ':');
    tmp >> value;
    if (tmp.rdbuf()->in_avail() != 0) {
      std::cout << "Extra characters?" << std::endl;
      break;
    }
    std::cout << "Key: " << key << std::endl;
    std::cout << "Value: " << value << std::endl;
    std::cout << "[" << key << "]: [" << value << "]" << std::endl;
  }
  */
  return 0;
}
