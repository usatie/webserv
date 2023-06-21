//
// WWW Server  File: http10.hpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#ifndef _HTTP10_HPP_
#define _HTTP10_HPP_

#include "headers.hpp"

// ------------------------------------------------------------------
//
// Function Prototypes.
//

void DoHttp10(Socket *, char *, char *);
char * ResolvePath(char *);
int FindType(char *);
char * ResolveExec(char *);
int DoPath(Socket *, char *, char *, char *, Headers *, char *);
int DoExec(Socket *, int, char *, Headers *);
int SendError(Socket *, char *, int, char *, Headers *);

// ------------------------------------------------------------------

#endif

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------