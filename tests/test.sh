#!/bin/bash

WEBSERV_PORT=8181

# 1. Initialize
# disown for not make the shell script failure when the server is pkilled
./webserv & disown
# wait for server warm up
sleep 1.0

# 2. Tests
echo -n "Test1    : "
nc localhost $WEBSERV_PORT <tests/requests/1 >out
diff tests/responses/1 out && echo "OK" || echo "NG"

echo -n "Test2    : "
nc localhost $WEBSERV_PORT <tests/requests/2 >out
diff tests/responses/2 out && echo "OK" || echo "NG"

# 3. Clean up
rm -f out
pkill webserv
