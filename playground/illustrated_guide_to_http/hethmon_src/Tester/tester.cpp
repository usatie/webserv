//
// WWW Server  File: Tester.cpp
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
#include <fcntl.h>
#include <io.h>
#include <ctype.h>

#ifdef __IBMCPP__
  #include <builtin.h>
#endif

#ifdef __OS2__
  #define INCL_DOS
  #define OS2
  #include <os2.h>
  #include <types.h>            // For the socket types.h
  #define Sleep(x) DosSleep(x)  // Portability definition
#endif

#include <iostream.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <sys\socket.h>

#include "socket.hpp"
#include "defines.hpp"
#include "headers.hpp"

void PrintHeaders(ofstream &, Headers *);

// ------------------------------------------------------------------
//
//

int main(int argc, char *argv[])
{
  ifstream ifList, ifTmp;
  ofstream ofOut;
  Socket *sMe;
  int i, j, k, iRc;
  char *szBuf, *szFile, *szHost, *szOut;
  Headers *hInfo;

  cout << "Starting ..." << endl;
  sock_init();

  if (argc < 2)
    {
      cerr << "Error!" << endl;
      cerr << "Usage:  Tester <input file>" << endl;
      return 1;
    }

  ifList.open(argv[1]);
  if (! ifList)
    {
      cerr << "Error!" << endl;
      cerr << "Unable to open file: " << argv[1] << endl;
      return 2;
    }

  if (argc == 3)
    {
      szOut = argv[2];
    }
  else
    {
      szOut = strdup("tester.out");
    }
  ofOut.open(szOut);
  if (! ofOut)
    {
      ifList.close();
      cerr << "Error!" << endl;
      cerr << "Unable to open output file." << endl;
      return 2;
    }

  cout << "Files open." << endl;

  szBuf = new char[SMALLBUF];
  szFile = new char[SMALLBUF];
  szHost = new char[SMALLBUF];
  ifList.getline(szHost, SMALLBUF, '\n');      // Grab the hostname.

  cout << "Connecting to host: " << szHost << endl;
  sMe = new Socket();
  sMe->Create();
  iRc = sMe->Connect(szHost, WWW_PORT);
  if (iRc < 0)
    {
      cerr << "Error!" << endl;
      cerr << "Cannot connect to " << szHost << "." << endl;
      cerr << "Error number = " << iRc << endl;
      delete [] szHost;
      delete [] szFile;
      delete [] szBuf;
      ifList.close();
      ofOut.close();
      delete sMe;
      return 3;
    }

  cerr << "sleeping 12 seconds ..." << endl;
  DosSleep(12 * 1000);

  while (! ifList.eof())
    {
      memset(szFile, 0, SMALLBUF);
      ifList.getline(szFile, SMALLBUF, '\n');  // Grab first filename.
      if (szFile[0] == NULL) continue;

      cout << "Sending file: " << szFile << endl;
      sMe->SendText(szFile);
      cout << "File sent." << endl;

      ofOut << "Filename: " << szFile << endl;
      ofOut << "Results:" << endl;

      cout << "Receiving response." << endl;

      sMe->RecvTeol(NO_EOL);
      ofOut << sMe->szOutBuf << endl;
      hInfo = new Headers();
      hInfo->RcvHeaders(sMe);
      PrintHeaders(ofOut, hInfo);
      ofOut << endl;
      ofOut.close();
      ofOut.open(szOut, ios::binary | ios::app);
      i = 0;
      cerr << "content-length = " << hInfo->ulContentLength << endl;
//      if (hInfo->ulContentLength == 0) hInfo->ulContentLength = 1024;
      while (i < hInfo->ulContentLength)
        {
          j = sMe->Recv((int)(hInfo->ulContentLength) - i);
          if (j < 1) break;
          i += j;
          ofOut.write(sMe->szOutBuf, j);
          ofOut.flush();
          cerr << "bytes = " << j << endl;
        }
      ofOut.close();
      ofOut.open(szOut, ios::app);
      ofOut << endl << "+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=" << endl;

      delete hInfo;
      cout << "Connection closed." << endl << endl;
    }

  sMe->Close();
  delete [] szBuf;
  delete [] szFile;
  delete [] szHost;
  delete sMe;
  ofOut.close();
  ifList.close();

  cout << "Done." << endl;
}

// ------------------------------------------------------------------

void PrintHeaders(ofstream &ofOut, Headers *hInfo)
{
  if (hInfo->szServer) ofOut << "Server: " << hInfo->szServer << endl;
  if (hInfo->szDate) ofOut << "Date: " << hInfo->szDate << endl;
  if (hInfo->szLastMod) ofOut << "Last-modified: " << hInfo->szLastMod << endl;
  if (hInfo->szContentType) ofOut << "Content-type: " << hInfo->szContentType << endl;
  if (hInfo->szContentLength) ofOut << "Content-length: " << hInfo->szContentLength << endl;
  if (hInfo->szWWWAuth) ofOut << "WWW-Authenticate: " << hInfo->szWWWAuth << endl;
  if (hInfo->szETag) ofOut << "Etag: " << hInfo->szETag << endl;
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------
//
// ConvertDate()
//
// This routine will convert from any of the three recognized
// date formats used in HTTP/1.0 to a time_t value, the number
// of seconds since the epoch date: 00:00:00 1 January 1970 GMT.

time_t ConvertDate(char *szDate)
{
  char szMonth[64];  // Allow extra for bad formats.
  struct tm tmData;

  if (strlen(szDate) > 34)  // Catch bad/unknown formatting.
    {
      return( (time_t) 0 );
    }

  if (szDate[3] == ',') // RFC 822, updated by RFC 1123
    {
      sscanf(szDate, "%*s %d %s %d %d:%d:%d %*s",
             &(tmData.tm_mday), szMonth, &(tmData.tm_year), &(tmData.tm_hour), 
             &(tmData.tm_min), &(tmData.tm_sec));
      tmData.tm_year -= 1900;
    }
  else if (szDate[3] == ' ') // ANSI C's asctime() format
    {
      sscanf(szDate, "%*s %s %d %d:%d:%d %d",
             szMonth, &(tmData.tm_mday), &(tmData.tm_hour), &(tmData.tm_min), 
             &(tmData.tm_sec), &(tmData.tm_year));
      tmData.tm_year -= 1900;
    }
  else if (isascii(szDate[3])) // RFC 850, obsoleted by RFC 1036
    {
      sscanf(szDate, "%*s %d-%3s-%d %d:%d:%d %*s",
             &(tmData.tm_mday), szMonth, &(tmData.tm_year), &(tmData.tm_hour), 
             &(tmData.tm_min), &(tmData.tm_sec));
    }
  else  // Unknown time format
    {
      return ((time_t)0);
    }

  // Now calculate the number of seconds since 1970.
  if (stricmp(szMonth, "jan") == 0) tmData.tm_mon = 0;
  else if (stricmp(szMonth, "feb") == 0) tmData.tm_mon = 1;
  else if (stricmp(szMonth, "mar") == 0) tmData.tm_mon = 2;
  else if (stricmp(szMonth, "apr") == 0) tmData.tm_mon = 3;
  else if (stricmp(szMonth, "may") == 0) tmData.tm_mon = 4;
  else if (stricmp(szMonth, "jun") == 0) tmData.tm_mon = 5;
  else if (stricmp(szMonth, "jul") == 0) tmData.tm_mon = 6;
  else if (stricmp(szMonth, "aug") == 0) tmData.tm_mon = 7;
  else if (stricmp(szMonth, "sep") == 0) tmData.tm_mon = 8;
  else if (stricmp(szMonth, "oct") == 0) tmData.tm_mon = 9;
  else if (stricmp(szMonth, "nov") == 0) tmData.tm_mon = 10;
  else if (stricmp(szMonth, "dec") == 0) tmData.tm_mon = 11;

  tmData.tm_isdst = 0;  // There should be no daylight savings time factor.

  return(mktime(&tmData));
}

// ------------------------------------------------------------------
// ------------------------------------------------------------------

