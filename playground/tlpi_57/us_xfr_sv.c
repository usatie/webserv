#include "us_xfr.h"
#define BACKLOG 5

// make this varg so that we can pass in errno
void errExit(const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	perror(" ");
	exit(EXIT_FAILURE);
}

int main() {
	struct sockaddr_un addr;
	int sfd, cfd;
	ssize_t numRead;
	char buf[BUF_SIZE];

	sfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sfd == -1)
		errExit("socket");

	/*
	 * 1. Construct server socket address
	 * 2. Bind socket to it
	 * 3. Make this a listening socket
	 */
	if (strlen(SV_SOCK_PATH) > sizeof(addr.sun_path) - 1)
		errExit("Server socket path too long");

	// remove any existing file with this pathname if it exist
	if (remove(SV_SOCK_PATH) == -1 && errno != ENOENT)
		errExit("remove-%s", SV_SOCK_PATH);

	// clear addr
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SV_SOCK_PATH, sizeof(addr.sun_path) - 1);

	// bind socket to addr
	if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1)
		errExit("bind");

	// make this a listening socket
	if (listen(sfd, BACKLOG) == -1)
		errExit("listen");

	for (;;) { /* Handle client connections iteratively */
		/* Accept a connection The connection is returned on a new
		 * socket, 'cfd'; the listening socket ('sfd') remains open
		 * and can be used to accept further connections. */
		cfd = accept(sfd, NULL, NULL);
		if (cfd == -1)
			errExit("accept");

		/* Transfer data from connected socket to stdout until EOF */
		while ((numRead = read(cfd, buf, BUF_SIZE)) > 0)
			if (write(STDOUT_FILENO, buf, numRead) != numRead)
				errExit("partial/failed write");
		if (numRead == -1)
			errExit("read");
		if (close(cfd) == -1)
			errExit("close");
	}
}
