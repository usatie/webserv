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

extern
char *szServerRoot,  // The root directory for serving files.
     *szHostName,    // The hostname given out. Overrides the call to hostname().
     *szWelcome,     // The default file to serve.
     *szAccessLog,   // The access log filename.
     *szErrorLog;    // The error log filename.
extern
short sPort;         // The port number to serve.
extern
BOOL bDnsLookup,     // Flag whether to do dns reverse lookups.
     bGmtTime;       // Flag whether to use GMT in access log file.

// ------------------------------------------------------------------

extern
int ReadConfig(char *);

// ------------------------------------------------------------------

#endif

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------