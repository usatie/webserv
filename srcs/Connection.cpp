#include "Connection.hpp"

#include <sys/wait.h>

#include <cerrno>
#include <map>

#include "DeleteHandler.hpp"

int Connection::resume() {  // throwable
  Log::debug("Connection::resume()");
  last_modified = time(NULL);
  // CGI timeout
  if (is_cgi_timeout()) {
    handle_cgi_timeout();
    return -1;
  }
  // 1. Socket I/O
  switch (io_status) {
    case CLIENT_RECV:
      client_socket->fill();  // throwable
      if (client_socket->isClosed()) {
        Log::info("client_socket->closed");
        status = DONE;
        return -1;
      }
      break;
    case CGI_SEND:
      cgi_socket->flush();  // throwable
      if (cgi_socket->isClosed()) {
        Log::info("cgi_socket->closed");
        status = HANDLE_CGI_PARSE;
      }
      break;
    case CGI_RECV:
      cgi_socket->fill();  // throwable
      if (cgi_socket->isClosed() || cgi_socket->hasReceivedEof) {
        Log::info("cgi_socket->closed or hasReceivedEof");
        status = HANDLE_CGI_PARSE;
      }
      break;
    case CLIENT_SEND:
      client_socket->flush();  // throwable
      if (client_socket->isClosed()) {
        Log::info("client_socket->closed");
        status = DONE;
        return -1;
      }
      break;
    case NO_IO:
      break;
  }
  bool cont = true;
  // If the socket is closed, we don't need to do anything
  while (cont) {
    switch (status) {
      case REQ_START_LINE:
        cont = parse_start_line();  // throwable
        break;
      case REQ_HEADER_FIELDS:
        cont = parse_header_fields();  // throwable
        break;
      case REQ_BODY:
        cont = parse_body();  // throwable
        break;
      case REQ_BODY_CONTENT_LENGTH:
        cont = parse_body_content_length();  // throwable
        break;
      case REQ_BODY_CHUNKED:
        cont = parse_body_chunked();  // throwable
        break;
      case REQ_BODY_CHUNK_DATA:
        cont = parse_body_chunk_data();  // throwable
        break;
      case REQ_BODY_CHUNK_TRAILER_SECTION:
        cont = parse_body_chunk_trailer_section();  // throwable
        break;
      case HANDLE:
        cont = handle();  // throwable
        break;
      case HANDLE_CGI_REQ:
        cont = handle_cgi_req();
        break;
      case HANDLE_CGI_RES:
        cont = handle_cgi_res();
        break;
      case HANDLE_CGI_PARSE:
        cont = handle_cgi_parse();  // throwable
        status = RESPONSE;
        break;
      case RESPONSE:
        cont = response();
        break;
      case DONE:
        cont = false;
        break;
      case CLEAR:
        cont = false;
        break;
    }
  }
  // Finally, check if there is any error while handling the request
  if (client_socket->bad()) {
    Log::info("client_socket->bad()");
    status = DONE;
    return -1;
  }
  if (cgi_socket != NULL) {
    if (cgi_socket->bad()) {
      Log::info("cgi_socket->bad()");
      status = DONE;
      return -1;
    }
  }
  // If the client socket has received EOF, and both buffers are empty,
  // we can close the connection.
  if (client_socket->hasReceivedEof && client_socket->isSendBufEmpty() &&
      client_socket->isRecvBufEmpty()) {
    if (cgi_socket == NULL || cgi_socket->isClosed() ||
        cgi_socket->hasReceivedEof) {
      Log::info("client_socket->hasReceivedEof");
      status = DONE;
      return -1;
    }
  }
  return 0;
}

int Connection::clear() {
  cgi_socket = util::shared_ptr<SocketBuf>();
  header.clear();
  status = REQ_START_LINE;
  body.clear();
  content_length = 0;
  chunk.clear();
  chunk_size = 0;
  cgi_pid = 0;
  srv_cf = NULL;
  loc_cf = NULL;
  cgi_handler_cf = NULL;
  cgi_ext_cf = NULL;
  return 0;
}

// TODO: make this noexcept
// https://datatracker.ietf.org/doc/html/rfc2616#section-5.1
// Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
int Connection::parse_start_line() {
  std::string line;

  if (client_socket->read_telnet_line(line) < 0) {  // throwable
    return 0;
  }
  if (line.empty()) {
    Log::cdebug() << "empty line" << std::endl;
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
    ErrorHandler::handle(*this, 400);
    status = RESPONSE;
    return 1;
  }
  // Get cwd
  char cwd[PATH_MAX];
  if (getcwd(cwd, PATH_MAX) == 0) {
    Log::cfatal() << "getcwd failed" << std::endl;
    ErrorHandler::handle(*this, 500);
    status = RESPONSE;
    return 1;
  }
  // TODO: Defense Directory traversal attack
  // TODO: Handle absoluteURI
  // TODO: Handle *
  // TODO: header.path must be a normalized URI
  //       1. decoding the text encoded in the “%XX” form
  //       2. resolving references to relative path components (“.” and “..”)
  //       3. possible compression of two or more adjacent slashes into a single
  //       slash
  // ss.bad()  : possibly bad alloc
  if (ss.bad()) {
    Log::cfatal() << "ss bad bit is set" << line << std::endl;
    ErrorHandler::handle(*this, 500);
    status = RESPONSE;
    return 1;
  }
  // ss.fail() : method or path or version is missing
  // !ss.eof()  : there are more than 3 tokens or extra white spaces
  if (ss.fail() || !ss.eof()) {
    Log::cinfo() << "Invalid start line: " << line << std::endl;
    ErrorHandler::handle(*this, 400);
    status = RESPONSE;
    return 1;
  }
  status = REQ_HEADER_FIELDS;
  return 1;
}

int Connection::split_header_field(const std::string &line, std::string &key,
                                   std::string &value) {  // throwable
  std::stringstream ss(line);  // throwable! (bad alloc)
  // 1. Extract key
  if (!std::getline(ss, key, ':')) {  // throwable
    Log::cerror() << "std::getline(ss, key, ':') failed. line: " << line
                  << std::endl;
    ErrorHandler::handle(*this, 500);
    status = RESPONSE;
    return 1;
  }
  // 2. Remove leading space from value
  ss >> std::ws;
  // 3. Extract remaining string to value
  std::getline(ss, value);  // throwable
  return 0;
}

int Connection::parse_header_fields() {  // throwable
  std::string line;
  while (client_socket->read_telnet_line(line) == 0) {  // throwable
    // Empty line indicates the end of header fields
    if (line == "") {
      status = REQ_BODY;
      return 1;
    }
    Log::cdebug() << "header line: " << line << std::endl;
    // Split line to key and value
    std::string key, value;
    if (split_header_field(line, key, value) < 0) {  // throwable
      ErrorHandler::handle(*this, 500);
      status = RESPONSE;
      return 1;
    }
    if (util::http::is_token(key) == false) {
      Log::cinfo() << "Header filed-name is not token: : " << key << std::endl;
      ErrorHandler::handle(*this, 400);
      status = RESPONSE;
      return 1;
    }
    header.fields[key] = value;  // throwable
  }
  return 0;
}

// https://datatracker.ietf.org/doc/html/rfc9112#section-6.1
int Connection::parse_body() {  // throwable
  Log::debug("parse_body");
  if (header.fields.find("Transfer-Encoding") != header.fields.end() &&
      header.fields["Transfer-Encoding"].find("chunked") != std::string::npos) {
    // TODO: Handle invalid Transfer-Encoding
    if (header.fields.find("Content-Length") != header.fields.end()) {
      Log::cinfo() << "Both Transfer-Encoding and Content-Length are "
                      "specified"
                   << std::endl;
      ErrorHandler::handle(*this, 400);
      status = RESPONSE;
      return 1;
    }
    status = REQ_BODY_CHUNKED;
    return 1;
  }
  status = REQ_BODY_CONTENT_LENGTH;
  return 1;
}

// https://datatracker.ietf.org/doc/html/rfc9112#section-7.1
//  chunked-body   = *chunk
//                   last-chunk
//                   trailer-section
//                   CRLF
//
//  chunk          = chunk-size [ chunk-ext ] CRLF
//                   chunk-data CRLF
//  chunk-size     = 1*HEXDIG
//  last-chunk     = 1*("0") [ chunk-ext ] CRLF
//
//  chunk-data     = 1*OCTET ; a sequence of chunk-size octets
int Connection::parse_body_chunked() {  // throwable
  Log::debug("parse_body_chunked");
  std::string chunk_size_line;
  while (client_socket->read_telnet_line(chunk_size_line) == 0) {  // throwable
    std::stringstream ss(chunk_size_line);                         // throwable
    ss >> std::hex >> chunk_size;                                  // no throw
    if (ss.fail()) {
      Log::cinfo() << "Invalid chunk size: " << chunk_size_line << std::endl;
      ErrorHandler::handle(*this, 400);
      status = RESPONSE;
      return 1;
    }
    // TODO: Handle chunk-ext

    // If chunk-size is zero, this is the last-chunk
    // So, copy chunked_body to body
    if (chunk_size == 0) {
      // TODO: Improve this inefficiency
      content_length = body.size();
      chunk_size = 0;
      status = REQ_BODY_CHUNK_TRAILER_SECTION;
      return 1;
    }
    // If chunk-size is non-zero, chunk-data is expected
    status = REQ_BODY_CHUNK_DATA;
    return 1;
  }
  // There is still more to read later
  return 0;
}

int Connection::parse_body_chunk_data() {  // throwable
  Log::debug("parse_body_chunk_data");
  Log::cdebug() << "chunk_size: " << chunk_size << std::endl;
  // Add 2 bytes for CRLF
  std::vector<char> buf(chunk_size + 2);  // throwable
  ssize_t ret = client_socket->read(&buf[0], 2 + chunk_size - chunk.size());

  if (ret < 0) {
    return 0;
  } else if (ret == 0) {
    return 0;
  }
  chunk.append(&buf[0], ret);
  if (chunk.size() == chunk_size + 2) {  // +2 for CRLF
    // Check if the last two bytes are CRLF
    if (chunk.substr(chunk.size() - 2) != CRLF) {  // throwable
      Log::cinfo() << "Invalid chunk data: " << chunk << std::endl;
      ErrorHandler::handle(*this, 400);
      status = RESPONSE;
      return 1;
    }
    // Remove CRLF
    chunk.erase(chunk.size() - 2);
    // Append a chunk to chunked_body
    body.append(chunk);
    chunk.clear();
    status = REQ_BODY_CHUNKED;
    return 1;
  }
  // There is more to read
  return 0;
}

int Connection::parse_body_chunk_trailer_section() {  // throwable
  Log::debug("parse_body_chunk_trailer_section");
  // TODO: Implement [trailer-section]
  status = HANDLE;
  return 1;
}

int Connection::parse_body_content_length() {  // throwable
  Log::debug("parse_body_content_length");
  if (body.empty()) {
    if (header.fields.find("Content-Length") == header.fields.end()) {
      status = HANDLE;
      return 1;
    }
    // TODO: Handle invalid Content-Length
    content_length = atoi(header.fields["Content-Length"].c_str());
    body.reserve(content_length);
  }
  std::vector<char> buf(content_length - body.size());
  ssize_t ret = client_socket->read(&buf[0], buf.size());  // throwable
  if (ret < 0) {
    return 0;
  } else if (ret == 0) {
    return 0;
  }
  body.append(&buf[0], ret);
  if (body.size() == content_length) {
    status = HANDLE;
    return 1;
  } else {
    return 0;
  }
}

bool eq_addr(const sockaddr_in *a, const sockaddr_in *b) {
  // If port is different, return false
  if (a->sin_port != b->sin_port) {
    return false;
  }
  // If a or b is wildcard, return true
  if (a->sin_addr.s_addr == INADDR_ANY || b->sin_addr.s_addr == INADDR_ANY) {
    return true;
  }
  // Otherwise, compare address
  // sin_addr.sin_addr is just a uint32_t, so we can compare it directly
  return a->sin_addr.s_addr == b->sin_addr.s_addr;
}

bool eq_addr6(const sockaddr_in6 *a, const sockaddr_in6 *b) {
  // If port is different, return false
  if (a->sin6_port != b->sin6_port) {
    return false;
  }
  // If a or b is wildcard, return true
  if (IN6_IS_ADDR_UNSPECIFIED(&a->sin6_addr) ||
      IN6_IS_ADDR_UNSPECIFIED(&b->sin6_addr)) {
    return true;
  }
  // Otherwise, compare address
  // sin6_addr.sin6_addr is just a uint8_t[16], so we can compare it by memcmp
  return memcmp(&a->sin6_addr, &b->sin6_addr, sizeof(in6_addr)) == 0;
}

bool eq_addr46(const sockaddr_storage *a, const sockaddr_storage *b) {
  if (a->ss_family != b->ss_family) {
    return false;
  }
  if (a->ss_family == AF_INET) {
    return eq_addr((const sockaddr_in *)a, (const sockaddr_in *)b);
  } else if (a->ss_family == AF_INET6) {
    return eq_addr6((const sockaddr_in6 *)a, (const sockaddr_in6 *)b);
  } else {
    return false;
  }
}

// Find config for this request
// 1. Find listen directive matching port
// 2. Find listen directive matching ip address
// 3. Find server_name directive matching host name (if not default server)
const config::Server *select_srv_cf(const config::Config &cf,
                                    const Connection &conn) throw() {
  struct sockaddr_storage *saddr = &(*conn.client_socket)->saddr;
  std::string host;
  if (conn.header.fields.find("Host") != conn.header.fields.end()) {
    host = conn.header.fields.find("Host")->second;
  }
  // Remove port number
  size_t pos = host.find(':');
  if (pos != std::string::npos) {
    host.erase(pos);
  }
  // struct sockaddr_storage* saddr = &(*client_socket)->saddr;
  const config::Server *srv_cf = NULL;
  for (unsigned int i = 0; i < cf.http.servers.size(); i++) {
    const config::Server &srv = cf.http.servers[i];
    for (unsigned int j = 0; j < srv.listens.size(); j++) {
      const config::Listen &listen = srv.listens[j];
      // 0. Filter by IPv4 or IPv6
      // 1. Filter by port
      // 2. Filter by address
      // These can be done by eq_addr46 and eq_addr6
      if (eq_addr46(&listen.addr, saddr) == false) {
        continue;
      }
      // This server is default server
      if (!srv_cf) {
        srv_cf = &srv;
        break;
      }

      // 3. Filter by host name
      if (util::vector::contains(srv.server_names, host)) {
        srv_cf = &srv;
        break;
      }
    }
  }
  // Some server context must be found because we only listen on
  // specified ports and addresses
  assert(srv_cf != NULL);
  return srv_cf;
}

// Note: We don't support regex
// https://nginx.org/en/docs/http/ngx_http_core_module.html#location
// To find location matching a given request, nginx first checks locations
// defined using the prefix strings (prefix locations). Among them, the location
// with the longest matching prefix is selected and remembered. Then regular
// expressions are checked, in the order of their appearance in the
// configuration file. The search of regular expressions terminates on the first
// match, and the corresponding configuration is used. If no match with a
// regular expression is found then the configuration of the prefix location
// remembered earlier is used.

template <typename ConfigItem>
void traverse_loc(const ConfigItem *cf, const std::string &path, int prefixLen,
                  int &maxLen, const config::Location *&loc) {
  // Find location for this request
  // 1. Find location directive matching prefix string
  // 2. location with the longest matching prefix is selected and remembered
  for (unsigned int i = 0; i < cf->locations.size(); i++) {
    const config::Location &l = cf->locations[i];
    // TODO: header.path must be a normalized URI
    //       1. decoding the text encoded in the “%XX” form
    //       2. resolving references to relative path components (“.” and “..”)
    //       3. possible compression of two or more adjacent slashes into a
    //       single slash
    // 1. Filter by prefix
    if (path.substr(prefixLen, l.path.size()) == l.path) {
      prefixLen += l.path.size();
      // location with the longest matching prefix is selected and remembered
      if (loc == NULL || maxLen < prefixLen) {
        loc = &l;
        maxLen = prefixLen;
      }
      traverse_loc(&l, path, prefixLen, maxLen, loc);
      prefixLen -= l.path.size();
    }
  }
}

const config::Location *select_loc_cf(const config::Server *srv_cf,
                                      const std::string &path) throw() {
  const config::Location *loc = NULL;
  int maxLen = 0;
  traverse_loc(srv_cf, path, 0, maxLen, loc);
  return loc;
}

// throwable
template <typename ConfigItem>
const config::CgiHandler *select_cgi_handler_cf(const ConfigItem *cf,
                                                const std::string &path) {
  if (cf->cgi_handlers.empty()) {
    return NULL;
  }
  // Find CGI handler for this request
  // TODO: check `index` directive
  std::string ext = util::path::get_extension(path);  // throwable
  for (unsigned int i = 0; i < cf->cgi_handlers.size(); i++) {
    const config::CgiHandler &cgi = cf->cgi_handlers[i];
    if (util::vector::contains(cgi.extensions, ext)) {
      return &cgi;
    }
  }
  return NULL;
}

// throwable
template <typename ConfigItem>
const config::CgiExtensions *select_cgi_ext_cf(const ConfigItem *cf,
                                               const std::string &path) {
  if (!cf->cgi_extensions.configured) {
    return NULL;
  }
  // Find CGI handler for this request
  std::string ext = util::path::get_extension(path);  // throwable
  if (util::vector::contains(cf->cgi_extensions, ext)) {
    return &cf->cgi_extensions;
  }
  return NULL;
}

int Connection::handle() {  // throwable
  srv_cf = select_srv_cf(cf, *this);
  loc_cf = select_loc_cf(srv_cf, header.path);
  cgi_handler_cf = loc_cf ? select_cgi_handler_cf(loc_cf, header.path)
                          : select_cgi_handler_cf(srv_cf, header.path);
  cgi_ext_cf = loc_cf ? select_cgi_ext_cf(loc_cf, header.path)
                      : select_cgi_ext_cf(srv_cf, header.path);

  // `return` directive
  if (loc_cf && loc_cf->returns.size() > 0) {
    RedirectHandler::handle(*this, loc_cf->returns[0].code,
                            loc_cf->returns[0].url);
    status = RESPONSE;
    return 1;
  }

  // generate fullpath
  // `root` and `alias` directive
  if (!loc_cf) {
    // Server Root    : Append path to root
    Log::cdebug() << "Server Root" << std::endl;
    header.fullpath = srv_cf->root + header.path;  // throwable
  } else if (!loc_cf->alias.configured) {
    // Location Root  : Append path to root
    Log::cdebug() << "Location Root: " << loc_cf->path << std::endl;
    header.fullpath = loc_cf->root + header.path;  // throwable
  } else {
    // Location Alias : Replace prefix with alias
    Log::cdebug() << "Location Alias: " << loc_cf->path << std::endl;
    header.fullpath =
        loc_cf->alias + header.path.substr(loc_cf->path.size());  // throwable
  }

  // `limit_except` directive
  if (loc_cf && loc_cf->limit_except.configured) {
    if (util::vector::contains(loc_cf->limit_except.methods, header.method) ==
        false) {
      Log::cinfo() << "Unsupported method: " << header.method << std::endl;
      ErrorHandler::handle(*this, 405);
      status = RESPONSE;
      return 1;
    }
  }

  // if CGI
  if (cgi_ext_cf || cgi_handler_cf) {
    Log::cdebug() << "CGI request" << std::endl;
    cgi_started = time(NULL);
    if (CgiHandler::handle(*this) < 0)
      status = RESPONSE;
    else
      status = HANDLE_CGI_REQ;
    return 1;
  }
  // If not CGI
  if (header.method == "GET") {
    GetHandler::handle(*this);  // throwable
  } else if (header.method == "POST") {
    PostHandler::handle(*this);  // throwable
  } else if (header.method == "PUT") {
    PostHandler::handle(*this);  // throwable
  } else if (header.method == "DELETE") {
    DeleteHandler::handle(*this);
  } else {
    Log::cinfo() << "Unsupported method: " << header.method << std::endl;
    ErrorHandler::handle(*this, 405);  // throwable
  }
  status = RESPONSE;
  return 1;
}

int Connection::handle_cgi_req() throw() {
  Log::debug("handle_cgi_req");
  Log::cdebug() << "isSendBufEmpty: " << cgi_socket->isSendBufEmpty()
                << std::endl;
  if (cgi_socket->isSendBufEmpty()) {
    shutdown(cgi_socket->get_fd(), SHUT_WR);
    status = HANDLE_CGI_RES;
    return 1;
  }
  return 0;
}

int Connection::handle_cgi_res() throw() {
  Log::debug("handle_cgi_res");
  // 1. If CGI process is still running, return 0
  // Question: If CGI process exit, isClosed will be set to true?
  if (!cgi_socket->isClosed()) return 0;
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
int Connection::handle_cgi_parse() {  // throwable
  Log::debug("handle_cgi_parse");
  // CGI Process is already terminated
  if (cgi_pid < 0) {
    return 1;
  }

  // Kill CGI process
  int exit_status = kill_and_reap_cgi_process();

  // CGI Script Error
  if (exit_status != 0) {
    Log::cdebug() << "CGI process terminated with status: " << status
                  << std::endl;
    ErrorHandler::handle(*this, 500);
    status = RESPONSE;
    return 1;
  }

  // TODO: parse
  std::string line;
  // Read header fields
  std::map<std::string, std::string> cgi_header_fields;
  while (cgi_socket->readline(line) == 0) {  // throwable
    Log::cdebug() << "CGI line: " << line << std::endl;
    // Empty line indicates the end of header fields
    if (line == "" || line == "\r") {
      status = RESPONSE;
      break;
    }
    std::string key, value;
    if (split_header_field(line, key, value) < 0) {
      Log::cwarn() << "Invalid CGI header field: " << line << std::endl;
      ErrorHandler::handle(*this, 500);
      status = RESPONSE;
      break;
    }
    cgi_header_fields[key] = value;
  }
  if (cgi_header_fields.find("Status") != cgi_header_fields.end()) {
    // TODO: validate status code
    Log::cdebug() << "Status found: " << cgi_header_fields["Status"]
                  << std::endl;
    *client_socket << "HTTP/1.1 " << cgi_header_fields["Status"] << CRLF;
  } else {
    Log::cdebug() << "Status not found" << std::endl;
    *client_socket << "HTTP/1.1 200 OK" << CRLF;
  }
  // Send header fields
  std::map<std::string, std::string>::const_iterator it;
  for (it = cgi_header_fields.begin(); it != cgi_header_fields.end(); ++it) {
    if (it->first == "Status") continue;
    // TODO: validate header field
    *client_socket << it->first << ": " << it->second << CRLF;
  }
  // Send response-body if any
  size_t size = cgi_socket->getReadBufSize();
  if (size > 0) {
    *client_socket << "Content-Length: " << size << CRLF;
    *client_socket << CRLF;  // End of header fields
    *cgi_socket >> *client_socket;
  } else {
    *client_socket << "Content-Length: 0" << CRLF;
    *client_socket << CRLF;  // End of header fields
  }
  status = RESPONSE;
  return 1;
}

int Connection::response() throw() {
  if (client_socket->isSendBufEmpty()) {
    status = CLEAR;
    return 1;
  }
  return 0;
}

bool Connection::is_timeout() const throw() {
  return (time(NULL) - last_modified) > TIMEOUT_SEC;
}

bool Connection::is_cgi_timeout() const throw() {
  switch (status) {
    case HANDLE_CGI_REQ:
    case HANDLE_CGI_RES:
      break;
    default:
      return false;
  }
  return (time(NULL) - cgi_started) > CGI_TIMEOUT_SEC;
}

int Connection::kill_and_reap_cgi_process() throw() {
  // Already handled
  if (cgi_pid <= 0) {
    return -1;
  }

  // Kill the cgi process
  if (kill(cgi_pid, SIGKILL) < 0) {
    Log::cerror() << "kill failed. (" << errno << ": " << strerror(errno) << ")"
                  << std::endl;
    return -1;
  }

  // Wait for CGI process to terminate
  pid_t ret;
  int exit_status;
  Log::cdebug() << "waitpid(" << cgi_pid << ")" << std::endl;
  while ((ret = waitpid(cgi_pid, &exit_status, WNOHANG)) <= 0) {
    if (ret == 0) {
      // If CGI process is still running, continue to waitpid
      // This is possible because of the timelag between kill() and actual
      // termination of CGI process
      continue;
    }
    // If interrupted by signal, continue to waitpid again
    if (errno == EINTR) continue;
    // Other errors(ECHILD/EFAULT/EINVAL), return 500
    // I don't think this will happen though.
    Log::cfatal() << "waitpid failed. (" << errno << ": " << strerror(errno)
                  << ")" << std::endl;
    cgi_pid = -1;
    return -1;
  }
  Log::cdebug() << "waitpid(" << cgi_pid << ") returns " << ret << std::endl;
  cgi_pid = -1;
  return exit_status;
}

void Connection::handle_cgi_timeout() throw() {
  Log::info("CGI timeout");
  // Already handled
  if (cgi_pid == -1) {
    return;
  }
  // kill cgi process
  if (kill_and_reap_cgi_process() < 0) {
    Log::cfatal() << "kill_and_reap_cgi_process failed" << std::endl;
    ErrorHandler::handle(*this, 500);
    status = RESPONSE;
    return;
  }
  ErrorHandler::handle(*this, 504);
  status = RESPONSE;
}
