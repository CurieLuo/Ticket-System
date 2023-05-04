#ifndef __SJTU_TICKETSYSTEM_HPP__
#define __SJTU_TICKETSYSTEM_HPP__

#include "TrainSystem.hpp"
#include "UserSystem.hpp"

enum Status { SUCCESS, PENDING, REFUNDED };

using PendingID = pair<TrainDay, int>; //(train_day, op_time)

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
 * @brief records order information
 */
struct Order {
  static const char *status_str[3];
  Status status;
  Train train;
  Station from, to;
  DateTime leave, arrive;
  int price, ticket_num;
  // extra information
  int l, r; // tr.sta[l] = from, tr.sta[r] = to
  PendingID pending_id;
  Order() {}
  Order(Status status_, const Train &tr, const Station &ss, const Station &ts,
        const DateTime &lv, const DateTime &arv, int price_, int tk, int l_,
        int r_, const PendingID &pd_id)
      : status(status_), train(tr), from(ss), to(ts), leave(lv), arrive(arv),
        price(price_), ticket_num(tk), l(l_), r(r_), pending_id(pd_id) {}
  friend ostream &operator<<(ostream &os, const Order &obj) {
    return os << obj.status_str[obj.status] << ' ' << obj.train << ' '
              << obj.from << ' ' << obj.leave << " -> " << obj.to << ' '
              << obj.arrive << ' ' << obj.price << ' ' << obj.ticket_num;
  }
};
const char *Order::status_str[3] = {"[success]", "[pending]", "[refunded]"};

/**
 * @brief maintains pending orders
 */
struct Pending {
  int handle; // handle: quick access to order information
  int l, r;
  int ticket_num;
  Pending() {}
  Pending(int hd, int l_, int r_, int tk)
      : handle(hd), l(l_), r(r_), ticket_num(tk) {}
};

/**
 * @brief processes ticket related operations
 */
class TicketSystem : public UserSystem, public TrainSystem {

protected:
  CachedBPT<pair<ID, int>, Order> orders; // key: (user, order_id)
  CachedBPT<ID, int> ord_num;             // key: user, value: number of orders
  CachedBPT<PendingID, Pending>
      pending; // key: ((train, start_date), pending_op_time)

public:
  TicketSystem()
      : orders("orders", RETRIEVE), ord_num("orderNumber", RETRIEVE),
        pending("ordersPending", RETRIEVE) {}

  void query_ticket(const Station &from, const Station &to, const Date &date,
                    bool by_cost) {
    vector<Ticket> ans;
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
      Date virtual_start_date = date - tr.leave[l] / MIN_IN_D;
      // must count the date as if we started from sta[0]
      if (tr.invalid_date(virtual_start_date))
        continue; // check starting date
      // found an answer
      const Train &train = psb.train;
      DateTime leave(virtual_start_date, tr.leave[l]),
          arrive(virtual_start_date, tr.arrive[r]);
      SeatInfo seatinfo =
          seats.get(TrainDay(tid, virtual_start_date - tr.date0));
      ans.push_back(Ticket(train, from, to, leave, arrive, tr.total_time(l, r),
                           tr.total_price(l, r), seatinfo.min(l, r)));
    }
    int n = ans.size();
    if (by_cost)
      sort(ans, 0, n - 1, less_cost);
    else
      sort(ans, 0, n - 1, less_time);
    cout << n << '\n';
    for (auto ticket : ans)
      cout << ticket << '\n';
  }

  /**
   * @brief take train from l to r, then take train2 from l2 to r2
   */
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

    Hashmap<Station, int, size_t(-1), std::hash<string>>
        map; // match train.station[r] with train2.station[l2]
    Transfer ans;
    bool flag = 0; // flag = 1: has found a potential answer
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

      map.clear();
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
          auto mp_it = map.find(mid);
          if (mp_it == map.end())
            continue;
          int r = mp_it->second; // tr[l] -> tr[r]=tr2[l2] -> tr2[r2]
          if (l >= r)
            continue;
          DateTime arrive(virtual_start_date, tr.arrive[r]);
          if (DateTime(tr2.date1, tr2.leave[l2]) < arrive)
            continue; // fail to catch the last train (tr2)
          // minimize leave2 s.t. leave2 >= arrive
          DateTime
              earliest = DateTime(tr2.date0, tr2.leave[l2]),
              leave2 =
                  earliest; // default: take the first train (tr2) available
          Date virtual_start_date2 = tr2.date0;
          if (leave2 < arrive) {
            // upon arrival, take the first train that hasn't left yet
            leave2.date = arrive.date + (int)(leave2.time < arrive.time);
            virtual_start_date2 += leave2.date - earliest.date;
          }
          DateTime arrive2(virtual_start_date2, tr2.arrive[r2]);

          Transfer res(Ticket(train, from, mid, leave, arrive, -1,
                              tr.total_price(l, r), 0),
                       Ticket(train2, mid, to, leave2, arrive2, -1,
                              tr2.total_price(l2, r2),
                              0)); // seats are not calculated yet
          if (!flag || (by_cost ? less_cost(res, ans) : less_time(res, ans))) {
            SeatInfo seatinfo = seats.get(
                         TrainDay(tid, virtual_start_date - tr.date0)),
                     seatinfo2 = seats.get(
                         TrainDay(tid2, virtual_start_date2 - tr2.date0));
            res.ticket.seat = seatinfo.min(l, r),
            res.ticket2.seat = seatinfo2.min(l2, r2);
            flag = 1, ans = res;
          }
        }
      }
    }
    if (!flag) {
      cout << "0\n";
    } else {
      cout << ans.ticket << '\n' << ans.ticket2 << '\n';
    }
  }

  /**
   * @param op_time the timestamp of the operation, needed since we have to
   * process the pending requests in ascend chronological order
   */
  void buy_ticket(const Usr &usr, const Train &train, const Date &date,
                  int ticket_num, const Station &from, const Station &to,
                  bool pending_allowed, int op_time) {

    ID uid = usr.hash(), tid = train.hash();
    if (!logged_in.count(uid))
      throw "buy_ticket() failed: user not logged in";
    TrainInfo tr = trains.get(tid); //! try catch throw for more error info
    if (!tr.released)
      throw "buy_ticket() failed: train not released yet";
    if (ticket_num > tr.seat)
      throw "buy_ticket() failed: not enough seats";
    int l = -1, r = -1;
    for (int i = 0; i < tr.size; i++) {
      if (tr.sta[i] == from)
        l = i;
      else if (tr.sta[i] == to)
        r = i;
    }
    if (l == -1 || r == -1 || l >= r)
      throw "buy_ticket() failed: invalid stations";
    Date virtual_start_date = date - tr.leave[l] / MIN_IN_D;
    if (tr.invalid_date(virtual_start_date))
      throw "buy_ticket() failed: invalid date";
    TrainDay train_day(tid, virtual_start_date - tr.date0);
    auto seat_it = seats.find(train_day);
    SeatInfo seatinfo = seat_it.value();
    int remainder = seatinfo.min(l, r), price = tr.total_price(l, r);
    if (remainder < ticket_num && !pending_allowed)
      throw "buy_ticket() failed: tickets sold out";
    Status status = Status(remainder < ticket_num); // SUCCESS=0, PENDING=1
    Order ord(status, train, from, to,
              DateTime(virtual_start_date, tr.leave[l]),
              DateTime(virtual_start_date, tr.arrive[r]), price, ticket_num, l,
              r, PendingID(train_day, op_time));
    int ord_id = ord_num.get_default(uid);
    // update the number of orders
    if (ord_id)
      ord_num.set(uid, ord_id + 1); // already exists
    else
      ord_num.insert(uid, ord_id + 1); // create
    int handle = orders.insert(make_pair(uid, ord_id), ord);
    if (status == SUCCESS) {
      seatinfo.add(l, r, -ticket_num); // buy
      seat_it.set(seatinfo);
      cout << (long long)price * ticket_num << '\n';
    } else {
      pending.insert(ord.pending_id, Pending(handle, l, r, ticket_num));
      cout << "queue\n";
    }
  }

  void query_order(const Usr &usr) {
    ID uid = usr.hash();
    if (!logged_in.count(uid))
      throw "query_order() failed: user not logged in";
    auto it = orders.lower_bound(make_pair(uid, 0)),
         end = orders.upper_bound(make_pair(uid, 1 << 30));
    vector<Order> vec;
    for (; it != end; ++it) {
      vec.push_back(it.value());
    }
    int n = vec.size();
    cout << n << '\n';
    for (int i = n - 1; i >= 0; i--)
      cout << vec[i] << '\n';
  }

  void refund_ticket(const Usr &usr, int ord_id) {
    ID uid = usr.hash();
    if (!logged_in.count(uid))
      throw "refund_ticket() failed: user not logged in";
    ord_id = ord_num.get_default(uid) - ord_id; // possibly negative
    if (ord_id < 0)
      throw "refund_ticket() failed: order not found";
    auto it0 = orders.find(make_pair(uid, ord_id));
    Order ord = it0.value();
    if (ord.status == REFUNDED)
      throw "refund_ticket() failed: ticket already refunded";
    TrainDay train_day = ord.pending_id.first;
    if (ord.status == SUCCESS) {
      auto seat_it = seats.find(train_day);
      SeatInfo seatinfo = seat_it.value();
      seatinfo.add(ord.l, ord.r, ord.ticket_num); // refund
      auto it = pending.lower_bound(PendingID(train_day, 0)),
           end = pending.upper_bound(PendingID(train_day, 1 << 30));
      for (; it != end; ++it) {
        // try to execute pending orders in ascending chronological order
        Pending pd = it.value();
        if (seatinfo.min(pd.l, pd.r) >= pd.ticket_num) {
          // enough tickets available
          seatinfo.add(pd.l, pd.r, -pd.ticket_num); // buy
          Order tmp = orders.get_by_handle(pd.handle);
          tmp.status = SUCCESS;
          orders.set_by_handle(pd.handle, tmp);
          pending.erase(tmp.pending_id);
        }
      }
      seat_it.set(seatinfo); // save modifications
    } else {
      // status == PENDING
      pending.erase(ord.pending_id);
    }
    ord.status = REFUNDED;
    it0.set(ord);
    cout << "0\n";
  }

  virtual void clean() override {
    UserSystem::clean();
    TrainSystem::clean();
    orders.clear();
    ord_num.clear();
    pending.clear();
    cout << "0\n";
  }

}; // class TrainSystem

#endif
