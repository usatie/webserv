#ifndef CGI_HANDLER_HPP
#define CGI_HANDLER_HPP

class Connection;

class CgiHandler {
 private:
  // Constructor/Destructor/Assignment Operator
  CgiHandler() throw();                              // Do not implement this
  CgiHandler(const CgiHandler&) throw();             // Do not implement this
  CgiHandler& operator=(const CgiHandler&) throw();  // Do not implement this
  ~CgiHandler() throw();                             // Do not implement this

 public:
  // Member fuctions
  static int handle(Connection& conn) throw();
};

#endif
