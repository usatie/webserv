#include <iostream>
#include <sstream>
#include <string>

// ss.in_avail() is not reliable
// should use `ss.str().size() - ss.tellg()` instead
int main() {
    std::stringstream ss;
    std::string w;
    ss << "abc def   ghi";
    std::cout << "avail: " << ss.rdbuf()->in_avail() << std::endl; // Before tellg(), in_avail() returns 1
    std::cout << "size: " << ss.str().size() << std::endl;
    std::cout << "avail: " << ss.rdbuf()->in_avail() << std::endl; // Before tellg(), in_avail() returns 1
    std::cout << "tellg: " << ss.tellg() << std::endl;
    std::cout << "avail: " << ss.rdbuf()->in_avail() << std::endl; // After tellg(), in_avail() returns 13
    std::cout << "tellp: " << ss.tellp() << std::endl;

    std::cout << "size: " << ss.str().size() << ", tellg: " << ss.tellg() << ", avail: " << ss.rdbuf()->in_avail() << std::endl;
    std::cout << ss.str().size() << ", " << ss.tellg() << ", " << ss.rdbuf()->in_avail() << std::endl;
    {
      ss >> w;
      std::cout << "size: " << ss.str().size() << ", tellg: " << ss.tellg() << ", avail: " << ss.rdbuf()->in_avail() << std::endl;
      ss >> std::ws;
      std::cout << "size: " << ss.str().size() << ", tellg: " << ss.tellg() << ", avail: " << ss.rdbuf()->in_avail() << std::endl;
    }
    {
      ss >> w;
      std::cout << "size: " << ss.str().size() << ", tellg: " << ss.tellg() << ", avail: " << ss.rdbuf()->in_avail() << std::endl;
      ss >> std::ws;
      std::cout << "size: " << ss.str().size() << ", tellg: " << ss.tellg() << ", avail: " << ss.rdbuf()->in_avail() << std::endl;
    }
    {
      ss >> w;
      std::cout << "size: " << ss.str().size() << ", tellg: " << ss.tellg() << ", avail: " << ss.rdbuf()->in_avail() << std::endl;
      ss >> std::ws;
      std::cout << "size: " << ss.str().size() << ", tellg: " << ss.tellg() << ", avail: " << ss.rdbuf()->in_avail() << std::endl;
    }
    std::cout << ss.str() << std::endl;
    return 0;
}
