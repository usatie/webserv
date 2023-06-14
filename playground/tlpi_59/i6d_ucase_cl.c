#include "i6d_ucase.h"

// make this varg so that we can pass in errno
void errExit(const char *msg, ...) {
	va_list ap;
	va_start(ap, msg);
	vfprintf(stderr, msg, ap);
	va_end(ap);
	perror(" ");
	exit(EXIT_FAILURE);
}

// ./client ::1 ciao
// ::1 means localhost in ipv6
int main(int argc, char *argv[])
{
	struct sockaddr_in6 svaddr;
	int sfd, j;
	size_t msgLen;
	ssize_t numBytes;
	char resp[BUF_SIZE];

	if (argc < 3 || strcmp(argv[1], "--help") == 0)
		errExit("%s host-address msg...\n", argv[0]);
	sfd = socket(AF_INET6, SOCK_DGRAM, 0); /* Create client socket */
	if (sfd == -1)
		errExit("socket");
	
	memset(&svaddr, 0, sizeof(struct sockaddr_in6));
	svaddr.sin6_family = AF_INET6;
	svaddr.sin6_port = htons(PORT_NUM);
	if (inet_pton(AF_INET6, argv[1], &svaddr.sin6_addr) <= 0)
		errExit("inet_pton failed for address '%s'", argv[1]);

	/* Send messages to server; echo responses on stdout */
	
	for (j = 2; j < argc; j++) {
		msgLen = strlen(argv[j]);
		if (sendto(sfd, argv[j], msgLen, 0, (struct sockaddr *) &svaddr,
					sizeof(struct sockaddr_in6)) != (ssize_t)msgLen)
			errExit("sendto");
		numBytes = recvfrom(sfd, resp, BUF_SIZE, 0, NULL, NULL);
		if (numBytes == -1)
			errExit("recvfrom");
		printf("Response %d: %.*s\n", j - 1, (int) numBytes, resp);
	}
	exit(EXIT_SUCCESS);
}
