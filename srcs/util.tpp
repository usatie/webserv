#ifndef UTIL_TPP
#define UTIL_TPP

#include <cstddef> // NULL
#include <map>
#include <vector>
#include <string>

namespace util {
  template <typename U>
  class Deleter;

  template <typename T, class D = Deleter<T> >
  class shared_ptr;

  template <typename E, typename T>
  bool contains(const std::vector<E>& container, const T& value);

  template <typename T, typename K, typename V>
  bool contains(const std::map<K, V>& container, const T& value);

  template <typename S>
  bool contains(const std::string &str, const S &substr);
}

template <typename S>
bool util::contains(const std::string &str, const S &substr) {
  return str.find(substr) != std::string::npos;
}

template <typename T, typename K, typename V>
bool util::contains(const std::map<K, V>& container, const T& value) {
  return container.find(value) != container.end();
}

template <typename E, typename T>
bool util::contains(const std::vector<E>& container, const T& value) {
  return std::find(container.begin(), container.end(), value) !=
         container.end();
}

template <typename U>
class util::Deleter {
 public:
  void operator()(U* ptr) const { delete ptr; }
};

template <typename T, class D>
class util::shared_ptr {
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
  shared_ptr() throw() : ptr(), ref_count(NULL), d(Deleter<T>()) {}
  // cppcheck-suppress noCopyConstructor ; I don't know why cppcheck thinks there is no copy constructor
  explicit shared_ptr(T* ptr) : ptr(ptr), ref_count(new int), d(Deleter<T>()) {
    *ref_count = 1;
  }
  // cppcheck-suppress noExplicitConstructor
  shared_ptr(const shared_ptr& other)
      // cppcheck-suppress copyCtorPointerCopying
      : ptr(other.ptr), ref_count(other.ref_count), d(other.d) {
    if (ref_count != NULL) {
      (*ref_count)++;
    }
  }
  shared_ptr& operator=(const shared_ptr& other) {
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
    d = other.d;
    if (ref_count != NULL) {
      (*ref_count)++;
    }
    return *this;
  }
  T* operator->() const { return ptr; }
  T& operator*() const { return *ptr; }
  bool operator==(const shared_ptr& other) const { return ptr == other.ptr; }
  bool operator==(const T* other) const { return ptr == other; }
  bool operator!=(const shared_ptr& other) const { return ptr != other.ptr; }
  bool operator!=(const T* other) const { return ptr != other; }
};

#endif
