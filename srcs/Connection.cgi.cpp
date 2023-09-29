#include "Connection.hpp"

int Connection::handle_cgi_req() throw() {
  Log::debug("handle_cgi_req");
  Log::cdebug() << "isSendBufEmpty: " << cgi_socket->isSendBufEmpty()
                << std::endl;
  if (cgi_socket->isSendBufEmpty()) {
    shutdown(cgi_socket->get_fd(), SHUT_WR);
    handler = &Connection::handle_cgi_res;
    return WSV_AGAIN;
  }
  return WSV_WAIT;
}

int Connection::handle_cgi_res() throw() {
  Log::debug("handle_cgi_res");
  if (cgi_socket->hasReceivedEof) {
    Log::info("cgi_socket has received EOF");
    handler = &Connection::handle_cgi_parse;
    return WSV_AGAIN;
  }
  return WSV_WAIT;
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
    handler = &Connection::response;
    return WSV_AGAIN;
  }

  // Kill CGI process
  int exit_status = kill_and_reap_cgi_process();

  // CGI Script Error
  if (exit_status != 0) {
    Log::cdebug() << "CGI process terminated with status: " << exit_status
                  << std::endl;
    ErrorHandler::handle(*this, 500);
    handler = &Connection::response;
    return WSV_AGAIN;
  }

  // TODO: parse
  std::string line;
  // Read header fields
  std::map<std::string, std::string> cgi_header_fields;
  while (cgi_socket->readline(line) == 0) {  // throwable
    Log::cdebug() << "CGI line: " << line << std::endl;
    // Empty line indicates the end of header fields
    if (line == "" || line == "\r") {
      handler = &Connection::response;
      break;
    }
    std::string key, value;
    if (split_header_field(line, key, value) < 0) {
      Log::cwarn() << "Invalid CGI header field: " << line << std::endl;
      ErrorHandler::handle(*this, 500);
      handler = &Connection::response;
      break;
    }
    key = util::http::canonical_header_key(key);
    cgi_header_fields[key] = value;
  }
  Header::const_iterator it;
  
  // 1. Status
  if ((it = cgi_header_fields.find("Status")) != cgi_header_fields.end()) {
    // TODO: validate status code
    Log::cdebug() << "Status found: " << it->second << std::endl;
    *client_socket << "HTTP/1.1 " << it->second << CRLF;
  } else {
    Log::cdebug() << "Status not found" << std::endl;

    *client_socket << "HTTP/1.1 200 OK" << CRLF;
  }
  
  // Send header fields
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
  handler = &Connection::response;
  return WSV_AGAIN;
}

