//
// WWW Server  File: util.hpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

// ------------------------------------------------------------------

#ifndef _UTIL_HPP_
#define _UTIL_HPP_

#include "headers.hpp"

// ------------------------------------------------------------------
//
// Function Prototypes.
//

time_t ConvertDate(char *);
char * CreateDate(time_t);
int CheckAuth(char *, Headers *, int);
int CheckFile(char *, Headers *);
int BasicCheck(char *, Headers *);

// ------------------------------------------------------------------
//
// Authorization codes.
//

#define ACCESS_OK      1  // Allow access.
#define ACCESS_DENIED  2  // Need authorization.
#define ACCESS_FAILED  3  // Credentials failed.

#define WRITE_ACCESS   1  // Check write access
#define READ_ACCESS    2  // Check read access

// ------------------------------------------------------------------

#endif

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------