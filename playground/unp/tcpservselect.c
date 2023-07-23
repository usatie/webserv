#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define MAXLINE 1024 
#define SERV_PORT 8181
#define SA struct sockaddr
#define LISTENQ 5

int Socket(int domain, int type, int protocol) {
	int fd;
	if ((fd = socket(domain, type, protocol)) < 0) {
		fprintf(stderr, "socket() failed");
		exit(1);
	}
	return fd;
}

void Bind(int socket, const struct sockaddr *address, socklen_t address_len) {
	if (bind(socket, address, address_len) < 0) {
		fprintf(stderr, "bind() failed");
		exit(1);
	}
}

void Listen(int socket, int backlog) {
	if (listen(socket, backlog) < 0) {
		fprintf(stderr, "listen() failed");
		exit(1);
	}
}

int Select(int nfds, fd_set *restrict readfds, fd_set *restrict writefds,
         fd_set *restrict errorfds, struct timeval *restrict timeout) {
	int nready;
	if ((nready = select(nfds, readfds, writefds, errorfds, timeout)) < 0) {
		fprintf(stderr, "select() failed");
		exit(1);
	}
	return nready;
};

int Accept(int socket, struct sockaddr *restrict address,
         socklen_t *restrict address_len) {
	int fd;
	if ((fd = accept(socket, (SA *) &address, &address_len)) < 0) {
		fprintf(stderr, "accept() failed");
		exit(1);
	}
	return fd;
}

ssize_t Read(int fildes, void *buf, size_t nbyte) {
	ssize_t size;
	if ((size = read(fildes, buf, nbyte)) < 0) {
		fprintf(stderr, "read() failed");
		exit(1);
	}
	return size;
}

void Close(int fildes) {
	if (close(fildes) < 0) {
		fprintf(stderr, "close() failed");
		exit(1);
	}
};

void err_quit(char *error_msg) {
	fprintf(stderr, "%s", error_msg);
	exit(1);
}

int Writen(int fd, char *buf, size_t nbyte) {
	size_t nleft;
	ssize_t nread;
	char *ptr;

	ptr = buf;
	nleft = nbyte;
	while(nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;
			else
				return -1;
		} else if (nread == 0)
			break;
		nleft -= nread;
		ptr += nread;
	}
	return (nbyte - nleft);
};

int main(int argc, char **argv) {
	int i, maxi, maxfd,  listenfd, connfd, sockfd;
	int nready, client[FD_SETSIZE];
	ssize_t n;
	fd_set rset, allset;
	char buf[MAXLINE];
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);

	maxfd = listenfd;
	maxi = -1;
	for (i = 0; i < FD_SETSIZE; i++) {
		client[i] = -1;
	}
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);

	for (;;) {
		rset = allset;
		nready = Select(maxfd+1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(listenfd, &rset)) {
			clilen = sizeof(cliaddr);
			connfd = Accept(listen, (SA *) &cliaddr, &clilen);

			for (i = 0; i < FD_SETSIZE; i++) {
				if (client[i] < 0) {
					client[i] = connfd;
					break;
				}
			}
			if (i == FD_SETSIZE) {
				err_quit("too many clients");
			}

			FD_SET(connfd, &allset);
			if (connfd > maxfd)
				maxfd = connfd;
			if (i > maxi)
				maxi = i;
			
			if (--nready <= 0)
				continue;
		}
		
		for (i = 0; i <= maxi; i++) {
			if ((sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if ((n = Read(sockfd, buf, MAXLINE)) == 0) {
					Close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				} else {
					Writen(sockfd, buf, n);
				}

				if (--nready <= 0)
					break;				
			}
		}
	}
}
