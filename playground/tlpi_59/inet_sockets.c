#define _BSD_SOURCE	/* To get NI_MAXHOST and NI_MAXSERV 
					   definitions from <netdb.h> */

#include <sys/socket.h>	/* To get defintion of sockaddr{} */
#include <netinet/in.h> /* To get defintion of sockaddr_in{} */
#include <arpa/inet.h>	/* To get defintion of inet(3) functions */
#include <netdb.h>		/* To get defintion of getaddrinfo() and 
						   getnameinfo() */

#include <stdio.h>	/* To get defintion of snprintf() */
#include <string.h>	/* To get defintion of memset() */
#include <unistd.h>	/* To get defintion of close() */
#include <errno.h>
#include <stdbool.h>
#include "inet_sockets.h"	/* Declares functions defined here */

int inetConnect(const char *host, const char *service, int type) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, s;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_family = AF_UNSPEC;	/* Allows IPv4 or IPv6 */
	hints.ai_socktype = type;

	s = getaddrinfo(host, service, &hints, &result);
	if (s != 0) {
		errno = ENOSYS;
		return -1;
	}

	/* Walk through returned list until we find an address structure
	 * that can be used to successfully connect a socket */
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;	/* On error, try next address */

		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;		/* Success */

		/* Connect failed: close this socket and try next address */
		close(sfd);
	}
	freeaddrinfo(result);

	return (rp == NULL) ? -1 : sfd;
}

static int inetPassiveSocket(const char *service, int type, socklen_t *addrlen, bool doListen, int backlog) {
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int sfd, optval, s;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_canonname = NULL;
	hints.ai_addr = NULL;
	hints.ai_next = NULL;
	hints.ai_socktype = type;
	hints.ai_family = AF_UNSPEC;	/* Allows IPv4 or IPv6 */
	hints.ai_flags = AI_PASSIVE;	/* Use wildcard IP address */

	s = getaddrinfo(NULL, service, &hints, &result);
	if (s != 0)
		return -1;

	/* Walk through returned list until we find an address structure
	 * that can be used to successfully create and bind a socket */
	// optval = 1 means that we can reuse the port
	optval = 1;
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		// why setting SOCK_CLOEXEC ?
		// because we want to close the socket when exec() is called
		//sfd = socket(rp->ai_family, rp->ai_socktype | SOCK_CLOEXEC, rp->ai_protocol);
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sfd == -1)
			continue;	/* On error, try next address */

		// If socket will be used for listening, then we set SO_REUSEADDR
		if (doListen) {
			if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
				close(sfd);
				freeaddrinfo(result);
				return -1;
			}
		}
		// Anyway, we bind the socket
		if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
			break;		/* Success */
		close(sfd);		/* bind() failed: close this socket and try next address */
	}

	// If we are here, then we have tried all the addresses and we have not found a suitable one
	if (rp != NULL && doListen) {
		if (listen(sfd, backlog) == -1) {
			// is it necessary to close the socket ?
			// I think it is not necessary, because we have not returned the socket yet
			// and we are going to return -1
			freeaddrinfo(result);
			return -1;
		}
	}
	if (rp != NULL && addrlen != NULL)
		*addrlen = rp->ai_addrlen;	/* Return address structure size */
	freeaddrinfo(result);
	return (rp == NULL) ? -1 : sfd;
}

// for TCP server
int inetListen(const char *service, int backlog, socklen_t *addrlen) {
	return inetPassiveSocket(service, SOCK_STREAM, addrlen, true, backlog);
}

// for UDP server
int inetBind(const char *service, int type, socklen_t *addrlen) {
	return inetPassiveSocket(service, type, addrlen, false, 0);
}

char *inetAddressStr(const struct sockaddr *addr, socklen_t addrlen, char *addrStr, int addrStrLen) {
	char host[NI_MAXHOST], service[NI_MAXSERV];
	
	if (getnameinfo(addr, addrlen, host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV) == 0)
		snprintf(addrStr, addrStrLen, "(%s, %s)", host, service);
	else
		snprintf(addrStr, addrStrLen, "(?UNKNOWN?)");
	return addrStr;
}
