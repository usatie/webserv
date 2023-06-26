#include <sys/types.h>
#include <sys/socket.h>

#define TRUE 1
#define FALSE 0

// Receive a command line terminated by a telnet eol sequence
int RecvLine(int iSocket, char *szBuf, int iLen)
{
  int   iBytesRead,
        iIdx,
        bNotDone;

  iBytesRead = recv(iSocket, &szBuf[0], 1, 0);
  iIdx = 1;
  bNotDone = TRUE;

  while ( bNotDone == TRUE )
  {
    iBytesRead = recv(iSocket, &szBuf[iIdx], 1, 0);
    if (iBytesRead < 0)
    {
      return ( -1 ); // error receiving
    }

    iIdx++;
    if (szBuf[iIdx - 2] == '\r' && szBuf[iIdx - 1] == '\n')
    {
      bNotDone = FALSE; // Got telnet eol
    }
    if (iIdx == iLen)
    {
      bNotDone = FALSE; // Error, buffer too small
    }
  }
  szBuf[iIdx - 2] = '\0'; // Append null termination
  return ( TRUE );
}
