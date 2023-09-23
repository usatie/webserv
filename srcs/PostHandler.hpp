#ifndef POST_HANDLER_HPP
#define POST_HANDLER_HPP

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "SocketBuf.hpp"
#include "webserv.hpp"

class Connection;

class PostHandler {
 private:
  // Constructor/Destructor/Assignment Operator
  PostHandler() throw();                               // Do not implement this
  PostHandler(const PostHandler&) throw();             // Do not implement this
  PostHandler& operator=(const PostHandler&) throw();  // Do not implement this
  ~PostHandler() throw();                              // Do not implement this

 public:
  // Member fuctions
  static void handle(Connection& conn);  // throwable
};

#endif
