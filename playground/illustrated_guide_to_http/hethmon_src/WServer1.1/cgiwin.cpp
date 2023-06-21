//
// WWW Server  File: cgiwin.cpp
//
//
// Copyright 1996-1997 Paul S. Hethmon
//
// Prepared for the book "Illustrated Guide to HTTP"
//

#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <process.h>
#include <iostream.h>
#include <fstream.h>
#include <sys\stat.h>

#include <windows.h>

#ifdef __IBMCPP__
  #include <builtin.h>
#endif

#include "defines.hpp"
#include "config.hpp"
#include "cgi.hpp"

#define SMALLBUF 4196

// ------------------------------------------------------------------

volatile int iCgiLock = 0;  // Ram semaphore for CGI access.

// The environment variables passed to the cgi process.
char szServerSoftware[64],
     szServerName[64],
     szGatewayInterface[64],
     szServerProtocol[64],
     szServerPort[64],
     szRequestMethod[64],
     szScriptName[64],
     * szQueryString,
     szRemoteHost[64],
     szRemoteAddr[64],
     szAuthType[64],
     szRemoteUser[64],
     szContentType[64],
     szContentLength[64];

// ------------------------------------------------------------------
//
// ExecCgi
//
// This function executes the specified cgi program passing it the
// necessary arguments. It then returns the output to the caller
// for them to return it to the client. We protect this function
// internally with a ram semaphore since there is only one stdin/stdout
// stream for all of our threads.
//

int ExecCgi(Cgi *cParms)
{
  HANDLE pIn[2], pOut[2];
  DWORD iNum;
  int iRc, iPost, iRead,
      iEnvLen;
  char szBuf[SMALLBUF],
       *szTmp,
       *szEnvStrings;
  ofstream ofOut;
  SECURITY_ATTRIBUTES saSecAtr;
  STARTUPINFO suInfo;
  PROCESS_INFORMATION piInfo;
  BOOL bRc;

  // Lock all the other threads out.
  while (__lxchg(&iCgiLock, 1) != 0)
    {
      Sleep(1);  // Sleep, not spin.
    }

  // Setting the environment variables.
  iEnvLen = strlen(cParms->hInfo->szQuery) + (13 * 64) + 32;
  szEnvStrings = new char[iEnvLen];
  memset(szTmp, '\0', iEnvLen);
  szTmp = szEnvStrings;

  sprintf(szRequestMethod, "REQUEST_METHOD=%s", cParms->hInfo->szMethod);
  memcpy(szTmp, szRequestMethod, strlen(szRequestMethod));
  szTmp += strlen(szRequestMethod) + 1;

  sprintf(szScriptName, "SCRIPT_NAME=%s", cParms->hInfo->szUri);
  memcpy(szTmp, szScriptName, strlen(szScriptName));
  szTmp += strlen(szScriptName) + 1;

  if (cParms->hInfo->szQuery != NULL)
    {
      szQueryString = new char[(strlen(cParms->hInfo->szQuery) + 15)];
      sprintf(szQueryString, "QUERY_STRING=%s", cParms->hInfo->szQuery);
    }
  else
    {
      szQueryString = new char[15];
      strcpy(szQueryString, "QUERY_STRING");
    }
  memcpy(szTmp, szQueryString, strlen(szQueryString));
  szTmp += strlen(szQueryString) + 1;

  if (cParms->sClient->szPeerName != NULL)
    {
      sprintf(szRemoteHost, "REMOTE_HOST=%s", cParms->sClient->szPeerName);
    }
  else
    {
      strcpy(szRemoteHost, "REMOTE_HOST");
    }
  memcpy(szTmp, szRemoteHost, strlen(szRemoteHost));
  szTmp += strlen(szRemoteHost) + 1;

  sprintf(szRemoteAddr, "REMOTE_ADDR=%s", cParms->sClient->szPeerIp);
  memcpy(szTmp, szRemoteAddr, strlen(szRemoteAddr));
  szTmp += strlen(szRemoteAddr) + 1;

  if (cParms->hInfo->szAuthType != NULL)
    {
      sprintf(szAuthType, "AUTH_TYPE=%s", cParms->hInfo->szAuthType);
      sprintf(szRemoteUser, "REMOTE_USER=%s", cParms->hInfo->szRemoteUser);
    }
  else
    {
      strcpy(szAuthType, "AUTH_TYPE");
      strcpy(szRemoteUser, "REMOTE_USER");
    }
  memcpy(szTmp, szAuthType, strlen(szAuthType));
  szTmp += strlen(szAuthType) + 1;
  memcpy(szTmp, szRemoteUser, strlen(szRemoteUser));
  szTmp += strlen(szRemoteUser) + 1;

  if (cParms->hInfo->szContentType != NULL)
    {
      sprintf(szContentType, "CONTENT_TYPE=%s", cParms->hInfo->szContentType);
    }
  else
    {
      strcpy(szContentType, "CONTENT_TYPE");
    }
  memcpy(szTmp, szContentType, strlen(szContentType));
  szTmp += strlen(szContentType) + 1;
  if (strcmp(cParms->hInfo->szMethod, "POST") == 0)
    {
      sprintf(szContentLength, "CONTENT_LENGTH=%d", cParms->hInfo->ulContentLength);
    }
  else
    {
      strcpy(szContentLength, "CONTENT_LENGTH=0");
    }
  memcpy(szTmp, szContentLength, strlen(szContentLength));
  szTmp += strlen(szContentLength) + 1;

  sprintf(szServerProtocol, "SERVER_PROTOCOL=%s", cParms->hInfo->szVer);
  memcpy(szTmp, szServerProtocol, strlen(szServerProtocol));
  szTmp += strlen(szServerProtocol) + 1;

  sprintf(szServerName, "SERVER_NAME=%s", cParms->hInfo->szHost);
  memcpy(szTmp, szServerName, strlen(szServerName));
  szTmp += strlen(szServerName) + 1;

  memcpy(szTmp, szServerSoftware, strlen(szServerSoftware));
  szTmp += strlen(szServerSoftware) + 1;
  memcpy(szTmp, szServerPort, strlen(szServerPort));
  szTmp += strlen(szServerPort) + 1;

  memset(&saSecAtr, 0, sizeof(SECURITY_ATTRIBUTES));
  saSecAtr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saSecAtr.lpSecurityDescriptor = NULL;
  saSecAtr.bInheritHandle = TRUE;

  iRc = CreatePipe(&(pOut[0]), &(pOut[1]), 
                   &saSecAtr, 4096);

  // Set the pipe handle to non-inherit.
  DuplicateHandle(GetCurrentProcess(),
                  pOut[0],
                  GetCurrentProcess(),
                  NULL,
                  0,
                  FALSE,
                  DUPLICATE_SAME_ACCESS);

  memset(&suInfo, 0, sizeof(STARTUPINFO));
  suInfo.cb = sizeof(STARTUPINFO);
  suInfo.dwFlags = STARTF_USESTDHANDLES;
  suInfo.hStdInput = NULL;
  suInfo.hStdOutput = (HANDLE)pOut[1];
  suInfo.hStdError = NULL;

  if (cParms->szPost != NULL) // Use POST method.
    {
      suInfo.hStdInput = CreateFile(cParms->szPost,
                                    GENERIC_READ,
                                    FILE_SHARE_READ,
                                    &saSecAtr,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    0);
    }

  CreateProcess((cParms->szProg),   // Command line.
                NULL,
                NULL,               // Process security attributes.
                NULL,               // Thread security attributes.
                TRUE,               // Inherit handles.
                DETACHED_PROCESS,   // Start detached.
                szEnvStrings,       // Environment variables.
                NULL,               // Current directory default.
                &suInfo,            // Startup information.
                &piInfo);           // Child information.

  CloseHandle(piInfo.hThread);  // Close handle to child, not needed.
  CloseHandle(pOut[1]);

  cParms->szOutput = new char[L_tmpnam]; // Create a temporary file for
  tmpnam(cParms->szOutput);              // the output.
  iRead = open(cParms->szOutput, O_WRONLY | O_CREAT | O_BINARY, S_IWRITE);
  // Grab all of the output from the child.
  memset(szBuf, 0, SMALLBUF);
  while ( ReadFile(pOut[0], szBuf, SMALLBUF, (LPDWORD)&iNum, NULL) )
    {
      iRc = write(iRead, szBuf, iNum);
      memset(szBuf, 0, SMALLBUF);
    }
  close(iRead);
  delete [] szQueryString;

  CloseHandle(pOut[0]);
  if (suInfo.hStdInput != NULL) CloseHandle(suInfo.hStdInput);

  // Unlock cgi access.
  __lxchg(&iCgiLock, 0);

  return (0);
}

// ------------------------------------------------------------------
//
// InitCgi
//
// Initialize some of the global variables needed for the CGI
// processing.
//

void InitCgi()
{
  // These are the same for all requests, so only set once.
  sprintf(szServerSoftware, "SERVER_SOFTWARE=%s", szServerVer);
  strcpy(szGatewayInterface, "GATEWAY_INTERFACE=CGI/1.1");
  sprintf(szServerPort, "SERVER_PORT=%d", sPort);
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------