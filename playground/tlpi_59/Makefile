all: test clean

i6d_server: i6d_ucase_sv.c
	$(CC) $(CFLAGS) -o $@ $<

i6d_client: i6d_ucase_cl.c
	$(CC) $(CFLAGS) -o $@ $<

test: test_i6d

test_i6d: i6d_server i6d_client
	./i6d_server &
	./i6d_client ::1 ciao hello world
	./i6d_client ::1 'long message is being sent'
	kill -9 `ps -f | grep i6d_server | grep -v grep | awk '{print $$2}'`
