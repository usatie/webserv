#ifndef UTIL_HPP
#define UTIL_HPP

#include <algorithm>
#include <string>
#include <vector>

namespace util {
template <typename U>
class Deleter {
 public:
  void operator()(U* ptr) { delete ptr; }
};

template <typename T, class D = Deleter<T> >
class shared_ptr {
 private:
  T* ptr;
  int* ref_count;
  D d;

 public:
  ~shared_ptr() {
    if (ref_count != NULL) {
      (*ref_count)--;
      if (*ref_count == 0) {
        d(ptr);
        delete ref_count;
      }
    }
  }
  shared_ptr() : ptr(), ref_count(NULL), d(Deleter<T>()) {}
  shared_ptr(T* ptr) : ptr(ptr), ref_count(new int), d(Deleter<T>()) {
    *ref_count = 1;
  }
  shared_ptr(const shared_ptr<T>& other)
      : ptr(other.ptr), ref_count(other.ref_count), d(other.d) {
    if (ref_count != NULL) {
      (*ref_count)++;
    }
  }
  shared_ptr<T>& operator=(const shared_ptr<T>& other) {
    if (this == &other) {
      return *this;
    }
    if (ref_count != NULL) {
      (*ref_count)--;
      if (*ref_count == 0) {
        d(ptr);
        delete ref_count;
      }
    }
    ptr = other.ptr;
    ref_count = other.ref_count;
    if (ref_count != NULL) {
      (*ref_count)++;
    }
    return *this;
  }
  T* operator->() const { return ptr; }
  T& operator*() const { return *ptr; }
  bool operator==(const shared_ptr<T> other) const { return ptr == other.ptr; }
  bool operator==(T* other) const { return ptr == other; }
  bool operator!=(const shared_ptr<T> other) const { return ptr != other.ptr; }
  bool operator!=(T* other) const { return ptr != other; }
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