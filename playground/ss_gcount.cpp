#include <iostream>
#include <sstream>
#include <string>

using namespace std;

int main() {
  stringstream ss("123 456 789");
  char buf[10] = {};

  ss.read(buf, 5);
  std::cout << "gcount: " << ss.gcount() << std::endl;
  std::cout << "buf: " << buf << std::endl;
  std::cout << "gcount: " << ss.gcount() << std::endl;
}
