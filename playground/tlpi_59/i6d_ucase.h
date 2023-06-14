#ifndef I6D_UCASE_H
#define I6D_UCASE_H

#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <ctype.h>

# include <stdio.h> // perror()
# include <stdlib.h> // exit()
# include <string.h> // memset(), strncpy()
# include <errno.h>
# include <stdarg.h> // va_list, va_start(), va_end()
# include <unistd.h> // read(), write()

#define BUF_SIZE 10		/* Maximum size of messages exchanged between client and server */
#define PORT_NUM 50002	/* Serber port number */

#endif
