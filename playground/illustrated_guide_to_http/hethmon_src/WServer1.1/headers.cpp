//
// WWW Server  File: headers.cpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <iostream.h>
#include <fstream.h>
#include <process.h>
#include <time.h>
#include <sys\types.h>
#include <sys\stat.h>

#ifdef __IBMCPP__
  #include <builtin.h>
#endif

#ifdef __OS2__
  #define INCL_DOS
  #include <os2.h>
  #define Sleep(x) DosSleep(x)  // Portability definition
#elif __WINDOWS__
  #include <windows.h>
#endif

#include "socket.hpp"
#include "defines.hpp"
#include "3wd.hpp"
#include "config.hpp"
#include "scodes.hpp"
#include "http10.hpp"
#include "util.hpp"
#include "headers.hpp"

// ------------------------------------------------------------------
//
// RcvHeaders()
//
// Receive the rest of the headers sent by the client.
//

int
Headers::RcvHeaders(Socket *sClient)
{
  char *szHdr,
       *szTmp,
       *szBuf;
  int iRc, i;

  szHdr = new char[SMALLBUF];

  do
    {
      iRc = sClient->RecvTeol(NO_EOL);  // Get the message.
      if (iRc < 0) break;
      if (sClient->szOutBuf[0] == NULL) break;

      szTmp = sClient->szOutBuf;
      if (! isspace(szTmp[0]) )  // Replace the header if not
        {                        // continuation.
          i = 0;
          while ((*szTmp != ':') && (*szTmp))  // Until the delimiter.
            {
              szHdr[i] = *szTmp; // Copy.
              i++;               // Advance.
              szTmp++;
            }
          szHdr[i] = NULL;  // Properly end string.
          strlwr(szHdr);    // Lowercase only.
        }
      szTmp++;          // Go past the ':' or ' '.
      while ((*szTmp == ' ') && (*szTmp))
        {
          szTmp++;  // Eliminate leading spaces.
        }

      switch(szHdr[0])
        {
          case 'a':
            {
              if (strcmp(szHdr, "accept") == 0)
                {
                  if (szAccept)
                    {
                      szBuf = new char[strlen(szAccept) + strlen(szTmp) + 2];
                      sprintf(szBuf, "%s,%s", szAccept, szTmp);
                      delete [] szAccept;
                      szAccept = szBuf;
                    }
                  else
                    {
                      szAccept = strdup(szTmp);
                    }
                }
              else if (strcmp(szHdr, "accept-charset") == 0)
                {
                  if (szAcceptCharset)
                    {
                      szBuf = new char[strlen(szAcceptCharset) + strlen(szTmp) + 2];
                      sprintf(szBuf, "%s,%s", szAcceptCharset, szTmp);
                      delete [] szAcceptCharset;
                      szAcceptCharset = szBuf;
                    }
                  else
                    {
                      szAcceptCharset = strdup(szTmp);
                    }
                }
              else if (strcmp(szHdr, "accept-encoding") == 0)
                {
                  if (szAcceptEncoding)
                    {
                      szBuf = new char[strlen(szAcceptEncoding) + strlen(szTmp) + 2];
                      sprintf(szBuf, "%s,%s", szAcceptEncoding, szTmp);
                      delete [] szAcceptEncoding;
                      szAcceptEncoding = szBuf;
                    }
                  else
                    {
                      szAcceptEncoding = strdup(szTmp);
                    }
                }
              else if (strcmp(szHdr, "accept-language") == 0)
                {
                  if (szAcceptLanguage)
                    {
                      szBuf = new char[strlen(szAcceptLanguage) + strlen(szTmp) + 2];
                      sprintf(szBuf, "%s,%s", szAcceptLanguage, szTmp);
                      delete [] szAcceptLanguage;
                      szAcceptLanguage = szBuf;
                    }
                  else
                    {
                      szAcceptLanguage = strdup(szTmp);
                    }
                }
              else if (strcmp(szHdr, "authorization") == 0)
                {
                  if (szAuth) delete [] szAuth;
                  szAuth = strdup(szTmp);
                }
              break;
            }
          case 'c':
            {
              if (strcmp(szHdr, "connection") == 0)
                {
                  if (szConnection) delete [] szConnection;
                  szConnection = strdup(szTmp);
                  if (stricmp(szConnection, "close") == 0)
                    {
                      bPersistent = FALSE;
                    }
                }
              else if (strcmp(szHdr, "content-length") == 0)
                {
                  if (szContentLength) delete [] szContentLength;
                  szContentLength = strdup(szTmp);
                  ulContentLength = atol(szContentLength);
                }
              else if (strcmp(szHdr, "content-type") == 0)
                {
                  if (szContentType) delete [] szContentType;
                  szContentType = strdup(szTmp);
                }
              break;
            }
          case 'd':
            {
              if (strcmp(szHdr, "date") == 0)
                {
                  if (szDate) delete [] szDate;
                  szDate = strdup(szTmp);
                }
              break;
            }
          case 'f':
            {
              if (strcmp(szHdr, "from") == 0)
                {
                  if (szFrom) delete [] szFrom;
                  szFrom = strdup(szTmp);
                }
              break;
            }
          case 'h':
            {
              if (strcmp(szHdr, "host") == 0)
                {
                  if (szHost) delete [] szHost;
                  szHost = strdup(szTmp);
                }
              break;
            }
          case 'i':
            {
              if (strcmp(szHdr, "if-modified-since") == 0)
                {
                  if (szIfModSince) delete [] szIfModSince;
                  szIfModSince = strdup(szTmp);
                  ttIfModSince = ConvertDate(szIfModSince);
                }
              else if (strcmp(szHdr, "if-match") == 0)
                {
                  if (szIfMatch)
                    {
                      szBuf = new char[strlen(szIfMatch) + strlen(szTmp) + 2];
                      sprintf(szBuf, "%s,%s", szIfMatch, szTmp);
                      delete [] szIfMatch;
                      szIfMatch = szBuf;
                    }
                  else
                    {
                      szIfMatch = strdup(szTmp);
                    }
                }
              else if (strcmp(szHdr, "if-none-match") == 0)
                {
                  if (szIfNoneMatch)
                    {
                      szBuf = new char[strlen(szIfNoneMatch) + strlen(szTmp) + 2];
                      sprintf(szBuf, "%s,%s", szIfNoneMatch, szTmp);
                      delete [] szIfNoneMatch;
                      szIfNoneMatch = szBuf;
                    }
                  else
                    {
                      szIfNoneMatch = strdup(szTmp);
                    }
                }
              else if (strcmp(szHdr, "if-range") == 0)
                {
                  if (szIfRange) delete [] szIfRange;
                  szIfRange = strdup(szTmp);
                }
              else if (strcmp(szHdr, "if-unmodified-since") == 0)
                {
                  if (szIfUnmodSince) delete [] szIfUnmodSince;
                  szIfUnmodSince = strdup(szTmp);
                  ttIfUnmodSince = ConvertDate(szIfUnmodSince);
                }
              break;
            }
          case 'r':
            {
              if (strcmp(szHdr, "range") == 0)
                {
                  if (szRange) delete [] szRange;
                  szRange = strdup(szTmp);
                }
              else if (strcmp(szHdr, "referer") == 0)
                {
                  if (szReferer) delete [] szReferer;
                  szReferer = strdup(szTmp);
                }
              break;
            }
          case 't':
            {
              if (strcmp(szHdr, "transfer-encoding") == 0)
                {
                  if (szTransferEncoding) delete [] szTransferEncoding;
                  szTransferEncoding = strdup(szTmp);
                  if (stricmp(szTransferEncoding, "chunked") == 0)
                    {
                      bChunked = TRUE;
                    }
                }
              break;
            }
          case 'u':
            {
              if (strcmp(szHdr, "upgrade") == 0)
                {
                  if (szUpgrade) delete [] szUpgrade;
                  szUpgrade = strdup(szTmp);
                }
              else if (strcmp(szHdr, "user-agent") == 0)
                {
                  if (szUserAgent) delete [] szUserAgent;
                  szUserAgent = strdup(szTmp);
                }
              break;
            }
        }
    }
  while (sClient->szOutBuf[0] != NULL);

  delete [] szHdr;

  // Now determine if we received any etags.
  if (szIfMatch != NULL) szIfMatchEtags = Etag(szIfMatch);
  if (szIfNoneMatch != NULL) szIfNoneMatchEtags = Etag(szIfNoneMatch);

  return iRc;
}

// ------------------------------------------------------------------
//
// CheckHeaders
//
// Check the headers received for inconsistent headers.
//

int
Headers::CheckHeaders()
{
  int i, j;

  // Check for the host header first.
  if (szHost == NULL) return FALSE;

  // First check to make sure the If-Unmodified-Since time
  // is not before the If-Modified-Since time.
  if ((szIfModSince != NULL) && (szIfUnmodSince != NULL))
    {
      if (ttIfUnmodSince <= ttIfModSince)
        {
          return FALSE;
        }
    }

  // Now check for etags which match between If-Match and
  // If-None-Match.
  if ((szIfMatch != NULL) && (szIfNoneMatch != NULL))
    {
      for (i = 0; szIfMatchEtags[i] != NULL; i++)
        {
          for (j = 0; szIfNoneMatchEtags[j] != NULL; j++)
            {
              if (strcmp(szIfMatchEtags[i], szIfNoneMatchEtags[j]) == 0)
                {
                  return FALSE;
                }
            }
        }
    }

  return TRUE;
}

// ------------------------------------------------------------------
//
// Etag
//
// Retrieve the etags sent by the client.
//

char **
Headers::Etag(char *szTags)
{
  char *szPtr, *szStart, **szEtags, cTmp;
  int i, j;

  // Find out how many tags are expected.
  i = 0;
  szPtr = szTags;
  while (*szPtr != NULL)
    {
      if (*szPtr == ',') i++;
      szPtr++;
    }

  // A minimum of 2. One for a tag and one for a NULL marker.
  i += 2;
  szEtags = new char * [i];
  for (j = 0; j < i; j++)
    {
      szEtags[j] = NULL;
    }

  j = 0;
  szPtr = szTags;
  while (*szPtr != NULL)
    {
      while ( (isspace(*szPtr)) && (*szPtr != NULL) )
        {
          szPtr++;
        }
      if (*szPtr == NULL) continue;  // Escape.
      szStart = szPtr;
      if (*szPtr == 'W') szPtr += 2; // Bypass weak indicator.
      if (*szPtr == '*')
        {
          szEtags[j] = strdup("*");  // Match any.
          break;
        }
      szPtr++;  // Advance past the <"> mark.
      while ( (*szPtr != '"') && (*szPtr != NULL) )
        {
          szPtr++;  // Look for end of etag.
        }
      if (*szPtr == NULL) continue;  // Escape.
      szPtr++;         // Past the ending <"> mark.
      cTmp = *szPtr;   // Save character temporarily.
      *szPtr = NULL;   // Mark end of string of current etag.
      szEtags[j] = strdup(szStart);  // Save it.
      j++;             // Count it.
      *szPtr = cTmp;   // Restore character.
      while ( (*szPtr != ',') && (*szPtr != NULL) )
        {
          szPtr++; // Advance to start of next etag or end of line.
        }
      if (*szPtr == ',') szPtr++;
    }

  return szEtags;
}

// ------------------------------------------------------------------
//
// Headers
//
// The ctor initializes most values to NULL for safety and easy
// checking.
//

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
  szServer = NULL;
  szTransferEncoding = NULL;
  szUpgrade = NULL;
  szUserAgent = NULL;
  szVary = NULL;
  szVia = NULL;
  szWarning = NULL;
  szWWWAuth = NULL;
  szDate = NULL;
  szRealm = NULL;
  ttIfModSince = 0;
  ttIfUnmodSince = 0;
  bPersistent = TRUE;
  ulContentLength = 0;
  szIfMatchEtags = NULL;
  szIfNoneMatchEtags = NULL;
  rRanges = NULL;
  iRangeNum = 0;
  bChunked = FALSE;
}

// ------------------------------------------------------------------
//
// ~Headers
//
// The dtor deletes any memory stored in the class instance.
//

Headers::~Headers()
{
  int i;

  if (szMethod) delete [] szMethod;
  if (szUri) delete [] szUri;
  if (szVer) delete [] szVer;
  if (szQuery) delete [] szQuery;
  if (szAuthType) delete [] szAuthType;
  if (szRemoteUser) delete [] szRemoteUser;
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
  if (szDate) delete [] szDate;
  if (szRealm) delete [] szRealm;
  if (szIfMatchEtags)
    {
      for (i = 0; szIfMatchEtags[i] != NULL; i++)
        {
          delete [] (szIfMatchEtags[i]);
        }
      delete [] szIfMatchEtags;
    }
  if (szIfNoneMatchEtags)
    {
      for (i = 0; szIfNoneMatchEtags[i] != NULL; i++)
        {
          delete [] (szIfNoneMatchEtags[i]);
        }
      delete [] szIfNoneMatchEtags;
    }
  if (rRanges != NULL) delete [] rRanges;
}

// ------------------------------------------------------------------
//
// FindRanges
//
// Locate and store the ranges sent by the client.
//

int
Headers::FindRanges(int iSize)
{
  char *szBuf, *szTmp;
  int i, iNum, iLength, iIdx,
      bError;

  if (szRange == NULL) return 1;  // Nothing to do.

  bError = FALSE;
  szTmp = szRange;
  iNum = 1;
  while (*szTmp != NULL)  // Count the number of ranges.
    {
      if (*szTmp == ',') iNum++;
      szTmp++;
    }

  rRanges = new Range[iNum];  // Space for them.
  szBuf = new char[SMALLBUF];

  // Now pull out the range numbers.
  iIdx = 0;
  szTmp = strchr(szRange, '=');
  szTmp++;
  while (*szTmp != NULL)
    {
      if (isdigit(*szTmp)) // Found range start.
        {
          i = 0;
          while (isdigit(*szTmp))  // Advance past the digits.
            {
              szBuf[i] = *szTmp;
              i++;
              szTmp++;
            }
          szBuf[i] = NULL;         // Mark NULL and grab the start.
          rRanges[iIdx].iStart = atoi(szBuf);

          if (*szTmp != '-') bError = TRUE; // Wrong format.
          szTmp++;
          if (isdigit(*szTmp))  // Found range end.
            {
              i = 0;
              while (isdigit(*szTmp))  // Advance past the digits.
                {
                  szBuf[i] = *szTmp;
                  i++;
                  szTmp++;
                }
              szBuf[i] = NULL;         // Mark NULL and grab the end.
              rRanges[iIdx].iEnd = atoi(szBuf);
            }
          else  // Use end of file as range end.
            {
              rRanges[iIdx].iEnd = iSize - 1;
            }
          iIdx++;  // Advance to next spot.
        }
      else if (*szTmp == '-')  // No start range given.
        {
          szTmp++;
          if (isdigit(*szTmp) != TRUE) bError = TRUE;
          i = 0;
          while (isdigit(*szTmp)) // Grab number of bytes.
            {
              szBuf[i] = *szTmp;
              i++;
              szTmp++;
            }
          szBuf[i] = NULL;
          i = atoi(szBuf);
          // The start will be i bytes from the end of the file.
          rRanges[iIdx].iStart = iSize - i - 1;
          rRanges[iIdx].iEnd = iSize - 1;
          iIdx++;
        }
      else
        {
          szTmp++;
        }
    }

  delete [] szBuf;
  iRangeNum = iIdx;
            
  if (bError == TRUE) // Error in ranges.
    {
      delete [] rRanges;
      rRanges = NULL;
      iRangeNum = 0;
      return 1;
    }
  return 0;
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------