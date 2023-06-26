#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
  int s,    // the socket handle
      rc;   // return code
  struct  sockaddr_in server;

  if (argc != 2)
  {
    cerr << "Error! Incorrect number of arguments." << endl;
    return 1;
  }

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0)
  {
    cerr << "Error! Cannot create socket." << endl;
    return 1;
  }

  bzero(&server, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_port = htons(7777);
  server.sin_addr.s_addr = inet_addr(argv[1]);

  rc = connect(s, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
  if (rc < 0)
  {
    cerr << "Error! Connect failed."  << endl;
    return 1;
  }

  // do some processing of data between client and server
  close(s);
}
