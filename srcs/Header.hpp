#ifndef HEADER_HPP
#define HEADER_HPP
#include <string>
#include <unordered_map>

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
  std::string fullpath;
  std::unordered_map<std::string, std::string> fields;
  // Constructor/Destructor
  Header() throw() {}
  ~Header() throw() {}
  // Member functions
};

#endif
