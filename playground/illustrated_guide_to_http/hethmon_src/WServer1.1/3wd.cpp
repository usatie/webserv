//
// WWW Server  File: 3wd.cpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

//
// This file contains the source code to the basic framework
// for the WWW server.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream.h>
#include <fstream.h>
#include <process.h>
#include <time.h>
#include <signal.h>
#include <sys\types.h>
#include <sys\stat.h>

#ifdef __IBMCPP__
  #include <builtin.h>
#endif

#ifdef __OS2__
  #define INCL_DOS
  #include <os2.h>
  #define Sleep(x) DosSleep(x)  // Portability definition
#elif __WINDOWS__
  #include <windows.h>
#endif

#include "socket.hpp"
#include "defines.hpp"
#include "3wd.hpp"
#include "config.hpp"
#include "scodes.hpp"
#include "headers.hpp"
#include "http10.hpp"
#include "http11.hpp"
#include "util.hpp"
#include "cgi.hpp"

// ------------------------------------------------------------------

volatile int iAccessLock = 0;  // Ram semaphore for access logfile.
volatile int iErrorLock = 0;   // Ram semaphore for error logfile.
void Stop(int iSig);

// For ease of indexing.
const
char szMonth[12][4] =
{
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
const
char szDay[7][4] =
{
  "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

// ------------------------------------------------------------------
//
// main()
//
// Our main function and entry point
//

int main(int argc, char *argv[])
{
  int iPort = WWW_PORT;
  int i,
      iRc;
  char szCmd[512];
  BOOL bNotDone = TRUE;

  cerr << "3wd Starting" << endl;

#ifdef __OS2__
  cerr << "OS/2 Version Starting" << endl;
  iRc = sock_init();       // Make sure socket services are available
  if (iRc != 0)
    {
      cerr << "Error!" << endl;
      cerr << "Socket services not available. Exiting." << endl;
      return 1;
    }
#elif __WINDOWS__
  cerr << "Windows Version Starting" << endl;
  WORD wVersionRequested; 
  WSADATA wsaData; 
  wVersionRequested = MAKEWORD(1, 1); 
  
  iRc = WSAStartup(wVersionRequested, &wsaData); 
  if (iRc != 0) 
    {
      cerr << "Error!" << endl;
      cerr << "Socket services not available. Exiting." << endl;
      return 1;
    }
#endif

  iRc = ReadConfig("3wd.cf");
  if ( iRc )
    {
      cerr << "Error!" << endl;
      cerr << "Error reading configuration file. Exiting." << endl;
      return 1;  // Exit on error.
    }
  InitCgi();

  cerr << "Config read." << endl;

  i = 1;
  while (i < argc)         // Check the command line args
    {
      if (strcmp(argv[i], "-p") == 0)
        {
          // Set the port to user requested
          sPort = (short) atoi(argv[i + 1]);
          i += 2;
        }
      else                 // Unknown arg, ignore it
        {
          cerr << "Unknown argument \"" << argv[i] << "\" ignored." << endl;
          i++;
        }
    }

  cerr << "3wd> Starting server on port number " << iPort << "." << endl;

  signal(SIGABRT, (_SigFunc)Stop);
  signal(SIGBREAK, (_SigFunc)Stop);
  signal(SIGINT, (_SigFunc)Stop);
  signal(SIGTERM, (_SigFunc)Stop);

  Server();

#ifdef __WINDOWS__
  WSACleanup();  // Cleanup for windows sockets.
#endif

  // Now we're done
  return 0;
}

// ------------------------------------------------------------------
//
// Stop
//
// Handle the signals and stop the server.

void Stop(int iSig)
{
  cerr << "Stopping server." << endl;

#ifdef __WINDOWS__
  WSACleanup();  // Cleanup for windows sockets.
#endif

  exit(0);
}

// ------------------------------------------------------------------
//
// This function accepts the incoming connections spawning the threads
// to handle the actual work.
//

void Server()
{
  Socket sSock,        // Our server socket for listening
         *sClient;     // The client socket
  int iRc;             // Integer return code

  if (! sSock.Create())    // If failure
    {
      cerr << "Error." << endl;
      cerr << "Cannot create socket to accept connections." << endl;
      return;
    }

  sSock.Passive(sPort, REUSE_PORT);    // Go to passive mode

  for ( ; ; )                    // Forever
    {
      sClient = sSock.Accept();  // Listen for incoming connections

      if (sClient != NULL)
        {
          // We established a good connection, start a thread to handle it
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

// ------------------------------------------------------------------
//
// W3Conn
//
// This is our worker thread to handle the actual request.
//

void _Optlink W3Conn(void *arg)
{
  Socket *sClient;
  char *szRequest, *szUri, *szVer;
  int iRc;
   
  sClient = (Socket *)arg;          // Get the pointer to the socket

  // Resolve the IP Name if requested.
  if (bDnsLookup == TRUE)
    {
      sClient->ResolveName();
    }

  szRequest = new char[SMALLBUF];
  szUri = new char[SMALLBUF];
  szVer = new char[SMALLBUF];

  iRc = sClient->RecvTeol(NO_EOL);  // Get the message

  // Parse the components of the request
  sscanf(sClient->szOutBuf, "%s %s %s", szRequest, szUri, szVer);

  if (stricmp(szVer, "http/1.0") == 0)
    {
      DoHttp10(sClient, szRequest, szUri);
    }
  else if (stricmp(szVer, "http/1.1") == 0)
    {
      iRc = DoHttp11(sClient, szRequest, szUri);
      while (iRc == TRUE)  // Do persistent connections.
        {
          sClient->RecvTeol(NO_EOL);
          sscanf(sClient->szOutBuf, "%s %s %s", szRequest, szUri, szVer);
          iRc = DoHttp11(sClient, szRequest, szUri);
        }
    }
  else  // Treat this request as a HTTP/0.9 request.
    {
      DoHttp09(sClient, szRequest, szUri);
    }
  delete [] szRequest;
  delete [] szUri;
  delete [] szVer;
  delete sClient;
  return;
}

// ------------------------------------------------------------------
//
// DoHttp09()
//
// Handle the basic HTTP request.
//

void DoHttp09(Socket *sClient, char *szRequest, char *szUri)
{
  char *szBuf,
       *szSearch = NULL,
       *szTmp,
       *szPath,
       szFile[PATH_LENGTH];
  int iRsp,
      iRc;
  Headers hInfo;
  struct stat sBuf;

  szBuf = new char[SMALLBUF];       // Memory for the filename.
  szBuf[0] = NULL;
  iRsp = 200;

  if (strcmp(szRequest, "GET") != 0)         // We only understand 'get'
    {
      sClient->Send(sz400);                  // Send them an error message
      WriteToLog(sClient, sClient->szOutBuf, 400, NULL); // Save the connection
      sClient->Close();                      // Close the socket.
      delete [] szBuf;
      return;  // we're done now
    }

  // Check for a query in the URI.
  if ((szTmp = strchr(szUri, '?')) != NULL)
    {
      // Break up the URI into document and and search parameters.
      *szTmp = NULL;  // Append NULL to shorter URI.
      szTmp++;        // Let szTmp point to the query terms.
      szSearch = strdup(szTmp);
    }

  DeHexify(szUri);              // Remove any escape sequences.
  szPath = ResolvePath(szUri);  // Resolve if possible.

  if (szPath == NULL)           // Cannot resolve.
    {
      sClient->Send(sz404);
      iRsp = 404;
    }
  else if (szPath[strlen(szPath) - 1] == '/')  // Add welcome doc.
    {
      sprintf(szBuf, "%s%s", szPath, szWelcome);
    }
  else  // Exact request.
    {
      strcpy(szBuf, szPath);
    }

  iRc = CheckAuth(szBuf, &hInfo, READ_ACCESS);  // Check for authorization.
  if (iRc != ACCESS_OK)
    {
      sClient->Send(sz403);         // Not allowed.
      iRsp = 403;
    }

  if ((iRsp == 200) && (szSearch != NULL))  // Do an index search.
    {
      iRc = Index(szBuf, szSearch, szFile, szUri);
      if (iRc != 0)
        {
          sClient->Send(sz404);
          iRsp = 404;
        }
      strcpy(szBuf, szFile);  // Save the new file if successful.
    }

  if (iRsp == 200)  // No error conditions yet.
    {
      iRc = sClient->SendText(szBuf);  // Send the document.
      if (iRc < 0)
        {
          sClient->Send(sz404);  // Error.
          iRsp = 404;
        }
    }

  stat(szBuf, &sBuf);
  WriteToLog(sClient, sClient->szOutBuf, iRsp, sBuf.st_size);
  sClient->Close();
  delete [] szBuf;
  if (szSearch != NULL)
    {
      unlink(szFile);     // Delete the temporary.
      delete [] szSearch; // And the search string.
    }

  return;
}

// ------------------------------------------------------------------
//
// WriteToLog
//
// This function writes our data to the log file.
//

void WriteToLog(Socket *sClient, char *szReq, int iCode, long lBytes)
{
  int iRc;
  struct stat sBuf;
  char szTmp[512];
  struct tm *tmPtr;
  time_t ttLocal;
  ofstream ofLog;

  time(&ttLocal);
  // Use GMT in the log file if true and available.
  if ((bGmtTime == TRUE) && ((tmPtr = gmtime(&ttLocal)) != NULL))
    {
      sprintf(szTmp, "%02d/%s/%4d:%02d:%02d:%02d 000",
              tmPtr->tm_mday,
              szMonth[tmPtr->tm_mon],
              (tmPtr->tm_year + 1900),
              tmPtr->tm_hour,
              tmPtr->tm_min,
              tmPtr->tm_sec);
    }
  else  // Use local time instead.
    {
      tmPtr = localtime(&ttLocal);
      sprintf(szTmp, "%02d/%s/%4d:%02d:%02d:%02d %s",
              tmPtr->tm_mday,
              szMonth[tmPtr->tm_mon],
              (tmPtr->tm_year + 1900),
              tmPtr->tm_hour,
              tmPtr->tm_min,
              tmPtr->tm_sec,
              getenv("TZ"));
    }

  // Lock the log file first.
  while (__lxchg(&iAccessLock, 1) != 0)
    {
      Sleep(1);  // Sleep, not spin.
    }

  ofLog.open(szAccessLog, ios::app); // Open log file for appending.
  if (bDnsLookup == TRUE)
    {
      ofLog << sClient->szPeerName << " - - [" << szTmp << "] \""
            << szReq << "\" " << iCode << " " << lBytes << endl;
    }
  else
    {
      ofLog << sClient->szPeerIp << " - - [" << szTmp << "] \""
            << szReq << "\" " << iCode << " " << lBytes << endl;
    }
  ofLog.close();

  // Unlock the file.
  __lxchg(&iAccessLock, 0);
  
  return;
}

// ------------------------------------------------------------------
//
// DeHexify
//
// Convert any hex escape sequences to regular characters.
//

char * DeHexify(char *szUri)
{
  char *szFrom, *szTo;

  if (! szUri) return szUri;  // Sanity check.

  szFrom = szUri;
  szTo = szUri;

  while ( *szFrom )           // While characters left.
    {
      if ( *szFrom  == '%' )  // Look for the % sign.
        {
          szFrom++;
          if (*szFrom)        // If valid.
            {
              *szTo = Hex2Char(*szFrom) * 16;       // Convert.
              szFrom++;
            }
          if (*szFrom) *szTo += Hex2Char(*szFrom);  // Add remainder.
        }
      else
        {
          *szTo = *szFrom;    // Plain characters.
        }
      szTo++;
      szFrom++;
    }
  *szTo = '\0';               // Append new null to string.

  return (szUri);
}

// ------------------------------------------------------------------
//
// Hex2Dec
//
// Convert a hex character to a decimal character.
//

char Hex2Char(char c)
{
  switch (c)
    {
      case 'A':
      case 'a':
        return 10;
      case 'B':
      case 'b':
        return 11;
      case 'C':
      case 'c':
        return 12;
      case 'D':
      case 'd':
        return 13;
      case 'E':
      case 'e':
        return 14;
      case 'F':
      case 'f':
        return 15;
      default:
        return c;
    }
}
      
// ------------------------------------------------------------------


