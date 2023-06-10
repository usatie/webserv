#ifndef UD_UCASE_H
# define UD_UCASE_H

# include <sys/un.h> // struct sockaddr_un
# include <sys/socket.h> // socket(), bind(), listen(), accept(), connect()
# include <ctype.h> // toupper()

# include <stdio.h> // perror()
# include <stdlib.h> // exit()
# include <string.h> // memset(), strncpy()
# include <errno.h>
# include <stdarg.h> // va_list, va_start(), va_end()
# include <unistd.h> // read(), write()

# define SV_SOCK_PATH "/tmp/ud_case"

# define BUF_SIZE 10

#endif
