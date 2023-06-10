#include <sys/un.h> // struct sockaddr_un
#include <sys/socket.h> // socket()
#include <stdio.h> // perror()
#include <stdlib.h> // exit()
#include <string.h> // memset(), strncpy()

void errExit(const char *msg) {
	perror(msg);
	exit(EXIT_FAILURE);
}

int main() {
	const char *SOCKNAME = "/tmp/mysock";
	int sfd ;
	struct sockaddr_un addr;

	sfd = socket(AF_UNIX, SOCK_STREAM, 0);/* Create server socket */
	if (sfd == -1)
		errExit("socket");

	memset(&addr, 0, sizeof(struct sockaddr_un)); /* Clear structure */
	addr.sun_family = AF_UNIX; /* UNIX domain address */
	strncpy(addr.sun_path, SOCKNAME, sizeof(addr.sun_path) - 1);
	if (bind(sfd, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) == -1)
		errExit("bind");
}
