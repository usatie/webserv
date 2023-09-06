#include "CgiHandler.hpp"
#include "Connection.hpp"
#include "ErrorHandler.hpp"
// fork
#include <unistd.h>

void CgiHandler::handle(Connection& conn) throw() {
  // Check if 404
  if (access(conn.header.fullpath.c_str(), F_OK) == -1) {
    ErrorHandler::handle(*conn.client_socket, 404);
    return;
  }
  // Check if fullpath is valid and executable
  if (access(conn.header.fullpath.c_str(), X_OK) == -1) {
    ErrorHandler::handle(*conn.client_socket, 403);
    return;
  }
  // Check if fullpath is a directory
  // Because, a directory returns true for X_OK
  struct stat statbuf;
  if (stat(conn.header.fullpath.c_str(), &statbuf) == -1) {
    ErrorHandler::handle(*conn.client_socket, 500);
    return;
  }
  if (S_ISDIR(statbuf.st_mode)) {
    ErrorHandler::handle(*conn.client_socket, 403);
    return;
  }

  // Fork and excute CGI
  pid_t pid = fork();
  if (pid == -1) {
    ErrorHandler::handle(*conn.client_socket, 500);
    return;
  }
  if (pid == 0) {
    // Child process
    const char * const argv[] = {conn.header.fullpath.c_str(), NULL};
    execve(conn.header.fullpath.c_str(), (char **)argv, NULL);
  } else {
    // Parent process
    int status;
    Log::debug("Waiting for CGI to exit");
    waitpid(pid, &status, 0);
    Log::debug("CGI exited");
    if (WIFEXITED(status)) {
      int exit_status = WEXITSTATUS(status);
      if (exit_status == 0) {
        Log::debug("CGI exited normally");
        // Do nothing
      } else if (exit_status == 1) {
        ErrorHandler::handle(*conn.client_socket, 500);
      } else if (exit_status == 2) {
        ErrorHandler::handle(*conn.client_socket, 404);
      } else if (exit_status == 3) {
        ErrorHandler::handle(*conn.client_socket, 403);
      } else {
        ErrorHandler::handle(*conn.client_socket, 500);
      }
    } else {
      ErrorHandler::handle(*conn.client_socket, 500);
    }

  }
}
