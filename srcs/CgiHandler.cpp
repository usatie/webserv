#include "CgiHandler.hpp"
#include "Connection.hpp"
#include "ErrorHandler.hpp"
// fork
#include <unistd.h>
#include <cstdlib> // exit

void CgiHandler::handle(Connection& conn) throw() {
  // Check if 404
  if (access(conn.header.fullpath.c_str(), F_OK) == -1) {
    ErrorHandler::handle(conn, 404);
    return;
  }
  // Check if fullpath is valid and executable
  if (access(conn.header.fullpath.c_str(), X_OK) == -1) {
    ErrorHandler::handle(conn, 403);
    return;
  }
  // Check if fullpath is a directory
  // Because, a directory returns true for X_OK
  struct stat statbuf;
  if (stat(conn.header.fullpath.c_str(), &statbuf) == -1) {
    ErrorHandler::handle(conn, 500);
    return;
  }
  if (S_ISDIR(statbuf.st_mode)) {
    ErrorHandler::handle(conn, 403);
    return;
  }

  // Fork and excute CGI
  int cgi_socket[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, cgi_socket) == -1) {
    ErrorHandler::handle(conn, 500);
    return;
  }
  pid_t pid = fork();
  if (pid == -1) {
    ErrorHandler::handle(conn, 500);
    return;
  }
  if (pid == 0) {
    // Child process
    const char * const argv[] = {conn.header.fullpath.c_str(), NULL};
    close(cgi_socket[0]);
    dup2(cgi_socket[1], STDOUT_FILENO);
    dup2(cgi_socket[1], STDIN_FILENO);
    // TODO: Create environment variables
    // TODO: Create appropriate argv
    execve(conn.header.fullpath.c_str(), (char **)argv, NULL);

    // This log would be printed to std::err and visible to the server process.
    Log::cfatal() << "execve error" << std::endl;
    std::exit(1);

    // For CGI script without shebang
    //const char * const argv[] = {"/opt/homebrew/bin/python3", conn.header.fullpath.c_str(), NULL};
    //execve("/opt/homebrew/bin/python3", (char **)argv, NULL);
  } else {
    // Parent process
    close(cgi_socket[1]);
    try {
      conn.cgi_socket = std::shared_ptr<SocketBuf>(new SocketBuf(cgi_socket[0]));
    } catch (std::exception& e) {
      Log::fatal("new SocketBuf(cgi_socket[0]) failed");
      close(cgi_socket[0]);
      ErrorHandler::handle(conn, 500);
      return;
    }
    conn.cgi_socket->write(conn.body, conn.body_size);
    conn.cgi_pid = pid;
  }
}
