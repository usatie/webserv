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
  #define OS2
  #include <os2.h>
  #define Sleep(x) DosSleep(x)  // Portability definition
#endif

#include "socket.hpp"
#include "defines.hpp"
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

//  cerr << "RcvHeaders." << endl;

  iRc = sClient->RecvTeol(NO_EOL);  // Get the message
  do
    {
      szTmp = sClient->szOutBuf;
      if (! isspace(szTmp[0]) )  // Replace the header if not continuation.
        {
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

//      cerr << "RcvHeaders. Header = " << szHdr << endl;
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
          case 'e':
            {
              if (strcmp(szHdr, "etag") == 0)
                {
                  if (szETag) delete [] szETag;
                  szETag = strdup(szTmp);
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
          case 'w':
            {
              if (strcmp(szHdr, "www-authenticate") == 0)
                {
                  if (szWWWAuth) delete [] szWWWAuth;
                  szWWWAuth = strdup(szTmp);
                }
              break;
            }
        }

      iRc = sClient->RecvTeol(NO_EOL);  // Get the message.
      if (iRc < 0) break;
    }
  while (sClient->szOutBuf[0] != NULL);

  delete [] szHdr;

  return iRc;
}

// ------------------------------------------------------------------




// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------


