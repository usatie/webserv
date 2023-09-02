#include <string>
#include <iostream>
#include <sstream>

int main() {
  {
    std::cout << "Experiment 1 : Untouched ss" << std::endl;
    std::stringstream ss("Hello, world!");
    char buf[100];
    ss.read(buf, 5);
    std::streamsize rc = ss.gcount();
    buf[rc] = '\0';
    std::cout << "  buf: " << buf << std::endl;
    std::cout << "  rc: " << rc << std::endl;
    std::cout << "  bad: " << ss.bad() << std::endl;
    std::cout << "  fail: " << ss.fail() << std::endl;

    ss.read(buf, 10);
    rc = ss.gcount();
    buf[rc] = '\0';
    std::cout << "  buf: " << buf << std::endl;
    std::cout << "  rc: " << rc << std::endl;
    std::cout << "  bad: " << ss.bad() << std::endl;
    std::cout << "  fail: " << ss.fail() << std::endl;
  }
}
