#include <iostream>
#include <sys/stat.h>

int main() {
  struct stat st;
  std::cout << "stat: " << stat("hoge", &st) << std::endl;
  if (errno)
    std::cout << "error: " << strerror(errno) << std::endl;
  std::cout << "dir: " << S_ISDIR(st.st_mode) << std::endl;
  std::cout << "reg: " << S_ISREG(st.st_mode) << std::endl;
  // If file is readable
  std::cout << "readable: " << (st.st_mode & S_IRUSR) << std::endl;
}
