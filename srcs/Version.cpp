#include "Version.hpp"

bool operator<(const Version& lhs, const Version& rhs) {
  return lhs.major < rhs.major || (lhs.major == rhs.major && lhs.minor < rhs.minor);
}

bool operator<=(const Version& lhs, const Version& rhs) {
  return !operator<(rhs, lhs);
}
