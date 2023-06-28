#include "socket.hpp"

Socket::Socket()
{
    iLen = sizeof(siUs);
    iSock = -1;
    iErr = 0;
    szOutBuf = new char[MAX_SOCK_BUFFER];
    szBuf1 = new char[MAX_SOCK_BUFFER/2];
    szBuf2 = new char[MAX_SOCK_BUFFER/2];
    iBeg1 = iEnd1 = iBeg2 = iEnd2 = 0;
    iBuf = 1;
    szPeerName = NULL;
    szPeerIp = NULL;
    ulTimeout = 5 * 60; // 5 minutes default.
}

Socket::~Socket()
{
    if (iSock > -1) close(iSock);
    delete [] szOutBuf;
    delete [] szBuf1;
    delete [] szBuf2;
    if (szPeerName) delete [] szPeerName;
    if (szPeerIp) delete [] szPeerIp;
}

// Allocate a socket for use
// Returns the socket number or -1 on error.
int Socket::Create()
{
    iSock = socket(AF_INET, SOCK_STREAM, 0);
    return iSock;
}

int Socket::Passive(short int sPort)
{
  return (Passive(sPort, 0));
}

int Socket::Passive(short int sPort, int iReuse)
{
  int optval = 1;
  if (iReuse > 0)
  {
    setsockopt(iSock, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(int));
  }
  sPortUs = sPort;

  bzero((void *)&siUs, iLen);
  siUs.sin_family = AF_INET;
  siUs.sin_port = htons(sPortUs);
  siUs.sin_addr.s_addr = INADDR_ANY;

  // Bind to the given port
  iErr = bind(iSock, (struct sockaddr *) &siUs, iLen);
  if (iErr < 0)
  {
    return iErr;
  }
  // change to passive socket
  iErr = listen(iSock, SOMAXCONN);
  if (iErr < 0)
  {
    return iErr;
  }
  return 0;
}

Socket *Socket::Accept()
{
  Socket *sSock;

  sSock = new Socket();
  bzero(&siThem, iLen);
  sSock->iSock = accept(iSock, (struct sockaddr *)&(sSock->siThem), &iLen);
  if (sSock->iSock < 0)
  {
    iErr = sSock->iSock;
    delete sSock;
    return NULL;
  }
  sSock->szPeerIp = new char[128];
  strncpy(sSock->szPeerIp, inet_ntoa(sSock->siThem.sin_addr), 128);
  return sSock;
}

int Socket::Connect(char *szBuf, short sPort)
{
  struct hostent *heHost;

  heHost = gethostbyname(szBuf);
  if (heHost == NULL)
  {
    return (iErr = 1);
  }

  bzero((void *)&siUs, iLen);
  siUs.sin_family = AF_INET;
  siUs.sin_port = htons(sPort);
  siUs.sin_addr.s_addr = *((u_long *)heHost->h_addr); // h_addr is (char *)
                                                      // so it needs to be converted to 
                                                      // (u_long *) before dereference.
  iErr = connect(iSock, (sockaddr *)&siUs, iLen);
  return (iErr);
}

int Socket::Recv()
{
  return (Recv(MAX_SOCK_BUFFER));
}

int Socket::RecvTeol()
{
  return (RecvTeol(0));
}

int Socket::Recv(int iBytes)
{
  fd_set fdsSocks;
  struct timeval stTimeout;
  
  memset(szOutBuf, 0, MAX_SOCK_BUFFER);
  
  if ((iBuf == 1) && (iEnd1 != 0)) // Copy the contents of buf 1.
  {
    if (iBytes >= (iEnd1 - iBeg1)) // Copy all the bytes.
    {
      memcpy(szOutBuf, szBuf1 + iBeg1, iEnd1 - iBeg1);
      iErr = iEnd1 - iBeg1;
      iBeg1 = iEnd1 = 0;
      iBuf = 2;
    }
    else // Only copy the requested number.
    {
      memcpy(szOutBuf, szBuf1 + iBeg1, iBytes);
      iErr = iBytes;    // This many bytes sent back.
      iBeg1 += iBytes;  // Advance to this location.
    }
  }
  else if ((iBuf == 2) && (iEnd2 != 0))
  {
    if (iBytes >= (iEnd2 - iBeg2))
    {
      memcpy(szOutBuf, szBuf2 + iBeg2, iEnd2 - iBeg2);
      iErr = iEnd2 - iBeg2;
      iBeg2 = iEnd2 = 0;
      iBuf = 1;
    }
    else
    {
      memcpy(szOutBuf, szBuf2 + iBeg2, iBytes);
      iErr = iBytes;
      iBeg2 += iBytes;
    }
  }
  else
  {
    FD_ZERO(&fdsSocks);
    FD_SET(iSock, &fdsSocks);
    stTimeout.tv_sec = ulTimeout;
    stTimeout.tv_usec = 0;
    iErr = select(iSock + 1, &fdsSocks, NULL, NULL, &stTimeout);
    if (iErr < 0) // Error occured
    {
      return -1;
    }
    else if (iErr == 0) // Timeout
    {
      return -1;
    }
    iErr = recv(iSock, szOutBuf, iBytes, 0);
    if (iErr == 0) return -1; // Connection closed.
  }
  return iErr;
}

int Socket::RecvTeol(int iToast)
{
  int i;
  int iState = 1,
      idx = 0;
  fd_set fdsSocks;
  struct timeval stTimeout;

  memset(szOutBuf, 0, MAX_SOCK_BUFFER);

  while ( iState != 0 )
  {
    switch (iState)
    {
      case 1:   // Figure out where to start
        {
          if ((iEnd1 == 0) && (iEnd2 == 0))   // Both buffers empty.
          {
            iState = 2;
          }
          else
          {
            iState = 3;
          }
          break;
        }
      case 2:   // Fill the buffers with data.
        {
          FD_ZERO(&fdsSocks);
          FD_SET(iSock, &fdsSocks);
          stTimeout.tv_sec = ulTimeout;
          stTimeout.tv_usec = 0;
          iErr = select(iSock + 1, &fdsSocks, NULL, NULL, &stTimeout);
          if (iErr < 0) // Error occured
          {
            return -1;
          }
          else if (iErr == 0) // Timeout
          {
            return -1;
          }
          iErr = recv(iSock, szBuf1, MAX_SOCK_BUFFER/2, 0);
          if (iErr == -1)   // Error receiving data.
          {
            iState = 0;
            break;
          }
          iBeg1 = 0;
          iEnd1 = iErr;
          if (iErr == MAX_SOCK_BUFFER/2)    // Filled up Buffer 1.
          {
            FD_ZERO(&fdsSocks);
            FD_SET(iSock, &fdsSocks);
            stTimeout.tv_sec = ulTimeout;
            stTimeout.tv_usec = 0;
            iErr = select(iSock + 1, &fdsSocks, NULL, NULL, &stTimeout);
            // TODO: Check for connection closed.
            if (iErr < 0) // Error occured
            {
              return -1;
            }
            else if (iErr == 0) // Timeout
            {
              return -1;
            }
            iErr = recv(iSock, szBuf2, MAX_SOCK_BUFFER/2, 0);
            if (iErr == -1)   // Error receiving data.
            {
              iState = 0;
              break;
            }
            iBeg2 = 0;
            iEnd2 = iErr;
          }
        }
      case 3:   // Look for the EOL sequence
        {
          if ((iBuf == 1) && (iEnd1 != 0))      // Use Buffer 1 first.
          {
            for ( ; iBeg1 < iEnd1; iBeg1++)
            {
              szOutBuf[idx] = szBuf1[iBeg1];    // Copy
              if ((szOutBuf[idx] == '\n') || (szOutBuf[idx-1] == '\r'))
              {
                iBeg1++;    // Count the char just read.
                if ((szOutBuf[idx] == '\n') && (szOutBuf[idx-1] == '\r'))
                {
                  // Using CRLF as end-of-line.
                  idx++;
                  szOutBuf[idx] = szBuf1[iBeg1];
                  iBeg1++;  // Consume LF.
                }
                szOutBuf[idx + 1] = '\0'; // True. Null Line
                iState = 4;               // Goto cleanup & exit
                break;                    // Break from for loop.
              }
              idx++;                      // Advance to next spot.
              if ((idx + 1) == MAX_SOCK_BUFFER) // Out of room.
              {
                szOutBuf[MAX_SOCK_BUFFER - 1] = '\0'; // Null terminate.
                iState = 4;                           // Goto cleanup & exit
                break;                                // Break from for loop.
              }
            } // End of for loop
            if (iBeg1 == iEnd1) iBeg1 = iEnd1 = 0;    // Reset.
            if (iState == 3)    iBuf = 2;             // EOL not found yet.
          }
          else if ((iBuf == 2) && (iEnd2 != 0)) // Use Buffer 2.
          {
            for ( ; iBeg2 < iEnd2; iBeg2++)
            {
              szOutBuf[idx] = szBuf2[iBeg2];    // Copy
              if ((szOutBuf[idx] == '\n') || (szOutBuf[idx-1] == '\r'))
              {
                iBeg2++;    // Count the char just read.
                if ((szOutBuf[idx] == '\n') && (szOutBuf[idx-1] == '\r'))
                {
                  // Using CRLF as end-of-line.
                  idx++;
                  szOutBuf[idx] = szBuf2[iBeg2];
                  iBeg2++;  // Consume LF.
                }
                szOutBuf[idx + 1] = '\0'; // True. Null Line
                iState = 4;               // Goto cleanup & exit
                break;                    // Break from for loop.
              }

            }
            if (iBeg2 == iEnd2) iBeg2 = iEnd2 = 0;    // Reset.
            if (iState == 3)    iBuf = 1;             // EOL not found yet.

          }
          else                                  // Both buffers empty and still no EOL.
          {
            if (idx < MAX_SOCK_BUFFER)
            {
              iState = 2;                       // Still room. Refill the buffer.
            }
            else
            {
              iState = 4;                       // Out of room. Return.
            }
          }
          break;
        }
      case 4:   // Cleanup and exit
        {
          iState = 0;
          break;
        }
    } // End of switch statement.
  } // End of while loop.

  if (iToast > 0)   // Remove the telnet end-of-line before returning.
  {
    while ( (szOutBuf[idx] == '\n') || (szOutBuf[idx] == '\r') )
    {
      szOutBuf[idx] = '\0';
      idx--;
    }
  }
  return (idx + 1);
}

int Socket::Send(char *szBuf, size_t iLen)
{
  return send(iSock, szBuf, iLen, 0);
}

int Socket::Send(char *szBuf)
{
  return send(iSock, szBuf, strlen(szBuf), 0);
}

int Socket::Send(const char *szBuf)
{
  return send(iSock, szBuf, strlen(szBuf), 0);
}

#define BUFSIZE         16384
int Socket::SendText(char *szFileName)
{
  std::ifstream ifIn;
  char *szBuf;

  ifIn.open(szFileName);
  if (! ifIn)
  {
    return -1;
  }

  szBuf = new char[BUFSIZE];
  iErr = 0;
  do {
    memset(szBuf, 0, BUFSIZE);
    ifIn.getline(szBuf, BUFSIZE, '\n');
    iErr += send(iSock, szBuf, strlen(szBuf), 0); // The line.
    if ( ifIn.eof() ) break;

    iErr += send(iSock, "\r\n", 2, 0); // The EOL.
  }
  while ( ! ifIn.eof() );

  ifIn.close();
  delete [] szBuf;
  return iErr;

}

int Socket::SendBinary(char *szFileName)
{
  std::ifstream ifIn;
  char *szBuf;

  ifIn.open(szFileName, std::ios::binary);
  if (! ifIn)
  {
    return -1;
  }

  szBuf = new char[BUFSIZE];
  do {
    ifIn.read(szBuf, BUFSIZE);
    iErr = send(iSock, szBuf, ifIn.gcount(), 0); // The line.
  }
  while ( ! ifIn.eof() );

  ifIn.close();
  delete [] szBuf;
  return iErr;
}

int Socket::ResolveName()
{
  struct hostent *hePeer;

  if (szPeerIp == NULL) // Only if we don't have it already
  {
    szPeerIp = new char[128];
    strncpy(szPeerIp, inet_ntoa(siThem.sin_addr), 128);
  }
  iErr = 0;
  if (szPeerName == NULL) // Only if we don't have it already
  {
    szPeerName = new char[128];
    hePeer = gethostbyaddr(&siThem.sin_addr, sizeof(struct in_addr), AF_INET);
    if (hePeer != NULL)   // We found the ip name.
    {
      strncpy(szPeerName, hePeer->h_name, 128);
      iErr = 0;
    }
    else                  // No name available for this host
    {
      strncpy(szPeerName, szPeerIp, 128);
      iErr = -1;          // Bad return.
    }
  }
  return iErr;
}

int Socket::Close()
{
  iBeg1 = iEnd1 = iBeg2 = iEnd2 = 0;
  memset(szOutBuf, 0, MAX_SOCK_BUFFER);
  memset(szBuf1, 0, MAX_SOCK_BUFFER/2);
  memset(szBuf2, 0, MAX_SOCK_BUFFER/2);
  if (szPeerIp) delete [] szPeerIp;
  if (szPeerName) delete [] szPeerName;
  szPeerIp = szPeerName = NULL;
  ulTimeout = 10;
  iErr = close(iSock);
  iSock = -1;
  return iErr;
}
