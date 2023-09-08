#include <iostream>
#include <string>
#include "test_util.hpp"

void title(const std::string &title) {
  static int cnt = 0;
  cnt++;
  // Print title of test case surrounded by '='
  std::cout << "====================" << std::endl;
  std::cout << cnt << ". " << title << std::endl;
  std::cout << "====================" << std::endl;
}

