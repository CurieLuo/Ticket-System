#ifndef __SJTU_TICKETSYSTEM_HPP__
#define __SJTU_TICKETSYSTEM_HPP__

#include "TrainSystem.hpp"
#include "UserSystem.hpp"

enum Status { SUCCESS, PENDING, REFUNDED };

using PendingID = pair<TrainDay, int>; //(train_day, op_time)
struct Order {
  static const char *status_str[3];
  Status status;
  Train train;
  Station from, to;
  DateTime leave, arrive;
  int price, ticket;
  // extra information
  int l, r; // tr.sta[l] = from, tr.sta[r] = to
  PendingID pending_id;
  Order() {}
  Order(Status status_, const Train &tr, const Station &ss, const Station &ts,
        const DateTime &lv, const DateTime &arv, int price_, int tk, int l_,
        int r_, const PendingID &pd_id)
      : status(status_), train(tr), from(ss), to(ts), leave(lv), arrive(arv),
        price(price_), ticket(tk), l(l_), r(r_), pending_id(pd_id) {}
  friend ostream &operator<<(ostream &os, const Order &obj) {
    return os << obj.status_str[obj.status] << ' ' << obj.train << ' '
              << obj.from << ' ' << obj.leave << " -> " << obj.to << ' '
              << obj.arrive << ' ' << obj.price << ' ' << obj.ticket;
  }
}; //? add some redundent info for speedup?
const char *Order::status_str[3] = {"[success]", "[pending]", "[refunded]"};

struct Pending {
  int handle; // handle: quick access to order information
  int l, r;
  int ticket;
  Pending() {}
  Pending(int hd, int l_, int r_, int tk)
      : handle(hd), l(l_), r(r_), ticket(tk) {}
}; //? add some redundent info for speedup?

class TicketSystem : public UserSystem, public TrainSystem {

protected:
  CachedBPT<pair<ID, int>, Order> orders; // key: (user, order_id)
  CachedBPT<ID, int> ord_num;             // key: user, value: number of orders
  CachedBPT<PendingID, Pending>
      pending; // key: ((train, start_date), pending_id)

public:
  TicketSystem()
      : orders("orders", RETRIEVE), ord_num("orderNumber", RETRIEVE),
        pending("ordersPending", RETRIEVE) {}

  void buy_ticket(const Usr &usr, const Train &train, const Date &date,
                  int ticket, const Station &from, const Station &to,
                  bool pending_allowed, int op_time) {

    ID uid = usr.hash(), tid = train.hash();
    if (!logged_in.count(uid))
      throw "buy_ticket() failed: user not logged in";
    TrainInfo tr = trains.get(tid); //! try catch throw
    if (!tr.released)
      throw "buy_ticket() failed: train not released yet";
    if (ticket > tr.seat)
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
    SeatInfo seat = seat_it.value();
    int remainder = seat.min(l, r), price = tr.total_price(l, r);
    if (remainder < ticket && !pending_allowed)
      throw "buy_ticket() failed: tickets sold out";
    Status status = Status(remainder < ticket); // SUCCESS=0, PENDING=1
    Order ord(status, train, from, to,
              DateTime(virtual_start_date, tr.leave[l]),
              DateTime(virtual_start_date, tr.arrive[r]), price, ticket, l, r,
              PendingID(train_day, op_time));
    int ord_id = ord_num.get_default(uid); //? speedup using iterator
    if (ord_id)
      ord_num.set(uid, ord_id + 1); // already exists
    else
      ord_num.insert(uid, ord_id + 1); // create
    int handle = orders.insert(make_pair(uid, ord_id), ord);
    if (status == SUCCESS) {
      seat.add(l, r, -ticket);
      seat_it.set(seat);
      cout << (long long)price * ticket << '\n';
    } else {
      pending.insert(ord.pending_id, Pending(handle, l, r, ticket));
      cout << "queue\n";
    }
  }

  void query_order(const Usr &usr) {
    ID uid = usr.hash();
    if (!logged_in.count(uid))
      throw "query_order() failed: user not logged in";
    auto it = orders.lower_bound(make_pair(uid, 0)),
         end = orders.upper_bound(make_pair(uid, 1 << 30));
    vector<Order> vec;        // for reveral
    for (; it != end; ++it) { //? reversed traversal
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
      throw "refund_ticket() failed: no such order";
    auto it0 = orders.find(make_pair(uid, ord_id));
    Order ord = it0.value();
    if (ord.status == REFUNDED)
      throw "refund_ticket() failed: ticket already refunded";
    ID tid = ord.train.hash();
    TrainDay train_day = ord.pending_id.first;
    if (ord.status == SUCCESS) {
      auto seat_it = seats.find(train_day);
      SeatInfo seat = seat_it.value();
      seat.add(ord.l, ord.r, ord.ticket);
      auto it = pending.lower_bound(PendingID(train_day, 0)),
           end = pending.upper_bound(PendingID(train_day, 1 << 30));
      for (; it != end; ++it) {
        Pending pd = it.value();
        if (seat.min(pd.l, pd.r) >= pd.ticket) {
          // enough tickets available
          seat.add(pd.l, pd.r, -pd.ticket);
          Order tmp = orders.get_by_handle(pd.handle);
          tmp.status = SUCCESS;
          orders.set_by_handle(pd.handle, tmp);
          pending.erase(tmp.pending_id);
        }
      }
      seat_it.set(seat); // save modifications
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