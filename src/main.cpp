#define RETRIEVE true

#ifdef DEBUG
#include <cassert>
#define SHOW(x) (cerr << #x "=" << (x) << endl, (x))
#else
#define SHOW(x) (x)
#endif

#include "CachedBPT.hpp"
#include "TicketSystem.hpp"
#include <cstdio>

struct ArgMap {
  string a[26];
  string &operator[](char idx) { return a[idx - 'a']; }
  const string &operator[](char idx) const { return a[idx - 'a']; }
};

int main() {
  ios::sync_with_stdio(0);
  TicketSystem sys;

  string input, op_time, op;
  char ch;

  while (1) {
    getline(cin, input);
    Scanner scan(input);
    op_time = scan.next();
    cout << op_time << ' ';
    op = scan.next();
    ArgMap arg;
    while (scan) {
      ch = scan.next_arg();
      arg[ch] = scan.next();
    }
    try {
      // UserSystem
      if (op == "add_user") {
        sys.add_user(arg['c'], arg['u'], arg['p'], arg['n'], arg['m'],
                     to_int(arg['g']));
      } else if (op == "login") {
        sys.login(arg['u'], arg['p']);
      } else if (op == "logout") {
        sys.logout(arg['u']);
      } else if (op == "query_profile") {
        sys.query_profile(arg['c'], arg['u']);
      } else if (op == "modify_profile") {
        sys.modify_profile(arg['c'], arg['u'], arg['p'], arg['n'], arg['m'],
                           (arg['g'].empty() ? -1 : to_int(arg['g'])));
      }

      // TrainSystem
      else if (op == "add_train") {
        sys.add_train(arg['i'], to_int(arg['n']), to_int(arg['m']), arg['s'],
                      arg['p'], arg['x'], arg['t'], arg['o'], arg['d'],
                      arg['y'][0]);
      } else if (op == "delete_train") {
        sys.delete_train(arg['i']);
      } else if (op == "release_train") {
        sys.release_train(arg['i']);
      } else if (op == "query_train") {
        sys.query_train(arg['i'], arg['d']);
      }

      // TicketSystem
      else if (op == "query_ticket") {
        sys.query_ticket(arg['s'], arg['t'], arg['d'], arg['p'] == "cost");
      } else if (op == "query_transfer") {
        sys.query_transfer(arg['s'], arg['t'], arg['d'], arg['p'] == "cost");
      } else if (op == "buy_ticket") {
        sys.buy_ticket(arg['u'], arg['i'], arg['d'], to_int(arg['n']), arg['f'],
                       arg['t'], arg['q'] == "true",
                       to_int(op_time.substr(1, op_time.size() - 2)));
      } else if (op == "query_order") {
        sys.query_order(arg['u']);
      } else if (op == "refund_ticket") {
        sys.refund_ticket(arg['u'], max(to_int(arg['n']), 1));
      }

      // global
      else if (op == "clean") {
        sys.clean();
      } else if (op == "exit") {
        cout << "bye" << endl;
        return 0;
      }
    } catch (const char *s) {
      cout << "-1\n";
    }
  }

  return cout << flush, 0;
}
