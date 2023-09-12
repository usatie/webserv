#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "ErrorHandler.hpp"
#include "GetHandler.hpp"
#include "Header.hpp"
#include "PostHandler.hpp"
#include "CgiHandler.hpp"
#include "SocketBuf.hpp"
#include "webserv.hpp"
#include "Config.hpp"
#include <cassert>
#include <limits.h>
#include <signal.h>

class Connection {
 public:
  // Class private enum
  typedef enum Status {
    REQ_START_LINE,     // CLIENT_RECV
    REQ_HEADER_FIELDS,  // CLIENT_RECV
    REQ_BODY,           // CLIENT_RECV
    HANDLE,             //
    HANDLE_CGI_REQ,     // CGI_SEND
    HANDLE_CGI_RES,     // CGI_RECV
    HANDLE_CGI_PARSE,   //
    RESPONSE,           // CLIENT_SEND
    DONE                // CLIENT_SEND
  } Status;

  typedef enum IOStatus {
    CLIENT_RECV, 
    CLIENT_SEND,
    CGI_RECV,
    CGI_SEND,
    NO_IO
  } IOStatus;

  // Member data
  std::shared_ptr<SocketBuf> client_socket;
  std::shared_ptr<SocketBuf> cgi_socket;
  Header header;
  Status status;
  char *body;
  size_t body_size;
  size_t content_length;
  pid_t cgi_pid;
  const Config& cf;
  const Config::Server* srv_cf;
  const Config::Location* loc_cf;

 public:
  // Constructor/Destructor
  Connection() throw();  // Do not implement this
  Connection(std::shared_ptr<Socket> sock, const Config& cf)
      : client_socket(std::shared_ptr<SocketBuf>(new SocketBuf(sock))),
        cgi_socket(NULL),
        header(),
        status(REQ_START_LINE),
        body(NULL),
        body_size(0),
        content_length(0),
        cgi_pid(-1),
        cf(cf),
        srv_cf(NULL),
        loc_cf(NULL) {}
  ~Connection() throw() {}
  Connection(const Connection &other) throw();  // Do not implement this
  Connection &operator=(
      const Connection &other) throw();  // Do not implement this

  // Accessors
  int get_fd() const throw() { return client_socket->get_fd(); }
  int get_cgifd() const throw() {
    if (cgi_socket == NULL) return -1;
    return cgi_socket->get_fd();
  }
  bool is_done() const throw() { return status == DONE; }

  // Member functions
  // Returns negative value when an exception is thrown from STL containers
  int resume() throw();

  IOStatus getIOStatus() const throw();

  // TODO: make this noexcept
  // https://datatracker.ietf.org/doc/html/rfc2616#section-5.1
  // Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
  int parse_start_line() throw();

  int split_header_field(const std::string &line, std::string &key, std::string &value);

  int parse_header_fields() throw();

  int parse_body() throw();

  int handle() throw();

  int handle_cgi_req() throw();

  int handle_cgi_res() throw();

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
  int handle_cgi_parse() throw();

  int response() throw();
};

#endif
