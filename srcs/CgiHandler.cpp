#include "CgiHandler.hpp"

#include "Connection.hpp"
#include "ErrorHandler.hpp"
// fork
#include <sys/stat.h>  // stat
#include <unistd.h>

#include <cerrno>
#include <cstdlib>  // exit

int CgiHandler::handle(Connection& conn) {  // throwable
  std::string script_path = conn.header.fullpath;
  std::string dir_path = script_path.substr(0, script_path.find_last_of('/'));
  std::string script_name =
      script_path.substr(script_path.find_last_of('/') + 1);
  // Check if 404
  if (access(script_path.c_str(), F_OK) == -1) {
    ErrorHandler::handle(conn, 404);
    return -1;
  }
  // Check if script_path is valid and executable
  // If cgi_ext_cf is NULL, then it will not be execed, but will be interpreted
  if (conn.cgi_ext_cf && access(script_path.c_str(), X_OK) == -1) {
    ErrorHandler::handle(conn, 403);
    return -1;
  }
  // Check if script_path is a directory
  // Because, a directory returns true for X_OK
  struct stat statbuf;
  if (stat(script_path.c_str(), &statbuf) == -1) {
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
    std::string query_string = "QUERY_STRING=" + conn.header.query;
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
    const char* const env[] = {// auth_type.c_str(),
                               content_length.c_str(),
                               // content_type.c_str(),
                               // gateway_interface.c_str(),
                               path_info.c_str(),
                               // path_translated.c_str(),
                               query_string.c_str(),
                               // remote_addr.c_str(),
                               // remote_host.c_str(),
                               // remote_ident.c_str(),
                               // remote_user.c_str(),
                               request_method.c_str(),
                               // script_name.c_str(),
                               // server_name.c_str(),
                               // server_port.c_str(),
                               server_protocol.c_str(), server_software.c_str(),
                               NULL};

    // Change directory to script directory to execute script
    if (chdir(dir_path.c_str()) == -1) {
      Log::cfatal() << "chdir failed. (" << errno << ": " << strerror(errno)
                    << ")" << std::endl;
      std::exit(1);
    }
    Log::cdebug() << "execve: " << conn.header.fullpath << std::endl;
    if (conn.cgi_ext_cf) {  // binary or script with shebang
      const char* const argv[] = {conn.header.fullpath.c_str(), NULL};
      close(cgi_socket[0]);
      dup2(cgi_socket[1], STDOUT_FILENO);
      dup2(cgi_socket[1], STDIN_FILENO);
      // TODO: Create environment variables
      // TODO: Create appropriate argv
      execve(script_name.c_str(), const_cast<char* const*>(argv),
             const_cast<char* const*>(env));
    } else {  // script without shebang
      const char* const argv[] = {conn.cgi_handler_cf->interpreter_path.c_str(),
                                  script_name.c_str(), NULL};
      close(cgi_socket[0]);
      dup2(cgi_socket[1], STDOUT_FILENO);
      dup2(cgi_socket[1], STDIN_FILENO);
      // TODO: Create environment variables
      // TODO: Create appropriate argv
      execve(conn.cgi_handler_cf->interpreter_path.c_str(),
             const_cast<char* const*>(argv), const_cast<char* const*>(env));
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
