#ifndef HEADER_HPP
#define HEADER_HPP
#include <string>

#include "Socket.hpp"
#include "webserv.hpp"

class Header {
 private:
  Header(const Header &src) throw();             // Do not implement
  Header &operator=(const Header &rhs) throw();  // Do not implement
 public:
  // Member data
  std::string method;
  std::string path;
  std::string version;
  // Constructor/Destructor
  Header() throw() {}
  ~Header() throw() {}
  // Member functions
};

#endif
