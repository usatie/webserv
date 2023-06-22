#include <stdio.h>
#include <string>

#define NO_EOL          1
#define STACKSIZE       50000
#define SMALLBUF        4196

class Socket;
class Headers;

void DoHttp09(Socket *sClient, char *szMethod, char *szUri);
void DoHttp10(Socket *sClient, char *szMethod, char *szUri);
bool DoHttp11(Socket *sClient, char *szMethod, char *szUri);

class Socket{
public:
  char *szOutBuf;                  // Used to return data in
  int Recv(int iBytes);            // Receive up to iBytes on this socket
  int RecvTeol(int iToast);        // Receive up to the telnet eol
                                   // and possibly remove the telnet eol
  int Close();                     // Close this socket
  Socket * Accept();               // Listen for connections
};

class Headers
{
  public:
    Headers();
    ~Headers();
    int RcvHeaders(Socket *sClient);
    int CheckHeaders();
    int FindRanges(int iSize);
    char *szMethod,
         *szUri,
         *szVer,
         *szAccept;
};

int Headers::RcvHeaders(Socket *sClient)
{
  char *szHdr,
       *szTmp,
       *szBuf;
  int iRc, i;

  szHdr = new char[SMALLBUF];
  // ...
  do {
    iRc = sClient->RecvTeol(NO_EOL);
    if ( iRc < 0 ) break;
    if (sClient->szOutBuf[0] == '\0') break;

    szTmp = sClient->szOutBuf;
    if ( !isspace(szTmp[0]) ) // Replace the header if not
    {
      i = 0;
      while ((*szTmp != ';') && (*szTmp)) // Until the delimiter
      {
        szHdr[i] = *szTmp;
        i++;
        szTmp++;
      }
      szHdr[i] = '\0';  // Properly end string.
      strlwr(szHdr);    // Lowercase only.
    }
    szTmp++;            // Go past the ':' or ' '
    // Why this code is checking (*szTmp)? And why this code is ignoring TAB?
    while ((*szTmp == ' ') && (*szTmp)) szTmp++; // Eliminate leading spaces.
    switch (szHdr[0])
    {
      case 'a':
        {
          if (strcmp(szHdr, "accept") == 0)
          {
            if (szAccept)
            {
              szBuf = new char[strlen(szAccept) + strlen(szTmp) + 2];
              sprintf(szBuf, "%s,%s", szAccept, szTmp);
              delete [] szAccept;
              szAccept = szBuf;
            }
            else
            {
              szAccept = strdup(szTmp);
            }
          }
        }
    }
  }
  while (sClient->szOutBuf[0] != '\0');

  delete [] szHdr;

  // ...
}

void W3Conn(void *arg)
{
  Socket *sClient;
  char *szRequest, *szUri, *szVer;
  bool iRc;
   
  sClient = (Socket *)arg;          // Get the pointer to the socket
  iRc = sClient->RecvTeol(NO_EOL); // Get the message
  // Parse the components of the request
  sscanf(sClient->szOutBuf, "%s %s %s", szRequest, szUri, szVer);
  if (strcasecmp(szVer, "http/1.0") == 0)
  {
    DoHttp10(sClient, szRequest, szUri);
  }
  else if (strcasecmp(szVer, "http/1.1") == 0)
  {
    iRc = DoHttp11(sClient, szRequest, szUri);
    while ( iRc )
    {
      sClient->RecvTeol(NO_EOL);
      sscanf(sClient->szOutBuf, "%s %s %s", szRequest, szUri, szVer);
      iRc = DoHttp11(sClient, szRequest, szUri);

    }
  }
  else // Treat this as a HTTP/0.9 request.
  {
    DoHttp09(sClient, szRequest, szUri);
  }
}

void serve() {
  Socket sSock,        // Our server socket for listening
         *sClient;     // The client socket
  int iRc;             // Integer return code

  // Initialize Listening socket
  // ...

  // Loop forever, accepting connections
  for ( ; ; )
  {
    sClient = sSock.Accept();
    if (sClient != NULL)
    {
      // We established a connection, start a thread to handle it
      iRc = _beginthread(W3Conn, 0, STACKSIZE, (void *)sClient);
      if ( iRc == -1 )
      {
        // Failure to start thread. Close the connection.
        sClient->Close();
        delete sClient;
      }
    }
  }
}
