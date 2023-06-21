//
// WWW Server  File: 3wd.hpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

//
// Function and Variable declarations for w3main.cpp
//

#ifndef _3WD_HPP_
#define _3WD_HPP_

// ------------------------------------------------------------------

void WriteToLog(Socket *, char *, int, long);
void _Optlink W3Conn(void *);
void Server();
char * DeHexify(char *);
char Hex2Dec(char);
int Index(char *, char *, char *, char *);
void DoHttp09(Socket *, char *, char *);

// ------------------------------------------------------------------

extern volatile int iAccessLock;
extern volatile int iErrorLock;
extern const char szMonth[12][4];
extern const char szDay[7][4];

// ------------------------------------------------------------------

#endif

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------