#ifndef VERSION_HPP
#define VERSION_HPP

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
