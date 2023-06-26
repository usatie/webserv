#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
using namespace std;

#include "receiveline.hpp"

int main(int argc, char *argv[])
{
  int s,                      // the socket handle
      rc;                     // return code
  char  szBuf[256];           // data buffer
  struct  sockaddr_in server; // server address

  if (argc != 2)
  {
    cerr << "Error! Incorrect number of arguments." << endl;
    return 1;
  }

  cout << "client socket() " << endl;
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0)
  {
    cerr << "Error! Cannot create socket." << endl;
    return 1;
  }

  bzero(&server, sizeof(struct sockaddr_in));
  server.sin_family = AF_INET;
  server.sin_port = htons(7777);
  server.sin_addr.s_addr = inet_addr(argv[1]);  // use command line address

  cout << "client connect() " << endl;
  rc = connect(s, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
  if (rc < 0)
  {
    cerr << "Error! Connect failed."  << endl;
    return 1;
  }

  // do some processing of data between client and server
  strcpy(szBuf, "HELLO\r\n");
  cout << "client send(HELLO) " << endl;
  rc = send(s, szBuf, strlen(szBuf), 0);
  if (rc < 0)
  {
    cerr << "Error! Send failed." << endl;
    return 1;
  }
  cout << "client recv() " << endl;
  rc = RecvLine(s, szBuf, 256);
  if (rc < 0)
  {
    cerr << "Error! Recv failed." << endl;
    return 1;
  }

  if (strcmp(szBuf, "OK") != 0)
  {
    cerr << "Error! Unknown reply from server." << endl;
    return 1;
  }

  strcpy(szBuf, "GOODBYE\r\n");
  cout << "client send(GOODBYE) " << endl;
  rc = send(s, szBuf, strlen(szBuf), 0);
  if (rc < 0)
  {
    cerr << "Error! Send failed." << endl;
    return 1;
  }

  cout << "client recv() " << endl;
  rc = RecvLine(s, szBuf, 256);
  if (rc < 0)
  {
    cerr << "Error! Recv failed." << endl;
    return 1;
  }

  if (strcmp(szBuf, "OK") != 0)
  {
    cerr << "Error! Unknown reply from server." << endl;
    return 1;
  }
  cout << "client close() " << endl;
  close(s);
}
