#ifndef ERROR_HANDLER_HPP
#define ERROR_HANDLER_HPP


#include "SocketBuf.hpp"

class Connection;

class ErrorHandler {
public:
  static void handle(Connection& conn, int status_code, bool noredirect = false) throw();
};
#endif
