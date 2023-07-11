#include "headers.hpp"
#define SMALLBUF        4196

Headers::Headers()
{
  szMethod = NULL;
  szUri = NULL;
  szVer = NULL;
  szQuery = NULL;
  szAuthType = NULL;
  szRemoteUser = NULL;
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
  szTransferEncoding = NULL;
  szUpgrade = NULL;
  szUserAgent = NULL;
  szVary = NULL;
  szVia = NULL;
  szWarning = NULL;
  szWWWAuth = NULL;
  szRealm = NULL;

  ttIfModSince = 0;
  ttIfUnmodSince = 0;

  bPersistent = true;
  bChunked = false;

  ulContentLength = 0;

  szIfMatchEtags = NULL;
  szIfNoneMatchEtags = NULL;
}

Headers::~Headers()
{
  int i;

  if (szMethod != NULL) delete [] szMethod;
  if (szUri != NULL) delete [] szUri;
  if (szVer != NULL) delete [] szVer;
  if (szQuery != NULL) delete [] szQuery;
  if (szAuthType != NULL) delete [] szAuthType;
  if (szRemoteUser != NULL) delete [] szRemoteUser;
  if (szAccept != NULL) delete [] szAccept;
  if (szAcceptCharset != NULL) delete [] szAcceptCharset;
  if (szAcceptEncoding != NULL) delete [] szAcceptEncoding;
  if (szAcceptLanguage != NULL) delete [] szAcceptLanguage;
  if (szAge != NULL) delete [] szAge;
  if (szAllow != NULL) delete [] szAllow;
  if (szAuth != NULL) delete [] szAuth;
  if (szCacheControl != NULL) delete [] szCacheControl;
  if (szConnection != NULL) delete [] szConnection;
  if (szContentBase != NULL) delete [] szContentBase;
  if (szContentEncoding != NULL) delete [] szContentEncoding;
  if (szContentLanguage != NULL) delete [] szContentLanguage;
  if (szContentLength != NULL) delete [] szContentLength;
  if (szContentLocation != NULL) delete [] szContentLocation;
  if (szContentMD5 != NULL) delete [] szContentMD5;
  if (szContentRange != NULL) delete [] szContentRange;
  if (szContentType != NULL) delete [] szContentType;
  if (szDate != NULL) delete [] szDate;
  if (szETag != NULL) delete [] szETag;
  if (szExpires != NULL) delete [] szExpires;
  if (szFrom != NULL) delete [] szFrom;
  if (szHost != NULL) delete [] szHost;
  if (szIfModSince != NULL) delete [] szIfModSince;
  if (szIfMatch != NULL) delete [] szIfMatch;
  if (szIfNoneMatch != NULL) delete [] szIfNoneMatch;
  if (szIfRange != NULL) delete [] szIfRange;
  if (szIfUnmodSince != NULL) delete [] szIfUnmodSince;
  if (szLastMod != NULL) delete [] szLastMod;
  if (szLocation != NULL) delete [] szLocation;
  if (szMaxForwards != NULL) delete [] szMaxForwards;
  if (szPragma != NULL) delete [] szPragma;
  if (szPublic != NULL) delete [] szPublic;
  if (szRange != NULL) delete [] szRange;
  if (szReferer != NULL) delete [] szReferer;
  if (szRetryAfter != NULL) delete [] szRetryAfter;
  if (szTransferEncoding != NULL) delete [] szTransferEncoding;
  if (szUpgrade != NULL) delete [] szUpgrade;
  if (szUserAgent != NULL) delete [] szUserAgent;
  if (szVary != NULL) delete [] szVary;
  if (szVia != NULL) delete [] szVia;
  if (szWarning != NULL) delete [] szWarning;
  if (szWWWAuth != NULL) delete [] szWWWAuth;
  if (szRealm != NULL) delete [] szRealm;

  if (szIfMatchEtags != NULL)
  {
    for (i = 0; i < iRangeNum; i++)
    {
      if (szIfMatchEtags[i] != NULL) delete [] szIfMatchEtags[i];
    }
    delete [] szIfMatchEtags;
  }

  if (szIfNoneMatchEtags != NULL)
  {
    for (i = 0; i < iRangeNum; i++)
    {
      if (szIfNoneMatchEtags[i] != NULL) delete [] szIfNoneMatchEtags[i];
    }
    delete [] szIfNoneMatchEtags;
  }
}

int Headers::RcvHeaders(Socket *sClient)
{
  char *szHdr,
       *szTmp,
       *szBuf;
  int iRc, i;
  szHdr = new char[SMALLBUF];
  do {
    iRc = sClient->RecvTeol(NO_EOL);    // Get the message.
    if (iRc < 0) break;
    if (sClient->szOutBuf[0] == '\0') break;

    szTmp = sClient->szOutBuf;
    if (! isspace(szTmp[0]) )           // Replace the header if not continuation
    {
      i = 0;
      while ((*szTmp != ':') && (*szTmp)) // Until the delimiter.
      {
        szHdr[i++] = *szTmp;              // Copy.
        szTmp++;
      }
      szHdr[i] = '\0';                  // Properly end string.
      // strlwr(szHdr);                    // Lowercase only.
      // for macos, strlwr is not available.
      for (i = 0; i < strlen(szHdr); i++) szHdr[i] = tolower(szHdr[i]);
    }
    szTmp++;                            // Go past the ':' or ' '.
    while ((*szTmp == ' ') && (*szTmp)) szTmp++; // Eliminate leading spaces.
  }
  while (sClient->szOutBuf[0] != '\0');

  delete [] szHdr;
  // Now determine if we received any etags.
  if (szIfMatch != NULL) szIfMatchEtags = Etag(szIfMatch);
  if (szIfNoneMatchEtags != NULL) szIfNoneMatchEtags = Etag(szIfNoneMatch);
  return iRc;
}
