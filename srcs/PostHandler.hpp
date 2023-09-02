#ifndef POST_HANDLER_HPP
#define POST_HANDLER_HPP

#include <sys/stat.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "Header.hpp"
#include "SocketBuf.hpp"
#include "http_special_response.hpp"
#include "webserv.hpp"
#include "ErrorHandler.hpp"
class Connection;

class PostHandler {
 private:
  // Constructor/Destructor/Assignment Operator
  PostHandler() throw();                              // Do not implement this
  PostHandler(const PostHandler&) throw();             // Do not implement this
  PostHandler& operator=(const PostHandler&) throw();  // Do not implement this
  ~PostHandler() throw();                             // Do not implement this
 
 public:
  // Member fuctions
  static void handle(Connection& conn) throw();  
};

#endif
