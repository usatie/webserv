//
// WWW Server  File: headers.hpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//
 
#ifndef _HEADERS_HPP_
#define _HEADERS_HPP_

#include "socket.hpp"

// ------------------------------------------------------------------
//
// This class used to hold range information.
//

class Range
{
  public:

  int iStart;
  int iEnd;
};

// ------------------------------------------------------------------
//
// The Headers class contains most of the information used during
// a connection with a client. The header lines, the method, the
// URI, and various other bits used to service a request.
//

class Headers
{
  public:

  Headers();
  ~Headers();
  int RcvHeaders(Socket *sClient);
  int CheckHeaders();
  int FindRanges(int iSize);

  char *szMethod,
       *szUri,
       *szVer,
       *szQuery,
       *szAuthType,
       *szRemoteUser,
       *szAccept,
       *szAcceptCharset,
       *szAcceptEncoding,
       *szAcceptLanguage,
       *szAcceptRanges,
       *szAge,
       *szAllow,
       *szAuth,
       *szCacheControl,
       *szConnection,
       *szContentBase,
       *szContentEncoding,
       *szContentLanguage,
       *szContentLength,
       *szContentLocation,
       *szContentMD5,
       *szContentRange,
       *szContentType,
       *szDate,
       *szETag,
       *szExpires,
       *szFrom,
       *szHost,
       *szIfModSince,
       *szIfMatch,
       *szIfNoneMatch,
       *szIfRange,
       *szIfUnmodSince,
       *szLastMod,
       *szLocation,
       *szMaxForwards,
       *szPragma,
       *szPublic,
       *szRange,
       *szReferer,
       *szRetryAfter,
       *szServer,
       *szTransferEncoding,
       *szUpgrade,
       *szUserAgent,
       *szVary,
       *szVia,
       *szWarning,
       *szWWWAuth,
       *szRealm;
  time_t ttIfModSince,
         ttIfUnmodSince;
  BOOL bPersistent,
       bChunked;
  ULONG ulContentLength;
  char **szIfMatchEtags,
       **szIfNoneMatchEtags;
  Range *rRanges;
  int iRangeNum;

  private:

  char ** Etag(char *);
};

// ------------------------------------------------------------------

#endif

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------