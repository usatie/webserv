#ifndef READ_LINE_H
#define READ_LINE_H
void errExit(const char *msg, ...);
ssize_t readline(int fd, void *buffer, size_t n) ;
#endif
