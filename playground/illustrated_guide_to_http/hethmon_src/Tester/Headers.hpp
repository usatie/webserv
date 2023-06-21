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

class Headers
{
  public:

  int RcvHeaders(Socket *sClient);

  char *szMethod,
       *szUri,
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
       *szIpName,
       *szIpAddr,
       *szRealm;
  time_t ttIfModSince,
         ttIfUnmodSince;
  BOOL bPersistent;
  ULONG ulContentLength;

  Headers()
    {
      szMethod = NULL;
      szUri = NULL;
      szAccept = NULL;
      szAcceptCharset = NULL;
      szAcceptEncoding = NULL;
      szAcceptLanguage = NULL;
      szAge = NULL;
      szAllow = NULL;
      szAuth = NULL;
      szCacheControl = NULL;
      szConnection = NULL;
      szContentBase = NULL;
      szContentEncoding = NULL;
      szContentLanguage = NULL;
      szContentLength = NULL;
      szContentLocation = NULL;
      szContentMD5 = NULL;
      szContentRange = NULL;
      szContentType = NULL;
      szDate = NULL;
      szETag = NULL;
      szExpires = NULL;
      szFrom = NULL;
      szHost = NULL;
      szIfModSince = NULL;
      szIfMatch = NULL;
      szIfNoneMatch = NULL;
      szIfRange = NULL;
      szIfUnmodSince = NULL;
      szLastMod = NULL;
      szLocation = NULL;
      szMaxForwards = NULL;
      szPragma = NULL;
      szPublic = NULL;
      szRange = NULL;
      szReferer = NULL;
      szRetryAfter = NULL;
      szServer = NULL;
      szTransferEncoding = NULL;
      szUpgrade = NULL;
      szUserAgent = NULL;
      szVary = NULL;
      szVia = NULL;
      szWarning = NULL;
      szWWWAuth = NULL;
      szIpName = NULL;
      szIpAddr = NULL;
      szRealm = NULL;
      ttIfModSince = 0;
      bPersistent = TRUE;
      ulContentLength = 0;
    }
  ~Headers()
    {
      if (szMethod) delete [] szMethod;
      if (szUri) delete [] szUri;
      if (szAccept) delete [] szAccept;
      if (szAcceptCharset) delete [] szAcceptCharset;
      if (szAcceptEncoding) delete [] szAcceptEncoding;
      if (szAcceptLanguage) delete [] szAcceptLanguage;
      if (szAge) delete [] szAge;
      if (szAllow) delete [] szAllow;
      if (szAuth) delete [] szAuth;
      if (szCacheControl) delete [] szCacheControl;
      if (szConnection) delete [] szConnection;
      if (szContentBase) delete [] szContentBase;
      if (szContentEncoding) delete [] szContentEncoding;
      if (szContentLanguage) delete [] szContentLanguage;
      if (szContentLength) delete [] szContentLength;
      if (szContentLocation) delete [] szContentLocation;
      if (szContentMD5) delete [] szContentMD5;
      if (szContentRange) delete [] szContentRange;
      if (szContentType) delete [] szContentType;
      if (szDate) delete [] szDate;
      if (szETag) delete [] szETag;
      if (szExpires) delete [] szExpires;
      if (szFrom) delete [] szFrom;
      if (szHost) delete [] szHost;
      if (szIfModSince) delete [] szIfModSince;
      if (szIfMatch) delete [] szIfMatch;
      if (szIfNoneMatch) delete [] szIfNoneMatch;
      if (szIfRange) delete [] szIfRange;
      if (szIfUnmodSince) delete [] szIfUnmodSince;
      if (szLastMod) delete [] szLastMod;
      if (szLocation) delete [] szLocation;
      if (szMaxForwards) delete [] szMaxForwards;
      if (szPragma) delete [] szPragma;
      if (szPublic) delete [] szPublic;
      if (szRange) delete [] szRange;
      if (szReferer) delete [] szReferer;
      if (szRetryAfter) delete [] szRetryAfter;
      if (szServer) delete [] szServer;
      if (szTransferEncoding) delete [] szTransferEncoding;
      if (szUpgrade) delete [] szUpgrade;
      if (szUserAgent) delete [] szUserAgent;
      if (szVary) delete [] szVary;
      if (szVia) delete [] szVia;
      if (szWarning) delete [] szWarning;
      if (szWWWAuth) delete [] szWWWAuth;
      if (szIpName) delete [] szIpName;
      if (szIpAddr) delete [] szIpAddr;
      if (szRealm) delete [] szRealm;
    }
};

// ------------------------------------------------------------------
// ------------------------------------------------------------------


#endif

