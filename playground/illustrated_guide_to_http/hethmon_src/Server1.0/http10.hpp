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

// ------------------------------------------------------------------

class ReqHeaders
{
  public:

  char *szMethod,
       *szUri,
       *szQuery,
       *szAuthType,
       *szRemoteUser,
       *szAuth,
       *szContentType,
       *szContentLength,
       *szFrom,
       *szIfModSince,
       *szReferer,
       *szUserAgent,
       *szDate,
       *szRealm;
  time_t ttIfModSince;
  unsigned long ulContentLength;

  ReqHeaders()
    {
      szMethod = NULL;
      szUri = NULL;
      szQuery = NULL;
      szAuthType = NULL;
      szRemoteUser = NULL;
      szAuth = NULL;
      szContentType = NULL;
      szContentLength = NULL;
      szFrom = NULL;
      szIfModSince = NULL;
      szReferer = NULL;
      szUserAgent = NULL;
      szDate = NULL;
      szRealm = NULL;
      ttIfModSince = 0;
      ulContentLength = 0;
    }
  ~ReqHeaders()
    {
      if (szMethod) delete [] szMethod;
      if (szUri) delete [] szUri;
      if (szQuery) delete [] szQuery;
      if (szRemoteUser) delete [] szRemoteUser;
      if (szAuth) delete [] szAuth;
      if (szContentType) delete [] szContentType;
      if (szContentLength) delete [] szContentLength;
      if (szFrom) delete [] szFrom;
      if (szIfModSince) delete [] szIfModSince;
      if (szReferer) delete [] szReferer;
      if (szUserAgent) delete [] szUserAgent;
      if (szDate) delete [] szDate;
      if (szRealm) delete [] szRealm;
    }
};

// ------------------------------------------------------------------

void DoHttp10(Socket *, char *, char *);
int RcvHeaders(Socket *, ReqHeaders *);
char * ResolvePath(char *);
int FindType(char *);
char * ResolveExec(char *);
int DoPath(Socket *, char *, char *, char *, ReqHeaders *, char *);
int DoExec(Socket *, char *, char *, ReqHeaders *);
int SendError(Socket *, char *, int, ReqHeaders *);

// ------------------------------------------------------------------

#endif

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------