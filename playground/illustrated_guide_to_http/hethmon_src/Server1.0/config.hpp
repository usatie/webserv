//
// WWW Server  File: config.hpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

//
// This file contains the variable definitions used in configuration
// of the www server.
//

#ifndef _CONFIG_HPP_
#define _CONFIG_HPP_

#ifdef __OS2__
  #include <os2.h>
#elif __WINDOWS__
  #include <windows.h>
#endif

// ------------------------------------------------------------------

class Paths
{
  public:

  char *szAlias;
  char *szTrue;

  Paths()
    {
      szAlias = NULL;
      szTrue = NULL;
    };
  ~Paths()
    {
      if (szAlias != NULL) delete [] szAlias;
      if (szTrue != NULL) delete [] szTrue;
    };
};

class Extensions
{
  public:

  char *szExt;
  char *szType;
  BOOL bBinary;

  Extensions()
    {
      szExt = NULL;
      szType = NULL;
      bBinary = FALSE;
    };
  ~Extensions()
    {
      if (szExt != NULL) delete [] szExt;
      if (szType != NULL) delete [] szType;
    };
};

// ------------------------------------------------------------------

const char szServerVer[] = "3wd/1.0";    // The server name and version.

#define HTACCESS "3wdaccess"             // Access filename.

// ------------------------------------------------------------------

extern
char *szServerRoot,  // The root directory for serving files.
     *szHostName,    // The hostname given out. Overrides the call to hostname().
     *szWelcome,     // The default file to serve.
     *szAccessLog,   // The access log filename.
     *szErrorLog,    // The error log filename.
     *szAccessName;  // The access file name.
extern
short sPort;         // The port number to serve.
extern
BOOL bDnsLookup,     // Flag whether to do dns reverse lookups.
     bGmtTime;       // Flag whether to use GMT in access log file.
extern
int iNumPathAliases, // The number of path aliases.
    iNumExecAliases, // The number of exec aliases.
    iNumTypes;       // The number of extension types.
extern
long lGmtOffset;     // The offset in minutes between local time and GMT.
extern
Paths *pAliasPath,   // The set of root aliases.
      *pAliasExec;   // The set of exec aliases.
extern
Extensions *eExtMap; // The set of extensions and types.

// ------------------------------------------------------------------

extern
int ReadConfig(char *);

// ------------------------------------------------------------------

#endif

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------