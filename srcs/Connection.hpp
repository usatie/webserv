#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include <limits.h>
#include <signal.h>

#include <cassert>

#include "CgiHandler.hpp"
#include "Config.hpp"
#include "ErrorHandler.hpp"
#include "GetHandler.hpp"
#include "Header.hpp"
#include "PostHandler.hpp"
#include "RedirectHandler.hpp"
#include "SocketBuf.hpp"
#include "webserv.hpp"

#define TIMEOUT_SEC 10
#define CGI_TIMEOUT_SEC 3

class Connection {
 public:
  // Class private enum
  typedef enum Status {
    REQ_START_LINE,                  // CLIENT_RECV
    REQ_HEADER_FIELDS,               // CLIENT_RECV
    REQ_BODY,                        //
    REQ_BODY_CONTENT_LENGTH,         // CLIENT_RECV
    REQ_BODY_CHUNKED,                // CLIENT_RECV
    REQ_BODY_CHUNK_DATA,             // CLIENT_RECV
    REQ_BODY_CHUNK_TRAILER_SECTION,  // CLIENT_RECV
    HANDLE,                          //
    HANDLE_CGI_REQ,                  // CGI_SEND
    HANDLE_CGI_RES,                  // CGI_RECV
    HANDLE_CGI_PARSE,                //
    RESPONSE,                        // CLIENT_SEND
    DONE,                            //
    CLEAR                            //
  } Status;

  typedef enum IOStatus {
    CLIENT_RECV,
    CLIENT_SEND,
    CGI_RECV,
    CGI_SEND,
    NO_IO
  } IOStatus;

  // Member data
  util::shared_ptr<SocketBuf> client_socket;
  util::shared_ptr<SocketBuf> cgi_socket;
  Header header;
  Status status;
  std::string body;
  size_t content_length;
  std::string chunk;  // single chunk
  size_t chunk_size;
  pid_t cgi_pid;
  const config::Config &cf;
  const config::Server *srv_cf;
  const config::Location *loc_cf;
  const config::CgiHandler *cgi_handler_cf;
  const config::CgiExtensions *cgi_ext_cf;
  time_t last_modified;
  time_t cgi_started;

 public:
  IOStatus io_status;

 public:
  // Constructor/Destructor
  Connection() throw();  // Do not implement this
  Connection(util::shared_ptr<Socket> sock, const config::Config &cf)
      : client_socket(util::shared_ptr<SocketBuf>(new SocketBuf(sock))),
        cgi_socket(NULL),
        header(),
        status(REQ_START_LINE),
        body(),
        content_length(0),
        chunk(),
        chunk_size(0),
        cgi_pid(-1),
        cf(cf),
        srv_cf(NULL),
        loc_cf(NULL),
        cgi_handler_cf(NULL),
        cgi_ext_cf(NULL),
        last_modified(time(NULL)),
        cgi_started(0),  // What should be the initial value?
        io_status(NO_IO) {}
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
  bool is_remove() const throw() { return status == DONE; }
  bool is_clear() const throw() { return status == CLEAR; }
  bool is_timeout() const throw();
  bool is_cgi_timeout() const throw();
  int kill_and_reap_cgi_process() throw();
  void handle_cgi_timeout() throw();
  // Member functions
  // Returns negative value when an exception is thrown from STL containers
  int resume();
  int clear();

 private:
  // TODO: make this noexcept
  // https://datatracker.ietf.org/doc/html/rfc2616#section-5.1
  // Request-Line   = Method SP Request-URI SP HTTP-Version CRLF
  int parse_start_line();  // throwable

  int split_header_field(const std::string &line, std::string &key,
                         std::string &value);  // throwable

  int parse_header_fields();  // throwable

  int parse_body();                        // throwable
  int parse_body_chunked();                // throwable
  int parse_body_content_length();         // throwable
  int parse_body_chunk_data();             // throwable
  int parse_body_chunk_trailer_section();  // throwable

  int handle();  // throwable

  int handle_cgi_req() throw();

  int handle_cgi_res() throw();

  // CGI Response syntax
  // (https://datatracker.ietf.org/doc/html/rfc3875#section-6)
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
  int handle_cgi_parse();  // throwable

  int response() throw();
};

#endif
