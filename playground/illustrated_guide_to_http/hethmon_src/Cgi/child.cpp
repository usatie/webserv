// ----- child.cpp -----
#define INCL_DOS
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream.h>

int main(int argc, char *argv[])
{
  char *szBuf, 
       *szServerSoftware = NULL,
       *szServerName = NULL,
       *szGatewayInterface = NULL,
       *szServerProtocol = NULL,
       *szServerPort = NULL,
       *szRequestMethod = NULL,
       *szScriptName = NULL,
       *szQueryString = NULL,
       *szRemoteHost = NULL,
       *szRemoteAddr = NULL,
       *szAuthType = NULL,
       *szRemoteUser = NULL,
       *szContentType = NULL,
       *szContentLength = NULL;
  unsigned long ulContentLength = 0;
  int i, iRead;

  if ((szBuf = getenv("SERVER_SOFTWARE")) != NULL)
    {
      szServerSoftware = strdup(szBuf);
    }
  if ((szBuf = getenv("SERVER_NAME")) != NULL)
    {
      szServerName = strdup(szBuf);
    }
  if ((szBuf = getenv("GATEWAY_INTERFACE")) != NULL)
    {
      szGatewayInterface = strdup(szBuf);
    }
  if ((szBuf = getenv("SERVER_PROTOCOL")) != NULL)
    {
      szServerProtocol = strdup(szBuf);
    }
  if ((szBuf = getenv("SERVER_PORT")) != NULL)
    {
      szServerPort = strdup(szBuf);
    }
  if ((szBuf = getenv("REQUEST_METHOD")) != NULL)
    {
      szRequestMethod = strdup(szBuf);
    }
  if ((szBuf = getenv("SCRIPT_NAME")) != NULL)
    {
      szScriptName = strdup(szBuf);
    }
  if ((szBuf = getenv("QUERY_STRING")) != NULL)
    {
      szQueryString = strdup(szBuf);
    }
  if ((szBuf = getenv("REMOTE_HOST")) != NULL)
    {
      szRemoteHost = strdup(szBuf);
    }
  if ((szBuf = getenv("REMOTE_ADDR")) != NULL)
    {
      szRemoteAddr = strdup(szBuf);
    }
  if ((szBuf = getenv("AUTH_TYPE")) != NULL)
    {
      szAuthType = strdup(szBuf);
    }
  if ((szBuf = getenv("REMOTE_USER")) != NULL)
    {
      szRemoteUser = strdup(szBuf);
    }
  if ((szBuf = getenv("CONTENT_TYPE")) != NULL)
    {
      szContentType = strdup(szBuf);
    }
  if ((szBuf = getenv("CONTENT_LENGTH")) != NULL)
    {
      szContentLength = strdup(szBuf);
      ulContentLength = atol(szContentLength);
    }

  cout << "Content-type: text/plain" << endl;
  cout << endl;

  szBuf = NULL;
  if (ulContentLength > 0)
    {
      szBuf = new char[ulContentLength + 1];
      iRead = 0;
      while (iRead < ulContentLength)
        {
          DosSleep(1);
          cin.read((szBuf + iRead), ulContentLength - iRead);
          iRead += cin.gcount();
          cout << "Read " << cin.gcount() << " bytes from stdin." << endl;
        }
    }

//  cout.setf(ios::unitbuf);
  cout << "Child Program #7" << endl;
  cout << "Another line of the program." << endl;
  cout << endl;

  // Now the the things we read.
  if (szBuf)
    {
      cout << "Stdin read:" << endl;
      cout << szBuf << endl;
      delete [] szBuf;
    }
  szBuf = getenv("TMP");
  cout << "TMP = " << szBuf << endl;
  if (szServerSoftware)
    {
      cout << "SERVER_SOFTWARE = " << szServerSoftware << endl;
      delete [] szServerSoftware;
    }
  if (szServerName)
    {
      cout << "SERVER_NAME = " << szServerName << endl;
      delete [] szServerName;
    }
  if (szGatewayInterface)
    {
      cout << "GATEWAY_INTERFACE = " << szGatewayInterface << endl;
      delete [] szGatewayInterface;
    }
  if (szServerProtocol)
    {
      cout << "SERVER_PROTOCOL = " << szServerProtocol << endl;
      delete [] szServerProtocol;
    }
  if (szServerPort)
    {
      cout << "SERVER_PORT = " << szServerPort << endl;
      delete [] szServerPort;
    }
  if (szRequestMethod)
    {
      cout << "REQUEST_METHOD = " << szRequestMethod << endl;
      delete [] szRequestMethod;
    }
  if (szScriptName)
    {
      cout << "SCRIPT_NAME = " << szScriptName << endl;
      delete [] szScriptName;
    }
  if (szQueryString)
    {
      cout << "QUERY_STRING = " << szQueryString << endl;
      delete [] szQueryString;
    }
  if (szRemoteHost)
    {
      cout << "REMOTE_HOST = " << szRemoteHost << endl;
      delete [] szRemoteHost;
    }
  if (szRemoteAddr)
    {
      cout << "REMOTE_ADDR = " << szRemoteAddr << endl;
      delete [] szRemoteAddr;
    }
  if (szAuthType)
    {
      cout << "AUTH_TYPE = " << szAuthType << endl;
      delete [] szAuthType;
    }
  if (szRemoteUser)
    {
      cout << "REMOTE_USER = " << szRemoteUser << endl;
      delete [] szRemoteUser;
    }
  if (szContentType)
    {
      cout << "CONTENT_TYPE = " << szContentType << endl;
      delete [] szContentType;
    }
  if (szContentLength)
    {
      cout << "CONTENT_LENGTH = " << szContentLength << endl;
      delete [] szContentLength;
    }
  cout << "Done with environment variables." << endl;
  cout << endl << endl;

  return 0;
}
// ----- child.cpp -----

