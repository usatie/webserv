//
// WWW Server  File: config.cpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

//
// This file contains the source code to read the configuration
// file for the WWW server.
//

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <iostream.h>
#include <fstream.h>
#include <process.h>
#include <sys\types.h>

#ifdef __OS2__
  #define INCL_DOS
  #include <os2.h>
#elif __WINDOWS__
  #include <windows.h>
#endif

#include "defines.hpp"
#include "config.hpp"

// ------------------------------------------------------------------

char *szServerRoot,  // The root directory for serving files.
     *szHostName,    // The hostname given out. Overrides the call to hostname().
     *szWelcome,     // The default file to serve.
     *szAccessLog,   // The access log filename.
     *szErrorLog;    // The error log filename.
short sPort;         // The port number to serve.
BOOL bDnsLookup,     // Flag whether to do dns reverse lookups.
     bGmtTime;       // Flag whether to use GMT in access log file.

// ------------------------------------------------------------------

void SetDefaults();

// ------------------------------------------------------------------

int ReadConfig(char *szConfigName)
{
  ifstream ifIn;     // The config file handle.
  char *szBuf,
       *szDirective,
       *szVal;

  ifIn.open(szConfigName);
  if ( ! ifIn )
    {
      cerr << "Error!" << endl;
      cerr << "Could not read configuration file: " << szConfigName << endl;
      return 1;
    }

  SetDefaults();

  szBuf = new char[SMALLBUF];
  szDirective = new char[SMALLBUF];
  szVal = new char[SMALLBUF];

  while ( ! ifIn.eof() )  // Until the end of the file.
    {
      ifIn.getline(szBuf, SMALLBUF, '\n');

      if (szBuf[0] == '#') continue;  // Skip comments.

      sscanf(szBuf, "%s %s", szDirective, szVal);  // Parse the line.

      if (stricmp(szDirective, "ServerRoot") == 0)
        {
          if (szServerRoot) delete [] szServerRoot;
          szServerRoot = new char[strlen(szVal) + 1];
          strcpy(szServerRoot, szVal);
        }
      else if (stricmp(szDirective, "HostName") == 0)
        {
          if (szHostName) delete [] szHostName;
          szHostName = new char[strlen(szVal) + 1];
          strcpy(szHostName, szVal);
        }
      else if (stricmp(szDirective, "Welcome") == 0)
        {
          if (szWelcome) delete [] szWelcome;
          szWelcome = new char[strlen(szVal) + 1];
          strcpy(szWelcome, szVal);
        }
      else if (stricmp(szDirective, "AccessLog") == 0)
        {
          if (szAccessLog) delete [] szAccessLog;
          szAccessLog = new char[strlen(szVal) + 1];
          strcpy(szAccessLog, szVal);
        }
      else if (stricmp(szDirective, "ErrorLog") == 0)
        {
          if (szErrorLog) delete [] szErrorLog;
          szErrorLog = new char[strlen(szVal) + 1];
          strcpy(szErrorLog, szVal);
        }
      else if (stricmp(szDirective, "Port") == 0)
        {
          sPort = (short) atoi(szVal);
        }
      else if (stricmp(szDirective, "DNSLookup") == 0)
        {
          if (stricmp(szVal, "Off") == 0)
            {
              bDnsLookup = FALSE;
            }
        }
      else if (stricmp(szDirective, "LogTime") == 0)
        {
          if (stricmp(szVal, "GMT") == 0)
            {
              bGmtTime = TRUE;
            }
        }
    }

  ifIn.close();

  delete [] szBuf;
  delete [] szDirective;
  delete [] szVal;

  return 0;
}

// ------------------------------------------------------------------

void SetDefaults()
{
  sPort = WWW_PORT;
  bDnsLookup = TRUE;
  bGmtTime = FALSE;
  iInputTimeOut = 5 * 60;     // 5 minutes
  iOutputTimeOut = 30 * 60;   // 30 minutes
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------