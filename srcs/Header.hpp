#ifndef HEADER_HPP
#define HEADER_HPP
#include <map>
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
  std::string fullpath;
  std::map<std::string, std::string> fields;  // unordered_map is preferred
                                              // but C++98 doesn't support it
  // Constructor/Destructor
  Header() throw() {}
  ~Header() throw() {}
  // Member functions
  int clear() {
    method.clear();
    path.clear();
    version.clear();
    fullpath.clear();
    fields.clear();
    return 0;
  }
};

#endif
