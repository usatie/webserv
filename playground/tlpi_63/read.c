#include <unistd.h>
#include <stdio.h>

int main() {
	char buf[10];
	read(10, buf, 9);
	buf[9] = '\0';
	printf("%s\n", buf);
	int res = write(10, "hello", 5);
	printf("res:%d\n", res);
}
