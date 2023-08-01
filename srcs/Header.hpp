#ifndef HEADER_HPP
#define HEADER_HPP
#include <string>

#include "Socket.hpp"
#include "webserv.hpp"

class Header {
 public:
  // Member data
  std::string method;
  std::string path;
  std::string version;
  // Constructor/Destructor
  Header() {}
  ~Header() {}
  // Member functions
};

#endif
