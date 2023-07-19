#include <signal.h>
#include <ctype.h>
#include <fcntl.h>
#include <termios.h>
#include <stdbool.h>
#include <unistd.h>
//#include "tty_functions.h"
//#include "tlpi_hdr.h"

static volatile sig_atomic_t gotSigio = 0;

static void sigioHandler(int sig)
{
	(void)sig;
	gotSigio = 1;
}

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
void errExit(const char *format, ...)
{
	va_list argList;
	va_start(argList, format);
	vfprintf(stderr, format, argList);
	va_end(argList);
	exit(EXIT_FAILURE);
}

int
ttySetCbreak(int fd, struct termios *prevTermios)
{
    struct termios t;

    if (tcgetattr(fd, &t) == -1)
        return -1;

    if (prevTermios != NULL)
        *prevTermios = t;

    t.c_lflag &= ~(ICANON | ECHO);
    t.c_lflag |= ISIG;

    t.c_iflag &= ~ICRNL;

    t.c_cc[VMIN] = 1;                   /* Character-at-a-time input */
    t.c_cc[VTIME] = 0;                  /* with blocking */

    if (tcsetattr(fd, TCSAFLUSH, &t) == -1)
        return -1;

    return 0;
}

int main(int argc, char *argv[])
{
	(void)argc, (void)argv;
	int flags, j, cnt;
	struct termios origTermios;
	char ch;
	struct sigaction sa;
	bool done;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	sa.sa_handler = sigioHandler;
	if (sigaction(SIGIO, &sa, NULL) == -1)
		errExit("sigaction");
	if (fcntl(STDIN_FILENO, F_SETOWN, getpid()) == -1)
		errExit("fcntl(F_SETOWN)");

	/* Enable "I/O possible" signaling and make I/O nonblocking
	   for file descriptor */

	flags = fcntl(STDIN_FILENO, F_GETFL);
	if (fcntl(STDIN_FILENO, F_SETFL, flags | O_ASYNC | O_NONBLOCK) == -1)
		errExit("fcntl(F_SETFL)");

	// Set tty cbreak using tcsetattr
	if (ttySetCbreak(STDIN_FILENO, &origTermios) == -1)
		errExit("ttySetCbreak");

	for (done = false, cnt = 0; !done; cnt++) {
		for (j = 0; j < 100000000; j++)
			continue; /* Slow main loop down a little */

		if (gotSigio) {
			gotSigio = 0;
			/* Read all available input until error (probably EAGAIN)
			 * or EOF (not actually possible in cbreak mode) or a 
			 * hash (#) character is read*/
			while (read(STDIN_FILENO, &ch, 1) > 0 && !done) {
				printf("cnt=%d; read %c\n", cnt, ch);
				done = ch == '#';
			}
		}
	}

	/* Restore original terminal settings */
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &origTermios) == -1)
		errExit("tcsetattr");
	exit(EXIT_SUCCESS);
}
