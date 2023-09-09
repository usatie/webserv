#!/bin/bash

WEBSERV_PORT=8181

# 1. Initialize
# disown for not make the shell script failure when the server is pkilled
./webserv & disown
# wait for server warm up
sleep 1.0
rm -f error.log

# 2. Tests
# arg for save error count
cnt=0
for i in {1..14}; do
  echo -n "Test${i}   : " | tee -a error.log
  echo "" >>error.log
  nc localhost $WEBSERV_PORT <tests/requests/$i >out
  # Count if any error for later use
  ok=0
  diff tests/responses/$i out >>error.log || let ok++
  if [ $ok -eq 0 ]; then
	echo "OK"
	# Trim the last line of the error.log
	sed -i '' '$ d' error.log
  else
	echo "NG"
	let cnt++
  fi
done

python3 tests/python/mock_server.py
exit_code=$?
if [ $exit_code -ne 0 ]; then
  echo "Python tests failed..."
  exit 1
fi

# 3. Clean up
rm -f out *.tmp
pkill webserv
if [ $cnt -eq 0 ]; then
  echo "All tests passed!"
else
  echo "Some tests failed..."
  exit 1
fi
