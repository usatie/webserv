#!/bin/bash
WEBSERV_PORT=8181

# 1. Initialize
# disown for not make the shell script failure when the server is pkilled
./webserv tests/test.conf >/dev/null 2>/dev/null & disown
# wait for server warm up
sleep 1.0

# 2. Tests
siege --concurrent=128 --time=60s http://localhost:$WEBSERV_PORT/kapouet/pouic/toto/pouet -b

# 3. Clean up
pkill webserv
