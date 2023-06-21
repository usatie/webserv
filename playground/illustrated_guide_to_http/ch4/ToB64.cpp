#include <string>
// -----------------------------------------------------------------------------
//
// 0xfc = 11111100   Bit sequences needed for masks
// 0x03 = 00000011
// 0xf0 = 11110000
// 0x0f = 00001111
// 0xc0 = 11000000
// 0x3f = 00111111
// 0x30 = 00110000
// 0x3c = 00111100
//
// -----------------------------------------------------------------------------
//
// The Bse64 alphabet
const char szB64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";


// -----------------------------------------------------------------------------
//
// ToB64()
// 
// This function takes a character string as input and transforms it to Base64
// encoding. The return value is dynamically allocated and must be freed by the
// caller.
char * ToB64(char *szStr)
{
  char *szEnc;
  int iLen, i, j;

  iLen = strlen(szStr);
  // It seems that the length is insufficient when the string is short like 
  // "a" or "ab"
  szEnc = new char[(int)((float)iLen * 1.5f)]; // Space for the encoded strings
  j = 0;
  for ( i = 0; i < (iLen - (iLen % 3 )); i += 3)
  {
    szEnc[j] = szB64[ (szStr[i] & 0xfc) >> 2 ];     // 0xfc = 11111100
    szEnc[j+1] = szB64[ ((szStr[i] & 0x03) << 4)    // 0x03 = 00000011
                | ((szStr[i+1] & 0xf0) >> 4) ];     // 0xf0 = 11110000
    szEnc[j+2] = szB64[ ((szStr[i+1] & 0x0f) << 2)  // 0x0f = 00001111
                | ((szStr[i+2] & 0xc0) >> 6) ];     // 0xc0 = 11000000
    szEnc[j+3] = szB64[ (szStr[i+2] & 0x3f) ];      // 0x3f = 00111111
    j += 4;
  }
  i = iLen - (iLen % 3);
  switch (iLen % 3) {
    case 2: // One character padding needed
      {
        szEnc[j] = szB64[ (szStr[i] & 0xfc) >> 2 ];       // 0xfc = 11111100
        szEnc[j+1] = szB64[ ((szStr[i] & 0x03) << 4)      // 0x03 = 00000011
                    | ((szStr[i+1] & 0xf0) >> 4) ];       // 0xf0 = 11110000
        szEnc[j+2] = szB64[ ((szStr[i+1] & 0x0f) << 2) ]; // 0x0f = 00001111
        szEnc[j+3] = szB64[64];
        break ;
      }
    case 1: // Two character padding needed
      {
        szEnc[j] = szB64[ (szStr[i] & 0xfc) >> 2 ];       // 0xfc = 11111100
        szEnc[j+1] = szB64[ ((szStr[i] & 0x03) << 4) ];   // 0x03 = 00000011
        szEnc[j+2] = szB64[64];
        szEnc[j+3] = szB64[64];
      }
  }
  szEnc[j+4] = '\0';
  return (szEnc);
}
