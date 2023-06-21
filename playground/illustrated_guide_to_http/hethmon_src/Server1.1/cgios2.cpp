//
// WWW Server  File: cgios2.cpp
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

#define INCL_DOS
#define INCL_DOSFILEMGR
#define INCL_DOSQUEUES
#include <os2.h>
#define Sleep(x) DosSleep(x)  // Portability definition

#ifdef __IBMCPP__
  #include <builtin.h>
#endif

#include "defines.hpp"
#include "config.hpp"
#include "cgi.hpp"

#define SMALLBUF 4196
#define STDIN    0x00000000
#define STDOUT   0x00000001

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
     szContentLength[64],
     *szEnvs[15];

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
  int pIn[2], pOut[2], iNum, iRc;
  FILE *fpin, *fpout;
  FILE *fpPost;
  char szBuf[SMALLBUF],
       *szArgs[2];
  int stdin_save = -1, stdout_save = -1;
  int hfStdin = STDIN, hfStdout = STDOUT;
  ofstream ofOut;

  // Lock all the other threads out.
  while (__lxchg(&iCgiLock, 1) != 0)
    {
      Sleep(1);  // Sleep, not spin.
    }

  _setmode(STDIN, O_BINARY);  // Binary mode for stdin/stdout.
  _setmode(STDOUT, O_BINARY);

  DosDupHandle(STDIN, (PHFILE)&stdin_save);
  DosDupHandle(STDOUT, (PHFILE)&stdout_save);

  DosCreatePipe((PHFILE)&(pIn[0]), (PHFILE)&(pIn[1]), 4096);  // Create the pipe
  DosCreatePipe((PHFILE)&(pOut[0]), (PHFILE)&(pOut[1]), 4096);

  DosSetFHState(pIn[0], OPEN_FLAGS_NOINHERIT);     // Child does not inherit
  DosSetFHState(pIn[1], OPEN_FLAGS_NOINHERIT);
  DosSetFHState(pOut[0], OPEN_FLAGS_NOINHERIT);
  DosSetFHState(pOut[1], OPEN_FLAGS_NOINHERIT);

  _setmode(pIn[0],O_BINARY);       // Binary mode on the pipes
  _setmode(pOut[1],O_BINARY);
  _setmode(pIn[0],O_BINARY);       // Binary mode on the pipes
  _setmode(pOut[1],O_BINARY);

  fpout = fdopen(pIn[1], "w");     // create FILE handle
  dup2(pIn[0], hfStdin);
  close(pIn[0]);                   // close the read handle

  fpin = fdopen(pOut[0], "r");     // create FILE handle
  dup2(pOut[1], hfStdout);
  close(pOut[1]);                  // close the write handle

  setbuf(fpin, NULL);              // Turn buffering off.
  setbuf(fpout, NULL);

  // Setting the environment variables.
  sprintf(szServerProtocol, "SERVER_PROTOCOL=%s", cParms->hInfo->szVer);
  sprintf(szRequestMethod, "REQUEST_METHOD=%s", cParms->hInfo->szMethod);
  sprintf(szScriptName, "SCRIPT_NAME=%s", cParms->hInfo->szUri);
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
  // Since szQueryString is dynamic memory, we must reassign it each time.
  szEnvs[7] = szQueryString;
  if (cParms->sClient->szPeerName != NULL)
    {
      sprintf(szRemoteHost, "REMOTE_HOST=%s", cParms->sClient->szPeerName);
    }
  else
    {
      strcpy(szRemoteHost, "REMOTE_HOST");
    }
  sprintf(szRemoteAddr, "REMOTE_ADDR=%s", cParms->sClient->szPeerIp);
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
  if (cParms->hInfo->szContentType != NULL)
    {
      sprintf(szContentType, "CONTENT_TYPE=%s", cParms->hInfo->szContentType);
    }
  else
    {
      strcpy(szContentType, "CONTENT_TYPE");
    }
  if (strcmp(cParms->hInfo->szMethod, "POST") == 0)
    {
      sprintf(szContentLength, "CONTENT_LENGTH=%d", cParms->hInfo->ulContentLength);
    }
  else
    {
      strcpy(szContentLength, "CONTENT_LENGTH=0");
    }

  szArgs[0] = cParms->szProg;  // The program to run.
  szArgs[1] = NULL;
  // Start it.
  spawnvpe(P_NOWAIT, cParms->szProg, szArgs, szEnvs);

  dup2(stdin_save, hfStdin); // Restore stdin/stdout
  dup2(stdout_save, hfStdout);

  if (cParms->szPost != NULL) // Use POST method.
    {
      fpPost = fopen(cParms->szPost, "rb");
      iNum = 1;
      while (iNum > 0)
        {
          iNum = fread(szBuf, sizeof(char), SMALLBUF, fpPost);
          fwrite(szBuf, sizeof(char), iNum, fpout);
        }
      fclose(fpPost);
    }
  fclose(fpout);

  cParms->szOutput = new char[L_tmpnam]; // Create a temporary file for
  tmpnam(cParms->szOutput);              // the output.
  ofOut.open(cParms->szOutput, ios::bin);
  // Grab all of the output from the child.
  while ( (iNum = fread(szBuf, sizeof(char), SMALLBUF, fpin)) != 0 )
    {
      ofOut.write(szBuf, iNum);
    }
  ofOut.close();
  delete [] szQueryString;
  fclose(fpin);

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
  char *szPtr;

  szEnvs[0] = szServerSoftware;
  szEnvs[1] = szServerName;
  szEnvs[2] = szGatewayInterface;
  szEnvs[3] = szServerProtocol;
  szEnvs[4] = szServerPort;
  szEnvs[5] = szRequestMethod;
  szEnvs[6] = szScriptName;
  szEnvs[7] = szQueryString;
  szEnvs[8] = szRemoteHost;
  szEnvs[9] = szRemoteAddr;
  szEnvs[10] = szAuthType;
  szEnvs[11] = szRemoteUser;
  szEnvs[12] = szContentType;
  szEnvs[13] = szContentLength;
  szEnvs[14] = NULL;

  // These are the same for all requests, so only set once.
  sprintf(szServerSoftware, "SERVER_SOFTWARE=%s", szServerVer);
  sprintf(szServerName, "SERVER_NAME=%s", szHostName);
  strcpy(szGatewayInterface, "GATEWAY_INTERFACE=CGI/1.1");
  sprintf(szServerPort, "SERVER_PORT=%d", sPort);
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
// ------------------------------------------------------------------