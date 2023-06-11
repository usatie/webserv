#include "error.h"
#include <stdio.h> // vfprintf()
#include <stdarg.h> // va_list, va_start(), va_end()
#include <stdlib.h> // exit(), EXIT_FAILURE

void errExit(char* msg, ...) {
	va_list args;
	va_start(args, msg);
	vfprintf(stderr, msg, args);
	va_end(args);
	perror("");
	exit(EXIT_FAILURE);
}

