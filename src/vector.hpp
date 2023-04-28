#ifndef SJTU_VECTOR_HPP
#define SJTU_VECTOR_HPP

#ifdef _CHECK_VECTOR_
#include <vector>
#endif

#include "exceptions.hpp"

namespace sjtu {
/**
 * a data container like std::vector
 * store data in a successive memory and support random access.
 */

#ifdef _CHECK_VECTOR_

using std::vector;

#else

template <typename T> class vector {
private:
  T *arr;                  // array
  size_t siz;              // size
  size_t cap;              // capacity
  std::allocator<T> alloc; // allocator

  /**
   * @brief expand to fit more elements
   */
  void expand() {
    if (siz >= cap) {
      size_t newcap = siz * 1.5 + 3;
      T *p = alloc.allocate(newcap);
      // copy
      for (size_t i = 0; i < siz; i++) {
        alloc.construct(p + i, arr[i]);
      }
      if (arr != nullptr) {
        for (size_t i = 0; i < siz; i++) {
          alloc.destroy(arr + i);
        }
        alloc.deallocate(arr, cap);
      }
      arr = p;
      cap = newcap;
    }
  }

public:
  /**
   * a type for actions of the elements of a vector, and you should write
   *   a class named const_iterator with same interfaces.
   */
  /**
   * you can see RandomAccessIterator at CppReference for help.
   */
  class const_iterator;
  class iterator {
    // The following code is written for the C++ type_traits library.
    // Type traits is a C++ feature for describing certain properties of a type.
    // For instance, for an iterator, iterator::value_type is the type that the
    // iterator points to.
    // STL algorithms and containers may use these type_traits (e.g. the
    // following typedef) to work properly. In particular, without the following
    // code,
    // @code{std::sort(iter, iter1);} would not compile.
    // See these websites for more information:
    // https://en.cppreference.com/w/cpp/header/type_traits
    // About value_type:
    // https://blog.csdn.net/u014299153/article/details/72419713 About
    // iterator_category: https://en.cppreference.com/w/cpp/iterator
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::output_iterator_tag;
    using iterator_type = iterator;

  private:
    /**
     * data members
     */
    T *beg, *ptr;
    // beg is for judging whether two iterators point to the same vector

  public:
    iterator(T *b, T *p) : beg(b), ptr(p) {} //! How to get rid of this?
    iterator() : beg(nullptr), ptr(nullptr) {}
    /**
     * return a new iterator which points to the nth next element
     * as well as operator-
     */
    iterator operator+(const int &n) const { return iterator(beg, ptr + n); }
    iterator operator-(const int &n) const { return iterator(beg, ptr - n); }
    // return the distance between two iterators,
    // if these two iterators point to different vectors, throw
    // invaild_iterator.
    int operator-(const iterator &rhs) const {
      if (beg != rhs.beg) {
        throw invalid_iterator();
      }
      return ptr - rhs.ptr;
    }
    iterator &operator+=(const int &n) {
      ptr += n;
      return *this;
    }
    iterator &operator-=(const int &n) {
      ptr -= n;
      return *this;
    }
    /**
     * iter++
     */
    iterator operator++(int) {
      iterator tmp = *this;
      *this += 1;
      return tmp;
    }
    /**
     * ++iter
     */
    iterator &operator++() { return *this += 1; }
    /**
     * iter--
     */
    iterator operator--(int) {
      iterator tmp = *this;
      *this -= 1;
      return tmp;
    }
    /**
     * --iter
     */
    iterator &operator--() { return *this -= 1; }
    /**
     * @brief *it
     */
    T &operator*() const { return *ptr; }
    /**
     * an operator for checking whether two iterators are the same (pointing to
     * the same memory address).
     */
    bool operator==(const iterator &rhs) const { return ptr == rhs.ptr; }
    bool operator==(const const_iterator &rhs) const { return ptr == rhs.ptr; }
    /**
     * some other operators for iterator.
     */
    bool operator!=(const iterator &rhs) const { return ptr != rhs.ptr; }
    bool operator!=(const const_iterator &rhs) const { return ptr != rhs.ptr; }
  }; // class iterator
  /**
   * has the same function as iterator, just for a const object.
   */
  class const_iterator {
  public:
    using difference_type = std::ptrdiff_t;
    using value_type = T;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::output_iterator_tag;

  private:
    /**
     * data members
     */
    T *beg, *ptr;
    // beg is for judging whether two iterators point to the same vector

  public:
    const_iterator(T *b, T *p) : beg(b), ptr(p) {}
    const_iterator() : beg(nullptr), ptr(nullptr) {}
    /**
     * return a new iterator which pointer n-next elements
     * as well as operator-
     */
    const_iterator operator+(const int &n) const {
      return const_iterator(beg, ptr + n);
    }
    const_iterator operator-(const int &n) const {
      return const_iterator(beg, ptr - n);
    }
    // return the distance between two iterators,
    // if these two iterators point to different vectors, throw
    // invaild_iterator.
    int operator-(const const_iterator &rhs) const {
      if (beg != rhs.beg) {
        throw invalid_iterator();
      }
      return ptr - rhs.ptr;
    }
    const_iterator &operator+=(const int &n) {
      ptr += n;
      return *this;
    }
    const_iterator &operator-=(const int &n) {
      ptr -= n;
      return *this;
    }
    /**
     * iter++
     */
    const_iterator operator++(int) {
      const_iterator tmp = *this;
      *this += 1;
      return tmp;
    }
    /**
     * ++iter
     */
    const_iterator &operator++() { return *this += 1; }
    /**
     * iter--
     */
    const_iterator operator--(int) {
      const_iterator tmp = *this;
      *this -= 1;
      return tmp;
    }
    /**
     * --iter
     */
    const_iterator &operator--() { return *this -= 1; }
    /**
     * @brief *it
     */
    const T &operator*() const { return *ptr; }
    /**
     * a operator to check whether two iterators are same (pointing to the same
     * memory address).
     */
    bool operator==(const iterator &rhs) const { return ptr == rhs.ptr; }
    bool operator==(const const_iterator &rhs) const { return ptr == rhs.ptr; }
    /**
     * some other operator for iterator.
     */
    bool operator!=(const iterator &rhs) const { return ptr != rhs.ptr; }
    bool operator!=(const const_iterator &rhs) const { return ptr != rhs.ptr; }
  }; // class const_iterator
  /**
   * Constructs
   * At least two: default constructor, copy constructor
   */
  vector() : arr(nullptr), siz(0), cap(0) {}
  vector(const vector &other) : siz(other.siz), cap(other.cap) {
    arr = alloc.allocate(cap);
    // copy
    for (size_t i = 0; i < siz; i++) {
      alloc.construct(arr + i, other.arr[i]);
    }
  }
  /**
   * Destructor
   */
  ~vector() {
    for (size_t i = 0; i < siz; i++) {
      alloc.destroy(arr + i);
    }
    alloc.deallocate(arr, cap);
    arr = nullptr;
    siz = cap = 0;
  }
  /**
   * Assignment operator
   */
  vector &operator=(const vector &other) {
    if (this == &other) {
      return *this;
    }
    for (size_t i = 0; i < siz; i++) {
      alloc.destroy(arr + i);
    }
    alloc.deallocate(arr, cap);
    siz = other.siz, cap = other.cap;
    arr = alloc.allocate(cap);
    // copy
    for (size_t i = 0; i < siz; i++) {
      alloc.construct(arr + i, other.arr[i]);
    }
    return *this;
  }
  /**
   * assigns specified element with bounds checking
   * throw index_out_of_bound if pos is not in [0, size)
   */
  T &at(const size_t &pos) {
    if (pos >= siz) {
      throw index_out_of_bound();
    }
    return arr[pos];
  }
  const T &at(const size_t &pos) const {
    if (pos >= siz) {
      throw index_out_of_bound();
    }
    return arr[pos];
  }
  /**
   * assigns specified element with bounds checking
   * throw index_out_of_bound if pos is not in [0, size)
   *   In STL this operator does not check the boundary but I want you to do so.
   */
  T &operator[](const size_t &pos) {
    if (pos >= siz) {
      throw index_out_of_bound();
    }
    return arr[pos];
  }
  const T &operator[](const size_t &pos) const {
    if (pos >= siz) {
      throw index_out_of_bound();
    }
    return arr[pos];
  }
  /**
   * access the first element.
   * throw container_is_empty if size == 0
   */
  const T &front() const {
    if (!siz) {
      throw container_is_empty();
    }
    return arr[0];
  }
  /**
   * access the last element.
   * throw container_is_empty if size == 0
   */
  const T &back() const {
    if (!siz) {
      throw container_is_empty();
    }
    return arr[siz - 1];
  }
  /**
   * returns an iterator to the beginning.
   */
  iterator begin() { return iterator(arr, arr); }
  const_iterator cbegin() const { return const_iterator(arr, arr); }
  /**
   * returns an iterator to the end.
   */
  iterator end() { return iterator(arr, arr + siz); }
  const_iterator cend() const { return const_iterator(arr, arr + siz); }
  /**
   * checks whether the container is empty
   */
  bool empty() const { return !siz; }
  /**
   * returns the number of elements
   */
  size_t size() const { return siz; }
  /**
   * clears the contents
   */
  void clear() {
    for (size_t i = 0; i < siz; i++) {
      alloc.destroy(arr + i);
    }
    alloc.deallocate(arr, cap);
    arr = nullptr;
    siz = cap = 0;
  }
  /**
   * inserts value before pos
   * returns an iterator pointing to the inserted value.
   */
  iterator insert(iterator pos, const T &value) {
    int ind = pos - begin();
    if (ind < 0 || ind > siz) {
      throw invalid_iterator(); //? what error?
    };
    return insert(ind, value);
  }
  /**
   * inserts value at index ind.
   * after inserting, this->at(ind) == value
   * returns an iterator pointing to the inserted value.
   * throw index_out_of_bound if ind > size (in this situation ind can be size
   * because after inserting the size will increase 1.)
   */
  iterator insert(const size_t &ind, const T &value) {
    if (ind > siz) {
      throw index_out_of_bound();
    }
    expand();
    for (size_t i = siz++; i > ind; i--) {
      alloc.construct(arr + i, arr[i - 1]); // use copy constructor
    }
    alloc.construct(arr + ind, value);
    return begin() + ind;
  }
  /**
   * removes the element at pos.
   * return an iterator pointing to the following element.
   * If the iterator pos refers the last element, the end() iterator is
   * returned.
   */
  iterator erase(iterator pos) {
    int ind = pos - begin();
    if (ind < 0 || ind >= siz) {
      throw invalid_iterator(); //!? what error?
    };
    return erase(ind);
  }
  /**
   * removes the element with index ind.
   * return an iterator pointing to the following element.
   * throw index_out_of_bound if ind >= size
   */
  iterator erase(const size_t &ind) {
    if (ind > siz) {
      throw index_out_of_bound();
    }
    siz--;
    for (size_t i = ind; i < siz; i++) {
      arr[i] = arr[i + 1];
    }
    alloc.destroy(arr + siz);
    return begin() + ind;
  }
  /**
   * adds an element to the end.
   */
  void push_back(const T &value) {
    expand();
    alloc.construct(arr + siz++, value);
    // must call copy constructor, otherwise the object may be uninitialized
  }
  /**
   * remove the last element from the end.
   * throw container_is_empty if size() == 0
   */
  void pop_back() {
    if (!siz) {
      throw container_is_empty();
    }
    alloc.destroy(arr + (--siz));
  }
}; // class vector
#endif

} // namespace sjtu

#endif
