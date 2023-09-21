#include <iostream>
#include <string>
#include <sstream>
#include <cstddef>

int main() {
  std::stringstream ss;
  size_t n;

  {
    std::cout << "Test1:" << std::endl;
    ss.str("ff");
    ss >> n;
    std::cout << n << std::endl;
  }
  {
    std::cout << "Test2:" << std::endl;
    ss.str("ff");
    ss.clear();
    ss >> std::hex >> n;
    std::cout << n << std::endl;
  }
}
