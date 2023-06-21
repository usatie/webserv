//
// WWW Server  File: http11.cpp
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
#include <fcntl.h>
#include <io.h>
#include <sys\stat.h>
#include <sys\types.h>

#ifdef __IBMCPP__
  #include <builtin.h>
#endif

#ifdef __OS2__
  #define INCL_DOS
  #include <os2.h>
  #define Sleep(x) DosSleep(x)  // Portability definition.
#elif __WINDOWS__
  #include <windows.h>
  #define DosCopy(x,y,z) CopyFile(x,y,z)
  #define DCPY_EXISTING FALSE
#endif

#include "socket.hpp"
#include "defines.hpp"
#include "3wd.hpp"
#include "config.hpp"
#include "scodes.hpp"
#include "headers.hpp"
#include "http11.hpp"
#include "http10.hpp"
#include "util.hpp"
#include "cgi.hpp"

// ------------------------------------------------------------------

// The alphabet used for MIME boundaries.
const
char szMime[] =
"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ'()=_,-./:=?";
// The number of characters in the alphabet.
const
int iNumMime = strlen(szMime);

// ------------------------------------------------------------------
//
// DoHttp11()
//
// This function handles our HTTP/1.0 requests.
//

int DoHttp11(Socket *sClient, char *szMethod, char *szUri)
{
  int iRc,
      iRsp,
      iType,
      iMethod;
  char *szReq,
       *szPath,
       *szCgi,
       *szTmp,
       *szSearch;
  Headers *hInfo;
  long lBytes = 0;
  BOOL bExec = FALSE,
       bCgi = FALSE,
       bPersistent;

  szReq = strdup(sClient->szOutBuf);  // Save the request line.
  iRsp = 200;
  szSearch = NULL;
  szPath = NULL;
  szCgi = NULL;
  hInfo = new Headers();
  iMethod = CheckMethod(szMethod);  // The request method.

  // First, check for TRACE method.
  if (iMethod == TRACE)
    {
      // Do a trace, saving connection.
      bPersistent = DoTrace(sClient, hInfo);
      DeHexify(szReq);
      WriteToLog(sClient, szReq, iRsp, hInfo->ulContentLength);
      delete [] szReq;
      delete hInfo;
      return bPersistent;
    }

  hInfo->RcvHeaders(sClient);  // Grab the request headers.
  bPersistent = hInfo->bPersistent;  // Find out if close was requested.
  iRc = hInfo->CheckHeaders(); // Make sure none are inconsistent.
  if (iRc == FALSE)            // Bad request.
    {
      iRsp = SendError(sClient, 
              "Missing Host header or incompatible headers detected.", 
              400, HTTP_1_1, hInfo);
      DeHexify(szReq);
      WriteToLog(sClient, szReq, iRsp, hInfo->ulContentLength);
      delete [] szReq;
      delete hInfo;
      return bPersistent;
    }

  // Check for a query in the URI.
  if ((szTmp = strchr(szUri, '?')) != NULL)
    {
      // Break up the URI into document and and search parameters.
      *szTmp = NULL;  // Append NULL to shorter URI.
      szTmp++;        // Let szTmp point to the query terms.
      szSearch = strdup(szTmp);
      hInfo->szQuery = strdup(szSearch);
      if (strchr(szSearch, '=') != NULL)
        {
          bCgi = TRUE;  // Only a cgi request can contain an equal sign.
        }
    }

  DeHexify(szUri);             // Remove any escape sequences.
  hInfo->szMethod = strdup(szMethod);  // Save a few items.
  hInfo->szUri = strdup(szUri);
  hInfo->szVer = strdup(HTTP_1_1);
  szPath = ResolvePath(szUri); // Check for path match.
  szCgi = ResolveExec(szUri);  // Check for exec match.

  // Now key on the request method and URI given.
  // OPTIONS with a match on Path.
  if ( (iMethod == OPTIONS) && (szPath != NULL) )
    {
      iRsp = DoOptions(sClient, szPath, hInfo, GET);
    }
  // OPTIONS with a match on Cgi Path.
  else if ( (iMethod == OPTIONS) && (szCgi != NULL) )
    {
      iRsp = DoOptions(sClient, szCgi, hInfo, POST);
    }
  // Generic OPTIONS.
  else if (iMethod == OPTIONS)
    {
      iRsp = DoOptions(sClient, "*", hInfo, UNKNOWN);
    }
  // Any POST request.
  else if (iMethod == POST)
    {
      iRsp = DoExec11(sClient, iMethod, szCgi, szSearch, hInfo);
    }
  // A GET or HEAD to process as a CGI request.
  else if ( (bCgi == TRUE) && ((iMethod == GET) || (iMethod == HEAD)) )
    {
      iRsp = DoExec11(sClient, iMethod, szCgi, szSearch, hInfo);
    }
  // Any PUT request.
  else if (iMethod == PUT)
    {
      iRsp = DoPut(sClient, hInfo, szPath, szCgi);
    }
  // Any valid DELETE request.
  else if (iMethod == DELETE)
    {
      iRsp = DoDelete(sClient, szPath, szCgi, hInfo);
    }
  // A simple GET or HEAD request.
  else if (((iMethod == GET) || (iMethod == HEAD)) && 
           (szPath != NULL) )
    {
      iRsp = DoPath11(sClient, iMethod, szPath, szSearch, hInfo);
    }
  // Unknown method used.
  else if (iMethod == UNKNOWN)
    {
      iRsp = SendError(sClient, "Request method not implemented.", 
                       501, HTTP_1_1, hInfo);
    }
  // Error Condition.
  else
    {
      iRsp = SendError(sClient, "Resource not found.", 
                       404, HTTP_1_1, hInfo);
    }

  // This request now finished. Log the results.
  DeHexify(szReq);
  WriteToLog(sClient, szReq, iRsp, hInfo->ulContentLength);
  delete [] szReq;
  delete hInfo;
  if ( (szSearch != NULL) && (bCgi == FALSE) )
    {
      unlink(szPath);      // The temporary search file.
      delete [] szSearch;
    }
  if (szPath) delete [] szPath;
  if (szCgi) delete [] szCgi;

  return bPersistent;
}

// ------------------------------------------------------------------
//
// DoPath11()
//
// This function checks to see if it can return the requested
// document back to the client.
//

int DoPath11(Socket *sClient, int iMethod, char *szPath, 
             char *szSearch, Headers *hInfo)
{
  struct stat sBuf;
  char *szTmp,
       *szExt,
       szBuf[PATH_LENGTH],
       szFile[PATH_LENGTH];
  ofstream ofTmp;
  int iRsp = 200,
      iRc,
      iType,
      iIfMod,
      iIfUnmod,
      iIfMatch,
      iIfNone,
      iIfRange,
      iRangeErr;

  if (szPath[strlen(szPath) - 1] == '/')
    {
      strcat(szPath, szWelcome);  // Append default welcome file.
    }

  iRc = CheckAuth(szPath, hInfo, READ_ACCESS);  // Check for authorization.
  if (iRc == ACCESS_DENIED)  // Send request for credentials.
    {
      sClient->Send("HTTP/1.1 401 \r\n");
      sClient->Send("Server: ");
      sClient->Send(szServerVer);
      sClient->Send("\r\n");
      szTmp = CreateDate(time(NULL));  // Create a date header.
      if (szTmp != NULL)
        {
          sClient->Send("Date: ");
          sClient->Send(szTmp);
          sClient->Send("\r\n");
          delete [] szTmp;
        }
      sprintf(szBuf, "WWW-Authenticate: Basic realm=\"%s\"\r\n", hInfo->szRealm);
      sClient->Send(szBuf);
      sClient->Send("Content-Type: text/html\r\n");
      sprintf(szBuf, "Content-Length: %d\r\n", strlen(sz401));
      sClient->Send(szBuf);
      sClient->Send("\r\n");
      sClient->Send(sz401);
      return 401;
    }
  else if (iRc == ACCESS_FAILED) // Send forbidden response.
    {
      sClient->Send("HTTP/1.1 403 Access Denied\r\n");
      sClient->Send("Server: ");
      sClient->Send(szServerVer);
      sClient->Send("\r\n");
      szTmp = CreateDate(time(NULL));  // Create a date header.
      if (szTmp != NULL)
        {
          sClient->Send("Date: ");
          sClient->Send(szTmp);
          sClient->Send("\r\n");
          delete [] szTmp;
        }
      sClient->Send("Content-Type: text/html\r\n");
      sprintf(szBuf, "Content-Length: %d\r\n", strlen(sz403));
      sClient->Send(szBuf);
      sClient->Send("\r\n");
      sClient->Send(sz403);
      return 403;
    }

  if (szSearch != NULL)  // Do an index search.
    {
      iRc = Index(szPath, szSearch, szFile, hInfo->szUri);
      if (iRc != 0)
        {
          iRc = SendError(sClient, "Resource not found.", 404, HTTP_1_1, hInfo);
          return iRc;
        }
      strcpy(szPath, szFile);
    }

  iRc = stat(szPath, &sBuf);
  if (iRc < 0)
    {
      iRsp = SendError(sClient, "Resource not found.", 404, HTTP_1_1, hInfo);
      return iRsp;
    }

  // Check If headers.
  iIfMod = IfModSince(hInfo, sBuf.st_mtime);
  iIfUnmod = IfUnmodSince(hInfo, sBuf.st_mtime);
  iIfMatch = IfMatch(hInfo, sBuf.st_mtime);
  iIfNone = IfNone(hInfo, sBuf.st_mtime);
  iIfRange = IfRange(hInfo, sBuf.st_mtime);
  iRangeErr = hInfo->FindRanges(sBuf.st_size);
            
  // Check to make sure any If headers are FALSE.
  // Either not-modified or no etags matched.
  if ( (iIfMod == FALSE) || (iIfNone == FALSE) )
    {
      sClient->Send("HTTP/1.1 304 Not Modified\r\n");
      iRsp = 304;
    }
  // No matching etags or it's been modified.
  else if ( (iIfMatch == FALSE) || (iIfUnmod == FALSE) )
    {
      sClient->Send("HTTP/1.1 412 Precondition Failed\r\n");
      iRsp = 412;
    }
  // Resource matched so send just the bytes requested.
  else if ((iIfRange == TRUE) && (iRangeErr == 0))
    {
      sClient->Send("HTTP/1.1 206 Partial Content\r\n");
      iRsp = 206;
    }
  // Resource didn't match, so send the entire entity.
  else if ((hInfo->szIfRange != NULL) && (iIfRange == FALSE))
    {
      sClient->Send("HTTP/1.1 200 OK\r\n");
      iRsp = 200;
    }
  // Only asked for a byte range.
  else if (iRangeErr == 0)
    {
      sClient->Send("HTTP/1.1 206 Partial Content\r\n");
      iRsp = 206;
    }
  // Must be a plain jane request.
  else
    {
      sClient->Send("HTTP/1.1 200 OK\r\n");
      iRsp = 200;
    }
      
  sClient->Send("Server: ");  // Standard server header.
  sClient->Send(szServerVer);
  sClient->Send("\r\n");
  szTmp = CreateDate(time(NULL));  // Create a date header.
  if (szTmp != NULL)
    {
      sClient->Send("Date: ");
      sClient->Send(szTmp);
      sClient->Send("\r\n");
      delete [] szTmp;
    }
  szTmp = CreateDate(sBuf.st_mtime);  // The last modified time header.
  if (szTmp != NULL)
    {
      sClient->Send("Last-Modified: ");
      sClient->Send(szTmp);
      sClient->Send("\r\n");
      delete [] szTmp;
    }
  sprintf(szBuf, "ETag: \"%d\"\r\n", sBuf.st_mtime); // Entity tag.
  sClient->Send(szBuf);

  if ((iRsp == 304) || (iRsp == 412))
    {
      sClient->Send("\r\n");
      return iRsp;  // Don't send anything else.
    }

  if (szSearch != NULL)    // Force search results to text/html type.
    {
      iType = FindType("x.html");
    }
  else
    {
      iType = FindType(szPath); // Figure out the MIME type to return.
    }

  if (iRsp == 206) // Sending partial content.
    {
      // Send byte range to client.
      SendByteRange(sClient, hInfo, szPath, &sBuf, iType, iMethod);
      return iRsp;
    }

  // Send full entity.
  sprintf(szBuf, "Content-Type: %s\r\n", eExtMap[iType].szType);
  sClient->Send(szBuf);
  sprintf(szBuf, "Content-Length: %d\r\n", sBuf.st_size);
  sClient->Send(szBuf);
  sClient->Send("\r\n");

  if (iMethod == GET)  // Don't send unless GET.
    {
      if (eExtMap[iType].bBinary == TRUE)
        {
          iRc = sClient->SendBinary(szPath);
        }
      else
        {
          iRc = sClient->SendText(szPath);
        }
    }

  return iRsp;
}    

// ------------------------------------------------------------------
//
// DoExec11()
//
// This function executes our CGI scripts.
//

int DoExec11(Socket *sClient, int iMethod, char *szPath, 
             char *szSearch, Headers *hInfo)
{
  struct stat sBuf;
  char *szTmp,
       *szVal,
       *szPtr,
       szBuf[SMALLBUF],
       szFile[PATH_LENGTH];
  int iRsp = 200,
      iRc,
      iType,
      iIfUnmod,
      iIfMatch,
      iIfNone,
      i,
      iCount;
  Cgi *cParms;
  ofstream ofOut;
  ifstream ifIn;

  iRc = CheckAuth(szPath, hInfo, READ_ACCESS);  // Check for authorization.
  if (iRc == ACCESS_DENIED)  // Send request for credentials.
    {
      sClient->Send("HTTP/1.1 401 \r\n");
      sClient->Send("Server: ");
      sClient->Send(szServerVer);
      sClient->Send("\r\n");
      szTmp = CreateDate(time(NULL));  // Create a date header.
      if (szTmp != NULL)
        {
          sClient->Send("Date: ");
          sClient->Send(szTmp);
          sClient->Send("\r\n");
          delete [] szTmp;
        }
      sprintf(szBuf, "WWW-Authenticate: Basic realm=\"%s\"\r\n", hInfo->szRealm);
      sClient->Send(szBuf);
      sClient->Send("Content-Type: text/html\r\n");
      sprintf(szBuf, "Content-Length: %d\r\n", strlen(sz401));
      sClient->Send(szBuf);
      sClient->Send("\r\n");
      sClient->Send(sz401);
      return 401;
    }
  else if (iRc == ACCESS_FAILED) // Send forbidden response.
    {
      sClient->Send("HTTP/1.1 403 Access Denied\r\n");
      sClient->Send("Server: ");
      sClient->Send(szServerVer);
      sClient->Send("\r\n");
      szTmp = CreateDate(time(NULL));  // Create a date header.
      if (szTmp != NULL)
        {
          sClient->Send("Date: ");
          sClient->Send(szTmp);
          sClient->Send("\r\n");
          delete [] szTmp;
        }
      sClient->Send("Content-Type: text/html\r\n");
      sprintf(szBuf, "Content-Length: %d\r\n", strlen(sz403));
      sClient->Send(szBuf);
      sClient->Send("\r\n");
      sClient->Send(sz403);
      return 403;
    }

  iRc = stat(szPath, &sBuf);
  if (iRc < 0)
    {
      iRsp = SendError(sClient, "Resource not found.", 404, HTTP_1_1, hInfo);
      return iRsp;
    }

  // Check If headers.
  iIfUnmod = IfUnmodSince(hInfo, sBuf.st_mtime);
  iIfMatch = IfMatch(hInfo, sBuf.st_mtime);
  iIfNone = IfNone(hInfo, sBuf.st_mtime);
            
  // Check to make sure any If headers are FALSE.
  // No match on etags or it's been modified or an etag did match.
  if ( (iIfMatch == FALSE) || (iIfUnmod == FALSE) || (iIfNone == FALSE) )
    {
      sClient->Send("HTTP/1.1 412 Precondition Failed\r\n");
      iRsp = 412;
    }
  // Go ahead and do the CGI.
  else
    {
      sClient->Send("HTTP/1.1 200 OK\r\n");
      iRsp = 200;
    }
      
  sClient->Send("Server: ");  // Standard server response.
  sClient->Send(szServerVer);
  sClient->Send("\r\n");
  szTmp = CreateDate(time(NULL));  // Create a date header.
  if (szTmp != NULL)
    {
      sClient->Send("Date: ");
      sClient->Send(szTmp);
      sClient->Send("\r\n");
      delete [] szTmp;
    }

  if (iRsp == 412)
    {
      hInfo->ulContentLength = 0;  // For the logfile.
      return iRsp;  // Don't send anything else.
    }

  // Execute the cgi program here.
  cParms = new Cgi();
  cParms->hInfo = hInfo;
  cParms->sClient = sClient;
  cParms->szProg = szPath;
  if (iMethod == POST)
    {
      // Grab the posted data.
      cParms->szOutput = NULL;
      tmpnam(szFile);
      strlwr(hInfo->szContentType);
      szPtr = strstr(hInfo->szContentType, "text/");
      if (szPtr != NULL)  // Receiving text data.
        {
          ofOut.open(szFile);
          iCount = 0;
          // Get the specified number of bytes.
          while (iCount < hInfo->ulContentLength)
            {
              i = sClient->RecvTeol();  // Keep eol for proper byte count.
              iCount += i;
              // Remove the end of line.
              while ((sClient->szOutBuf[i] == '\r') ||
                     (sClient->szOutBuf[i] == '\n')   )
                {
                  sClient->szOutBuf[i] = NULL;
                  i--;
                }
              ofOut << sClient->szOutBuf << endl;  // Write to temp file.
            }
        }
      else  // Binary data.
        {
          ofOut.open(szFile, ios::bin);  // Open in binary mode.
          iCount = 0;
          while (iCount < hInfo->ulContentLength)
            {
              i = sClient->Recv(hInfo->ulContentLength - iCount);
              iCount += i;
              ofOut.write(sClient->szOutBuf, i);
            }
        }
      ofOut.close();
      cParms->szPost = szFile;
    }

  ExecCgi(cParms);  // Run the cgi program.

  stat(cParms->szOutput, &sBuf);
  ifIn.open(cParms->szOutput);  // Open the output file.
  iCount = 0;
  ifIn.getline(szBuf, SMALLBUF, '\n');
  // Parse the cgi output for headers.
  while (szBuf[0] != NULL)
    {
      iCount += strlen(szBuf) + 2;
      szVal = strchr(szBuf, ':');
      if (szVal == NULL)
        {
          ifIn.getline(szBuf, SMALLBUF, '\n');
          continue;
        }
      *szVal = NULL;
      szVal++;
      strlwr(szBuf);
      // Look for and allow proper response headers.
      if (strcmp(szBuf, "cache-control") == 0)
        {
          sClient->Send("Cache-Control: ");
        }
      else if (strcmp(szBuf, "content-type") == 0)
        {
          sClient->Send("Content-Type: ");
        }
      else if (strcmp(szBuf, "content-base") == 0)
        {
          sClient->Send("Content-Base: ");
        }
      else if (strcmp(szBuf, "content-encoding") == 0)
        {
          sClient->Send("Content-Encoding: ");
        }
      else if (strcmp(szBuf, "content-language") == 0)
        {
          sClient->Send("Content-Language: ");
        }
      else if (strcmp(szBuf, "content-location") == 0)
        {
          sClient->Send("Content-Location: ");
        }
      else if (strcmp(szBuf, "etag") == 0)
        {
          sClient->Send("Etag: ");
        }
      else if (strcmp(szBuf, "expires") == 0)
        {
          sClient->Send("Expires: ");
        }
      else if (strcmp(szBuf, "from") == 0)
        {
          sClient->Send("From: ");
        }
      else if (strcmp(szBuf, "location") == 0)
        {
          sClient->Send("Location: ");
        }
      else if (strcmp(szBuf, "last-modified") == 0)
        {
          sClient->Send("Last-Modified: ");
        }
      else if (strcmp(szBuf, "vary") == 0)
        {
          sClient->Send("Vary: ");
        }
      else  // No match. Don't send this unknown header.
        {
          ifIn.getline(szBuf, SMALLBUF, '\n');
          continue;
        }
      sClient->Send(szVal);  // Send the parameter for the header line.
      sClient->Send("\r\n");
      ifIn.getline(szBuf, SMALLBUF, '\n');
    }
  ifIn.close();
  iCount += 2;  // The last CRLF isn't counted within the loop.
  sprintf(szBuf, "Content-Length: %d\r\n\r\n", sBuf.st_size - iCount);
  sClient->Send(szBuf);

  if (iMethod != HEAD) // Only send the entity if not HEAD.
    {
      hInfo->ulContentLength = sBuf.st_size - iCount;
      ifIn.open(cParms->szOutput, ios::bin);
      ifIn.seekg(iCount, ios::beg);
      while (!ifIn.eof())
        {
          ifIn.read(szBuf, SMALLBUF);
          i = ifIn.gcount();
          sClient->Send(szBuf, i);
        }
      ifIn.close();
    }
  else
    {
      hInfo->ulContentLength = 0;
    }

  // Remove the temporary files and memory.
  unlink(cParms->szOutput);
  delete [] (cParms->szOutput);
  if (cParms->szPost != NULL) unlink(cParms->szPost);
  delete cParms;

  return iRsp;
}

// ------------------------------------------------------------------
//
// CheckMethod
//
// Determine which method the client is sending. Remember
// that methods *ARE* case-sensitive, unlike most of HTTP/1.1.
//

int CheckMethod(char *szMethod)
{
  if (strcmp(szMethod, "GET") == 0)
    {
      return GET;
    }
  else if (strcmp(szMethod, "POST") == 0)
    {
      return POST;
    }
  else if (strcmp(szMethod, "HEAD") == 0)
    {
      return HEAD;
    }
  else if (strcmp(szMethod, "OPTIONS") == 0)
    {
      return OPTIONS;
    }
  else if (strcmp(szMethod, "PUT") == 0)
    {
      return PUT;
    }
  else if (strcmp(szMethod, "DELETE") == 0)
    {
      return DELETE;
    }
  else if (strcmp(szMethod, "TRACE") == 0)
    {
      return TRACE;
    }
  return UNKNOWN;
}

// --------------------------------------------------------
//
// MakeUnique()
//
// Create a unique filename in the specified directory with the
// specified extension.
//

char * MakeUnique(char *szDir, char *szExt)
{
  ULONG ulNum = 0;
  BOOL bNotUnique = TRUE;
  int iRc;
  char *szFileName;

  szFileName = new char[PATH_LENGTH];

  while (bNotUnique)
    {
      sprintf(szFileName, "%s%08d.%s", szDir, ulNum, szExt);
      iRc = open(szFileName, O_CREAT | O_EXCL | O_WRONLY | O_TEXT, S_IWRITE);
      if (iRc != -1)
        {
          // Success. This file didn't exist before.
          close(iRc);
          bNotUnique = FALSE;
          continue;
        }

      ulNum++;
      if (ulNum > 99999999)
        {
          delete [] szFileName;
          szFileName = NULL;
          bNotUnique = FALSE;
        }
    }
  return (szFileName);
}

// --------------------------------------------------------
//
// DoTrace
//
// Perform a HTTP trace on the request just received.
//

int DoTrace(Socket *sClient, Headers *hInfo)
{
  ofstream ofOut;
  char *szName, szBuf[SMALLBUF], *szTmp;
  struct stat sBuf;
  int iRc;
  BOOL bPersistent = TRUE;

  szName = tmpnam(NULL);  // Request temporary filename.
  ofOut.open(szName);
  if (! ofOut)
    {
      hInfo->RcvHeaders(sClient);
      bPersistent = hInfo->bPersistent;
      delete hInfo;
      SendError(sClient, "Server error.", 500, HTTP_1_1, hInfo);
      return bPersistent;
    }

  while (sClient->szOutBuf[0] != NULL)
    {
      ofOut << sClient->szOutBuf << endl;
      // Look for Connection header.
      szTmp = strchr(sClient->szOutBuf, ':');
      if (szTmp != NULL)
        {
          *szTmp = NULL;
          szTmp++;
          if (stricmp(sClient->szOutBuf, "connection") == 0)
            {
              sscanf(szTmp, "%s", szBuf);
              if (stricmp(szBuf, "close") == 0)
                {
                  bPersistent = FALSE;
                }
            }
        }
      sClient->RecvTeol(NO_EOL);
    }
  ofOut.close();
  iRc = stat(szName, &sBuf);
  if (iRc == 0)
    {
      sClient->Send("HTTP/1.1 200 \r\n");
      sClient->Send("Server: ");
      sClient->Send(szServerVer);
      sClient->Send("\r\n");
      szTmp = CreateDate(time(NULL));  // Create a date header.
      if (szTmp != NULL)
        {
          sClient->Send("Date: ");
          sClient->Send(szTmp);
          sClient->Send("\r\n");
          delete [] szTmp;
        }
      sClient->Send("Content-Type: text/http\r\n");

      hInfo->ulContentLength = sBuf.st_size; // Save the entity size.
      sprintf(szBuf, "Content-Length: %d\r\n", sBuf.st_size);
      sClient->Send(szBuf);
      sClient->Send("\r\n");

      iRc = sClient->SendText(szName);
    }
  unlink(szName);
  return bPersistent;
}

// --------------------------------------------------------
//
// DoOptions
//
// Figure out the options available for the specified resource.
//

int DoOptions(Socket *sClient, char *szPath, Headers *hInfo, int iType)
{
  char *szTmp;

  sClient->Send("HTTP/1.1 200 OK \r\n");
  sClient->Send("Server: ");
  sClient->Send(szServerVer);
  sClient->Send("\r\n");
  szTmp = CreateDate(time(NULL));  // Create a date header.
  if (szTmp != NULL)
    {
      sClient->Send("Date: ");
      sClient->Send(szTmp);
      sClient->Send("\r\n");
      delete [] szTmp;
    }
  sClient->Send("Accept-Encodings: \r\n");

  if (strcmp(szPath, "*") == 0)  // General options requested.
    {
      sClient->Send("Allow: GET, HEAD, POST, PUT, DELETE, TRACE\r\n");
      sClient->Send("Accept-Ranges: bytes\n\n");
    }
  else if (iType == GET)
    {
      sClient->Send("Allow: GET, HEAD \r\n");
      sClient->Send("Accept-Ranges: bytes\n\n");
    }
  else if (iType == POST)
    {
      sClient->Send("Allow: POST \r\n");
    }
  sClient->Send("\r\n");
  hInfo->ulContentLength = 0;

  return 200;
}

// --------------------------------------------------------
//
// DoPut
//
// Save the entity sent as the specified URI.
//

int DoPut(Socket *sClient, Headers *hInfo, char *szPath, char *szCgi)
{
  struct stat sBuf;
  char *szTmp,
       *szExt,
       *szLoc,
       szBuf[PATH_LENGTH],
       szFile[PATH_LENGTH];
  ofstream ofTmp;
  int iRsp = 200,
      iRc,
      iType,
      iIfUnmod,
      iIfMatch,
      iIfNone,
      i, j;
  unsigned long ulRc;
  BOOL bChunked = FALSE;

  // Figure out where to store it.
  if (szPath != NULL)
    {
      szLoc = szPath;
    }
  else if (szCgi != NULL)
    {
      szLoc = szCgi;
    }
  else // Error. Cannot resolve location.
    {
      SendError(sClient, "Location not found.", 404, HTTP_1_1, hInfo);
      return 404;
    }

  iRc = CheckAuth(szLoc, hInfo, WRITE_ACCESS);  // Check for authorization.
  if (iRc == ACCESS_DENIED)  // Send request for credentials.
    {
      sClient->Send("HTTP/1.1 401 \r\n");
      sClient->Send("Server: ");
      sClient->Send(szServerVer);
      sClient->Send("\r\n");
      szTmp = CreateDate(time(NULL));  // Create a date header.
      if (szTmp != NULL)
        {
          sClient->Send("Date: ");
          sClient->Send(szTmp);
          sClient->Send("\r\n");
          delete [] szTmp;
        }
      sprintf(szBuf, "WWW-Authenticate: Basic realm=\"%s\"\r\n", hInfo->szRealm);
      sClient->Send(szBuf);
      sClient->Send("Content-Type: text/html\r\n");
      sprintf(szBuf, "Content-Length: %d\r\n", strlen(sz401));
      sClient->Send(szBuf);
      sClient->Send("\r\n");
      sClient->Send(sz401);
      return 401;
    }
  else if (iRc == ACCESS_FAILED) // Send forbidden response.
    {
      sClient->Send("HTTP/1.1 403 Access Denied\r\n");
      sClient->Send("Server: ");
      sClient->Send(szServerVer);
      sClient->Send("\r\n");
      szTmp = CreateDate(time(NULL));  // Create a date header.
      if (szTmp != NULL)
        {
          sClient->Send("Date: ");
          sClient->Send(szTmp);
          sClient->Send("\r\n");
          delete [] szTmp;
        }
      sClient->Send("Content-Type: text/html\r\n");
      sprintf(szBuf, "Content-Length: %d\r\n", strlen(sz403));
      sClient->Send(szBuf);
      sClient->Send("\r\n");
      sClient->Send(sz403);
      return 403;
    }

  if (hInfo->szRange != NULL)  // Range not allowed for PUT.
    {
      SendError(sClient, "Range header not accepted for PUT.", 501, 
                HTTP_1_1, hInfo);
      return 501;
    }
  if (hInfo->szIfModSince != NULL)  // If-Modified-Since
    {                               // not allowed for PUT.
      SendError(sClient, 
                "If-Modified-Since header not accepted for PUT.", 
                501, HTTP_1_1, hInfo);
      return 501;
    }

  // Now check the If headers.
  iIfUnmod = IfUnmodSince(hInfo, sBuf.st_mtime);
  iIfMatch = IfMatch(hInfo, sBuf.st_mtime);
  iIfNone = IfNone(hInfo, sBuf.st_mtime);
  if ((iIfUnmod == FALSE) || (iIfMatch == FALSE) || (iIfNone == FALSE))
    {
      SendError(sClient, "Precondition failed.", 412, HTTP_1_1, hInfo);
      return 412;
    }

  // Accept the resource.
  if (hInfo->bChunked == TRUE)
    {
      bChunked = TRUE;
    }
  else if (hInfo->szContentLength == NULL) // They must supply a length.
    {
      SendError(sClient, "Length required.", 411, HTTP_1_1, hInfo);
      return 411;
    }
  tmpnam(szFile);
  ofTmp.open(szFile, ios::binary);
  if (! ofTmp)
    {
      SendError(sClient, "Local processing error.", 500, HTTP_1_1, hInfo);
      return 500;
    }

  if (bChunked == TRUE)
    {
      GetChunked(sClient, ofTmp, hInfo);
    }
  else  // Use Content-Length instead.
    {
      i = 0;
      while (i < hInfo->ulContentLength)  // The actual resource.
        {
          j = sClient->Recv(hInfo->ulContentLength - i);
          i += j;
          ofTmp.write(sClient->szOutBuf, j);
        }
    }
  ofTmp.close();

  iRc = stat(szLoc, &sBuf);  // Check for the resource.

  ulRc = DosCopy(szFile, szLoc, DCPY_EXISTING);
  unlink(szFile);  // Remove the temporary always.
  if (ulRc != 0)
    {
      SendError(sClient, "Local processing error.", 500, HTTP_1_1, hInfo);
      return 500;
    }

  if (iRc == 0)  // File exists. Overwrite it.
    {
      sClient->Send("HTTP/1.1 204 No Content\r\n");
      iRsp = 204;
    }
  else // New resource
    {
      sClient->Send("HTTP/1.1 201 Created\r\n");
      iRsp = 201;
    }
  sClient->Send("Server: ");
  sClient->Send(szServerVer);
  sClient->Send("\r\n");
  szTmp = CreateDate(time(NULL));  // Create a date header.
  if (szTmp != NULL)
    {
      sClient->Send("Date: ");
      sClient->Send(szTmp);
      sClient->Send("\r\n");
      delete [] szTmp;
    }
  sClient->Send("\r\n");

  hInfo->ulContentLength = 0;
  return iRsp;
}


// ------------------------------------------------------------------
//
// DoDelete
//
// This function checks to see if it can delete the resource
// specified by the client.
//

int DoDelete(Socket *sClient, char *szPath, char *szCgi, Headers *hInfo)
{
  struct stat sBuf;
  char *szTmp,
       *szExt,
       szBuf[PATH_LENGTH],
       szFile[PATH_LENGTH];
  ofstream ofTmp;
  int iRsp = 200,
      iRc,
      iType,
      iIfMod,
      iIfUnmod,
      iIfMatch,
      iIfNone;

  iRc = CheckAuth(szPath, hInfo, WRITE_ACCESS);  // Check for authorization.
  if (iRc == ACCESS_DENIED)  // Send request for credentials.
    {
      sClient->Send("HTTP/1.1 401 \r\n");
      sClient->Send("Server: ");
      sClient->Send(szServerVer);
      sClient->Send("\r\n");
      szTmp = CreateDate(time(NULL));  // Create a date header.
      if (szTmp != NULL)
        {
          sClient->Send("Date: ");
          sClient->Send(szTmp);
          sClient->Send("\r\n");
          delete [] szTmp;
        }
      sprintf(szBuf, "WWW-Authenticate: Basic realm=\"%s\"\r\n", hInfo->szRealm);
      sClient->Send(szBuf);
      sClient->Send("Content-Type: text/html\r\n");
      sprintf(szBuf, "Content-Length: %d\r\n", strlen(sz401));
      sClient->Send(szBuf);
      sClient->Send("\r\n");
      sClient->Send(sz401);
      return 401;
    }
  else if (iRc == ACCESS_FAILED) // Send forbidden response.
    {
      sClient->Send("HTTP/1.1 403 Access Denied\r\n");
      sClient->Send("Server: ");
      sClient->Send(szServerVer);
      sClient->Send("\r\n");
      szTmp = CreateDate(time(NULL));  // Create a date header.
      if (szTmp != NULL)
        {
          sClient->Send("Date: ");
          sClient->Send(szTmp);
          sClient->Send("\r\n");
          delete [] szTmp;
        }
      sClient->Send("Content-Type: text/html\r\n");
      sprintf(szBuf, "Content-Length: %d\r\n", strlen(sz403));
      sClient->Send(szBuf);
      sClient->Send("\r\n");
      sClient->Send(sz403);
      return 403;
    }

  if (hInfo->szRange != NULL)  // Range not allowed for DELETE.
    {
      SendError(sClient, "Range header not accepted for DELETE.", 
                501, HTTP_1_1, hInfo);
      return 501;
    }
  if (hInfo->szIfModSince != NULL)  // If-Modified-Since 
    {                               // not allowed for DELETE.
      SendError(sClient, 
                "If-Modified-Since header not accepted for DELETE.", 
                501, HTTP_1_1, hInfo);
      return 501;
    }

  // Now check the If headers.
  iIfUnmod = IfUnmodSince(hInfo, sBuf.st_mtime);
  iIfMatch = IfMatch(hInfo, sBuf.st_mtime);
  iIfNone = IfNone(hInfo, sBuf.st_mtime);
  if ((iIfUnmod == FALSE) || (iIfMatch == FALSE) || (iIfNone == FALSE))
    {
      SendError(sClient, "Precondition failed.", 412, HTTP_1_1, hInfo);
      return 412;
    }

  if (szDeleteDir != NULL)  // Save the deleted resource.
    {
      // Use the same file extension as the current resource.
      szExt = strrchr(szPath, '.');
      if (szExt != NULL)
        {
          szExt++;
        }
      else
        {
          szExt = "del";
        }
      szTmp = MakeUnique(szDeleteDir, szExt);
      DosCopy(szPath, szTmp, DCPY_EXISTING);
      delete [] szTmp;
    }
  iRc = unlink(szPath);
  if (iRc == 0) // Resource deleted.
    {
      sClient->Send("HTTP/1.1 204 No Content\r\n");
      iRsp = 204;
    }
  else // Delete failed.
    {
      sClient->Send("HTTP/1.1 500 Server Error\r\n");
      iRsp = 500;
    }
  sClient->Send("Server: ");
  sClient->Send(szServerVer);
  sClient->Send("\r\n");
  szTmp = CreateDate(time(NULL));  // Create a date header.
  if (szTmp != NULL)
    {
      sClient->Send("Date: ");
      sClient->Send(szTmp);
      sClient->Send("\r\n");
      delete [] szTmp;
    }
  sClient->Send("\r\n");
  hInfo->ulContentLength = 0;
  return iRsp;
}

// ------------------------------------------------------------------
//
// IfModSince
//
// Check whether the file had been modifed since the date
// given by the client.
//

int IfModSince(Headers *hInfo, time_t ttMtime)
{
  if (hInfo->szIfModSince != NULL)
    {
      if ((hInfo->ttIfModSince > 0) && (hInfo->ttIfModSince < ttMtime))
        {
          return TRUE;
        }
      else
        {
          return FALSE;
        }
    }
  return TRUE;  // Default is TRUE.
}

// ------------------------------------------------------------------
//
// IfUnmodSince
//
// Check whether the file has not been modified since the date
// given by the client.
//

int IfUnmodSince(Headers *hInfo, time_t ttMtime)
{
  if (hInfo->szIfUnmodSince != NULL)
    {
      if ((hInfo->ttIfUnmodSince > 0) && (hInfo->ttIfUnmodSince > ttMtime))
        {
          return TRUE;
        }
      else
        {
          return FALSE;
        }
    }
  return TRUE;  // Default is TRUE.
}

// ------------------------------------------------------------------
//
// IfMatch
//
// Check the etag of the resource against that given by the client
// for a match.
//

int IfMatch(Headers *hInfo, time_t ttMtime)
{
  int iIfMatch = TRUE,
      i;
  char *szBuf,
       szEtagStar[] = "*";

  // Check to see if any etags match.
  if (hInfo->szIfMatch != NULL)
    {
      iIfMatch = FALSE; // We fail unless we match.
      szBuf = new char[SMALLBUF];
      sprintf(szBuf, "\"%d\"", ttMtime);
      for (i = 0; hInfo->szIfMatchEtags[i] != NULL; i++)
        {
          if (strcmp(hInfo->szIfMatchEtags[i], szBuf) == 0)
            {
              iIfMatch = TRUE;
              break;
            }
          if (strcmp(hInfo->szIfMatchEtags[i], szEtagStar) == 0)
            {
              iIfMatch = TRUE;
              break;
            }
        }
      delete [] szBuf;
    }
  return iIfMatch;
}

// ------------------------------------------------------------------
//
// IfNone
//
// Check to make sure no etags match the resource.
//

int IfNone(Headers *hInfo, time_t ttMtime)
{
  int iIfNone = TRUE,
      i;
  char *szBuf,
       szEtagStar[] = "*";

  // Check to see if any of the If-None-Match etags match
  if (hInfo->szIfNoneMatch != NULL)
    {
      iIfNone = TRUE;  // We're ok unless we match.
      szBuf = new char[SMALLBUF];
      sprintf(szBuf, "\"%d\"", ttMtime);
      for (i = 0; hInfo->szIfNoneMatchEtags[i] != NULL; i++)
        {
          if (strcmp(hInfo->szIfNoneMatchEtags[i], szBuf) == 0)
            {
              iIfNone = FALSE;
              break;
            }
          if (strcmp(hInfo->szIfNoneMatchEtags[i], szEtagStar) == 0)
            {
              iIfNone = FALSE;
              break;
            }
        }
      delete [] szBuf;
    }
  return iIfNone;
}

// ------------------------------------------------------------------
//
// IfRange
//
// Find out whether the If-Range tag matches the resource.
//

int IfRange(Headers *hInfo, time_t ttMtime)
{
  char *szBuf;
  time_t ttDate;

  // Check the If-Range header. We must have Range also to be valid.
  if ((hInfo->szIfRange != NULL) && (hInfo->szRange != NULL))
    {
      // Figure out whether it is an etag or date.
      if ((hInfo->szIfRange[0] == '"') || (hInfo->szIfRange[2] == '"'))
        {
          szBuf = new char[SMALLBUF];                // An etag.
          sprintf(szBuf, "\"%d\"", ttMtime);
          if (strcmp(szBuf, hInfo->szIfRange) == 0)
            {
              delete [] szBuf;
              return TRUE;  // Match, send them the resource.
            }
          delete [] szBuf;
        }
      else
        {
          ttDate = ConvertDate(hInfo->szIfRange); // We found a date.
          if (ttDate >= ttMtime)
            {
              return TRUE;  // Match, send them the resource.
            }
        }
    }

  return FALSE; // No match.
}

// ------------------------------------------------------------------
//
// SendByteRange
//
// Send the given byte ranges back to the client.
//

int SendByteRange(Socket *sClient, Headers *hInfo, char *szPath,
                  struct stat *sBuf, int iType, int iMethod)
{
  ifstream ifIn;
  int iBytes, iCount, iLen,
      i, j;
  char *szBuf, *szBoundary;

  szBuf = new char[SMALLBUF];

  if (hInfo->iRangeNum == 1)  // Simple response, only one part.
    {
      iLen = hInfo->rRanges[0].iEnd - hInfo->rRanges[0].iStart + 1;
      sprintf(szBuf, "Content-Length: %d\r\n", iLen);
      sClient->Send(szBuf);
      sprintf(szBuf, "Content-Type: %s\r\n", eExtMap[iType].szType);
      sClient->Send(szBuf);
      sClient->Send("\r\n");

      if (iMethod == HEAD)  // Don't send an entity.
        {
          delete [] szBuf;
          hInfo->ulContentLength = 0;
          return 0;
        }

      ifIn.open(szPath, ios::binary);  // Open the file, binary mode.
      ifIn.seekg(hInfo->rRanges[0].iStart, ios::beg);
      iCount = 0;
      while (iCount < iLen)
        {
          ifIn.read(szBuf, (SMALLBUF < iLen-iCount ? SMALLBUF : iLen-iCount));
          iBytes = ifIn.gcount();
          iCount += iBytes;
          sClient->Send(szBuf, iBytes);
        }
      ifIn.close();
    }
  else  // Do a multi-part MIME type.
    {
      szBoundary = new char[70];
      srand(sBuf->st_mtime);
      for (i = 0; i < 68; i++)
        {
          j = rand();
          szBoundary[i] = szMime[ j % iNumMime ];
        }
      szBoundary[69] = NULL;

      sprintf(szBuf, "Content-Type: multipart/byteranges; boundary=\"%s\"\r\n",
              szBoundary);
      sClient->Send(szBuf);

      if (iMethod == HEAD)  // Don't send an entity.
        {
          delete [] szBuf;
          hInfo->ulContentLength = 0;
          return 0;
        }

      ifIn.open(szPath, ios::binary);  // Open the file, binary mode.

      for (i = 0; i < hInfo->iRangeNum; i++)
        {
          sClient->Send("\r\n--");     // The boundary marker first.
          sClient->Send(szBoundary);
          sClient->Send("\r\n");
          sprintf(szBuf, "Content-Type: %s\r\n", eExtMap[iType].szType);
          sClient->Send(szBuf);      // Now content-type.
          sprintf(szBuf, "Content-Range: bytes %d-%d/%d\r\n\r\n",
                  hInfo->rRanges[i].iStart, hInfo->rRanges[i].iEnd, sBuf->st_size);
          sClient->Send(szBuf);      // Now content-range.

          ifIn.seekg(hInfo->rRanges[i].iStart, ios::beg);
          iLen = hInfo->rRanges[i].iEnd - hInfo->rRanges[i].iStart + 1;
          iCount = 0;
          // Read the specified number of bytes.
          while (iCount < iLen)
            {
              ifIn.read(szBuf, (SMALLBUF < iLen-iCount ? SMALLBUF : iLen-iCount));
              iBytes = ifIn.gcount();
              iCount += iBytes;
              sClient->Send(szBuf, iBytes);
            }
        }
      sClient->Send("\r\n--");     // The ending boundary marker.
      sClient->Send(szBoundary);
      sClient->Send("--\r\n");
      delete [] szBoundary;
      ifIn.close();
    }

  delete [] szBuf;
  return 0;
}

// ------------------------------------------------------------------
//
// GetChunked
//
// Receive the entity using the chunked method.
//

int GetChunked(Socket *sClient, ofstream &ofOut, Headers *hInfo)
{
  BOOL bNotDone = TRUE;
  char *szPtr;
  int iBytes, i, j, l, iFactor;

  while (bNotDone == TRUE)
    {
      sClient->RecvTeol(NO_EOL);  // Grab a line. Should have chunk size.
      if (strcmp(sClient->szOutBuf, "0") == 0)
        {
          bNotDone = FALSE;  // The end of the chunks.
          continue;
        }

      szPtr = strchr(sClient->szOutBuf, ';');
      if (szPtr != NULL) *szPtr = NULL;  // Mark end of chunk-size.

      l = strlen(sClient->szOutBuf); // Find last hex digit.
      l--;
      iBytes = 0;
      iFactor = 1;
      // Convert to decimal bytes.
      while (l >= 0)
        {
          iBytes += iFactor * Hex2Dec(sClient->szOutBuf[l]);
          l--;
          iFactor *= 16;
        }
      i = 0;
      // Now receive the specified number of bytes.
      while (i < iBytes)
        {
          j = sClient->Recv(iBytes - i);     // Some data.
          i += j;                            // Total the bytes.
          ofOut.write(sClient->szOutBuf, j); // Save to disk.
        }
      sClient->RecvTeol(NO_EOL);  // Discard end of chunk marker.
    }

  // Now consume anything in the footer.
  hInfo->RcvHeaders(sClient);  
  return 0;
}

// ------------------------------------------------------------------
//
// Hex2Dec
//
// Convert a hex character to a decimal character.
//

int Hex2Dec(char c)
{
  switch (c)
    {
      case 'A':
      case 'a':
        return 10;
      case 'B':
      case 'b':
        return 11;
      case 'C':
      case 'c':
        return 12;
      case 'D':
      case 'd':
        return 13;
      case 'E':
      case 'e':
        return 14;
      case 'F':
      case 'f':
        return 15;
      default:
        return (c - 48);
    }
}
      
// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------