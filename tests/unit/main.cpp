#include "Log.hpp"

void test_socketbuf();
void test_configparser();

int main() {
  Log::setLevel(Log::Warn);
  test_socketbuf();
  test_configparser();
}
