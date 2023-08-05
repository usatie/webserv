#!/bin/bash

WEBSERV_PORT=8181

# 1. Initialize
# limit memory and test that webserver is not stopped by handling bad alloc error
sudo bash -c 'ulimit -v 6000 && (./webserv &)'
# wait for server warm up
sleep 0.5

echo -n "Server(before test): "
ps aux | grep webserv | grep -v grep >/dev/null && echo "OK(running)" || echo "NG(died)"

# 2. Test
yes 'a' | head -c 9000000 >tests/static/bigfile # Create a 9MB file
nc localhost $WEBSERV_PORT <tests/requests/badalloc >out # Request the bigfile
# diff is not used because the response is not defined
# echo -n "Response: "
# grep "500 Internal Server Error" out && echo "OK" || echo "NG"
# test if the webserver process is still runninng
echo -n "Server(after test): "
ps aux | grep webserv | grep -v grep && echo "OK(running)" || echo "NG(died)"

# 3. Clean up
rm -f out
pkill webserv
