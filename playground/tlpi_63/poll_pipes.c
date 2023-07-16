#include <stdio.h>
#include <sys/poll.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <poll.h>

/* poll_pipes.c

   Example of the use of poll() to monitor multiple file descriptors.

   Usage: poll_pipes num-pipes [num-writes]
                                  def = 1

   Create 'num-pipes' pipes, and perform 'num-writes' writes to
   randomly selected pipes. Then use poll() to inspect the read ends
   of the pipes to see which pipes are readable.
*/

void	err_exit(const char *arg)
{
		fprintf(stderr, "%s\n", arg);
		exit(1);
}

int	main(int argc, char *argv[])
{
	int num_pipes, ready, num_writes;
	struct pollfd	*pollfd;
	int				(*pfds)[2];

	if (argc < 2 || strcmp(argv[1], "--help") == 0) {
		fprintf(stderr, "%s num-pipes [num-writes]\n", argv[0]);
		exit(1);
	}

	num_pipes = atoi(argv[1]);
	num_writes = (argc > 2) ? atoi(argv[2]) : 1;
	pfds = calloc(num_pipes, sizeof(int[2]));
	if (!pfds) {
		err_exit("calloc");
	}
	pollfd = calloc(num_pipes, sizeof(struct pollfd));
	if (!pollfd) {
		err_exit("calloc");
	}

	for (int j = 0; j < num_pipes; j++)
	{
		if (pipe(pfds[j]) == -1)
			err_exit("pipe");
	}

	srandom(time(NULL));
	for (int j = 0; j < num_writes; j++)
	{
		int rand_pipe = random() % num_pipes;
		printf("write to fd %3d (read fd %3d)\n",
			   	pfds[rand_pipe][1], pfds[rand_pipe][0]);
		if (write(pfds[rand_pipe][1], "a", 1) == -1)
			err_exit("write");
	}

	for (int j = 0; j < num_pipes; j++)
	{
		pollfd[j].fd = pfds[j][0];
		pollfd[j].events = POLLIN;
	}

	ready = poll(pollfd, num_pipes, 0);
	if (ready == -1)
		err_exit("poll");
	printf("poll() returned : %d\n", ready);

	for (int j = 0; j < num_pipes; j++)
	{
		if (pollfd[j].revents & POLLIN)
			printf("readable: %3d\n", pollfd[j].fd);
	}
}
