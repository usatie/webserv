CC = cc
CFLAGS = -Iinclude
DEPS = HTTP_Server.h
exec = server.o
sources = $(wildcard src/*.c)
objects = $(sources:.c=.o)
flags = -g -Wall -lm -ldl -fPIC -rdynamic -I./include
# flags = -I./include

$(exec): $(objects)
	$(CC) -o $@ $^ $(flags)

%.o: %.c %.h
	$(CC) -c -o $@ $< $(flags)

clean:
	rm -f $(objects) $(exec)
