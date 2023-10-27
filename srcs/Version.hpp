#ifndef VERSION_HPP
#define VERSION_HPP

#define WSV_HTTP_VERSION_10 Version(1, 0)
#define WSV_HTTP_VERSION_11 Version(1, 1)
#define WSV_HTTP_VERSION_20 Version(2, 0)

struct Version {
  int major;
  int minor;
  Version() : major(0), minor(0) {}
  Version(int major, int minor) : major(major), minor(minor) {}

  void clear() {
    major = 0;
    minor = 0;
  }
};

bool operator<(const Version& lhs, const Version& rhs);
bool operator<=(const Version& lhs, const Version& rhs);

#endif
