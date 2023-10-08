#include "Connection.hpp"

static const char* http_header_fields[] = {
    //"Content-Type", // This will be set automatically
    //"Content-Length", // This will be set automatically
    //"Location", // This will be set automatically
    "Set-Cookie", "Server", NULL};

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

static bool is_valid_status_code(const std::string& status_code) {
  if (status_code.size() != 3) {
    return false;
  }
  for (size_t i = 0; i < status_code.size(); i++) {
    if (!std::isdigit(status_code[i])) {
      return false;
    }
  }
  return true;
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
    // https://datatracker.ietf.org/doc/html/rfc3875#section-6.3
    // each CGI field MUST NOT appear more than once in the response.
    if (util::contains(cgi_header_fields, key)) {
      Log::cwarn() << "Duplicate CGI header field: " << line << std::endl;
      ErrorHandler::handle(*this, 500);
      handler = &Connection::response;
      break;
    }
    cgi_header_fields[key] = value;
  }
  // https://datatracker.ietf.org/doc/html/rfc3875#section-6.3
  // At least one CGI field MUST be supplied;
  if (cgi_header_fields.empty()) {
    Log::cwarn() << "No CGI header field" << std::endl;
    ErrorHandler::handle(*this, 500);
    handler = &Connection::response;
    return WSV_AGAIN;
  }

  Header::const_iterator it;
  // 1. Status
  /*
   The Status header field contains a 3-digit integer result code that
   indicates the level of success of the script's attempt to handle the
   request.
      Status         = "Status:" status-code SP reason-phrase NL
      status-code    = "200" | "302" | "400" | "501" | extension-code
      extension-code = 3digit
      reason-phrase  = *TEXT
  */
  if ((it = cgi_header_fields.find("Status")) != cgi_header_fields.end()) {
    // TODO: validate status code
    Log::cdebug() << "Status found: " << it->second << std::endl;
    std::stringstream ss(it->second);
    std::string status_code, reason_phrase;
    ss >> status_code;
    ss >> std::ws;
    std::getline(ss, reason_phrase);
    if (!is_valid_status_code(status_code) || ss.fail()) {
      Log::cwarn() << "Invalid CGI status code: " << status_code << std::endl;
      ErrorHandler::handle(*this, 500);
      handler = &Connection::response;
      return WSV_AGAIN;
    }
    ss.str(status_code);
    ss.clear();
    ss >> res.status_code;
  } else {
    Log::cdebug() << "Status not found" << std::endl;
    res.status_code = 200;
  }

  // 2. Location
  /*
   The Location header field is used to specify to the server that the
   script is returning a reference to a document rather than an actual
   document (see sections 6.2.3 and 6.2.4).  It is either an absolute
   URI (optionally with a fragment identifier), indicating that the
   client is to fetch the referenced document, or a local URI path
   (optionally with a query string), indicating that the server is to
   fetch the referenced document and return it to the client as the
   response.
      Location        = local-Location | client-Location
      client-Location = "Location:" fragment-URI NL
      local-Location  = "Location:" local-pathquery NL
      fragment-URI    = absoluteURI [ "#" fragment ]
      fragment        = *uric
      local-pathquery = abs-path [ "?" query-string ]
      abs-path        = "/" path-segments
      path-segments   = segment *( "/" segment )
      segment         = *pchar
      pchar           = unreserved | escaped | extra
      extra           = ":" | "@" | "&" | "=" | "+" | "$" | ","
  */
  if ((it = cgi_header_fields.find("Location")) != cgi_header_fields.end()) {
    Log::cdebug() << "Location found: " << it->second << std::endl;
    // TODO: validate location
    res.location = it->second;
  }

  // 3. Content-Type
  /*
   If an entity body is returned, the script MUST supply a Content-Type
   field in the response.  If it fails to do so, the server SHOULD NOT
   attempt to determine the correct content type.  The value SHOULD be
   sent unmodified to the client, except for any charset parameter
   changes.
  */
  if ((it = cgi_header_fields.find("Content-Type")) !=
      cgi_header_fields.end()) {
    Log::cdebug() << "Content-Type found: " << it->second << std::endl;
    res.content_type = it->second;
  } else if (cgi_socket->getReadBufSize() > 0) {
    Log::cdebug() << "Content-Type must be found" << std::endl;
    ErrorHandler::handle(*this, 500);
    handler = &Connection::response;
    return WSV_AGAIN;
  }

  // 4. Content-Length
  size_t size = cgi_socket->getReadBufSize();
  res.content_length = size;

  // 5. Protocol-Specific Header Fields
  for (it = cgi_header_fields.begin(); it != cgi_header_fields.end(); ++it) {
    if (it->first == "Status" || it->first == "Location" ||
        it->first == "Content-Type" || it->first == "Content-Length") {
      continue;
    }
    // Check if key is valid HTTP header field name
    for (const char* p = http_header_fields[0]; p; ++p) {
      if (it->first == p) {
        Log::cdebug() << "Valid HTTP Header field: " << it->first << ": "
                      << it->second << std::endl;
        //*client_socket << it->first << ": " << it->second << CRLF;
        // TODO: Add header field to response
        break;
      }
    }
  }

  client_socket->send_response(res);

  // 6. Entity Body (if any)
  if (size > 0) {
    *cgi_socket >> *client_socket;
  }
  handler = &Connection::response;
  return WSV_AGAIN;
}
