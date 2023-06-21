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
     *szErrorLog,    // The error log filename.
     *szAccessName;  // The access file name.
short sPort;         // The port number to serve.
BOOL bDnsLookup,     // Flag whether to do dns reverse lookups.
     bGmtTime;       // Flag whether to use GMT in access log file.
int iNumPathAliases, // The number of path aliases.
    iNumExecAliases, // The number of exec aliases.
    iNumTypes;       // The number of extension types.
long lGmtOffset;     // The offset in minutes between local time and GMT.
Paths *pAliasPath,   // The set of root aliases.
      *pAliasExec;   // The set of exec aliases.
Extensions *eExtMap; // The set of extensions and types.

// ------------------------------------------------------------------

void SetDefaults();

// ------------------------------------------------------------------

#define Convert(str)  for (i = 0; i < strlen(str); i++)     \
                          if (str[i] == '\\') str[i] = '/'  

// ------------------------------------------------------------------

int ReadConfig(char *szConfigName)
{
  ifstream ifIn;     // The config file handle.
  char *szBuf,
       *szDirective,
       *szVal1,
       *szVal2;
  int iNum1 = 0,
      iNum2 = 0,
      iNum3 = 1,
      i;

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
  szVal1 = new char[SMALLBUF];
  szVal2 = new char[SMALLBUF];

  pAliasPath = new Paths[MAX_ALIASES];
  pAliasExec = new Paths[MAX_ALIASES];
  eExtMap = new Extensions[MAX_EXTENSIONS];
  eExtMap[0].szExt = new char[1];
  eExtMap[0].szExt[0] = NULL;
  eExtMap[0].szType = strdup("application/octet-stream");

  while ( ! ifIn.eof() )  // Until the end of the file.
    {
      memset(szBuf, 0, SMALLBUF);
      memset(szDirective, 0, SMALLBUF);
      ifIn.getline(szBuf, SMALLBUF, '\n');

      if ((szBuf[0] == '#') || (szBuf[0] == NULL)) continue;   // Skip comments.

      sscanf(szBuf, "%s %s %s", szDirective, szVal1, szVal2);  // Parse the line.

      if (stricmp(szDirective, "ServerRoot") == 0)
        {
          if (szServerRoot) delete [] szServerRoot;
          Convert(szVal1);
          if (szVal1[strlen(szVal1) - 1] != '/')
            {
              szServerRoot = new char[strlen(szVal1) + 2];
              sprintf(szServerRoot, "%s/", szVal1);
            }
          else
            {
              szServerRoot = strdup(szVal1);
            }
        }
      else if (stricmp(szDirective, "HostName") == 0)
        {
          if (szHostName) delete [] szHostName;
          szHostName = strdup(szVal1);
        }
      else if (stricmp(szDirective, "GMTOffset") == 0)
        {
          lGmtOffset = 60 * (atol(szVal1) / 100);  // Number of hours in minutes
          lGmtOffset += (atol(szVal1) % 100);      // Number of minutes specified
          lGmtOffset *= 60;                        // Convert minutes to seconds
        }
      else if (stricmp(szDirective, "Welcome") == 0)
        {
          if (szWelcome) delete [] szWelcome;
          szWelcome = strdup(szVal1);
        }
      else if (stricmp(szDirective, "AccessLog") == 0)
        {
          if (szAccessLog) delete [] szAccessLog;
          szAccessLog = strdup(szVal1);
        }
      else if (stricmp(szDirective, "ErrorLog") == 0)
        {
          if (szErrorLog) delete [] szErrorLog;
          szErrorLog = strdup(szVal1);
        }
      else if (stricmp(szDirective, "Port") == 0)
        {
          sPort = (short) atoi(szVal1);
        }
      else if (stricmp(szDirective, "DNSLookup") == 0)
        {
          if (stricmp(szVal1, "Off") == 0)
            {
              bDnsLookup = FALSE;
            }
        }
      else if (stricmp(szDirective, "LogTime") == 0)
        {
          if (stricmp(szVal1, "GMT") == 0)
            {
              bGmtTime = TRUE;
            }
        }
      else if (stricmp(szDirective, "AccessName") == 0)
        {
          if (szAccessName) delete [] szAccessName;
          szAccessName = strdup(szVal1);
        }
      else if (stricmp(szDirective, "PathAlias") == 0)
        {
          if (iNum1 == MAX_ALIASES)
            {
              cerr << "Exceeded maximum path aliases. " << szVal1 << " ignored." << endl;
              continue;
            }
          pAliasPath[iNum1].szAlias = strdup(szVal1);
          pAliasPath[iNum1].szTrue = new char[strlen(szVal2) + 2];
          strcpy(pAliasPath[iNum1].szTrue, szVal2);
          Convert(pAliasPath[iNum1].szAlias);
          Convert(pAliasPath[iNum1].szTrue);

          i = strlen(pAliasPath[iNum1].szTrue);
          if (pAliasPath[iNum1].szTrue[i - 1] != '/')
            {
              pAliasPath[iNum1].szTrue[i] = '/';
              pAliasPath[iNum1].szTrue[i + 1] = NULL;
            }

          iNum1++;
        }
      else if (stricmp(szDirective, "ExecAlias") == 0)
        {
          if (iNum2 == MAX_ALIASES)
            {
              cerr << "Exceeded maximum exec aliases. " << szVal1 << " ignored." << endl;
              continue;
            }
          pAliasExec[iNum2].szAlias = strdup(szVal1);
          pAliasExec[iNum2].szTrue = new char[strlen(szVal2) + 2];
          strcpy(pAliasExec[iNum2].szTrue, szVal2);
          Convert(pAliasExec[iNum2].szAlias);
          Convert(pAliasExec[iNum2].szTrue);

          i = strlen(pAliasExec[iNum2].szTrue);
          if (pAliasExec[iNum2].szTrue[i - 1] != '/')
            {
              pAliasExec[iNum2].szTrue[i] = '/';
              pAliasExec[iNum2].szTrue[i + 1] = NULL;
            }

          iNum2++;
        }
      else if (stricmp(szDirective, "ExtType") == 0)
        {
          if (iNum3 == MAX_EXTENSIONS)
            {
              cerr << "Exceeded maximum extensions. " << szVal1 << " ignored." << endl;
              continue;
            }
          eExtMap[iNum3].szExt = strdup(szVal1);
          eExtMap[iNum3].szType = strdup(szVal2);

          iNum3++;
        }
    }

  ifIn.close();

  iNumPathAliases = iNum1;
  iNumExecAliases = iNum2;
  iNumTypes = iNum3;

  delete [] szBuf;
  delete [] szDirective;
  delete [] szVal1;
  delete [] szVal2;

  for (i = 0; i < iNumTypes; i++)
    {
      if (strstr(eExtMap[i].szType, "text/") == NULL)
        {
          eExtMap[i].bBinary = TRUE;
        }
    }

  return 0;
}

// ------------------------------------------------------------------

void SetDefaults()
{
  sPort = WWW_PORT;
  bDnsLookup = TRUE;
  bGmtTime = FALSE;
  szAccessName = strdup(HTACCESS);
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------