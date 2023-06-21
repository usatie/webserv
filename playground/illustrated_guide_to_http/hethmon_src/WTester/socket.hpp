//
// Socket  File: socket.hpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

//
// Socket Class
//

#ifndef _SOCKET_HPP_
#define _SOCKET_HPP_

#include <sys\types.h>
#include <memory.h>
#include <string.h>
#include <iostream.h>
#include <fstream.h>

#ifdef __OS2__
  #include <os2.h>
  #define OS2        // This is needed for the socket header files
  #include <types.h>
  #include <netdb.h>
  #include <sys\socket.h>
  #include <netinet\in_systm.h>
  #include <netinet\in.h>
  #include <netinet\ip.h>
#elif __WINDOWS__
  #include <windows.h>
  #include <winsock.h>
  #define soclose(x) closesocket(x)
  #define bzero(x, y) memset((x), '\0', (y))
#endif

#include "defines.hpp"

// Define this to the size of the largest ascii line of
// data your application expects to receive.
#define MAX_SOCK_BUFFER  16384
#define NO_EOL 1
#define REUSE_PORT 1

class Socket
{
  public:

  Socket()
    {
      iLen = sizeof(siUs);
      iSock = -1;
      iErr = 0;
      szOutBuf = new char[MAX_SOCK_BUFFER];
      szBuf1 = new char[MAX_SOCK_BUFFER/2];
      szBuf2 = new char[MAX_SOCK_BUFFER/2];
      iBeg1 = iEnd1 = iBeg2 = iEnd2 = 0;
      iBuf = 1;
      szPeerIp = NULL;
      szPeerName = NULL;
    };
  ~Socket()
    {
      if (iSock > -1) soclose(iSock);
      delete [] szOutBuf;
      delete [] szBuf1;
      delete [] szBuf2;
      if (szPeerIp) delete [] szPeerIp;
      if (szPeerName) delete [] szPeerName;
    };

  int Create()                     // Allocate a socket for use
    {
//      char bOpt = 1;
      iSock = socket(AF_INET, SOCK_STREAM, 0);
//      setsockopt(iSock, IPPROTO_TCP, TCP_NODELAY, &bOpt, sizeof(BOOL));
      return iSock;
    };      
  int Passive(short int sPort)     // Turn the socket into a passive socket
    {                              // Do not set SO_REUSEADDR
      return(Passive(sPort, 0));
    }
  int Passive(short int sPort,     // Turn the socket into a passive socket
              int iReuse);         // Allow setting of SO_REUSEADDR
  Socket * Accept();               // Listen for connections
  int Connect(char *szBuf, short sPort);  // Connect the socket to the remote host
  int Recv()                       // Receive bytes on this socket
    {
      return(Recv(MAX_SOCK_BUFFER));
    }
  int Recv(int iBytes);            // Receive up to iBytes on this socket
  int RecvTeol()                   // Receive up to the telnet eol
    {
      return(RecvTeol(0));         // Include the telnet eol
    }
  int RecvTeol(int iToast);        // Receive up to the telnet eol
                                   // and possibly remove the telnet eol
  int Send(char *szBuf, int iLen)  // Send the buffer on this socket
    {
      return send(iSock, szBuf, iLen, 0);
    };
  int Send(char *szBuf)            // Send the text buffer on this socket
    {
      return send(iSock, szBuf, strlen(szBuf), 0);
    };
  int Send(const char *szBuf)      // Send the text buffer on this socket
    {
      return send(iSock, (char *)szBuf, strlen(szBuf), 0);
    };
  int SendText(char *szFileName);  // Send this text file across the socket
  int SendBinary(char *szFileName);// Send this binary file across the socket
  int ResolveName();               // Look up the ip address and name of the peer
  int Close()                      // Close this socket
    {
      iSock = -1;
      iErr = 0;
      iBeg1 = iEnd1 = iBeg2 = iEnd2 = 0;
      iBuf = 1;
      memset(szOutBuf, 0, MAX_SOCK_BUFFER);
      memset(szBuf1, 0, MAX_SOCK_BUFFER/2);
      memset(szBuf2, 0, MAX_SOCK_BUFFER/2);
      if (szPeerIp) delete [] szPeerIp;
      if (szPeerName) delete [] szPeerName;
      szPeerIp = NULL;
      szPeerName = NULL;
      return shutdown(iSock, 2);
    };

  int iSock;                       // The socket number allocated
  int iErr;                        // The last error code on a socket call
  char *szOutBuf;                  // Used to return data in
  char *szPeerName;                // The ip name of the peer connected.
  char *szPeerIp;                  // The ip address of the peer connected.

  protected:

  struct sockaddr_in siUs;   // Our address
  struct sockaddr_in siThem; // Their address
  short int sPortUs;         // Our port
  short int sPortThem;       // Their port
  int iLen;                  // The size of siUs and siThem
  int iBuf;                  // Active buffer flag.
  char *szBuf1, *szBuf2;     // Internal buffers.
  int iBeg1, iEnd1,          // Buffer markers.
      iBeg2, iEnd2;
};

#endif




