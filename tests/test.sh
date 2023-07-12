#!/bin/bash -x

NGINX_PORT=8080
WEBSERV_PORT=8181
nc localhost $NGINX_PORT <tests/requests/1 >expected
nc localhost $WEBSERV_PORT <tests/requests/1 >out
diff expected out
