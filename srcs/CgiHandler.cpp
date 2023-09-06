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
  int cgi_socket[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, cgi_socket) == -1) {
    ErrorHandler::handle(*conn.client_socket, 500);
    return;
  }
  pid_t pid = fork();
  if (pid == -1) {
    ErrorHandler::handle(*conn.client_socket, 500);
    return;
  }
  if (pid == 0) {
    // Child process
    const char * const argv[] = {conn.header.fullpath.c_str(), NULL};
    close(cgi_socket[0]);
    dup2(cgi_socket[1], STDOUT_FILENO);
    dup2(cgi_socket[1], STDIN_FILENO);
    execve(conn.header.fullpath.c_str(), (char **)argv, NULL);
    // TODO: handle execve error
    //
    //
    // For CGI script without shebang
    //const char * const argv[] = {"/opt/homebrew/bin/python3", conn.header.fullpath.c_str(), NULL};
    //execve("/opt/homebrew/bin/python3", (char **)argv, NULL);
  } else {
    // Parent process
    close(cgi_socket[1]);
    // TODO: Select on cgi_socket[0]
    write(cgi_socket[0], conn.body, conn.body_size);
    // Send EOF to CGI Script process
    shutdown(cgi_socket[0], SHUT_WR);
    int status;
    Log::debug("Waiting for CGI to exit");
    // TODO: Do not wait
    waitpid(pid, &status, 0);
    char buf[1024];
    // TODO: Select on cgi_socket[0]
    ssize_t ret = read(cgi_socket[0], buf, 1024);
    buf[ret] = '\0';
    // TODO: Parse CGI response and convert to HTTP response
    *conn.client_socket << buf;
    Log::debug("CGI exited");
    if (WIFEXITED(status)) {
      int exit_status = WEXITSTATUS(status);
      if (exit_status == 0) {
        Log::debug("CGI exited normally");
      } else {
        ErrorHandler::handle(*conn.client_socket, 500);
      }
    } else {
      ErrorHandler::handle(*conn.client_socket, 500);
    }

  }
}
