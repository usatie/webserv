#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void errExit(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

int main() {
	struct sockaddr_un addr;

	// The Linux Abstract Socket Namespace
	// which is a non-filesystem-based namespace for Unix domain sockets
	// 
	// To create an abstract socket address, we set the first byte of the
	// sun_path[] field to 0, and then copy the socket name (which must be
	// null-terminated) into the remaining bytes of the field.
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	/* addr.sun_path[0] has already been set to 0 by memset() */

	char *str = "xyz";
	strncpy(&addr.sun_path[1], str, strlen(str));
	int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sockfd == -1)
		errExit("socket");
	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(sa_family_t) + strlen(str) + 1) == -1)
		errExit("bind");
}
