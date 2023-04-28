#ifndef __SJTU_TRAINSYSTEM_HPP__
#define __SJTU_TRAINSYSTEM_HPP__

#include "CachedBPT.hpp"
#include "utility.hpp"
#include <iostream>

constexpr int STA_NUM = 101;

struct TrainInfo {
  bool released = 0;
  char type;            // D/G/etc.
  int size;             // stationNum
  Station sta[STA_NUM]; // stations (0-base)
  int seat;             // seatNum
  int price[STA_NUM];   // prefix sum of prices

  Date date0, date1;                   // saleDate
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
          leave[i] = arrive[i] + scan.next_int(); // atoi(invalid string)=0
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

using TrainDay = pair<ID, int>; //(train, start_date)
/**
 * @brief keeps track of the number of seats available on every released
 * train seat[i]: remaining seats from station no. i to no. i+1 supports
 * interval addition and interval minimum query
 * TODO speadup using buckets
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
 * @brief record released trains that pass by a given station
 */
struct Passby {
  Train train;
  int handle; // handle: quick access to train information
  int idx;    // TrainInfo.sta[idx] = station
  Passby() {}
  Passby(const Train &tr, int hd, int i = 0) : train(tr), handle(hd), idx(i) {}
}; //? add some redundent info for speedup?

/**
 * @brief record passage from one station to another
 */
struct PassInfo {
  Train train;
  Station from, to;
  DateTime leave, arrive;
  int time, price, seat;
  PassInfo() {}
  PassInfo(const Train &tr, const Station &ss, const Station &ts,
           const DateTime &lv, const DateTime &arv, int tm, int p, int st)
      : train(tr), from(ss), to(ts), leave(lv), arrive(arv), time(tm), price(p),
        seat(st) {}
  friend ostream &operator<<(ostream &os, const PassInfo &p) {
    return os << p.train << ' ' << p.from << ' ' << p.leave << " -> " << p.to
              << ' ' << p.arrive << ' ' << p.price << ' ' << p.seat;
  }
};
struct Transfer {
  PassInfo pass, pass2;
  int time = -1, cost = -1;
  Transfer() {}
  Transfer(const PassInfo &ps, const PassInfo &ps2) : pass(ps), pass2(ps2) {
    time = ps2.arrive - ps.leave;
    cost = ps.price + ps2.price;
  }
};
struct LessTime {
  bool operator()(const PassInfo &lhs, const PassInfo &rhs) const {
    return lhs.time < rhs.time || lhs.time == rhs.time && lhs.train < rhs.train;
  }
  bool operator()(const Transfer &lhs, const Transfer &rhs) const {
    return make_tuple(lhs.time, lhs.cost, lhs.pass.train, lhs.pass2.train) <
           make_tuple(rhs.time, rhs.cost, rhs.pass.train, rhs.pass2.train);
  }
} less_time;
struct LessCost {
  bool operator()(const PassInfo &lhs, const PassInfo &rhs) const {
    return lhs.price < rhs.price ||
           lhs.price == rhs.price && lhs.train < rhs.train;
  }
  bool operator()(const Transfer &lhs, const Transfer &rhs) const {
    return make_tuple(lhs.cost, lhs.time, lhs.pass.train, lhs.pass2.train) <
           make_tuple(rhs.cost, rhs.time, rhs.pass.train, rhs.pass2.train);
  }
} less_cost;

class TrainSystem {

protected:
  CachedBPT<ID, TrainInfo> trains; // key: train
  // TODO caching values for some BPTs
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
    TrainInfo tr = trains.get(tid); //! try catch throw
    if (tr.released)
      throw "delete_train() failed: train already released";
    trains.erase(tid);
    cout << "0\n";
  }

  void release_train(const Train &train) {
    ID tid = train.hash();
    auto it = trains.find(tid);
    TrainInfo tr = it.value(); //! try catch throw
    if (tr.released)
      throw "delete_train() failed: train already released";
    tr.released = 1;
    it.set(tr);
    SeatInfo seat = SeatInfo(tr.seat, tr.size - 1);
    for (int i = 0, d = tr.date1 - tr.date0; i <= d; i++)
      seats.insert(TrainDay(tid, i), seat); //? lazy

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
    TrainInfo tr = trains.get(tid); //! try catch throw
    if (tr.invalid_date(date))
      throw "query_train() failed: invalid date";
    cout << train << ' ' << tr.type << '\n';
    SeatInfo seat;
    if (tr.released)
      seat = seats.get(TrainDay(tid, date - tr.date0)); //? lazy
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
        st = seat[i];
      }
      cout << st << '\n';
    }
  }

  void query_ticket(const Station &from, const Station &to, const Date &date,
                    bool by_cost) {
    vector<PassInfo> ans;
    ID sid = from.hash(), sid2 = to.hash();
    auto it = passby.lower_bound(make_pair(sid, 0)),
         end = passby.upper_bound(make_pair(sid, ID(-1))),
         it2 = passby.lower_bound(make_pair(sid2, 0)),
         end2 = passby.upper_bound(make_pair(sid2, ID(-1)));
    for (; it != end; ++it) { // enumerate trains passing station "from"
      ID tid = it.key().second;
      while (it2 != end2 && it2.key().second < tid)
        ++it2; //$ two pointers method
      if (it2 == end2)
        break;
      if (it2.key().second != tid)
        continue;
      Passby psb = it.value(), psb2 = it2.value();
      int l = psb.idx, r = psb2.idx;
      if (l > r)
        continue;
      TrainInfo tr = trains.get_by_handle(psb.handle);
      Date virtual_start_date =
          date - tr.leave[l] / MIN_IN_D; //! must count from sta[0] (deduce)
      if (tr.invalid_date(virtual_start_date))
        continue; // check starting date
      // found an answer
      const Train &train = psb.train;
      DateTime leave(virtual_start_date, tr.leave[l]),
          arrive(virtual_start_date, tr.arrive[r]);
      SeatInfo seat = seats.get(TrainDay(tid, virtual_start_date - tr.date0));
      ans.push_back(PassInfo(train, from, to, leave, arrive,
                             tr.total_time(l, r), tr.total_price(l, r),
                             seat.min(l, r)));
    }
    int n = ans.size();
    if (by_cost)
      sort(ans, 0, n - 1, less_cost);
    else
      sort(ans, 0, n - 1, less_time);
    cout << n << '\n';
    for (auto ps : ans)
      cout << ps << '\n';
  }

  void query_transfer(const Station &from, const Station &to, const Date &date,
                      bool by_cost) {
    ID sid = from.hash(), sid2 = to.hash();
    auto it = passby.lower_bound(make_pair(sid, 0)),
         end = passby.upper_bound(make_pair(sid, ID(-1))),
         it2 = passby.lower_bound(make_pair(sid2, 0)),
         end2 = passby.upper_bound(make_pair(sid2, ID(-1)));
    vector<Passby> vec; // for caching repeated range iteration
    for (; it2 != end2; ++it2) {
      vec.push_back(it2.value());
    }

    Transfer ans;
    bool flag = 0; // has found a potential answer
    for (; it != end; ++it) {
      Passby psb = it.value();
      TrainInfo tr = trains.get_by_handle(psb.handle);
      int l = psb.idx;
      Date virtual_start_date = date - tr.leave[l] / MIN_IN_D;
      if (tr.invalid_date(virtual_start_date))
        continue; // check starting date

      const Train &train = psb.train;
      ID tid = it.key().second;
      DateTime leave(virtual_start_date, tr.leave[l]);

#ifdef _CHECK_HASHMAP_
      std::map<Station, int> map; //!!!!!!!!!! debug
#else
      Hashmap<Station, int, size_t(-1), std::hash<string>>
          map; // TODO test create new vs. clear
#endif
      for (int r = l + 1; r < tr.size; r++)
        map.insert(make_pair(tr.sta[r], r)); // maps station to index in tr

      for (const auto &psb2 : vec) {
        int r2 = psb2.idx;
        const Train &train2 = psb2.train;
        ID tid2 = train2.hash();
        if (tid2 == tid)
          continue; // must take two different trains
        TrainInfo tr2 = trains.get_by_handle(psb2.handle);
        for (int l2 = r2 - 1; l2 >= 0; l2--) {
          const Station &mid = tr2.sta[l2]; // transfer station
          auto mp_it_ = map.find(mid);
          if (mp_it_ == map.end())
            continue;
          int r = mp_it_->second; // tr[l] -> tr[r]=tr2[l2] -> tr2[r2]
          if (l >= r)
            continue;
          DateTime arrive(virtual_start_date, tr.arrive[r]);
          if (DateTime(tr2.date1, tr2.leave[l2]) < arrive)
            continue; // fail to catch the last train (tr2)
          // minimize leave2 s.t. leave2 >= arrive
          DateTime earliest = DateTime(tr2.date0, tr2.leave[l2]),
                   leave2 = earliest;
          Date virtual_start_date2 = tr2.date0;
          if (leave2 < arrive) {
            leave2.date = arrive.date + (int)(leave2.time < arrive.time);
            virtual_start_date2 += leave2.date - earliest.date;
          }
          DateTime arrive2(virtual_start_date2, tr2.arrive[r2]);

          Transfer res(PassInfo(train, from, mid, leave, arrive, -1,
                                tr.total_price(l, r), 0),
                       PassInfo(train2, mid, to, leave2, arrive2, -1,
                                tr2.total_price(l2, r2),
                                0)); // seats are not calculated yet
          if (!flag || (by_cost ? less_cost(res, ans) : less_time(res, ans))) {
            SeatInfo seat = seats.get(
                         TrainDay(tid, virtual_start_date - tr.date0)),
                     seat2 = seats.get(
                         TrainDay(tid2, virtual_start_date2 - tr2.date0));
            res.pass.seat = seat.min(l, r), res.pass2.seat = seat2.min(l2, r2);
            flag = 1, ans = res;
          }
        }
      }
    }
    if (!flag) {
      cout << "0\n";
    } else {
      cout << ans.pass << '\n' << ans.pass2 << '\n';
    }
  }

}; // class TrainSystem

#endif
