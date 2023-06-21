//
// WWW Server  File: http10.cpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#include <stdlib.h>
#include <string.h>
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
#include "http11.hpp"
#include "util.hpp"
#include "cgi.hpp"

// ------------------------------------------------------------------
//
// DoHttp10()
//
// This function handles our HTTP/1.0 requests.
//

void DoHttp10(Socket *sClient, char *szMethod, char *szUri)
{
  int iRc,
      iRsp,
      iType,
      iMethod;
  char *szReq,
       *szCgi,
       *szPath,
       *szTmp,
       *szSearch;
  Headers *hInfo;
  long lBytes = 0;
  BOOL bExec = FALSE,
       bCgi = FALSE;

  szReq = strdup(sClient->szOutBuf);  // Save the request line.
  hInfo = new Headers();
  iRsp = 200;
  szSearch = NULL;

  hInfo->RcvHeaders(sClient);  // The request headers.
  hInfo->szMethod = strdup(szMethod);  // Save a few items.
  hInfo->szUri = strdup(szUri);
  hInfo->szVer = strdup(HTTP_1_0);

  iMethod = CheckMethod(szMethod);

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

  DeHexify(szUri);  // Remove any escape sequences.
  szPath = ResolvePath(szUri); // Check for path match.
  szCgi = ResolveExec(szUri);  // Check for exec match.
  // Any POST request.
  if (iMethod == POST)
    {
      iRsp = DoExec(sClient, iMethod, szCgi, hInfo);
    }
  // A GET or HEAD to process as a CGI request.
  else if ( (bCgi == TRUE) && ((iMethod == GET) || (iMethod == HEAD)) )
    {
      iRsp = DoExec(sClient, iMethod, szCgi, hInfo);
    }
  // The default, probably a simple GET or HEAD.
  else if (szPath != NULL)
    {
      iRsp = DoPath(sClient, szMethod, szPath, szSearch, hInfo, szUri);
    }
  else  // Error Condition.
    {
      iRsp = SendError(sClient, "Resource not found.", 404, HTTP_1_0, hInfo);
    }

  DeHexify(szReq);
  WriteToLog(sClient, szReq, iRsp, hInfo->ulContentLength);
  sClient->Close();
  delete [] szReq;
  delete hInfo;
  if ((szSearch != NULL) && (bCgi == FALSE))
    {
      unlink(szPath);      // The temporary search file.
      delete [] szSearch;
    }
  if (szPath) delete [] szPath;
  if (szCgi) delete [] szCgi;
 

  return;
}

// ------------------------------------------------------------------
//
// DoPath()
//
// This function checks to see if it can return the requested
// document back to the client.
//

int DoPath(Socket *sClient, char *szMethod, char *szPath, 
           char *szSearch, Headers *hInfo, char *szUri)
{
  struct stat sBuf;
  char *szTmp,
       szBuf[PATH_LENGTH],
       szFile[PATH_LENGTH];
  ofstream ofTmp;
  int iRsp = 200,
      iRc,
      iType;

  if (szPath[strlen(szPath) - 1] == '/')
    {
      strcat(szPath, szWelcome);  // Prepend default welcome file.
    }

  iRc = CheckAuth(szPath, hInfo, READ_ACCESS);  // Check for authorization.
  if (iRc == ACCESS_DENIED)  // Send request for credentials.
    {
      sClient->Send("HTTP/1.0 401\r\n");
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
      sClient->Send("\r\n");
      sClient->Send(sz401);
      return 401;
    }
  else if (iRc == ACCESS_FAILED) // Send forbidden response.
    {
      cerr << "Auth says ACCESS_FAILED" << endl;
      sClient->Send("HTTP/1.0 403 Access Denied\r\n");
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
      sClient->Send("\r\n");
      sClient->Send(sz403);
      return 403;
    }

  if (szSearch != NULL)  // Do an index search.
    {
      iRc = Index(szPath, szSearch, szFile, szUri);
      if (iRc != 0)
        {
          iRc = SendError(sClient, "Resource not found.", 404, 
                          HTTP_1_0, hInfo);
          return iRc;
        }
      strcpy(szPath, szFile);
    }

  iRc = stat(szPath, &sBuf);
  if (iRc == 0)  // File exists. Send it.
    {
      // Check the last modified timestamp against the Request header.
      if ((hInfo->ttIfModSince > 0) && (hInfo->ttIfModSince < sBuf.st_mtime))
        {
          sClient->Send("HTTP/1.0 304\r\n");  // No need to update.
          szPath[0] = NULL;
          iRsp = 304;
        }
      else
        {
          sClient->Send("HTTP/1.0 200\r\n");
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
      szTmp = CreateDate(sBuf.st_mtime);  // The last modified time header.
      if (szTmp != NULL)
        {
          sClient->Send("Last-Modified: ");
          sClient->Send(szTmp);
          sClient->Send("\r\n");
          delete [] szTmp;
        }

      if (iRsp == 304) return 304;  // Don't send anything else.

      if (szSearch != NULL)    // Force search results to text/html type.
        {
          iType = FindType("x.html");
        }
      else
        {
          iType = FindType(szPath);
        }
      sprintf(szBuf, "Content-Type: %s\r\n", eExtMap[iType].szType);
      sClient->Send(szBuf);

      sprintf(szBuf, "Content-Length: %d\r\n", sBuf.st_size);
      sClient->Send(szBuf);
      sClient->Send("\r\n");

      if (strcmp(szMethod, "GET") == 0)  // Don't send unless GET.
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
    }
  if (iRc < 0)
    {
      iRsp = SendError(sClient, "Resource not found.", 404, 
                       HTTP_1_0, hInfo);
    }


  return iRsp;
}    

// ------------------------------------------------------------------
//
// DoExec
//
// This function executes our CGI scripts.
//

int DoExec(Socket *sClient, int iMethod, char *szPath, Headers *hInfo)
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
      i,
      iCount;
  Cgi *cParms;
  ofstream ofOut;
  ifstream ifIn;

  iRc = CheckAuth(szPath, hInfo, READ_ACCESS);  // Check for authorization.
  if (iRc == ACCESS_DENIED)  // Send request for credentials.
    {
      sClient->Send("HTTP/1.0 401 \r\n");
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
      sClient->Send("\r\n");
      sClient->Send(sz401);
      return 401;
    }
  else if (iRc == ACCESS_FAILED) // Send forbidden response.
    {
      sClient->Send("HTTP/1.0 403 Access Denied\r\n");
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

  sClient->Send("HTTP/1.0 200 OK\r\n");
  iRsp = 200;
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
      sClient->Send("Content-Length: 0\r\n\r\n");
      hInfo->ulContentLength = 0;  // For the logfile.
      return iRsp;  // Don't send anything else.
    }

  // Execute the cgi program here.
  cParms = new Cgi();
  cParms->hInfo = hInfo;
  cParms->sClient = sClient;
  cParms->szProg = szPath;
  if ((iMethod == GET) || (iMethod == HEAD))
    {
      cParms->szPost = NULL;
      cParms->szOutput = NULL;
    }
  else if (iMethod == POST)
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
              i--;  // Allow for C style indexing.
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
      if (strcmp(szBuf, "content-type") == 0)
        {
          sClient->Send("Content-Type: ");
        }
      else if (strcmp(szBuf, "content-encoding") == 0)
        {
          sClient->Send("Content-Encoding: ");
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
  if (cParms->szPost != NULL) unlink(cParms->szPost);
  delete cParms;

  return iRsp;
}

// ------------------------------------------------------------------
//
// FindType()
//
// This function compares the file extension of the document
// being sent to determine the proper MIME type to return
// to the client.
//

int FindType(char *szPath)
{
  char *szTmp;
  int i;

  szTmp = szPath + strlen(szPath) - 1;

  while ((*szTmp != '.') && (szTmp != szPath) && (*szTmp != '/'))
    {
      szTmp--;
    }

  szTmp++;  // Advance past the '.' or '/'.

  for (i = 0; i < iNumTypes; i++)
    {
      if (stricmp(eExtMap[i].szExt, szTmp) == 0)
        {
          return i;
        }
    }
  return 0;
}

// ------------------------------------------------------------------
//
// ResolvePath()
//
// Resolve the path given by the client to the real path
// by checking against the aliases.
//

char * ResolvePath(char *szUri)
{
  int i;
  char *szRest,
       *szRoot;
  BOOL bFound = FALSE;

  if (strcmp(szUri, "/") == 0)  // They asked for the root directory doc.
    {
      szRoot = strdup(szServerRoot);
      return szRoot;
    }

  // Now isolate the first component of the requested path.
  szRest = szUri;
  szRoot = new char[PATH_LENGTH];

  szRoot[0] = *szRest;
  szRest++;  // Advance past the initial '/'.
  i = 1;
  while ((*szRest != '/') && (*szRest != '\\') && (*szRest))
    {
      szRoot[i] = *szRest;
      i++;
      szRest++;
    }

  // Now we have the first component.
  szRoot[i] = NULL;
  if (*szRest != NULL) szRest++;  // Advance past the '/'.

  // Compare it to our list of aliases.
  for (i = 0; i < iNumPathAliases; i++)
    {
      // Case insensitive comparison.
      if (stricmp(szRoot, pAliasPath[i].szAlias) == 0)
        {
          memset(szRoot, 0, PATH_LENGTH);
          sprintf(szRoot, "%s%s", pAliasPath[i].szTrue, szRest);
          bFound = TRUE;
          break;
        }
    }

  if (bFound == TRUE) return (szRoot);  // Found.

  // Give them a path based on the default root.
  if (*szRest == NULL)
    {
      if (*szUri == '/')
        {
          szRest = szUri + 1;
        }
      else
        {
          szRest = szUri;
        }
      memset(szRoot, 0, PATH_LENGTH);
      sprintf(szRoot, "%s%s", szServerRoot, szRest);
      return (szRoot);
    }

  return(NULL);
}

// ------------------------------------------------------------------
//
// ResolveExec()
//
// Resolve the given exec path to the real path.
//

char * ResolveExec(char *szUri)
{
  int i;
  char *szRest,
       *szRoot;
  BOOL bFound = FALSE;

  szRest = szUri;
  szRoot = new char[PATH_LENGTH];

  szRoot[0] = *szRest;
  szRest++;  // Advance past the initial '/'.
  i = 1;
  while ((*szRest != '/') && (*szRest != '\\') && (*szRest))
    {
      szRoot[i] = *szRest;
      i++;
      szRest++;
    }

  szRoot[i] = NULL;
  if (*szRest != NULL) szRest++;  // Advance past the '/'.

  // Compare to the list of exec path aliases.
  for (i = 0; i < iNumExecAliases; i++)
    {
      if (stricmp(szRoot, pAliasExec[i].szAlias) == 0)
        {
          memset(szRoot, 0, PATH_LENGTH);
          sprintf(szRoot, "%s%s", pAliasExec[i].szTrue, szRest);
          bFound = TRUE;
          break;
        }
    }

  // Return true if found. NULL otherwise.
  if (bFound == TRUE) return (szRoot);
  delete [] szRoot;
  return(NULL);
}

// ------------------------------------------------------------------
//
// SendError()
//
// Send an error message back to the client.
//

int SendError(Socket *sClient, char *szReason, int iCode, char *szVer,
              Headers *hInfo)
{
  struct stat sBuf;
  ofstream ofTmp;
  char *szTmp,
       szBuf[PATH_LENGTH];
  int iRc;
 
  szTmp = tmpnam(NULL);
  ofTmp.open(szTmp);
  if (! ofTmp)
    {
      sClient->Send(sz500);  // Unable to get temp file, fail.
      return 500;
    }
  // Write the temp file with the info.
  ofTmp << "<!doctype html public \"-//IETF//DTD HTML 2.0//EN\">" << endl;
  ofTmp << "<html><head>" << endl;
  ofTmp << "<title>Error</title></head>" << endl;
  ofTmp << "<body><h2>Error...</h2>Your request could not be honored.<hr><b>" << endl;
  ofTmp << szReason << endl;
  ofTmp << "</b><hr><em>HTTP Response Code:</em> " << iCode << "<br>" << endl;
  ofTmp << "<em>From server at:</em> " << szHostName << "<br>" << endl;
  ofTmp << "<em>Running:</em> " << szServerVer << "</body></html>" << endl;
  ofTmp.close();

  sprintf(szBuf, "%s %d\r\n", szVer, iCode);
  sClient->Send(szBuf);
  sClient->Send("Server: ");
  sClient->Send(szServerVer);
  sClient->Send("\r\nContent-Type: text/html\r\n");
  iRc = stat(szTmp, &sBuf);
  if (iRc == 0)
    {
      hInfo->ulContentLength = sBuf.st_size;
      sprintf(szBuf, "Content-Length: %d\r\n", sBuf.st_size);
      sClient->Send(szBuf);
      sClient->Send("\r\n");
      sClient->SendText(szTmp);  // Send the file.
    }
  else
    {
      hInfo->ulContentLength = 0;
      strcpy(szBuf, "Content-Length: 0\r\n");
      sClient->Send(szBuf);
      sClient->Send("\r\n");
    }
  unlink(szTmp);             // Get rid of it.
  return iCode;
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------