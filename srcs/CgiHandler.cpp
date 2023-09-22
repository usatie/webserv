#include "CgiHandler.hpp"

#include "Connection.hpp"
#include "ErrorHandler.hpp"
// fork
#include <unistd.h>

#include <cstdlib>  // exit

int CgiHandler::handle(Connection& conn) {  // throwable
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
    std::stringstream ss;
    ss << "CONTENT_LENGTH=" << conn.content_length;
    // std::string auth_type = "AUTH_TYPE=";
    std::string content_length = ss.str();
    // std::string content_type = "CONTENT_TYPE=" +
    // conn.header.fields["Content-Type"]; std::string gateway_interface =
    // "GATEWAY_INTERFACE=CGI/1.1";
    std::string path_info = "PATH_INFO=/";  // TODO: Implement RFC3875 4.1.5
    // std::string path_translated = "PATH_TRANSLATED=" + conn.header.fullpath;
    // std::string query_string = "QUERY_STRING=";
    // std::string remote_addr = "REMOTE_ADDR=";
    // std::string remote_host = "REMOTE_HOST=";
    // std::string remote_ident = "REMOTE_IDENT=";
    // std::string remote_user = "REMOTE_USER=";
    std::string request_method = "REQUEST_METHOD=" + conn.header.method;
    // std::string script_name = "SCRIPT_NAME=" + conn.header.path;
    // std::string server_name = "SERVER_NAME=" + conn.header.fields["Host"]; //
    // std::string server_port = "SERVER_PORT=";
    std::string server_protocol = "SERVER_PROTOCOL=HTTP/1.1";
    std::string server_software = "SERVER_SOFTWARE=webserv/0.0.1";
    char* const env[] = {//(char*)auth_type.c_str(),
                         (char*)content_length.c_str(),
                         //(char*)content_type.c_str(),
                         //(char*)gateway_interface.c_str(),
                         (char*)path_info.c_str(),
                         //(char*)path_translated.c_str(),
                         //(char*)query_string.c_str(),
                         //(char*)remote_addr.c_str(),
                         //(char*)remote_host.c_str(),
                         //(char*)remote_ident.c_str(),
                         //(char*)remote_user.c_str(),
                         (char*)request_method.c_str(),
                         //(char*)script_name.c_str(),
                         //(char*)server_name.c_str(),
                         //(char*)server_port.c_str(),
                         (char*)server_protocol.c_str(),
                         (char*)server_software.c_str(), NULL};
    Log::cdebug() << "execve: " << conn.header.fullpath << std::endl;
    if (conn.cgi_ext_cf) {  // binary or script with shebang
      const char* const argv[] = {conn.header.fullpath.c_str(), NULL};
      close(cgi_socket[0]);
      dup2(cgi_socket[1], STDOUT_FILENO);
      dup2(cgi_socket[1], STDIN_FILENO);
      // TODO: Create environment variables
      // TODO: Create appropriate argv
      execve(conn.header.fullpath.c_str(), (char**)argv, env);
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
    conn.cgi_socket =
        util::shared_ptr<SocketBuf>(new SocketBuf(cgi_socket[0]));  // throwable
    Log::cdebug() << "body.size: " << conn.body.size() << std::endl;
    conn.cgi_socket->write(conn.body.c_str(), conn.body.size());
    conn.cgi_pid = pid;
  }
  return 0;
}
