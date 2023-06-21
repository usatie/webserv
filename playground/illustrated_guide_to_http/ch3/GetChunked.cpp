#include <fstream>
#define NO_EOL 1
class Socket{
public:
  char *szOutBuf;                  // Used to return data in
  int Recv(int iBytes);            // Receive up to iBytes on this socket
  int RecvTeol(int iToast);        // Receive up to the telnet eol
                                   // and possibly remove the telnet eol
};
class Headers{
  public:
  int RcvHeaders(Socket *sClient);
};

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
      return c - '0';
  }
}

int GetChunked(Socket *sClient, std::ofstream &ofOut, Headers *hInfo)
{
  bool bNotDone = true;
  char *szPtr;
  int iBytes, i, j, l, iFactor;

  while (bNotDone) {
    sClient->RecvTeol(NO_EOL); // Grab a line. Should have chunk size.
    if (strcmp(sClient->szOutBuf, "0") == 0)
    {
      bNotDone = false; // The end of the chunks.
      continue;
    }
    szPtr = strchr(sClient->szOutBuf, ';');
    if (szPtr != NULL) *szPtr = '\0'; // Mark end of chunk-size.
    l = strlen(sClient->szOutBuf); // Find last hex digit.
    l--;
    iBytes = 0;
    iFactor = 1;
    // Convert to decimal bytes.
    while (l >= 0) {
      iBytes += iFactor * Hex2Dec(sClient->szOutBuf[l]);
      l--;
      iFactor *= 16;
    }
    i = 0;
    // Now receive the specified number of bytes.
    while ( i < iBytes )
    {
      j = sClient->Recv(iBytes - i);      // Some data.
      i += j;                             // Total the bytes
      ofOut.write(sClient->szOutBuf, j);  // Save to disk
    }
    sClient->RecvTeol(NO_EOL);            // Discard end of chunk marker .
  }
  // Now consume anything in the footer
  hInfo->RcvHeaders(sClient);
  return 0;
}
