//
// Socket  File: socket.cpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#include "socket.hpp"

// ------------------------------------------------------------------
//
// Passive
//
// Place the socket into passive mode.
//

int
Socket::Passive(short int sPort, int iReuse)
{
  int optval = 1;

  if (iReuse > 0)  // Force reuse of the address.
    {
      setsockopt(iSock, SOL_SOCKET, SO_REUSEADDR, (char *) &optval, sizeof(int));
    }

  sPortUs = sPort;

  bzero((void *)&siUs, iLen);              // make sure everything zero
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

// ------------------------------------------------------------------
//
// Connect
//
// Connect to the specified remote host.
//

int
Socket::Connect(char *szBuf, short sPort)
{
  struct hostent *heHost;

  heHost = gethostbyname(szBuf);
  if (heHost == NULL)
    {
      return (iErr = 1);
    }

  bzero((void *)&siUs, iLen);              // make sure everything zero
  siUs.sin_family = AF_INET;
  siUs.sin_port = htons(sPort);
  siUs.sin_addr.s_addr = *((u_long *)heHost->h_addr);

  iErr = connect(iSock, (sockaddr *)&siUs, iLen);

  return (iErr);
}

// ------------------------------------------------------------------
//
// Accept
//
// Accept an incoming connection on the socket.
//

Socket *
Socket::Accept()
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

// ------------------------------------------------------------------
//
// SendText
//
// Send the specified file across the socket. This assumes
// a text file.
//

int
Socket::SendText(char *szFileName)
{
  ifstream ifIn;
  char *szBuf;

  ifIn.open(szFileName);
  if (! ifIn)
    {
      return -1;
    }

  szBuf = new char[BUFSIZE];
  iErr = 0;
  do
    {
      memset(szBuf, 0, BUFSIZE);
      ifIn.getline(szBuf, BUFSIZE, '\n');
      iErr += send(iSock, szBuf, strlen(szBuf), 0);    // The line.
      if ( ifIn.eof() ) break;                         // The last line
                                                       // doesn't get an
                                                       // eol appended.
      iErr += send(iSock, "\r\n", strlen("\r\n"), 0);  // The eol.
    }
  while ( ! ifIn.eof() );

  ifIn.close();
  delete [] szBuf;
  return iErr;
}

// ------------------------------------------------------------------
//
// SendBinary
//
// Send the specified file across the socket. This assumes
// a binary file.
//

int
Socket::SendBinary(char *szFileName)
{
  ifstream ifIn;
  char *szBuf;

  ifIn.open(szFileName, ios::binary);
  if (! ifIn)
    {
      return -1;
    }

  szBuf = new char[BUFSIZE];

  while ( ! ifIn.eof() )
    {
      ifIn.read(szBuf, BUFSIZE);
      iErr = send(iSock, szBuf, ifIn.gcount(), 0);    // The line
    }

  ifIn.close();
  delete [] szBuf;
  return iErr;
}

// ------------------------------------------------------------------
//
// Recv
//
// Receive up to iBytes on this socket.
//

int
Socket::Recv(int iBytes)
{
  memset(szOutBuf, 0, MAX_SOCK_BUFFER);

  if ((iBuf == 1) && (iEnd1 != 0))  // Copy the contents of buf 1.
    {
      if (iBytes >= (iEnd1 - iBeg1))  // Copy all the bytes.
        {
          memcpy(szOutBuf, szBuf1 + iBeg1, iEnd1 - iBeg1);
          iErr = iEnd1 - iBeg1;
          iBeg1 = iEnd1 = 0;
          iBuf = 2;
        }
      else  // Only copy the requested number.
        {
          memcpy(szOutBuf, szBuf1 + iBeg1, iBytes);
          iErr = iBytes;    // This many bytes sent back.
          iBeg1 += iBytes;  // Advance to this location.
        }
    }      
  else if ((iBuf == 2) && (iEnd2 != 0))  // Copy the contents of buf 2.
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
          iBeg1 += iBytes;
        }
    }      
  else
    {
      iErr = recv(iSock, szOutBuf, iBytes, 0);
    }

  return iErr;
}

// ------------------------------------------------------------------
//
// RecvTeol
//
// Receive a line delimited nominally by the telnet end-of-line
// sequence -- CRLF. This one also accepts just CR or just LF
// also.
//

int
Socket::RecvTeol(int iToast)
{
  int i;
  int iState = 1,
      idx = 0;

  memset(szOutBuf, 0, MAX_SOCK_BUFFER);

//  cerr << "RecvTeol" << endl;
  while (iState != 0)
    {
      switch (iState)
        {
          case 1:  // Figure out where to start.
            {
              if ((iEnd1 == 0) && (iEnd2 == 0)) // Both buffers empty.
                {
                  iState = 2;
                }
              else
                {
                  iState = 3;
                }
              break;
            }
          case 2:  // Fill the buffers with data.
            {
//              cerr << "State 2. Filling buffers." << endl;
              iErr = recv(iSock, szBuf1, MAX_SOCK_BUFFER/2, 0);
//              cerr << "Got " << iErr << " bytes of data." << endl;
              if (iErr == -1)  // Error receiving data.
                {
                  iState = 0;
                  break;
                }
              iBeg1 = 0;
              iEnd1 = iErr;
              if (iErr == MAX_SOCK_BUFFER/2)  // Filled up Buffer 1.
                {
//                  cerr << "State 2. Filling buffer 2." << endl;
                  iErr = recv(iSock, szBuf2, MAX_SOCK_BUFFER/2, 0);
//                  cerr << "Got " << iErr << " bytes of data." << endl;
                  if (iErr == -1)  // Error receiving data.
                    {
                      iState = 0;
                      break;
                    }
                  iBeg2 = 0;
                  iEnd2 = iErr;
                }
              iBuf = 1;
              iState = 3;  // Advance to the next state.
              break;
            }
          case 3:  // Look for the EOL sequence.
            {
//              cerr << "State 3. Looking for eol." << endl;
              if ((iBuf == 1) && (iEnd1 != 0))  // Use Buffer 1 first.
                {
//                  cerr << "State 3. Checking buffer 1." << endl;
                  for ( ; iBeg1 < iEnd1; iBeg1++)
                    {
                      szOutBuf[idx] = szBuf1[iBeg1];   // Copy.
                      if ((szOutBuf[idx] == '\n') ||
                          (szOutBuf[idx] == '\r')    )
                        {
                          iBeg1++;                     // Count the char just read.
                          if ((szOutBuf[idx] == '\r') &&
                              (szBuf1[iBeg1] == '\n')    )
                            {
                              // Using CRLF as end-of-line.
                              idx++;
                              szOutBuf[idx] = szBuf1[iBeg1];
                              iBeg1++;                 // Consume LF.
                            }
                          szOutBuf[idx + 1] = '\0';    // True. Null line.
                          iState = 4;                  // Goto cleanup & exit.
                          break;                       // Break from for loop.
                        }
                      idx++;                           // Advance to next spot.
                      if ((idx+1) == MAX_SOCK_BUFFER)     // Out of room.
                        {
                          szOutBuf[MAX_SOCK_BUFFER] = '\0';
                          iState = 4;
                          break;
                        }
                    }
                  if (iBeg1 == iEnd1) iBeg1 = iEnd1 = 0;   // Reset.
                  if (iState == 3)    iBuf = 2;            // EOL not found yet.
                }
              else if ((iBuf == 2) && (iEnd2 != 0))    // Use Buffer 2.
                {
//                  cerr << "State 3. Checking buffer 2." << endl;
                  for ( ; iBeg2 < iEnd2; iBeg2++)
                    {
                      szOutBuf[idx] = szBuf2[iBeg2];   // Copy.
                      if ((szOutBuf[idx] == '\n') ||
                          (szOutBuf[idx] == '\r')    )
                        {
                          iBeg2++;                     // Count the char just read
                          if ((szOutBuf[idx] == '\r') &&
                              (szBuf2[iBeg2] == '\n')    )
                            {
                              // Using CRLF as end-of-line.
                              idx++;
                              szOutBuf[idx] = szBuf2[iBeg2];
                              iBeg2++;
                            }
                          szOutBuf[idx + 1] = '\0';    // True. Null line.
                          iState = 4;                  // Goto cleanup & exit.
                          break;                       // Break from for loop.
                        }
                      idx++;                           // Advance to next spot.
                      if ((idx+1) == MAX_SOCK_BUFFER)     // Out of room.
                        {
                          szOutBuf[MAX_SOCK_BUFFER] = '\0';
                          iState = 4;
                          break;
                        }
                    }
                  if (iBeg2 == iEnd2) iBeg2 = iEnd2 = 0;   // Reset.
                  if (iState == 3)    iBuf = 1;            // EOL not found yet.
                }
              else  // Both buffers empty and still no eol.
                {
//                  cerr << "State 3. Both buffers empty." << endl;
                  if (idx < MAX_SOCK_BUFFER)
                    {
                      iState = 2;  // Still room. Refill the buffers.
                    }
                  else
                    {
                      iState = 4;  // Out of room. Return.
                    }
                }
              break;
            }
          case 4:  // Cleanup and exit.
            {
//              cerr << "State 4. Cleanup." << endl;
              iState = 0;
              break;
            }
        } // End of switch statement.
    } // End of while loop.

//  cerr << "RecvTeol. Out of loop." << endl;
  if (iToast > 0)  // Remove the telnet end of line before returning.
    {
      while ( (szOutBuf[idx] == '\r') || (szOutBuf[idx] == '\n') )
        {
          szOutBuf[idx] = '\0';
          idx--;
        }
    }

//  cerr << "RecvTeol. Returning " << idx + 1 << " bytes." << endl;
  return (idx + 1);
}
              
// ------------------------------------------------------------------
//
// ResolveName
//
// Look up the name of the peer connected to this socket.
//

int
Socket::ResolveName()
{
  struct hostent *hePeer;

  if (szPeerIp == NULL)  // Only if we don't have it already.
    {
      szPeerIp = new char[128];
      strncpy(szPeerIp, inet_ntoa(siThem.sin_addr), 128);
    }
  szPeerName = new char[128];
  hePeer = gethostbyaddr((char *)&(siThem.sin_addr),
                          sizeof(struct in_addr), AF_INET);
  if (hePeer != NULL)  // We found the ip name.
    {
      strncpy(szPeerName, hePeer->h_name, 128);
      iErr = 0;        // Good return.
    }
  else                 // No name available for this host.
    {
      strncpy(szPeerName, szPeerIp, 128);
      iErr = -1;       // Bad return.
    }

  return iErr;
}          

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------