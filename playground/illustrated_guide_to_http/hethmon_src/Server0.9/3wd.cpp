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

// ------------------------------------------------------------------

volatile int iAccessLock = 0;  // Ram semaphore for access logfile.
volatile int iErrorLock = 0;   // Ram semaphore for error logfile.
void Stop(int iSig);

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

#ifdef __OS2__
  iRc = sock_init();       // Make sure socket services are available
  if (iRc != 0)
    {
      cerr << "Error!" << endl;
      cerr << "Socket services not available. Exiting." << endl;
      return 1;
    }
#elif __WINDOWS__
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

  signal(SIGABRT, (_SigFunc)Stop);
  signal(SIGBREAK, (_SigFunc)Stop);
  signal(SIGINT, (_SigFunc)Stop);
  signal(SIGTERM, (_SigFunc)Stop);

  cout << "w3d> Starting server on port number " << iPort << "." << endl;

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
  char *szBuf, *szRequest, *szUri;
  int iRc, iRsp;
  struct stat *sBuf;
   
  sClient = (Socket *)arg;          // Get the pointer to the socket

  szRequest = new char[SMALLBUF];
  szUri = new char[SMALLBUF];
  szBuf = new char[SMALLBUF];       // Memory for the filename.
  szBuf[0] = NULL;
  iRsp = 200;


  iRc = sClient->RecvTeol(NO_EOL);  // Get the message

  // Parse the components of the request (HTTP 0.9 only)
  sscanf(sClient->szOutBuf, "%s %s", szRequest, szUri);

  if (stricmp(szRequest, "get") != 0)        // We only understand 'get'
    {
      sClient->Send(sz400);                  // Send them an error message
      WriteToLog(sClient, sClient->szOutBuf, 400, NULL); // Save the connection
      sClient->Close();                      // Close the socket.
      delete sClient;                        // Delete the socket.

      return;  // we're done now
    }

  // Check for a query in the URI.
  if (strstr(szUri, "?") != NULL)
    {
      // We found a query present.
      iRc = Index(szUri, szBuf);
      if (iRc > 0)
        {
          sClient->Send(sz404);  // Error.
          iRsp = 404;
        }
      else
        {
          sClient->SendText(szBuf);
        }
      WriteToLog(sClient, sClient->szOutBuf, iRsp, szBuf);
      unlink(szBuf);       // Delete the temp file.
      sClient->Close();    // Close down as normal.
      delete sClient;
      delete [] szBuf;
      delete [] szUri;
      delete [] szRequest;
      return;
    }

  DeHexify(szUri);  // Remove any escape sequences.

  // We received a 'get' request
  if (stricmp(szUri, "/") == 0)
    {
      sprintf(szBuf, "%s\\%s", szServerRoot, szWelcome);
      iRc = sClient->SendText(szBuf);
      if (iRc < 0)
        {
          sClient->Send(sz404);
          iRsp = 404;
        }
    }
  else
    {
      sprintf(szBuf, "%s%s", szServerRoot, szUri);
      iRc = sClient->SendText(szBuf);
      if (iRc < 0)
        {
          sClient->Send(sz404);
          iRsp = 404;
        }
    }
    
  WriteToLog(sClient, sClient->szOutBuf, iRsp, szBuf);
  sClient->Close();
  delete sClient;

  return;
}

// ------------------------------------------------------------------
//
// WriteToLog
//
// This function writes our data to the log file.
//

// For ease of indexing.
const
char szMonth[12][4] =
{
  "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

void WriteToLog(Socket *sClient, char *szReq, int iCode, char *szFile)
{
  int iRc;
  struct stat sBuf;
  long lBytes = 0;
  char szTmp[512];
  struct tm *tmPtr;
  time_t ttLocal;
  ofstream ofLog;

  iRc = stat(szFile, &sBuf);
  if (iRc == 0)
    {
      lBytes = sBuf.st_size;
    }

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

  // Resolve the IP Name if requested.
  if (bDnsLookup == TRUE)
    {
      sClient->ResolveName();
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
// Hex2Char
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
// ------------------------------------------------------------------
// ------------------------------------------------------------------