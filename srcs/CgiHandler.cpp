#include "CgiHandler.hpp"

#include "Connection.hpp"
#include "ErrorHandler.hpp"
#include "util.hpp"
// fork
#include <sys/stat.h>  // stat
#include <unistd.h>

#include <cerrno>
#include <cstdlib>  // exit
#include <string>

int CgiHandler::handle(Connection& conn) {  // throwable
  const Request& req = conn.req;
  std::string script_path = util::path::get_script_path(req.header.fullpath);
  std::string dir_path = script_path.substr(0, script_path.find_last_of('/'));
  std::string script_name =
      script_path.substr(script_path.find_last_of('/') + 1);
  std::string path_info = util::path::get_path_info(req.header.fullpath);
  // Check if 404
  if (access(script_path.c_str(), F_OK) == -1) {
    ErrorHandler::handle(conn, 404);
    return -1;
  }
  // Check if script_path is valid and executable
  // If cgi_ext_cf is NULL, then it will not be execed, but will be interpreted
  if (req.cgi_ext_cf && access(script_path.c_str(), X_OK) == -1) {
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
    ss << "CONTENT_LENGTH=" << req.content_length;
    // std::string auth_type = "AUTH_TYPE=";
    std::string content_length = ss.str();
    std::string content_type = "CONTENT_TYPE=";
    Header::const_iterator it = req.header.fields.find("Content-Type");
    if (it != req.header.fields.end()) content_type += it->second;
    std::string gateway_interface = "GATEWAY_INTERFACE=CGI/1.1";
    std::string path_info_ = "PATH_INFO=" + path_info;
    // std::string path_translated = "PATH_TRANSLATED=" + conn.header.fullpath;
    std::string query_string = "QUERY_STRING=" + req.header.query;
    std::string remote_addr =
        "REMOTE_ADDR=" + conn.client_socket->socket->get_client_ip_address();
    // std::string remote_host = "REMOTE_HOST=";
    // std::string remote_ident = "REMOTE_IDENT=";
    // std::string remote_user = "REMOTE_USER=";
    std::string request_method = "REQUEST_METHOD=" + req.header.method;
    std::string script_name_ = "SCRIPT_NAME=" + script_name;
    // server_names is guaranteed to be non-empty
    std::string server_name = "SERVER_NAME=" + req.srv_cf->server_names[0];
    std::string server_port =
        "SERVER_PORT=" + conn.client_socket->socket->get_server_port_string();
    std::string server_protocol = "SERVER_PROTOCOL=HTTP/1.1";
    std::string server_software = "SERVER_SOFTWARE=webserv/0.0.1";
    const char* const env[] = {
        // auth_type.c_str(),
        content_length.c_str(), gateway_interface.c_str(), path_info_.c_str(),
        // path_translated.c_str(),
        query_string.c_str(), remote_addr.c_str(),
        // remote_host.c_str(),
        // remote_ident.c_str(),
        // remote_user.c_str(),
        request_method.c_str(), script_name_.c_str(), server_name.c_str(),
        server_port.c_str(), server_protocol.c_str(), server_software.c_str(),
        util::contains(req.header.fields, "Content-Type") ? content_type.c_str()
                                                          : NULL,
        NULL};

    // Change directory to script directory to execute script
    if (chdir(dir_path.c_str()) == -1) {
      Log::cfatal() << "chdir failed. (" << errno << ": " << strerror(errno)
                    << ")" << std::endl;
      std::exit(1);
    }
    Log::cdebug() << "execve: " << req.header.fullpath << std::endl;
    if (req.cgi_ext_cf) {  // binary or script with shebang
      const char* const argv[] = {req.header.fullpath.c_str(), NULL};
      close(cgi_socket[0]);
      dup2(cgi_socket[1], STDOUT_FILENO);
      dup2(cgi_socket[1], STDIN_FILENO);
      // TODO: Create environment variables
      // TODO: Create appropriate argv
      execve(script_name.c_str(), const_cast<char* const*>(argv),
             const_cast<char* const*>(env));
    } else {  // script without shebang
      const char* const argv[] = {req.cgi_handler_cf->interpreter_path.c_str(),
                                  script_name.c_str(), NULL};
      close(cgi_socket[0]);
      dup2(cgi_socket[1], STDOUT_FILENO);
      dup2(cgi_socket[1], STDIN_FILENO);
      // TODO: Create environment variables
      // TODO: Create appropriate argv
      execve(req.cgi_handler_cf->interpreter_path.c_str(),
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
    Log::cdebug() << "body.size: " << req.body.size() << std::endl;
    conn.cgi_socket->write(req.body.c_str(), req.body.size());
    conn.cgi_pid = pid;
  }
  return 0;
}
