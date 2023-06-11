all: test clean

us_server: us_xfr_sv.c
	$(CC) $(CFLAGS) -o us_server us_xfr_sv.c

ud_server: ud_ucase_sv.c
	$(CC) $(CFLAGS) -o ud_server ud_ucase_sv.c

us_client: us_xfr_cl.c
	$(CC) $(CFLAGS) -o us_client us_xfr_cl.c

ud_client: ud_ucase_cl.c
	$(CC) $(CFLAGS) -o ud_client ud_ucase_cl.c

test: test_us test_ud

test_us: us_server us_client
	./us_server > b &
	cat *.c > a
	./us_client < a
	kill -9 `ps -f | grep us_server | grep -v grep | awk '{print $$2}'`
	diff a b

test_ud: ud_server ud_client
	./ud_server &
	./ud_client hello world
	./ud_client 'long message is being sent'
	kill -9 `ps -f | grep ud_server | grep -v grep | awk '{print $$2}'`

clean:
	rm -f a b us_server us_client ud_server ud_client
