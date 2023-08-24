#ifndef ERROR_PAGE_HPP
#define ERROR_PAGE_HPP

#include "webserv.hpp"

#define ERROR_TAIL "<hr><center>" WEBSERV_VER "</center>" CRLF \
"</body>" CRLF \
"</html>" CRLF

static const char http_error_400_page[] = 
"<html>" CRLF
"<head><title>400 Bad Request</title></head>" CRLF
"<body>" CRLF
"<center><h1>400 Bad Request</h1></center>" CRLF
ERROR_TAIL ;

static const char http_error_404_page[] = 
"<html>" CRLF
"<head><title>404 Not Found</title></head>" CRLF
"<body>" CRLF
"<center><h1>404 Not Found</h1></center>" CRLF
ERROR_TAIL ;

static const char http_error_405_page[] = 
"<html>" CRLF
"<head><title>405 Not Allowed</title></head>" CRLF
"<body>" CRLF
"<center><h1>405 Not Allowed</h1></center>" CRLF
ERROR_TAIL ;

static const char http_error_500_page[] = 
"<html>" CRLF
"<head><title>500 Internal Server Error</title></head>" CRLF
"<body>" CRLF
"<center><h1>500 Internal Server Error</h1></center>" CRLF
ERROR_TAIL ;

#endif
