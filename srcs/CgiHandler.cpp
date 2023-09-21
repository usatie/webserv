#include "CgiHandler.hpp"

#include "Connection.hpp"
#include "ErrorHandler.hpp"
// fork
#include <unistd.h>

#include <cstdlib>  // exit

int CgiHandler::handle(Connection& conn) throw() {
  // Check if 404
  if (access(conn.header.fullpath.c_str(), F_OK) == -1) {
    ErrorHandler::handle(conn, 404);
    return -1;
  }
  // Check if fullpath is valid and executable
  // If cgi_ext_cf is NULL, then it will not be execed, but will be interpreted
  if (conn.cgi_ext_cf && access(conn.header.fullpath.c_str(), X_OK) == -1) {
    ErrorHandler::handle(conn, 403);
    return -1;
  }
  // Check if fullpath is a directory
  // Because, a directory returns true for X_OK
  struct stat statbuf;
  if (stat(conn.header.fullpath.c_str(), &statbuf) == -1) {
    ErrorHandler::handle(conn, 500);
    return -1;
  }
  if (S_ISDIR(statbuf.st_mode)) {
    ErrorHandler::handle(conn, 403);
    return -1;
  }

  // Fork and excute CGI
  int cgi_socket[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, cgi_socket) == -1) {
    ErrorHandler::handle(conn, 500);
    return -1;
  }
  pid_t pid = fork();
  if (pid == -1) {
    ErrorHandler::handle(conn, 500);
    return -1;
  }
  if (pid == 0) {
    // Child process
    char* const env[] = {strdup("REQUEST_METHOD=GET"),
                         strdup("SERVER_PROTOCOL=HTTP/1.1"),
                         strdup("PATH_INFO=/"), NULL};
    if (conn.cgi_ext_cf) {  // binary or script with shebang
      const char* const argv[] = {conn.header.fullpath.c_str(), NULL};
      close(cgi_socket[0]);
      dup2(cgi_socket[1], STDOUT_FILENO);
      dup2(cgi_socket[1], STDIN_FILENO);
      // TODO: Create environment variables
      // TODO: Create appropriate argv
      execve(conn.header.fullpath.c_str(), (char**)argv, NULL);
    } else {  // script without shebang
      const char* const argv[] = {conn.cgi_handler_cf->interpreter_path.c_str(),
                                  conn.header.fullpath.c_str(), NULL};
      close(cgi_socket[0]);
      dup2(cgi_socket[1], STDOUT_FILENO);
      dup2(cgi_socket[1], STDIN_FILENO);
      // TODO: Create environment variables
      // TODO: Create appropriate argv
      execve(conn.cgi_handler_cf->interpreter_path.c_str(), (char**)argv, env);
    }
    // This log would be printed to std::err and visible to the server process.
    Log::cfatal() << "execve error" << std::endl;
    std::exit(1);
  } else {
    // Parent process
    close(cgi_socket[1]);
    try {
      conn.cgi_socket =
          util::shared_ptr<SocketBuf>(new SocketBuf(cgi_socket[0]));
    } catch (std::exception& e) {
      Log::fatal("new SocketBuf(cgi_socket[0]) failed");
      close(cgi_socket[0]);
      ErrorHandler::handle(conn, 500);
      return -1;
    }
    Log::cdebug() << "body_size: " << conn.body_size;
    conn.cgi_socket->write(conn.body, conn.body_size);
    conn.cgi_pid = pid;
  }
  return 0;
}
