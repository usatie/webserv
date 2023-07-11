#include <sys/time.h>
#include "../ch7/socket.hpp"

class Headers
{
  public:
    Headers();
    ~Headers();
    int RcvHeaders(Socket *sClient);
    int CheckHeaders();
    int FindRanges(int iSize);

    char *szMethod,
         *szUri,
         *szVer,
         *szQuery,
         *szAuthType,
         *szRemoteUser,
         *szAccept,
         *szAcceptCharset,
         *szAcceptEncoding,
         *szAcceptLanguage,
         *szAge,
         *szAllow,
         *szAuth,
         *szCacheControl,
         *szConnection,
         *szContentBase,
         *szContentEncoding,
         *szContentLanguage,
         *szContentLength,
         *szContentLocation,
         *szContentMD5,
         *szContentRange,
         *szContentType,
         *szDate,
         *szETag,
         *szExpires,
         *szFrom,
         *szHost,
         *szIfModSince,
         *szIfMatch,
         *szIfNoneMatch,
         *szIfRange,
         *szIfUnmodSince,
         *szLastMod,
         *szLocation,
         *szMaxForwards,
         *szPragma,
         *szPublic,
         *szRange,
         *szReferer,
         *szRetryAfter,
         *szTransferEncoding,
         *szUpgrade,
         *szUserAgent,
         *szVary,
         *szVia,
         *szWarning,
         *szWWWAuth,
         *szRealm;

  time_t ttIfModSince,
        ttIfUnmodSince;
  bool bPersistent,
       bChunked;
  unsigned long ulContentLength;
  char **szIfMatchEtags,
       **szIfNoneMatchEtags;
  int iRangeNum;

  private:
  char ** Etag(char *);
};
