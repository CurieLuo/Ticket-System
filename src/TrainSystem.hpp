#ifndef __SJTU_TRAINSYSTEM_HPP__
#define __SJTU_TRAINSYSTEM_HPP__

#include "CachedBPT.hpp"
#include "utility.hpp"
#include <iostream>

constexpr int STA_NUM = 101;

using TrainDay = pair<ID, int>;
//(train, start_date), encodes a train that starts on a given day

struct TrainInfo {
  bool released = 0;
  char type;            // D/G/etc.
  int size;             // stationNum
  Station sta[STA_NUM]; // stations (0-base)
  int seat;             // seatNum
  int price[STA_NUM];   // prefix sum of prices

  Date date0, date1;                   // saleDate (start, end)
  int arrive[STA_NUM], leave[STA_NUM]; // time in the day

  TrainInfo() {}
  TrainInfo(int sta_num_, int seat_num_, const string &sta_str,
            const string &prices_str, const Time &st_time,
            const string &trav_times_str, const string &stop_times_str,
            const string &sale_date_str, char type_)
      : size(sta_num_), seat(seat_num_), type(type_) {
    Scanner scan(sta_str, '|');
    for (int i = 0; i < size; i++)
      sta[i] = scan.next();

    scan.init(prices_str, '|');
    price[0] = 0;
    for (int i = 1; i < size; i++)
      price[i] = price[i - 1] + scan.next_int();

    scan.init(sale_date_str, '|');
    date0 = scan.next(), date1 = scan.next();

    arrive[0] = leave[0] = st_time;
    scan.init(trav_times_str, '|');
    for (int i = 1; i < size; i++)
      arrive[i] = scan.next_int(); // temporary
    scan.init(stop_times_str, '|');
    for (int i = 1; i < size; i++)
      arrive[i] += leave[i - 1],
          leave[i] = arrive[i] + scan.next_int(); // atoi(invalid string) = 0
  }

  /**
   * @warning must consider offset if the starting station is not sta[0]
   */
  bool invalid_date(const Date &dt) const { return dt < date0 || date1 < dt; }
  /**
   * @brief total price from station no. l to no. r
   */
  int total_price(int l, int r) const { return price[r] - price[l]; }
  int total_time(int l, int r) const { return arrive[r] - leave[l]; }
};

/**
 * @brief keeps track of the number of seats available on every released.
 * train seat[i]: remaining seats from station no. i to no. i+1.
 * supports interval addition and interval minimum query
 */
struct SeatInfo {
  int seat[STA_NUM];
  int size;

  SeatInfo(int mx = 0, int sz = 0) : size(sz) {
    std::fill(seat, seat + sz, mx);
  }
  int min(int l, int r) const {
    int ret = seat[l];
    for (int i = l + 1; i < r; i++)
      getmin(ret, seat[i]);
    return ret;
  }
  void add(int l, int r, int x) {
    for (int i = l; i < r; i++)
      seat[i] += x;
  }
  int operator[](int idx) const { return seat[idx]; }
};

/**
 * @brief  maintains released trains that pass by a given station
 */
struct Passby {
  Train train;
  int handle; // handle: quick access to train information
  int idx;    // station index, i.e. TrainInfo.sta[idx] = station
  Passby() {}
  Passby(const Train &tr, int hd, int i = 0) : train(tr), handle(hd), idx(i) {}
};

/**
 * @brief records ticket (passage) information
 */
struct Ticket {
  Train train;
  Station from, to;
  DateTime leave, arrive;
  int time, price, seat; // total time/price/seats
  Ticket() {}
  Ticket(const Train &tr, const Station &ss, const Station &ts,
         const DateTime &lv, const DateTime &arv, int tm, int p, int st)
      : train(tr), from(ss), to(ts), leave(lv), arrive(arv), time(tm), price(p),
        seat(st) {}
  friend ostream &operator<<(ostream &os, const Ticket &p) {
    return os << p.train << ' ' << p.from << ' ' << p.leave << " -> " << p.to
              << ' ' << p.arrive << ' ' << p.price << ' ' << p.seat;
  }
};
/**
 * @brief records transfer ticket information
 */
struct Transfer {
  Ticket ticket, ticket2;
  int time = -1, cost = -1;
  Transfer() {}
  Transfer(const Ticket &tk, const Ticket &tk2) : ticket(tk), ticket2(tk2) {
    time = tk2.arrive - tk.leave;
    cost = tk.price + tk2.price;
  }
};
/**
 * @brief comparators by time/cost
 */
struct LessTime {
  bool operator()(const Ticket &lhs, const Ticket &rhs) const {
    return lhs.time < rhs.time ||
           (lhs.time == rhs.time && lhs.train < rhs.train);
  }
  bool operator()(const Transfer &lhs, const Transfer &rhs) const {
    return make_tuple(lhs.time, lhs.cost, lhs.ticket.train, lhs.ticket2.train) <
           make_tuple(rhs.time, rhs.cost, rhs.ticket.train, rhs.ticket2.train);
  }
} less_time;
struct LessCost {
  bool operator()(const Ticket &lhs, const Ticket &rhs) const {
    return lhs.price < rhs.price ||
           (lhs.price == rhs.price && lhs.train < rhs.train);
  }
  bool operator()(const Transfer &lhs, const Transfer &rhs) const {
    return make_tuple(lhs.cost, lhs.time, lhs.ticket.train, lhs.ticket2.train) <
           make_tuple(rhs.cost, rhs.time, rhs.ticket.train, rhs.ticket2.train);
  }
} less_cost;

/**
 * @brief processes train related operations
 */
class TrainSystem {
protected:
  CachedBPT<ID, TrainInfo> trains;        // key: train
  CachedBPT<TrainDay, SeatInfo> seats;    // key: (train, day)
  CachedBPT<pair<ID, ID>, Passby> passby; // key: (station, train)

  virtual void clean() {
    trains.clear();
    seats.clear();
    passby.clear();
  }

public:
  TrainSystem()
      : trains("trains", RETRIEVE), seats("seats", RETRIEVE),
        passby("trainsPassing", RETRIEVE) {}

  void add_train(const Train &train, int sta_num, int seat_num,
                 const string &sta_str, const string &prices_str,
                 const Time &st_time, const string &trav_times_str,
                 const string &stop_times_str, const string &sale_date_str,
                 char type) {
    ID tid = train.hash();
    if (trains.count(tid))
      throw "add_train() failed: train already exists";
    TrainInfo tr(sta_num, seat_num, sta_str, prices_str, st_time,
                 trav_times_str, stop_times_str, sale_date_str, type);
    trains.insert(tid, tr);
    cout << "0\n";
  }

  void delete_train(const Train &train) {
    ID tid = train.hash();
    TrainInfo tr = trains.get(tid); //! try catch throw for further error info
    if (tr.released)
      throw "delete_train() failed: train already released";
    trains.erase(tid);
    cout << "0\n";
  }

  void release_train(const Train &train) {
    ID tid = train.hash();
    auto it = trains.find(tid);
    TrainInfo tr = it.value(); //! try catch throw for further error info
    if (tr.released)
      throw "delete_train() failed: train already released";
    tr.released = 1;
    it.set(tr);
    SeatInfo seatinfo = SeatInfo(tr.seat, tr.size - 1);
    for (int i = 0, d = tr.date1 - tr.date0; i <= d; i++)
      seats.insert(TrainDay(tid, i), seatinfo);

    int handle = it.handle();
    Passby psb(train, handle);
    for (int i = 0; i < tr.size; i++) {
      psb.idx = i;
      passby.insert(make_pair(tr.sta[i].hash(), tid), psb);
    }
    cout << "0\n";
  }

  void query_train(const Train &train, const Date &date) {
    ID tid = train.hash();
    TrainInfo tr = trains.get(tid); //! try catch throw for further error info
    if (tr.invalid_date(date))
      throw "query_train() failed: invalid date";
    cout << train << ' ' << tr.type << '\n';
    SeatInfo seatinfo;
    if (tr.released)
      seatinfo = seats.get(TrainDay(tid, date - tr.date0));
    for (int i = 0; i < tr.size; i++) {
      cout << tr.sta[i] << ' ';
      if (i == 0)
        cout << "xx-xx xx:xx";
      else
        cout << DateTime(date, tr.arrive[i]);
      cout << " -> ";
      if (i == tr.size - 1)
        cout << "xx-xx xx:xx";
      else
        cout << DateTime(date, tr.leave[i]);
      cout << ' ' << tr.price[i] << ' ';
      if (i == tr.size - 1) {
        cout << "x\n";
        break;
      }
      int st = tr.seat;
      if (tr.released) {
        st = seatinfo[i];
      }
      cout << st << '\n';
    }
  }

}; // class TrainSystem

#endif
