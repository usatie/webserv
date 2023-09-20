#ifndef LOG_HPP
#define LOG_HPP
#include <fstream>
#include <iostream>

#define MAX_LOG_LENGTH 1024

namespace Log {
enum Level { Debug, Info, Warn, Error, Fatal };
void setLevel(Level level) throw();
void debug(const char* format, ...) throw();
void info(const char* format, ...) throw();
void warn(const char* format, ...) throw();
void error(const char* format, ...) throw();
void fatal(const char* format, ...) throw();

std::ostream& cdebug() throw();
std::ostream& cinfo() throw();
std::ostream& cwarn() throw();
std::ostream& cerror() throw();
std::ostream& cfatal() throw();
}  // namespace Log
#endif
