/* This code derived from the mddriver.c code in RFC1321 */


/* MDDRIVER.C - test driver for MD2, MD4 and MD5
 */

/* Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
rights reserved.

RSA Data Security, Inc. makes no representations concerning either
the merchantability of this software or the suitability of this
software for any particular purpose. It is provided "as is"
without express or implied warranty of any kind.

These notices must be retained in any copies of any part of this
documentation and/or software.
 */

#include <stdio.h>
#include <time.h>
#include <string.h>
#include "global.h"
#include "md5.h"

void MD5Encode(char *szSrc, char *szResult);


int main(int argc, char *argv[])
{
  char szNonce[] = "dcd98b7102dd2f0e8b11d0f600bfb0c093",
       szRealm[] = "sales@www.example.com",
       szUserName[] = "john.salesman",
       szPass[] = "5+5=10",
       szUri[] = "/private/prices.html",
       szMethod[] = "GET",
       szResp[] = "e966c932a9242554e42c8ee200cec7f6",
       szBuf[256], szBufr[256],
       szA1[256], szA1r[256],
       szA2[256], szA2r[256];

  sprintf(szA1, "%s:%s:%s", szUserName, szRealm, szPass);
  MD5Encode(szA1, szA1r);
  printf("a1      = %s\n", szA1);
  printf("md5(a1) = %s\n", szA1r);

  sprintf(szA2, "%s:%s", szMethod, szUri);
  MD5Encode(szA2, szA2r);
  printf("a2      = %s\n", szA2);
  printf("md5(a2) = %s\n", szA2r);

  sprintf(szBuf, "%s:%s:%s", szA1r, szNonce, szA2r);
  MD5Encode(szBuf, szBufr);

  printf("buf  = %s\n", szBuf);
  printf("calc = %s\n", szBufr);

  return 0;
}

void MD5Encode(char *szSrc, char *szResult)
{
  MD5_CTX context;
  unsigned char digest[16];
  unsigned int len = strlen (szSrc);
  unsigned int i;
  char szTmp[32];

  MD5Init (&context);
  MD5Update (&context, (unsigned char *)szSrc, len);
  MD5Final (digest, &context);
  memset(szResult, 0, 256);

  for (i = 0; i < 16; i++)
    {
      sprintf(szTmp, "%02x", digest[i]);
      strcat(szResult, szTmp);
    }
}

/* the original function from RFC1321 */

/* Digests a string and prints the result.
 */
/*
static void MDString (string)
char *string;
{
  MD_CTX context;
  unsigned char digest[16];
  unsigned int len = strlen (string);

  MDInit (&context);
  MDUpdate (&context, string, len);
  MDFinal (digest, &context);

  printf ("MD%d (\"%s\") = ", MD, string);
  MDPrint (digest);
  printf ("\n");
}

*/

