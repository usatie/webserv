#define _BSD_SOURCE
#include <netdb.h>
#include "is_seqnum.h"
#define BACKLOG 50

int main(int argc, char *argv[]) {
	uint32_t seqNum;
	char reqLenStr[INT_LEN]; /* Length of requested sequence */
	char seqNumStr[INT_LEN]; /* Start of granted sequence */
	struct sockaddr_storage claddr;
	int lfd, cfd, optval, reqLen;
	socklen_t addrlen;
	struct addrinfo hints;
	struct addrinfo *result, *rp;
#define ADDRSTRLEN (NI_MAXHOST + NI_MAXSERV + 10)
	char addrStr[ADDRSTRLEN];
	char host[NI_MAXHOST];
	char service[NI_MAXSERV];

	if (argc > 1 && strcmp(argv[1], "--help") == 0)
		errExit("%s [init-seq-num]\n", argv[0]);

	seqNum = (argc > 1) ? atoi(argv[1]) : 0;

	/* Ignore the SIGPIPE signal, so that we find out about broken connection
	 * errors via a failure from write() with the error EPIPE. */
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
		errExit("signal");

	/* Call getaddrinfo() to obtain a list of addresses that
	 * we can try binding to */
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC; /* Allows IPv4 or IPv6 */
	hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV; /* Wildcard IP address; service name is numeric */

	/* getaddrinfo() returns a list of address structures.
	 * Try each address until we successfully bind(2).
	 * If socket(2) (or bind(2)) fails, we (close the socket
	 * and) try the next address. */
	if (getaddrinfo(NULL, PORT_NUM, &hints, &result) != 0)
		errExit("getaddrinfo");

	/* Walk through returned list until we find an address structure
	 * that can be used to successfully create and bind a socket */
	optval = 1;
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		// lfd stands for listening file descriptor
		lfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (lfd == -1)
			continue; /* On error, try next address */

		/* TCP server should usually set SO_REUSEADDR option on its listening socket */
		if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
			errExit("setsockopt");

		if (bind(lfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break; /* Success */

		/* bind() failed: close this socket and try next address */
		close(lfd);
	}
	if (rp == NULL)
		errExit("Could not bind socket to any address");

	if (listen(lfd, BACKLOG) == -1)
		errExit("listen");
	for (;;) {
		/* Accept a client connection, obtaining client's address */
		addrlen = sizeof(struct sockaddr_storage);
		cfd = accept(lfd, (struct sockaddr *) &claddr, &addrlen);
		if (cfd == -1) {
			fprintf(stderr, "Failed to accept() connection on svfd %d\n", lfd);
			continue;
		}
		if (getnameinfo((struct sockaddr *) &claddr, addrlen, host, NI_MAXHOST, service, NI_MAXSERV, 0) == 0)
			snprintf(addrStr, ADDRSTRLEN, "(%s, %s)", host, service);
		else
			snprintf(addrStr, ADDRSTRLEN, "(?UNKNOWN?)");
		printf("Connection from %s\n", addrStr);

		/* Read client request, send sequence number back */
		if (readline(cfd, reqLenStr, INT_LEN) <= 0) {
			close(cfd);
			continue; /* Failed read; skip request */
		}
		reqLen = atoi(reqLenStr);
		if (reqLen <= 0) { /* Watch for misbehaving clients */
			close(cfd);
			continue; /* Bad request; skip it */
		}

		snprintf(seqNumStr, INT_LEN, "%d\n", seqNum);
		if (write(cfd, &seqNumStr, strlen(seqNumStr)) != (ssize_t)strlen(seqNumStr))
			fprintf(stderr, "Error on write");

		seqNum += reqLen; /* Update sequence number */
		if (close(cfd) == -1)
			fprintf(stderr, "Error on close");
	}
}
