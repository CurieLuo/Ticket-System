#ifndef __SJTU_UTILITY_HPP__
#define __SJTU_UTILITY_HPP__
#include "Scanner.hpp"
#include "String.hpp"
#include "datetime.hpp"
#include "vector.hpp"
#include <cstddef>
#include <functional>
#include <iostream>
#include <utility>

using sjtu::vector;

using std::make_pair;
using std::make_tuple;
using std::pair;
using std::tuple;

using std::string;

using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::flush;
using std::ios;
using std::istream;
using std::ostream;

using std::max;
using std::min;
using std::swap;

using ID = size_t; // hash value

/**
 * @brief sorts vector a[l, l+1, ..., r] with comparator (function) cmp
 */
template <class T, class Cmp>
void qsort(vector<T> &a, int l, int r, const Cmp &cmp) {
  if (l >= r) {
    return;
  }
  int i = l - 1, j = r + 1;
  T x = a[(l + r) >> 1];
  while (1) {
    do {
      i++;
    } while (cmp(a[i], x));
    do {
      j--;
    } while (cmp(x, a[j]));
    if (i >= j) {
      break;
    }
    swap(a[i], a[j]);
  }
  qsort(a, l, j, cmp), qsort(a, j + 1, r, cmp);
}
template <class T, class Cmp>
inline void sort(vector<T> &a, int l, int r, const Cmp &cmp) {
  qsort(a, l, r, cmp);
}

template <class T1, class T2> inline void getmax(T1 &x, const T2 &y) {
  if (x < y)
    x = y;
}
template <class T1, class T2> inline void getmin(T1 &x, const T2 &y) {
  if (y < x)
    x = y;
}

#endif
