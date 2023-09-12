#!/bin/bash

WEBSERV_PORT=8181

# 1. Initialize
# disown for not make the shell script failure when the server is pkilled
./webserv tests/test.conf & disown
# wait for server warm up
sleep 1.0
rm -f error.log
mkdir -p /tmp/www/pouic/toto && echo "pouet" > /tmp/www/pouic/toto/pouet

# 2. Tests
# arg for save error count
cnt=0
for i in {1..21}; do
  echo -n "Test${i}   : " | tee -a error.log
  echo "" >>error.log
  if [[ "$(uname -s)" == "Linux" ]]; then
  	nc -N localhost $WEBSERV_PORT <tests/requests/$i >out
  else
  	nc localhost $WEBSERV_PORT <tests/requests/$i >out
  fi
  # Count if any error for later use
  err=0
  diff tests/responses/$i out >>error.log || let err++
  if [ $err -eq 0 ]; then
	echo "OK"
	# Trim the last line of the error.log
	sed -i -e '$ d' error.log
  else
	echo "NG"
	let cnt++
  fi
done

function python_test() {
	echo -n "Python tests: " | tee -a error.log
	err=0
	python3 tests/python/test_server_response.py 2>error.log || let err++
	if [ $err -eq 0 ]; then
		echo "OK"
		# Trim the last line of the error.log
		sed -i -e '$ d' error.log
	else
		echo "NG"
		let cnt++
	fi
}

python_test

# 3. Clean up
rm -f out *.tmp error.log-e
pkill webserv
if [ $cnt -eq 0 ]; then
  echo "All tests passed!"
else
  echo "Some tests failed..."
  exit 1
fi
