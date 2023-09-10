#ifndef GET_HANDLER_HPP
#define GET_HANDLER_HPP

#include "Header.hpp"
#include "SocketBuf.hpp"

class GetHandler {
 private:
  // Constructor/Destructor/Assignment Operator
  GetHandler() throw();                              // Do not implement this
  GetHandler(const GetHandler&) throw();             // Do not implement this
  GetHandler& operator=(const GetHandler&) throw();  // Do not implement this
  ~GetHandler() throw();                             // Do not implement this
 public:
  // Member functions
  static void handle(SocketBuf& client_socket, const Header& header) throw();
};
#endif
