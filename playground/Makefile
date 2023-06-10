all: test clean

server:
	$(CC) $(CFLAGS) -o server us_xfr_sv.c

client:
	$(CC) $(CFLAGS) -o client us_xfr_cl.c

test: server client
	./server > b &
	cat *.c > a
	./client < a
	kill -9 `ps -f | grep server | grep -v grep | awk '{print $$2}'`
	diff a b

clean:
	rm -f server client a b
