#include "Response.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char * render_static_file(char * fileName) {
	FILE *file = fopen(fileName, "r");

	if (file == NULL) {
		return NULL;
	} else {
		printf("%s does exist \n", fileName);
	}
	if (fseek(file, 0, SEEK_END) != 0) {
		return NULL;
	}
	// The ftell() function obtains the current value of the file position indicator for the stream pointed to by stream.
	long fsize = ftell(file);
	if (fsize == -1) {
		return NULL;
	}
	// Use fseek instead of rewind because error handling is easier.
	if (fseek(file, 0, SEEK_SET) != 0) {
		return NULL;
	}

	char* buffer = malloc(fsize + 1);
	// fread() reads nmemb items of data, each size bytes long, from the stream pointed to by stream, storing them at the location given by ptr.
	fread(buffer, fsize, 1, file);
	char ch;
	int i = 0;
	while ((ch = fgetc(file)) != EOF) {
		buffer[i] = ch;
		i++;
	}
	fclose(file);
	return buffer;
}
