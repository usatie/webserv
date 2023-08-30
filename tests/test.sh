#!/bin/bash

WEBSERV_PORT=8181

# 1. Initialize
# disown for not make the shell script failure when the server is pkilled
./webserv & disown
# wait for server warm up
sleep 1.0

# 2. Tests
for i in {1..6}; do
  echo -n "Test${i}   : "
  nc localhost $WEBSERV_PORT <tests/requests/$i >out
  diff tests/responses/$i out && echo "OK" || echo "NG"
done

# 3. Clean up
rm -f out
pkill webserv
