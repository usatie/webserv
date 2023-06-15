all: test clean

is_seqnum_sv: is_seqnum_sv.c
	$(CC) $(CFLAGS) -o $@ $<

is_seqnum_cl: is_seqnum_cl.c
	$(CC) $(CFLAGS) -o $@ $<

i6d_server: i6d_ucase_sv.c
	$(CC) $(CFLAGS) -o $@ $<

i6d_client: i6d_ucase_cl.c
	$(CC) $(CFLAGS) -o $@ $<

test: test_i6d test_is_seqnum

test_i6d: i6d_server i6d_client
	./i6d_server &
	./i6d_client ::1 ciao hello world
	./i6d_client ::1 'long message is being sent'
	kill -9 `ps -f | grep i6d_server | grep -v grep | awk '{print $$2}'`

test_is_seqnum: is_seqnum_sv is_seqnum_cl
	./is_seqnum_sv &
	./is_seqnum_cl localhost
	./is_seqnum_cl localhost 10
	./is_seqnum_cl localhost 100
	./is_seqnum_cl localhost
