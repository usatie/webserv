#ifndef HEADER_HPP
#define HEADER_HPP
#include <map>
#include <string>

#include "Socket.hpp"
#include "Version.hpp"
#include "webserv.hpp"

class Header {
 private:
  Header(const Header &src) throw();             // Do not implement
  Header &operator=(const Header &rhs) throw();  // Do not implement
 public:
  typedef std::map<std::string, std::string>::iterator iterator;
  typedef std::map<std::string, std::string>::const_iterator const_iterator;
  // Member data
  std::string method;
  std::string path;
  //std::string version;
  Version version;
  std::string fullpath;
  std::string query;
  std::string fragment;
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
    query.clear();
    fragment.clear();
    return 0;
  }
};

#endif
