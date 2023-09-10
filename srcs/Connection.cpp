#include "Connection.hpp"

int Connection::resume() throw() {
  // 1. Socket I/O
  IOStatus io_status = getIOStatus();
  switch (io_status) {
    case CLIENT_RECV:
      client_socket->fill();
      break;
    case CLIENT_SEND:
      client_socket->flush();
      break;
    case CGI_RECV:
      cgi_socket->fill();
      break;
    case CGI_SEND:
      cgi_socket->flush();
      break;
    case NO_IO:
      break;
  }
  // After socket i/o, check if the socket is still open
  if (io_status == CLIENT_RECV || io_status == CLIENT_SEND) {
    if (client_socket->isClosed()) {
      Log::info("client_socket->closed");
      status = DONE;
      return -1;
    }
  } else if (io_status == CGI_RECV || io_status == CGI_SEND) {
    if (cgi_socket->isClosed()) {
      Log::info("cgi_socket->closed");
      status = HANDLE_CGI_PARSE;
    }
  }
  if (io_status == CGI_SEND && cgi_socket->isSendBufEmpty()) {
    // Send EOF to CGI Script process
    Log::debug("send EOF to CGI Script process");
    shutdown(cgi_socket->get_fd(), SHUT_WR);
  }
  bool cont = true;
  // If the socket is closed, we don't need to do anything
  while (cont) {
    switch (status) {
      case REQ_START_LINE:
        cont = parse_start_line();
        break;
      case REQ_HEADER_FIELDS:
        cont = parse_header_fields();
        break;
      case REQ_BODY:
        cont = parse_body();
        break;
      case HANDLE:
        cont = handle();
        break;
      case HANDLE_CGI_REQ:
        cont = handle_cgi_req();
        break;
      case HANDLE_CGI_RES:
        cont = handle_cgi_res();
        break;
      case HANDLE_CGI_PARSE:
        cont = handle_cgi_parse();
        status = RESPONSE;
        break;
      case RESPONSE:
        cont = response();
        break;
      case DONE:
        cont = false;
    }
  }
  // Finally, check if there is any error while handling the request
  if (client_socket->bad()) {
    Log::info("client_socket->bad()");
    status = DONE;
    return -1;
  }
  return 0;
}

Connection::IOStatus Connection::getIOStatus() const throw() {
  switch (status) {
    case REQ_START_LINE:
    case REQ_HEADER_FIELDS:
    case REQ_BODY:
      return CLIENT_RECV;
    case HANDLE_CGI_REQ:
      return CGI_SEND;
    case HANDLE_CGI_RES:
      return CGI_RECV;
    case RESPONSE:
      return CLIENT_SEND;
    case HANDLE:
    case HANDLE_CGI_PARSE:
    case DONE:
    default:
      return NO_IO;
  }
}

// TODO: make this noexcept
// https://datatracker.ietf.org/doc/html/rfc2616#section-5.1
// Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
int Connection::parse_start_line() throw() {
  std::string line;

  if (client_socket->read_telnet_line(line) < 0) {
    return 0;
  }
  Log::cdebug() << "start line: " << line << std::endl;
  std::stringstream ss;
  ss << line;
  ss >> header.method;   // ss does not throw (cf. playground/fuga.cpp)
  ss >> header.path;     // ss does not throw
  ss >> header.version;  // ss does not throw
  // Path must be starting with /
  if (header.path[0] != '/') {
    Log::cinfo() << "Invalid path: " << header.path << std::endl;
    ErrorHandler::handle(*client_socket, 400);
    status = RESPONSE;
    return 1;
  }
  // Get cwd
  char cwd[PATH_MAX];
  if (getcwd(cwd, PATH_MAX) == 0) {
    Log::cfatal() << "getcwd failed" << std::endl;
    ErrorHandler::handle(*client_socket, 500);
    status = RESPONSE;
    return 1;
  }
  // TODO: Defense Directory traversal attack
  // TODO: Handle absoluteURI
  // TODO: Handle *
  try {
    header.fullpath = std::string(cwd) + header.path;  // throwable
  } catch (std::exception &e) {
    Log::cfatal()
        << "\"header.fullpath = std::string(cwd) + header.path\" failed"
        << std::endl;
    ErrorHandler::handle(*client_socket, 500);
    status = RESPONSE;
    return 1;
  }
  // ss.bad()  : possibly bad alloc
  if (ss.bad()) {
    Log::cfatal() << "ss bad bit is set" << line << std::endl;
    ErrorHandler::handle(*client_socket, 500);
    status = RESPONSE;
    return 1;
  }
  // ss.fail() : method or path or version is missing
  // !ss.eof()  : there are more than 3 tokens or extra white spaces
  if (ss.fail() || !ss.eof()) {
    Log::cinfo() << "Invalid start line: " << line << std::endl;
    ErrorHandler::handle(*client_socket, 400);
    status = RESPONSE;
    return 1;
  }
  status = REQ_HEADER_FIELDS;
  return 1;
}

int Connection::split_header_field(const std::string &line, std::string &key, std::string &value)
{
  try {
    std::stringstream ss(line);  // throwable! (bad alloc)
                                 // TODO: playground
    // 1. Extract key
    if (std::getline(ss, key, ':') == 0) {  // throwable
      Log::cerror() << "std::getline(ss, key, ':') failed. line: " << line
                    << std::endl;
      ErrorHandler::handle(*client_socket, 500);
      status = RESPONSE;
      return 1;
    }
    // 2. Remove leading space from value
    ss >> std::ws;
    // 3. Extract remaining string to value
    std::getline(ss, value);     // throwable
  } catch (std::exception &e) {   // bad alloc
    Log::cfatal() << "Caught exception in split_line_to_kv: " << std::endl;
    return -1;
  }
  return 0;
}

int Connection::parse_header_fields() throw() {
  std::string line;
  while (client_socket->read_telnet_line(line) == 0) {
    // Empty line indicates the end of header fields
    if (line == "") {
      status = REQ_BODY;
      return 1;
    }
    Log::cdebug() << "header line: " << line << std::endl;
    // Split line to key and value
    std::string key, value;
    if (split_header_field(line, key, value) < 0) {
      ErrorHandler::handle(*client_socket, 500);
      status = RESPONSE;
      return 1;
    }
    if (util::http::is_token(key) == false) {
      Log::cinfo() << "Header filed-name is not token: : " << key
                   << std::endl;
      ErrorHandler::handle(*client_socket, 400);
      status = RESPONSE;
      return 1;
    }
    header.fields[key] = value;  // throwable
  }
  return 0;
}

int Connection::parse_body() throw() {
  if (body == NULL) {
    if (header.fields.find("Content-Length") == header.fields.end()) {
      status = HANDLE;
      return 1;
    }
    // TODO: Handle invalid Content-Length
    content_length = atoi(header.fields["Content-Length"].c_str());
    try {
      body = new char[content_length];
    } catch (std::exception &e) {
      Log::cfatal() << "Exception: " << e.what() << std::endl;
      ErrorHandler::handle(*client_socket, 500);
      status = RESPONSE;
      return 1;
    }
  }
  assert(body != NULL);  // body is guaranteed to be non null
  ssize_t ret =
      client_socket->read(body + body_size, content_length - body_size);
  if (ret < 0) {
    return 0;
  } else if (ret == 0) {
    return 0;
  } else {
    body_size += ret;
    if (body_size == content_length) {
      status = HANDLE;
      return 1;
    }
    return 0;
  }
}

int Connection::handle() throw() {
  // if CGI 
  if (header.path.find("/cgi/") != std::string::npos) {
    // TODO: write(body, body_size) in handle()
    CgiHandler::handle(*this);
    status = HANDLE_CGI_REQ;
    return 1;
  }
  // If not CGI
  if (header.method == "GET") {
    GetHandler::handle(*client_socket, header);
  } else if (header.method == "POST") {
    PostHandler::handle(*this);
  } else {
    Log::cinfo() << "Unsupported method: " << header.method << std::endl;
    ErrorHandler::handle(*client_socket, 405);
  }
  status = RESPONSE;
  return 1;
}

int Connection::handle_cgi_req() throw() {
  if (cgi_socket->isSendBufEmpty()) {
    status = HANDLE_CGI_RES;
    return 1;
  }
  return 0;
}

int Connection::handle_cgi_res() throw() {
  // 1. If CGI process is still running, return 0
  // Question: If CGI process exit, isClosed will be set to true?
  if (!cgi_socket->isClosed())
    return 0;

  // 2. If CGI process is not running, handle CGI response
  // To remove zombie process, wait or kill CGI process
  pid_t ret;
  int status;
  while ((ret = waitpid(cgi_pid, &status, WNOHANG)) < 0) {
    // If interrupted by signal, continue to waitpid again
    if (errno == EINTR) continue;
    // If other errors(ECHILD/EFAULT/EINVAL), kill CGI process and return 500
    // (I don't think this will happen though.)
    Log::cfatal() << "waitpid error" << std::endl;
    kill(cgi_pid, SIGKILL);
    ErrorHandler::handle(*client_socket, 500);
    status = RESPONSE;
    return 1;
  }

  // 3. ret == 0 means CGI is still running
  // (I don't think this will happen though.)
  if (ret == 0) {
    Log::cfatal() << "CGI still running" << std::endl;
    kill(cgi_pid, SIGKILL);
    ErrorHandler::handle(*client_socket, 500);
    status = RESPONSE;
    return 1;
  }
  status = HANDLE_CGI_PARSE;
  return 1;
}

// CGI Response syntax (https://datatracker.ietf.org/doc/html/rfc3875#section-6)
//
// generic-response   = 1*header-field NL [ response-body ]
// CGI-Response       = document-response | local-redir-response |
//                      client-redir-response | client-redirdoc-response
//
// document-response        = Content-Type [ Status ] *other-field NL
//                            response-body
// local-redir-response     = local-Location NL
// client-redir-response    = client-Location *extension-field NL
// client-redirdoc-response = client-Location Status Content-Type
//                            *other-field NL response-body
//
// header-field    = CGI-field | other-field
// CGI-field       = Content-Type | Location | Status
// other-field     = protocol-field | extension-field
// protocol-field  = generic-field
// extension-field = generic-field
// generic-field   = field-name ":" [ field-value ] NL
// field-name      = token
// field-value     = *( field-content | LWSP )
// field-content   = *( token | separator | quoted-string )
//
// Content-Type = "Content-Type:" media-type NL
//
// Location        = local-Location | client-Location
// client-Location = "Location:" fragment-URI NL
// local-Location  = "Location:" local-pathquery NL
// fragment-URI    = absoluteURI [ "#" fragment ]
// fragment        = *uric
// local-pathquery = abs-path [ "?" query-string ]
// abs-path        = "/" path-segments
// path-segments   = segment *( "/" segment )
// segment         = *pchar
// pchar           = unreserved | escaped | extra
// extra           = ":" | "@" | "&" | "=" | "+" | "$" | ","
//
// Status         = "Status:" status-code SP reason-phrase NL
// status-code    = "200" | "302" | "400" | "501" | extension-code
// extension-code = 3digit
// reason-phrase  = *TEXT
int Connection::handle_cgi_parse() throw() {
  // TODO: parse
  std::string line;
  // Read header fields
  std::unordered_map<std::string, std::string> cgi_header_fields;
  while (cgi_socket->readline(line) == 0) {
    Log::cdebug() << "CGI line: " << line << std::endl;
    // Empty line indicates the end of header fields
    if (line == "") {
      status = RESPONSE;
      break;
    }
    std::string key, value;
    if (split_header_field(line, key, value) < 0) {
      Log::cwarn() << "Invalid CGI header field: " << line << std::endl;
      ErrorHandler::handle(*client_socket, 500);
      status = RESPONSE;
      break;
    }
    cgi_header_fields[key] = value;
  }
  if (cgi_header_fields.find("Status") != cgi_header_fields.end()) {
    // TODO: validate status code
    *client_socket << "HTTP/1.1 " << cgi_header_fields["Status"] << CRLF;
  } else {
    *client_socket << "HTTP/1.1 200 OK" << CRLF;
  }
  // Send header fields
  std::unordered_map<std::string, std::string>::const_iterator it;
  for (it = cgi_header_fields.begin(); it != cgi_header_fields.end(); ++it) {
    if (it->first == "Status") continue;
    // TODO: validate header field
    *client_socket << it->first << ": "
                  << it->second << CRLF;
  }
  // Send response-body if any
  size_t size = cgi_socket->getReadBufSize();
  if (size > 0) {
    *client_socket << "Content-Length: " << size << CRLF;
    *client_socket << CRLF; // End of header fields
    *cgi_socket >> *client_socket;
  } else {
    *client_socket << CRLF; // End of header fields
  }
  status = RESPONSE;
  return 1;
}

int Connection::response() throw() {
  if (client_socket->isSendBufEmpty()) {
    status = DONE;
    return 1;
  }
  return 0;
}
