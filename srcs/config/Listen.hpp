#ifndef CONFIG_LISTEN_HPP
#define CONFIG_LISTEN_HPP

#include <netinet/in.h>  // sockaddr_storage, socklen_t

#include <string>

namespace config {
class Listen {
 public:
  std::string address;  // ip address, wildcard, or hostname
  int port;
  bool configured;
  struct sockaddr_storage addr;
  socklen_t addrlen;
  Listen(const std::string &address, const int &port)
      : address(address), port(port), configured(true), addr(), addrlen(0) {
    if (address.empty()) this->address = "*";
  }
  Listen() : address("*"), port(8181), configured(false) {}
};
}  // namespace config

#endif
