#include <string>
#include <iostream>
#include <sstream>

int main() {
  {
    std::cout << "Experiment 1 : Untouched ss" << std::endl;
    std::stringstream ss("Hello, world!");
    std::cout << "  << ss.str(): " << ss.str() << std::endl;
    std::cout << "  << ss.rdbuf(): " << ss.rdbuf() << std::endl;

    std::cout << "Experiment 2 : Consumed ss" << std::endl;
    ss.str("Hello, world!");
    std::string s;
    ss >> s;
    std::cout << "  << ss.str(): " << ss.str() << std::endl;
    std::cout << "  << ss.rdbuf(): " << ss.rdbuf() << std::endl;

    std::cout<< "Experiment 3 : ss itself" << std::endl;
    std::cout << "  << ss: " << ss << std::endl;

    std::cout << "Experiment 4 : Multiple times << rdbuf() will fail" << std::endl;
    std::stringstream tmp;
    ss.str("Hello, world!");
    {
      std::cout << "  [1st time] tmp << ss.rdbuf()" << std::endl;
      tmp << ss.rdbuf();
      std::cout << "  << tmp.fail(): " << tmp.fail() << std::endl;
    }
    {
      std::cout << "  [2nd time] tmp << ss.rdbuf()" << std::endl;
      tmp << ss.rdbuf();
      std::cout << "  << tmp.fail(): " << tmp.fail() << std::endl;
    }
  }
}
