#include <stdio.h>

int main() {
	printf("Content-type: text/html\n\n");
	printf("Status: 200 OK\n");
	printf("<html><head><title>CGI</title></head><body>");
	printf("<h1>CGI</h1>");
	printf("<p>CGI is a standard for interfacing external applications with information servers, such as HTTP or web servers.</p>");
	printf("</body></html>");
	return 0;
}
