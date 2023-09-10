#ifndef ERROR_HANDLER_HPP
#define ERROR_HANDLER_HPP


#include "SocketBuf.hpp"

class ErrorHandler {
public:
  static void handle(SocketBuf& client_socket, int status_code) throw();
};
#endif
