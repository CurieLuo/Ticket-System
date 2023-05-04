#ifndef __SJTU_DATETIME_HPP__
#define __SJTU_DATETIME_HPP__

#include "Scanner.hpp"
#include <iostream>
#include <string>

using std::endl;
using std::ostream;
using std::string;

constexpr int D_IN_M[] = {31, 31, 28, 31, 30, 31, 30,
                          31, 31, 30, 31, 30, 31}; // days in each month
constexpr int D_IN_M_SUM[] = {
    0,   0,   31,  59,  90,  120, 151,
    181, 212, 243, 273, 304, 334}; // prefix sum of D_IN_M
constexpr int MIN_IN_D = 1440;     // minutes in a day

/**
 * @brief records date information
 */
struct Date {
  int month, day;

public:
  Date(int m = 0, int d = 0) : month(m), day(d) {}
  Date(const string &s) {
    month = (s[0] - '0') * 10 + (s[1] - '0'),
    day = (s[3] - '0') * 10 + (s[4] - '0');
  }
  Date(const char *s) : Date(string(s)) {}
  // I/O operator(s)
  operator string() const { return to_string2(month) + '-' + to_string2(day); }
  friend ostream &operator<<(ostream &os, const Date &obj) {
    return os << string(obj);
  }
  // arithmetic operator(s)
  Date &operator+=(int days) {
    day += days;
    while (day > D_IN_M[month])
      day -= D_IN_M[month++];
    return *this;
  }
  Date operator+(int days) const {
    Date ret = *this;
    return ret += days;
  }
  Date &operator-=(int days) {
    day -= days;
    while (day < 1)
      day += D_IN_M[--month];
    return *this;
  }
  Date operator-(int days) const {
    Date ret = *this;
    return ret -= days;
  }
  int operator-(const Date &rhs) const {
    return day - rhs.day + D_IN_M_SUM[month] - D_IN_M_SUM[rhs.month];
  }
  // comparision operator(s)
  bool operator<(const Date &rhs) const {
    return month < rhs.month || (month == rhs.month && day < rhs.day);
  }
  bool operator==(const Date &rhs) const {
    return month == rhs.month && day == rhs.day;
  }
  bool operator!=(const Date &rhs) const {
    return month != rhs.month || day != rhs.day;
  }
};

/**
 * @brief records time information (time in a day)
 */
struct Time {
  int minite;

public:
  Time(int min = -1) : minite(min) {}
  Time(const string &s) {
    int h = (s[0] - '0') * 10 + (s[1] - '0'),
        min = (s[3] - '0') * 10 + (s[4] - '0');
    minite = h * 60 + min;
  }
  Time(const char *s) : Time(string(s)) {}
  Time &operator=(int min) {
    minite = min;
    return *this;
  }
  // arithmetic operator(s)
  Time operator+(int min) { return Time(minite + min); }
  Time &operator+=(int min) {
    minite += min;
    return *this;
  }
  int operator-(const Time &rhs) const { return minite - rhs.minite; }
  // I/O operator(s)
  operator string() const {
    return to_string2(minite / 60) + ':' + to_string2(minite % 60);
  }
  operator int() const { return minite; }
  friend ostream &operator<<(ostream &os, const Time &obj) {
    return os << string(obj);
  }
  // comparision operator(s)
  bool operator<(const Time &rhs) const { return minite < rhs.minite; }
  bool operator==(const Time &rhs) const { return minite == rhs.minite; }
  bool operator!=(const Time &rhs) const { return minite != rhs.minite; }
};

/**
 * @brief records date and time information
 */
struct DateTime {
  Date date;
  Time time;

public:
  DateTime() {}
  DateTime(const Date &date_, const Time &time_) : date(date_), time(time_) {
    if (time.minite >= MIN_IN_D)
      date += time.minite / MIN_IN_D, time.minite %= MIN_IN_D;
  }
  // I/O operator(s)
  operator string() const { return string(date) + ' ' + string(time); }
  friend ostream &operator<<(ostream &os, const DateTime &obj) {
    return os << string(obj.date) << ' ' << string(obj.time);
  }
  // arithmetic operator(s)
  Time operator-(const DateTime &rhs) const {
    return time - rhs.time + (date - rhs.date) * MIN_IN_D;
  }
  // comparision operator(s)
  bool operator<(const DateTime &rhs) const {
    return date < rhs.date || (date == rhs.date && time < rhs.time);
  }
  bool operator==(const DateTime &rhs) const {
    return date == rhs.date && time == rhs.time;
  }
  bool operator!=(const DateTime &rhs) const {
    return date != rhs.date || time != rhs.time;
  }
};

#endif
