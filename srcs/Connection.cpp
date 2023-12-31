#include "Connection.hpp"

#include <sys/wait.h>

#include <cctype>
#include <cerrno>
#include <map>
#include <string>

#include "DeleteHandler.hpp"
#include "Server.hpp"

int Connection::resume() {  // throwable
  Log::debug("Connection::resume()");
  last_modified = time(NULL);
  // 1. Socket I/O
  switch (io_status) {
    case CLIENT_RECV:
      client_socket->fill();  // throwable
      break;
    case CGI_SEND:
      cgi_socket->flush();  // throwable
      break;
    case CGI_RECV:
      cgi_socket->fill();  // throwable
      break;
    case CLIENT_SEND:
      client_socket->flush();  // throwable
      break;
    case NO_IO:
      break;
  }
  int ret = WSV_AGAIN;
  while (ret == WSV_AGAIN) {
    ret = (this->*handler)();  // throwable
  }
  // While handling the request, can we notice the socket status change?
  // Finally, check if there is any error while handling the request
  if (client_socket->bad()) {
    Log::info("client_socket->bad()");
    return WSV_REMOVE;
  }
  if (cgi_socket != NULL) {
    if (cgi_socket->bad()) {
      Log::info("cgi_socket->bad()");
      return WSV_REMOVE;
    }
  }
  // If
  //  1. client socket has received EOF
  //  2. client socket's both buffers are empty
  //  3. cgi socket will not produce any more data
  // we can close the connection.
  //
  // Main scenario: After sending a response, the client close the connection.
  // In this case, the client socket will receive EOF and the client socket's
  // both buffers will be empty.
  if (client_socket->hasReceivedEof && client_socket->isSendBufEmpty() &&
      client_socket->isRecvBufEmpty()) {
    if (cgi_socket == NULL || cgi_socket->hasReceivedEof) {
      Log::info("client_socket->hasReceivedEof and all buffers are empty");
      return WSV_REMOVE;
    }
  }
  return ret;
}

int Connection::clear() {
  cgi_socket = util::shared_ptr<SocketBuf>();
  cgi_pid = -1;
  last_modified = time(NULL);
  cgi_started = 0;
  handler = &Connection::parse_start_line;
  req = Request();
  res = Response();
  io_status = NO_IO;
  return 0;
}

// RFC[URI] : https://datatracker.ietf.org/doc/html/rfc3986
//            https://datatracker.ietf.org/doc/html/rfc3986#appendix-A
// RFC[HTTP] : https://datatracker.ietf.org/doc/html/rfc9110#uri
//
// URI references are used to target requests, indicate redirects, and define
// relationships.
//
// The definitions of "URI-reference", "absolute-URI", "relative-part",
// "authority", "port", "host", "path-abempty", "segment", and "query" are
// adopted from the URI generic syntax. An "absolute-path" rule is defined for
// protocol elements that can contain a non-empty path component. (This rule
// differs slightly from the path-abempty rule of RFC 3986, which allows for an
// empty path, and path-absolute rule, which does not allow paths that begin
// with "//".) A "partial-URI" rule is defined for protocol elements that can
// contain a relative URI but not a fragment component.
//
//
// 1. URI-reference
// URI-reference = URI / relative-ref
// URI           = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
// relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
// hier-part     = "//" authority path-abempty
//               / path-absolute
//               / path-rootless
//               / path-empty
//
// 2. absolute-URI
// absolute-URI  = scheme ":" hier-part [ "?" query ]
//
// 3. relative-part
// relative-part = "//" authority path-abempty
//               / path-absolute
//               / path-noscheme
//               / path-empty
//
// 4. authority
// authority     = [ userinfo "@" ] host [ ":" port ]
//
// 5. host
// host          = IP-literal / IPv4address / reg-name
// reg-name      = *( unreserved / pct-encoded / sub-delims )
//
// 6. port
// port          = *DIGIT
//
// 7. path-abempty
// path-abempty  = *( "/" segment )
// * empty path is not allowed in HTTP
//
// 8. segment
// segment       = *pchar
//
// 9. query
// query         = *( pchar / "/" / "?" )
//
// absolute-path = 1*( "/" segment )
// partial-URI   = relative-part [ "?" query ]

// It is RECOMMENDED that all senders and recipients support, at a minimum, URIs
// with lengths of 8000 octets in protocol elements. Note that this implies some
// structures and on-wire representations (for example, the request line in
// HTTP/1.1) will necessarily be larger in some cases.
#define MAX_URI_LENGTH 8000

static bool is_valid_path(std::string const &path) {
  // Empty path is allowed
  if (path.empty()) return true;

  if (path.size() > MAX_URI_LENGTH) return false;  // Too long
  if (path[0] != '/') return false;                // TODO: Allow absoluteURI
  /*
   * Nginx does not check the following
  for (size_t i = 0; i < path.size();) {
    if (path[i] != '/') return false;
    i++;
    while (i < path.size() && util::http::is_pchar(path, i)) {
      i++;
    }
  }
  */
  return true;
}

static bool is_valid_decoded_path(std::string const &path) {
  // Prohibit path component ".."
  // 1. path contains "/../" is not allowed
  // 2. path ends with "/.." is not allowed
  std::string::size_type s = 0,
                         i = 0;  // s points to the first char of the segment
  for (; i < path.size();) {
    if (path[i] != '/') {
      i++;
      continue;
    }
    if ((i - s) == 2 && path[s] == '.' && path[s + 1] == '.') return false;
    i++;
    s = i;  // s does not point to '/'
  }
  if ((i - s) == 2 && path[s] == '.' && path[s + 1] == '.') return false;
  return true;
}

static bool decode_percent(std::string &path) {
  std::string dst;
  for (size_t i = 0; i < path.size();) {
    if (path[i] != '%') {
      dst += path[i];
      i++;
      continue;
    }
    if (i + 2 >= path.size()) return false;
    if (!(std::isxdigit(path[i + 1]) && std::isxdigit(path[i + 2])))
      return false;
    dst += strtol(path.substr(i + 1, 2).c_str(), NULL, 16);
    i += 3;
  }
  path = dst;
  return true;
}

// Return 0 if success, -1 if failed
static int parse_http_version(std::string const &s, Version &version) {
  if (s.size() != 8) return -1;
  if (s.find_first_of("HTTP/") != 0) return -1;
  if (s[6] != '.') return -1;
  version.major = s[5] - '0';
  version.minor = s[7] - '0';
  return 0;
}

// TODO: make this noexcept
// https://datatracker.ietf.org/doc/html/rfc2616#section-5.1
// Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
int Connection::parse_start_line() {
  Log::cdebug() << "parse_start_line" << std::endl;
  std::string line;

  // CRLF hoge CRLF fuga CRLF
  while (1) {
    // If buffer has not enough data to read a line, wait for more data
    // TODO: What if client socket is closed?
    if (client_socket->read_telnet_line(line) < 0) {  // throwable
      return WSV_WAIT;
    }

    // If line is empty, ignore it
    if (line.empty()) continue;

    // If line is not empty, break to parse it
    break;
  }
  Log::cdebug() << "start line: " << line << std::endl;
  std::stringstream ss;
  std::string version;
  ss << line;
  ss >> req.header.method;  // ss does not throw (cf. playground/fuga.cpp)
  ss >> req.header.path;    // ss does not throw
  ss >> version;            // ss does not throw

  // URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
  // relative-ref = relative-part [ "?" query ] [ "#" fragment ]
  {
    std::string::size_type i;
    // [ "#" fragment ]
    if ((i = req.header.path.find_first_of('#')) != std::string::npos) {
      req.header.fragment = req.header.path.substr(i + 1);
      req.header.path = req.header.path.substr(0, i);
    }
    // [ "?" query ]
    if ((i = req.header.path.find_first_of('?')) != std::string::npos) {
      req.header.query = req.header.path.substr(i + 1);
      req.header.path = req.header.path.substr(0, i);
    }
  }

  // Path must be starting with /
  if (!is_valid_path(req.header.path) || !decode_percent(req.header.path) ||
      !decode_percent(req.header.query) ||
      !decode_percent(req.header.fragment) ||
      !is_valid_decoded_path(req.header.path)) {
    Log::cinfo() << "Invalid path: " << req.header.path << std::endl;
    ErrorHandler::handle(*this, 400);
    handler = &Connection::response;
    return WSV_AGAIN;
  }

  // Parse HTTP version
  // Before HTTP/1.0 (i.e. HTTP/0.9) does not have HTTP-Version
  if (parse_http_version(version, req.header.version) < 0 ||
      req.header.version < WSV_HTTP_VERSION_10) {
    Log::cinfo() << "Invalid HTTP version: " << version << std::endl;
    ErrorHandler::handle(*this, 400);
    handler = &Connection::response;
    return WSV_AGAIN;
  }

  // Validate HTTP version (Only 1.1 or later is supported)
  if (WSV_HTTP_VERSION_20 <= req.header.version) {
    Log::cinfo() << "Unsupported HTTP version: " << version << std::endl;
    res.keep_alive = false;
    ErrorHandler::handle(*this, 505);
    handler = &Connection::response;
    return WSV_AGAIN;
  }

  // Keep-Alive is default for HTTP/1.1 or later
  if (WSV_HTTP_VERSION_11 <= req.header.version) {
    res.keep_alive = true;
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
    handler = &Connection::response;
    return WSV_AGAIN;
  }
  // ss.fail() : method or path or version is missing
  // !ss.eof()  : there are more than 3 tokens or extra white spaces
  if (!ss.eof()) ss >> std::ws;  // Trailing white spaces are allowed
  if (ss.fail() || !ss.eof()) {
    Log::cinfo() << "Invalid start line: " << line << std::endl;
    ErrorHandler::handle(*this, 400);
    handler = &Connection::response;
    return WSV_AGAIN;
  }
  handler = &Connection::read_header_fields;
  return WSV_AGAIN;
}

int Connection::split_header_field(const std::string &line, std::string &key,
                                   std::string &value) {  // throwable
  std::stringstream ss(line);  // throwable! (bad alloc)
  // 1. Extract key
  if (!std::getline(ss, key, ':')) {  // throwable
    Log::cerror() << "std::getline(ss, key, ':') failed. line: " << line
                  << std::endl;
    ErrorHandler::handle(*this, 500);
    handler = &Connection::response;
    return 1;
  }
  // 2. Remove leading space from value
  ss >> std::ws;
  // 3. Extract remaining string to value
  std::getline(ss, value);  // throwable
  return 0;
}

int Connection::read_header_fields() {  // throwable
  std::string line;
  while (client_socket->read_telnet_line(line) == 0) {  // throwable
    // Empty line indicates the end of header fields
    if (line == "") {
      handler = &Connection::parse_header_fields;
      return WSV_AGAIN;
    }
    Log::cdebug() << "header line: " << line << std::endl;
    // Split line to key and value
    std::string key, value;
    if (split_header_field(line, key, value) < 0) {  // throwable
      ErrorHandler::handle(*this, 500);
      handler = &Connection::response;
      return WSV_AGAIN;
    }
    if (util::http::is_token(key) == false) {
      Log::cinfo() << "Header filed-name is not token: : " << key << std::endl;
      ErrorHandler::handle(*this, 400);
      handler = &Connection::response;
      return WSV_AGAIN;
    }
    req.header.fields[util::http::canonical_header_key(key)] =
        value;  // throwable
  }
  return WSV_WAIT;
}

int Connection::parse_header_fields() {  // throwable
  // Host header is mandatory for HTTP/1.1 or later
  if (WSV_HTTP_VERSION_11 <= req.header.version) {
    if (!util::contains(req.header.fields, "Host")) {
      Log::cinfo() << "Host header is missing" << std::endl;
      ErrorHandler::handle(*this, 400);
      handler = &Connection::response;
      return WSV_AGAIN;
    }
  }
  Header::const_iterator it = req.header.fields.find("Connection");
  if (it != req.header.fields.end()) {
    if (it->second == "close") {
      res.keep_alive = false;
    } else if (it->second == "keep-alive") {
      res.keep_alive = true;
    } else {
      Log::cinfo() << "Invalid Connection header: " << it->second << std::endl;
      ErrorHandler::handle(*this, 400);
      handler = &Connection::response;
      return WSV_AGAIN;
    }
  }
  handler = &Connection::parse_body;
  return WSV_AGAIN;
}

// https://datatracker.ietf.org/doc/html/rfc9112#section-6.1
int Connection::parse_body() {  // throwable
  Log::debug("parse_body");
  Header::const_iterator it = req.header.fields.find("Transfer-Encoding");
  if (it != req.header.fields.end() && util::contains(it->second, "chunked")) {
    if (req.header.version < WSV_HTTP_VERSION_11) {
      Log::cinfo()
          << "Chunked encoding is not supported in HTTP/1.0.\n"
          << "A server or client that receives an HTTP/1.0 message containing "
             "a Transfer-Encoding header field MUST treat the message as if "
             "the framing is faulty, even if a Content-Length is present, and "
             "close the connection after processing the message."
          << std::endl;
      handler = &Connection::parse_body_content_length;
      return WSV_AGAIN;
    }
    // TODO: Handle invalid Transfer-Encoding
    if (util::contains(req.header.fields, "Content-Length")) {
      Log::cinfo() << "Both Transfer-Encoding and content-length are "
                      "specified"
                   << std::endl;
      ErrorHandler::handle(*this, 400);
      handler = &Connection::response;
      return WSV_AGAIN;
    }
    handler = &Connection::parse_body_chunked;
    return WSV_AGAIN;
  }
  handler = &Connection::parse_body_content_length;
  return WSV_AGAIN;
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
    ss >> std::hex >> req.chunk_size;                              // no throw
    if (ss.fail()) {
      Log::cinfo() << "Invalid chunk size: " << chunk_size_line << std::endl;
      ErrorHandler::handle(*this, 400);
      handler = &Connection::response;
      return WSV_AGAIN;
    }
    // TODO: Handle chunk-ext

    // If chunk-size is zero, this is the last-chunk
    // So, copy chunked_body to body
    if (req.chunk_size == 0) {
      // TODO: Improve this inefficiency
      req.content_length = req.body.size();
      req.chunk_size = 0;
      handler = &Connection::parse_body_chunk_trailer_section;
      return WSV_AGAIN;
    }
    // If chunk-size is non-zero, chunk-data is expected
    handler = &Connection::parse_body_chunk_data;
    return WSV_AGAIN;
  }
  // There is still more to read later
  return WSV_WAIT;
}

int Connection::parse_body_chunk_data() {  // throwable
  Log::debug("parse_body_chunk_data");
  Log::cdebug() << "chunk_size: " << req.chunk_size << std::endl;
  // Add 2 bytes for CRLF
  std::vector<char> buf(req.chunk_size + 2);  // throwable
  ssize_t ret =
      client_socket->read(&buf[0], 2 + req.chunk_size - req.chunk.size());

  if (ret < 0) {
    return WSV_WAIT;
  } else if (ret == 0) {
    return WSV_WAIT;
  }
  req.chunk.append(&buf[0], ret);
  if (req.chunk.size() == req.chunk_size + 2) {  // +2 for CRLF
    // Check if the last two bytes are CRLF
    if (req.chunk.substr(req.chunk.size() - 2) != CRLF) {  // throwable
      Log::cinfo() << "Invalid chunk data: " << req.chunk << std::endl;
      ErrorHandler::handle(*this, 400);
      handler = &Connection::response;
      return WSV_AGAIN;
    }
    // Remove CRLF
    req.chunk.erase(req.chunk.size() - 2);
    // Append a chunk to chunked_body
    req.body.append(req.chunk);
    req.chunk.clear();
    handler = &Connection::parse_body_chunked;
    return WSV_AGAIN;
  }
  // There is more to read
  return WSV_WAIT;
}

int Connection::parse_body_chunk_trailer_section() {  // throwable
  Log::debug("parse_body_chunk_trailer_section");
  // TODO: Implement [trailer-section]
  handler = &Connection::handle;
  return WSV_AGAIN;
}

int Connection::parse_body_content_length() {  // throwable
  Log::debug("parse_body_content_length");
  if (req.body.empty()) {
    Header::iterator it = req.header.fields.find("Content-Length");
    if (it == req.header.fields.end()) {
      handler = &Connection::handle;
      return WSV_AGAIN;
    }
    // TODO: Handle invalid Content-Length
    req.content_length = atoi(it->second.c_str());
    req.body.reserve(req.content_length);
  }
  std::vector<char> buf(req.content_length - req.body.size());
  ssize_t ret = client_socket->read(&buf[0], buf.size());  // throwable
  if (ret < 0) {
    return WSV_WAIT;
  } else if (ret == 0) {
    return WSV_WAIT;
  }
  req.body.append(&buf[0], ret);
  if (req.body.size() == req.content_length) {
    handler = &Connection::handle;
    return WSV_AGAIN;
  } else {
    return WSV_WAIT;
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
  Header::const_iterator it = conn.req.header.fields.find("Host");
  if (it != conn.req.header.fields.end()) {
    host = it->second;
  }
  // Remove port number
  size_t pos = host.find(':');
  if (pos != std::string::npos) {
    host.erase(pos);
  }
  host = util::http::normalized_host(host);
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
      if (util::inet::eq_addr46(&listen.addr, saddr, true) == false) {
        continue;
      }
      // This server is default server
      if (!srv_cf) {
        srv_cf = &srv;
        break;
      }

      // 3. Filter by host name
      if (util::contains(srv.server_names, host)) {
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
    if (util::contains(cgi.extensions, ext)) {
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
  if (util::contains(cf->cgi_extensions, ext)) {
    return &cf->cgi_extensions;
  }
  return NULL;
}

int Connection::handle() {  // throwable
  req.srv_cf = select_srv_cf(server->cf, *this);
  req.loc_cf = select_loc_cf(req.srv_cf, req.header.path);
  req.cgi_handler_cf = req.loc_cf
                           ? select_cgi_handler_cf(req.loc_cf, req.header.path)
                           : select_cgi_handler_cf(req.srv_cf, req.header.path);
  req.cgi_ext_cf = req.loc_cf ? select_cgi_ext_cf(req.loc_cf, req.header.path)
                              : select_cgi_ext_cf(req.srv_cf, req.header.path);

  // `return` directive
  if (req.loc_cf && req.loc_cf->returns.size() > 0) {
    RedirectHandler::handle(*this, req.loc_cf->returns[0].code,
                            req.loc_cf->returns[0].url);
    handler = &Connection::response;
    return WSV_AGAIN;
  }

  // generate fullpath
  // `root` and `alias` directive
  if (!req.loc_cf) {
    // Server Root    : Append path to root
    Log::cdebug() << "Server Root" << std::endl;
    req.header.fullpath = req.srv_cf->root + req.header.path;  // throwable
  } else if (!req.loc_cf->alias.configured) {
    // Location Root  : Append path to root
    Log::cdebug() << "Location Root: " << req.loc_cf->path << std::endl;
    req.header.fullpath = req.loc_cf->root + req.header.path;  // throwable
  } else {
    // Location Alias : Replace prefix with alias
    Log::cdebug() << "Location Alias: " << req.loc_cf->path << std::endl;
    req.header.fullpath =
        req.loc_cf->alias +
        req.header.path.substr(req.loc_cf->path.size());  // throwable
  }

  // `limit_except` directive
  if (req.loc_cf && req.loc_cf->limit_except.configured) {
    if (!util::contains(req.loc_cf->limit_except.methods, req.header.method)) {
      Log::cinfo() << "Unsupported method: " << req.header.method << std::endl;
      ErrorHandler::handle(*this, 405);
      handler = &Connection::response;
      return WSV_AGAIN;
    }
  }

  // if CGI
  if (req.cgi_ext_cf || req.cgi_handler_cf) {
    Log::cdebug() << "CGI request" << std::endl;
    cgi_started = time(NULL);
    if (CgiHandler::handle(*this) < 0) {
      handler = &Connection::response;
    } else {
      handler = &Connection::handle_cgi_req;
    }

    return WSV_AGAIN;
  }
  // If not CGI
  if (req.header.method == "GET") {
    GetHandler::handle(*this);  // throwable
  } else if (req.header.method == "POST") {
    PostHandler::handle(*this);  // throwable
  } else if (req.header.method == "PUT") {
    PostHandler::handle(*this);  // throwable
  } else if (req.header.method == "DELETE") {
    DeleteHandler::handle(*this);
  } else {
    Log::cinfo() << "Unsupported method: " << req.header.method << std::endl;
    ErrorHandler::handle(*this, 405);  // throwable
  }
  handler = &Connection::response;
  return WSV_AGAIN;
}

int Connection::response() throw() {
  client_socket->send_response(res);
  handler = &Connection::flush_response;
  return WSV_AGAIN;
}

int Connection::flush_response() throw() {
  if (client_socket->isSendBufEmpty()) {
    if (client_socket->hasReceivedEof && client_socket->isRecvBufEmpty()) {
      Log::info(
          "Client socket has received EOF and recvbuf is empty, remove "
          "connection");
      return WSV_REMOVE;
    } else if (!res.keep_alive) {
      Log::info("Connection: close, remove connection");
      return WSV_REMOVE;
    } else if (!client_socket->isRecvBufEmpty()) {
      Log::info("Client socket has data, clear and keep connection");
      if (get_cgifd() >= 0) {
        server->clear_fd(get_cgifd());
      }
      clear();
      return WSV_AGAIN;
    } else {
      Log::info("Request is done, clear connection");
      return WSV_CLEAR;
    }
  }
  return WSV_WAIT;
}

bool Connection::is_timeout() const throw() {
  return (time(NULL) - last_modified) > TIMEOUT_SEC;
}

bool Connection::is_cgi_timeout() const throw() {
  if (cgi_pid <= 0) {
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
    return -1;
  }
  Log::cdebug() << "waitpid(" << cgi_pid << ") returns " << ret << std::endl;
  Log::cdebug() << "exit_status: " << exit_status << std::endl;
  cgi_pid = -1;

  return exit_status;
}

void Connection::handle_cgi_timeout() throw() {
  Log::info("handle_cgi_timeout");
  assert(cgi_pid > 0);  // cgi_pid should be set before calling this function
  // kill cgi process
  if (kill_and_reap_cgi_process() < 0) {
    Log::cfatal() << "kill_and_reap_cgi_process failed" << std::endl;
    ErrorHandler::handle(*this, 500);
    handler = &Connection::response;
    return;
  }
  ErrorHandler::handle(*this, 504);
  response();
}
