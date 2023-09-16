#include <stdio.h>

int main() {
	printf("Content-type: text/html\n");
	printf("Status: 200 OK\n\n");
	printf("<html><head><title>CGI</title></head><body>\n");
	printf("<h1>CGI</h1>\n");
	printf("<p>CGI is a standard for interfacing external applications with information servers, such as HTTP or web servers.</p>\n");
	printf("</body></html>\n");
	return 0;
}
