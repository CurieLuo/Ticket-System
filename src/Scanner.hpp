#ifndef __SJTU_SCANNER_HPP__
#define __SJTU_SCANNER_HPP__

#include <string>

using std::string;

inline int to_int(const string &s) { return atoi(s.data()); }
inline string to_string2(int x) {
  string ret;
  ret += x / 10 + '0', ret += x % 10 + '0';
  return ret;
}
class Scanner {
private:
  string s;
  size_t len;
  size_t cur;
  char sep;

public:
  Scanner(char sp = ' ') : cur(0), len(0), sep(sp) {}
  Scanner(const string &input, char sp = ' ')
      : cur(0), s(input), len(input.size()), sep(sp) {}
  void init(const string &input, char sp = ' ') {
    cur = 0, s = input, len = input.size(), sep = sp;
  }
  operator bool() {
    while (cur < len && s[cur] == sep)
      cur++;
    return cur < len;
  }
  string next() {
    string ret;
    while (cur < len && s[cur] == sep)
      cur++;
    while (cur < len && s[cur] != sep)
      ret += s[cur++];
    return ret;
  }
  int next_int() { return to_int(next()); }
  char next_arg() { return next()[1]; }
};

#endif