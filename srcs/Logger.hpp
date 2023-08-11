#include <iostream>
#include <fstream>

class Logger {
 private:
  std::ofstream devnull_;
  std::ostream* log_stream_;

 public:
  Logger() {
#ifdef DEBUG
    log_stream_ = &std::cerr;
#else
    devnull_.open("/dev/null", std::ios::out);
    log_stream_ = &devnull_;
#endif
  }

  ~Logger() {
#ifndef DEBUG
    if (devnull_.is_open()) {
      devnull_.close();
    }
#endif
  }

  void log(const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::vector<char> buf(vsnprintf(nullptr, 0, format, args) + 1);
    va_end(args);

    va_start(args, format);
    vsnprintf(buf.data(), buf.size(), format, args);
    va_end(args);

    (*log_stream_) << buf.data() << std::endl;
  }
};
