#ifndef REDIRECTHANDLER_HPP
#define REDIRECTHANDLER_HPP

#include "Header.hpp"

class Connection;

class RedirectHandler {
 private:
  // Constructor/Destructor/Assignment Operator
  RedirectHandler() throw();                        // Do not implement this
  RedirectHandler(const RedirectHandler&) throw();  // Do not implement this
  RedirectHandler& operator=(
      const RedirectHandler&) throw();  // Do not implement this
  ~RedirectHandler() throw();           // Do not implement this
 public:
  // Member functions
  static void handle(Connection& conn, int status_code,
                     const std::string& location) throw();
};

#endif
