/*  self_pipe.c

   Employ the self-pipe trick so that we can avoid race conditions while both
   selecting on a set of file descriptors and also waiting for a signal.

   Usage as shown in synopsis below; for example:

        self_pipe - 0
*/
#include <sys/time.h>
#include <sys/select.h>
#include <fcntl.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <sys/param.h>

void usageErr(const char *format, ...)
{
	va_list argList;
	va_start(argList, format);
	vfprintf(stderr, format, argList);
	va_end(argList);
	exit(EXIT_FAILURE);
}

void errExit(const char *format, ...)
{
	va_list argList;
	va_start(argList, format);
	vfprintf(stderr, format, argList);
	va_end(argList);
	exit(EXIT_FAILURE);
}

static int pfd[2];

static void handler(int sig)
{
	int	saved_errno;
	saved_errno = errno;
	if (write(pfd[1],"x", 1)==-1 && errno != EAGAIN)
		errExit("write");
	errno = saved_errno;
}

int	main(int argc, char*argv[])
{
	if (argc < 2)
		usageErr("%s {timeout|-} fd...\n"
				"\t\t('-' means infinite timeout)\n", argv[0]);

	struct timeval timeout;
	struct timeval *pto;
	if (strcmp(argv[1], "-") == 0) {
		pto = NULL;
	} else {
		pto = &timeout;
		timeout.tv_sec = atoi(argv[1]);
        timeout.tv_usec = 0;
	}
	
	int nfds = 0;
	/* Build the 'readfds' from the fd numbers given in command line */
	fd_set	readfds;
	FD_ZERO(&readfds);
	for (int j = 2; j < argc; j++) {
		int fd = atoi(argv[j]);
		if (fd >= FD_SETSIZE)
			errExit("file descriptor exceeds limit (%d)\n", FD_SETSIZE);
		if (fd >= nfds)
			nfds = fd + 1;
		FD_SET(fd, &readfds);
	}

	/* Create pipe before establishing signal handler to prevent race */
	if (pipe(pfd) == -1)
		errExit("pipe");
	FD_SET(pfd[0], &readfds);
	nfds = MAX(nfds, pfd[0]+1);

	/* Make read and write ends of pipe nonblocking */
	int	flags;
	flags = fcntl(pfd[0], F_GETFL);
	if (flags == -1)
		errExit("fcntl(F_GETFL)");
	flags |= O_NONBLOCK; /* Make read end nonblocking */
	if (fcntl(pfd[0], F_SETFL, flags) == -1)
		errExit("fcntl(F_SETFL)");

	flags = fcntl(pfd[1], F_GETFL);
	if (flags == -1)
		errExit("fcntl(F_GETFL)");
	flags |= O_NONBLOCK; /* Make write end nonblocking */
	if (fcntl(pfd[1], F_SETFL, flags) == -1)
		errExit("fcntl(F_SETFL)");

	struct sigaction	sa;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART; /* Restart interrupted read()s */
	sa.sa_handler = handler;
	if (sigaction(SIGINT, &sa, NULL) == -1)
		errExit("sigaction");

	int	ready;
	while ((ready = select(nfds, &readfds, NULL, NULL, pto)) == -1 && errno == EINTR)
		continue;
	if (ready == -1)
		errExit("select");

	if (FD_ISSET(pfd[0], &readfds)) { /* Handler was called */
		printf("A signal was caught\n");
		for (;;) {
			char ch;
			if (read(pfd[0], &ch, 1) == -1) {
				if (errno == EAGAIN)
					break;
				else	
					errExit("read");
			}
	/* Perform any actions that should be taken in response to signal */
		}
	}

/* Examine file descriptor sets returned by select() to see which other file descriptors are ready */
	printf("ready = %d\n", ready);
	for (int j = 2; j < argc; j++) {
		int fd = atoi(argv[j]);
		printf(" %d: %s\n", fd, (FD_ISSET(fd, &readfds) ? "r":""));
	}
	printf(" %d: %s (read end of pipe)\n", pfd[0], (FD_ISSET(pfd[0], &readfds) ? "r":""));
	if (pto)
		printf("timeout after select(): %ld.%03ld\n", (long) timeout.tv_sec, (long) timeout.tv_usec / 1000);
}
