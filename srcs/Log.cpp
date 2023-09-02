#include "Log.hpp"
#include <stdarg.h>

static Log::Level log_level_ = Log::Debug;
static std::ofstream devnull_;

void Log::setLevel(Level level) throw() { log_level_ = level; }

// debug, info, warn, error, fatal are for use with printf-style formatting
void Log::debug(const char* format, ...) throw() {
  if (log_level_ <= Log::Debug) {
    char buf[MAX_LOG_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, MAX_LOG_LENGTH, format, args);
    std::cerr << "[DEBUG] " << buf << std::endl;
    va_end(args);
  }
}

void Log::info(const char* format, ...) throw() {
  if (log_level_ <= Log::Info) {
    char buf[MAX_LOG_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, MAX_LOG_LENGTH, format, args);
    std::cerr << "[INFO] " << buf << std::endl;
    va_end(args);
  }
}

void Log::warn(const char* format, ...) throw() {
  if (log_level_ <= Log::Warn) {
    char buf[MAX_LOG_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, MAX_LOG_LENGTH, format, args);
    std::cerr << "[WARN] " << buf << std::endl;
    va_end(args);
  }
}

void Log::error(const char* format, ...) throw() {
  if (log_level_ <= Log::Error) {
    char buf[MAX_LOG_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, MAX_LOG_LENGTH, format, args);
    std::cerr << "[ERROR] " << buf << std::endl;
    va_end(args);
  }
}

void Log::fatal(const char* format, ...) throw() {
  if (log_level_ <= Log::Fatal) {
    char buf[MAX_LOG_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(buf, MAX_LOG_LENGTH, format, args);
    std::cerr << "[FATAL] " << buf << std::endl;
    va_end(args);
  }
}

// cdebug, cinfo, cwarn, cerror, cfatal are for use with std::ostream
std::ostream& Log::cdebug() throw() {
  if (log_level_ <= Log::Debug) {
    return std::cerr << "[DEBUG] ";
  }
  if (!devnull_.is_open()) {
    devnull_.open("/dev/null");
  }
  return devnull_;
}

std::ostream& Log::cinfo() throw() {
  if (log_level_ <= Log::Info) {
    return std::cerr << "[INFO] ";
  }
  if (!devnull_.is_open()) {
    devnull_.open("/dev/null");
  }
  return devnull_;
}

std::ostream& Log::cwarn() throw() {
  if (log_level_ <= Log::Warn) {
    return std::cerr << "[WARN] ";
  }
  if (!devnull_.is_open()) {
    devnull_.open("/dev/null");
  }
  return devnull_;
}

std::ostream& Log::cerror() throw() {
  if (log_level_ <= Log::Error) {
    return std::cerr << "[ERROR] ";
  }
  if (!devnull_.is_open()) {
    devnull_.open("/dev/null");
  }
  return devnull_;
}

std::ostream& Log::cfatal() throw() {
  if (log_level_ <= Log::Fatal) {
    return std::cerr << "[FATAL] ";
  }
  if (!devnull_.is_open()) {
    devnull_.open("/dev/null");
  }
  return devnull_;
}
