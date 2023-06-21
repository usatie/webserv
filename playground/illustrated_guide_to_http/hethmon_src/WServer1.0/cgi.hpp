//
// WWW Server  File: cgi.hpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#ifndef _CGI_HPP_
#define _CGI_HPP_

#include "socket.hpp"
#include "http10.hpp"

// ------------------------------------------------------------------
//
// This class organizes the information needed to start a CGI
// process.
//

class Cgi
{
  public:

  Cgi()
    {
      szPost = NULL;
      szOutput = NULL;
    };

  char *szProg;         // The program to execute.
  char *szPost;         // File containing post data.
  char *szOutput;       // File containing output data.
  ReqHeaders *rhInfo;   // Various info needed.
  Socket *sClient;      // Various info needed.
};


// ------------------------------------------------------------------
//
// Function prototypes.
//

void InitCgi();
int ExecCgi(Cgi *);

// ------------------------------------------------------------------

#endif

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------