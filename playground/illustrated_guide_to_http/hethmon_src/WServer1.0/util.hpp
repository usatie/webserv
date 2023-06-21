//
// WWW Server  File: util.hpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

//
// Function and Variable declarations for util.cpp
//

#ifndef _UTIL_HPP_
#define _UTIL_HPP_

// ------------------------------------------------------------------

time_t ConvertDate(char *);
char * CreateDate(time_t);
int CheckAuth(char *, ReqHeaders *);
int CheckFile(char *, ReqHeaders *);
int BasicCheck(char *, ReqHeaders *);

// ------------------------------------------------------------------

#define ACCESS_OK      1  // Allow access.
#define ACCESS_DENIED  2  // Need authorization.
#define ACCESS_FAILED  3  // Credentials failed.

// ------------------------------------------------------------------

#endif

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------