#include <ctime>
#include <string>

// asctime style date format: "Mon Jan 02 22:04:05 2006"
// asctime-date = wkday SP date3 SP time SP 4DIGIT
// date3        = month SP ( 2DIGIT | SP 1DIGIT )
// wkday        = "Mon" | "Tue" | "Wed" | "Thu" | "Fri" | "Sat" | "Sun"
// month        = "Jan" | "Feb" | "Mar" | "Apr" | "May" | "Jun" | "Jul" |
//               "Aug" | "Sep" | "Oct" | "Nov" | "Dec"
// time         = 2DIGIT ":" 2DIGIT ":" 2DIGIT

// RFC850 style date format: "Monday, 02-Jan-06 22:04:05 GMT"
// rfc850-date = weekday "," SP date2 SP time SP "GMT"
// weekday     = "Monday" | "Tuesday" | "Wednesday" | "Thursday" | "Friday" |
//               "Saturday" | "Sunday"
// date2       = 2DIGIT "-" month "-" 2DIGIT

// RFC1123 style date format: "Mon, 02 Jan 2006 22:04:05 GMT"
// rfc1123-date = wkday "," SP date SP time SP "GMT"
// date         = 2DIGIT SP month SP 4DIGIT

// time_t value is the number of seconds since 00:00:00 GMT, January 1, 1970
time_t ConvertDate(char *szDate)
{
  // What does sz stands for?
  // sz stands for "string zero-terminated"
  char szMonth[64]; // Allow extra for bad formats
  struct tm tmData;

  if (strlen(szDate) > 34) // Catch bad/unknown formatting
  {
    return ( (time_t) 0 ) ;
  }

  if (szDate[3] == ',') // RFC 822, update by RFC 1123
  // "Mon, 02 Jan 2006 22:04:05 GMT"
  {
    // * : Suppresses assignment.
    //     The conversion that follows occurs as usual, but no pointer is used;
    //     the result of the conversion is simply discarded.
    sscanf(szDate, "%*s %d %s %d %d:%d:%d %*s",
      &tmData.tm_mday, szMonth, &tmData.tm_year,
      &tmData.tm_hour, &tmData.tm_min, &tmData.tm_sec);
    tmData.tm_year -= 1900;
  }
  else if (szDate[3] == ' ') // ANSI C's asctime() format
  // "Mon Jan 02 22:04:05 2006"
  {
    sscanf(szDate, "%*s %s %d %d:%d:%d %d",
      szMonth, &tmData.tm_mday, &tmData.tm_hour,
      &tmData.tm_min, &tmData.tm_sec, &tmData.tm_year);
    tmData.tm_year -= 1900;
  }
  else if (isascii(szDate[3])) // RFC 850, obsoleted by RFC 1036
  // "Monday, 02-Jan-06 22:04:05 GMT"
  {
    sscanf(szDate, "%*s %d-%3s-%d %d:%d:%d %*s",
      &tmData.tm_mday, szMonth, &tmData.tm_year,
      &tmData.tm_hour, &tmData.tm_min, &tmData.tm_sec);

  }
  else // Unknown time format
  {
    return ( (time_t) 0 ) ;
  }

  if (strcasecmp(szMonth, "jan") == 0) tmData.tm_mon = 0;
  else if (strcasecmp(szMonth, "feb") == 0) tmData.tm_mon = 1;
  else if (strcasecmp(szMonth, "mar") == 0) tmData.tm_mon = 2;
  else if (strcasecmp(szMonth, "apr") == 0) tmData.tm_mon = 3;
  else if (strcasecmp(szMonth, "may") == 0) tmData.tm_mon = 4;
  else if (strcasecmp(szMonth, "jun") == 0) tmData.tm_mon = 5;
  else if (strcasecmp(szMonth, "jul") == 0) tmData.tm_mon = 6;
  else if (strcasecmp(szMonth, "aug") == 0) tmData.tm_mon = 7;
  else if (strcasecmp(szMonth, "sep") == 0) tmData.tm_mon = 8;
  else if (strcasecmp(szMonth, "oct") == 0) tmData.tm_mon = 9;
  else if (strcasecmp(szMonth, "nov") == 0) tmData.tm_mon = 10;
  else if (strcasecmp(szMonth, "dec") == 0) tmData.tm_mon = 11;
  else return ( (time_t) 0 ) ; // Unknown month

  tmData.tm_isdst = 0; // No daylight saving time
  return mktime(&tmData);
}
