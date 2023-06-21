//
// WWW Server  File: http11.hpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#ifndef _HTTP11_HPP_
#define _HTTP11_HPP_

#include "headers.hpp"
#include "socket.hpp"

// ------------------------------------------------------------------
// The methods available.

#ifdef DELETE
#undef DELETE
#endif

#define UNKNOWN   0
#define GET       1
#define POST      2
#define HEAD      3
#define OPTIONS   4
#define PUT       5
#define DELETE    6
#define TRACE     7

// ------------------------------------------------------------------
//
// Function Prototypes.
//

int DoHttp11(Socket *, char *, char *);
int DoPath11(Socket *, int, char *, char *, Headers *);
int DoExec11(Socket *, int, char *, char *, Headers *);
int DoTrace(Socket *, Headers *);
int DoOptions(Socket *, char *, Headers *, int);
int DoPut(Socket *, Headers *, char *, char *);
int DoDelete(Socket *, char *, char *, Headers *);
int SendError(Socket *, char *, int);
int CheckMethod(char *);
char * MakeUnique(char *, char *);
int IfModSince(Headers *, time_t);
int IfUnmodSince(Headers *, time_t);
int IfMatch(Headers *, time_t);
int IfNone(Headers *, time_t);
int IfRange(Headers *, time_t);
int SendByteRange(Socket *, Headers *, char *, struct stat *, int, int);
int GetChunked(Socket *, ofstream &, Headers *);
int Hex2Dec(char c);

// ------------------------------------------------------------------

#endif

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------