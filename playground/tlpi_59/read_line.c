#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
void errExit(const char *msg) {
	printf("%s\n", msg);
	exit(EXIT_FAILURE);
}

ssize_t readline(int fd, void *buffer, size_t n) {
	ssize_t numRead;
	size_t totRead;
	char *buf;
	char ch;

	if ( n <= 0 || buffer == NULL ) {
		errno = EINVAL;
		return -1;
	}
	buf = buffer;
	totRead = 0;
	for (;;) {
		// read data byte per byte
		numRead = read(fd, &ch, 1);
		if (numRead == -1) {
			if (errno == EINTR) {
				continue;
			} else {
				return -1;
			}
		} else if (numRead == 0) {
			if (totRead == 0) // no bytes read; return 0
				return 0;
			else // some bytes read; add '\0'
				break;
		} else {
			if (totRead < n - 1) {
				totRead++;
				*buf++ = ch;
			}
			if (ch == '\n')
				break;
		}
	}
	*buf = '\0';
	return totRead;
}
