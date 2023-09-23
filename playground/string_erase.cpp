#include <iostream>
#include <string>

int main() {
  std::string s = "Hello, world!";

  {
    // std::cout << "Test1: not found '?' and erase\n";
    // std::size_t pos = s.find('?');
    // s.erase(pos); // libc++abi: terminating due to uncaught exception of type std::out_of_range: basic_string
    // std::cout << s << '\n';
  }
  {
    std::cout << "Test: found ',' and erase\n";
    std::cout << "Before: \"" << s << '"' << '\n';
    std::size_t pos = s.find(',');
    s.erase(pos);
    std::cout << "After : \"" << s << '"' << '\n';
  }
}
