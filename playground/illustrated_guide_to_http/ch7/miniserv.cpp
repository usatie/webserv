#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
using namespace std;

#include "receiveline.hpp"

//int socket(int family, int type, int protocol);
//int bind(int socket, struct sockaddr *addr, int addrlen);
//int listen(int socket, int backlog);
//int accept(int socket, struct sockaddr *addr, int *addrlen);

void  TalkToClient(int iSocket);

int  main(int argc, char *argv[])
{
  int s,    // socket
      rc,   // return code
      c,    // client socket
      len;  // length of client address data structure
  struct sockaddr_in server, client;

  cout << "SOMAXCONN: " << SOMAXCONN << endl;
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s < 0)
  {
    cerr << "Error! Cannot create socket." << endl ;
    return (1);
  }

  len = sizeof(struct sockaddr_in);
  bzero(&server, len);   // clear the data
  server.sin_family = AF_INET;
  server.sin_port = htons(7777);
  server.sin_addr.s_addr = INADDR_ANY;
  rc = bind(s, (struct sockaddr *)&server, len);
  if (rc < 0)
  {
    cerr << "Error! Bind failed." << endl ;
    return (1);
  }

  rc = listen(s, SOMAXCONN);
  if (rc < 0)
  {
    cerr << "Error! Listen failed." << endl ;
    return (1);
  }

  for ( ; ; )
  {
    bzero(&client, len);
    cout << "server accept() " << endl;
    c = accept(s, (struct sockaddr *)&client, (socklen_t *)&len);
    if ( c < 0 )
    {
      cerr << "Error! Accept failed." << endl ;
      return (1);
    }
    // do some work with new socket c to client
    if (fork() == 0)
    {
      close(s);
      TalkToClient(c);
      return 0;
    }
  }
}

void  TalkToClient(int iSocket)
{
  int   iRc,
        bNotDone;
  char  szBuf[256],
        szOk[] = "OK\r\n",
        szErr[] = "ERR\r\n";
  bNotDone = TRUE;

  while (bNotDone == TRUE)
  {
    cout << "server recv() " << endl;
    iRc = RecvLine(iSocket, szBuf, 256);
    if (iRc < 0)
    {
      cerr << "Error! RecvLine failed." << endl;
      bNotDone = FALSE;
    }
    if (strcmp(szBuf, "HELLO") == 0)
    {
      cout << "server got HELLO, send() " << endl;
      iRc = send(iSocket, szOk, strlen(szOk), 0);
      if (iRc < 0)
      {
        cerr << "Error! Send failed." << endl;
        bNotDone = FALSE;
      }
    }
    else if (strcmp(szBuf, "GOODBYE") == 0)
    {
      cout << "server got GOODBYE, send() " << endl;
      iRc = send(iSocket, szOk, strlen(szOk), 0);
      if (iRc < 0)
      {
        cerr << "Error! Send failed." << endl;
        bNotDone = FALSE;
      }
      bNotDone = FALSE; // close connection on GOODBYE
    }
    else // Unknown message
    {
      cout << "server got unknown message, send() " << endl;
      send(iSocket, szErr, strlen(szErr), 0);
      bNotDone = FALSE;
    }
  }
  close(iSocket);
}
