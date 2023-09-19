#ifndef UTIL_HPP
#define UTIL_HPP

#include <algorithm>
#include <string>
#include <vector>
#include "Log.hpp"

namespace util {
  template <typename T>
  class shared_ptr {
    private:
      T* ptr;
      int* ref_count;
      void (*deleter)( void* );
    public:
      ~shared_ptr() {
        Log::debug("shared_ptr: destroctor");
        if (ref_count != NULL) {
          (*ref_count)--;
          Log::cdebug() << ptr << " " << *ref_count << std::endl;
          if (*ref_count == 0) {
            deleter(ptr);
            delete ref_count;
          }
        } else {
          Log::debug("shared_ptr: ref_count is NULL");
        }
      }
      shared_ptr(): ptr(), ref_count(NULL), deleter(operator delete) {
        Log::debug("shared_ptr: default constructor");
      }
      shared_ptr(T* ptr): ptr(ptr), ref_count(new int), deleter(operator delete) {
        Log::debug("shared_ptr: constructor");
        *ref_count = 1;
        Log::cdebug() << ptr << " " << *ref_count << std::endl;
      }
      shared_ptr(const shared_ptr<T>& other): ptr(other.ptr), ref_count(other.ref_count), deleter(other.deleter) {
        Log::debug("shared_ptr: copy constructor");
        if (ref_count != NULL) {
          (*ref_count)++;
        }
        Log::cdebug() << ptr << " " << *ref_count << std::endl;
      }
      shared_ptr<T>& operator=(const shared_ptr<T>& other) {
        Log::debug("shared_ptr: copy assignment");
        if (this == &other) {
          return *this;
        }
        if (ref_count != NULL) {
          (*ref_count)--;
          if (*ref_count == 0) {
            Log::cdebug() << ptr << " " << *ref_count << std::endl;
            deleter(ptr);
            delete ref_count;
          }
        }
        ptr = other.ptr;
        ref_count = other.ref_count;
        if (ref_count != NULL) {
          (*ref_count)++;
        }
        Log::cdebug() << ptr << " " << *ref_count << std::endl;
        return *this;
      }
      T* operator->() const {
        return ptr;
      }
      T& operator*() const {
        return *ptr;
      }
      bool operator==(const shared_ptr<T> other) const {
        return ptr == other.ptr;
      }
      bool operator==(nullptr_t other) const {
        return ptr == other;
      }
      bool operator!=(const shared_ptr<T> other) const {
        return ptr != other.ptr;
      }
      bool operator!=(nullptr_t other) const {
        return ptr != other;
      }
  };

namespace string {
bool ends_with(const std::string& str, const std::string& suffix);
}
namespace path {
std::string get_extension(const std::string& str);
}

namespace vector {
template <typename T>
bool contains(const std::vector<T>& vec, const T& str) {
  return std::find(vec.begin(), vec.end(), str) != vec.end();
}
}  // namespace vector
namespace http {
bool is_tchar(const char c);
bool is_token(const std::string& str);
}  // namespace http
}  // namespace util

#endif
