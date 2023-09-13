#ifndef GET_HANDLER_HPP
#define GET_HANDLER_HPP

#include "Header.hpp"

class Connection;

class GetHandler {
 private:
  // Constructor/Destructor/Assignment Operator
  GetHandler() throw();                              // Do not implement this
  GetHandler(const GetHandler&) throw();             // Do not implement this
  GetHandler& operator=(const GetHandler&) throw();  // Do not implement this
  ~GetHandler() throw();                             // Do not implement this
 public:
  // Member functions
  static void handle(Connection& conn) throw();
};
#endif
