#ifndef _SJTU_STRING_HPP_
#define _SJTU_STRING_HPP_

#include <cstring>
#include <string>

using std::string;

std::hash<string> hash_str_;
/**
 * @brief unsafe string with maximum size MaxLen
 */
template <size_t MaxLen_> class String {
  static constexpr size_t MaxLen = MaxLen_;
  size_t len;
  char s[MaxLen + 2];

public:
  String() : len(0) { memset(s, 0, sizeof(s)); }
  String(const char *str) : len(strlen(str)) { strcpy(s, str); }
  String(const string &str) : len(str.size()) { strcpy(s, str.data()); }

  size_t size() const { return len; }
  const char *data() const { return s; }
  operator string() const { return string(s); }
  operator bool() const { return len; }
  char &operator[](size_t idx) { return s[idx]; }
  const char &operator[](size_t idx) const { return s[idx]; }
  bool operator<(const String &rhs) const { return strcmp(s, rhs.s) < 0; }
  bool operator>(const String &rhs) const { return strcmp(s, rhs.s) > 0; }
  bool operator==(const String &rhs) const { return strcmp(s, rhs.s) == 0; }
  bool operator!=(const String &rhs) const { return strcmp(s, rhs.s) != 0; }
  bool operator>=(const String &rhs) const { return strcmp(s, rhs.s) >= 0; }
  bool operator<=(const String &rhs) const { return strcmp(s, rhs.s) <= 0; }
  friend std::istream &operator>>(std::istream &is, String &obj) {
    return is >> obj.s;
  }
  friend std::ostream &operator<<(std::ostream &os, const String &obj) {
    return os << obj.s;
  }
  size_t hash() const { return hash_str_(*this); }
}; // class String

using Usr = String<20>;
using Pwd = String<30>;
using Name = String<5 * 3>;
using Mail = String<30>;
using Train = String<20>;
using Station = String<10 * 3>;

#endif
