#ifndef __SJTU_USERSYSTEM_HPP__
#define __SJTU_USERSYSTEM_HPP__

#include "CachedBPT.hpp"
#include "utility.hpp"

/**
 * @brief records user information
 */
struct UserInfo {
  Pwd pwd;
  Name name;
  Mail mail;
  int pri; // privilege

public:
  UserInfo() {}
  UserInfo(const Pwd &p, const Name &n, const Mail &m, int g)
      : pwd(p), name(n), mail(m), pri(g) {}
  friend ostream &operator<<(ostream &os, const UserInfo &user) {
    return os << user.name << ' ' << user.mail << ' ' << user.pri;
  }
};

/**
 * @brief processes user related operations
 */
class UserSystem {
protected:
  CachedBPT<ID, UserInfo> users; // key: userID
  Hashmap<ID, int> logged_in;    // value: privilege

  virtual void clean() {
    users.clear();
    logged_in.clear();
  }

public:
  UserSystem() : users("users", RETRIEVE) {}

  void add_user(const Usr &cur_usr, const Usr &usr, const Pwd &pwd,
                const Name &name, const Mail &mail, int pri) {
    ID cur_uid = cur_usr.hash(), uid = usr.hash();
    if (users.empty()) {
      pri = 10;
    } else {
      auto it = logged_in.find(cur_uid);
      if (it == logged_in.end())
        throw "add_user() failed: current user not found";
      if (it->second <= pri)
        throw "add_user() failed: operation unauthorized";
    }
    users.insert(uid, UserInfo(pwd, name, mail,
                               pri)); //! try catch throw for more error info
    cout << "0\n";
  }

  void login(const Usr &usr, const Pwd &pwd) {
    ID uid = usr.hash();
    if (logged_in.count(uid))
      throw "login() failed: user logged in already";
    auto it = users.find(uid);
    if (!it)
      throw "login() failed: no such user";
    UserInfo userinfo;
    if ((userinfo = it.value()).pwd != pwd)
      throw "login() failed: wrong password";
    logged_in.insert(make_pair(uid, userinfo.pri));
    cout << "0\n";
  }

  void logout(const Usr &usr) {
    ID uid = usr.hash();
    auto it = logged_in.find(uid);
    if (it == logged_in.end()) {
      throw "logout() failed";
    }
    logged_in.erase(it);
    cout << "0\n";
  }

  void query_profile(const Usr &cur_usr, const Usr &usr) {
    ID cur_uid = cur_usr.hash(), uid = usr.hash();
    auto it = logged_in.find(cur_uid);
    if (it == logged_in.end()) {
      throw "add_user() failed: current user not logged in";
    }
    int cur_pri = it->second;
    UserInfo userinfo = users.get(uid); //! try catch throw for more error info

    if (uid != cur_uid && cur_pri <= userinfo.pri)
      throw "add_user() failed: access unauthorized";
    cout << usr << ' ' << userinfo << '\n';
  }

  void modify_profile(const Usr &cur_usr, const Usr &usr, const Pwd &pwd = "",
                      const Name &name = "", const Mail &mail = "",
                      int pri = -1) {
    ID cur_uid = cur_usr.hash(), uid = usr.hash();
    auto it = logged_in.find(cur_uid);
    if (it == logged_in.end()) {
      throw "modify_profile() failed: current user not logged in";
    }
    int cur_pri = it->second;
    auto it2 = users.find(uid);
    if (!it2)
      throw "modify_profile() failed: target user not found";
    UserInfo userinfo = it2.value();
    if (cur_pri <= pri || (uid != cur_uid && cur_pri <= userinfo.pri))
      throw "modify_profile() failed: access unauthorized";
    if (pwd)
      userinfo.pwd = pwd;
    if (name)
      userinfo.name = name;
    if (mail)
      userinfo.mail = mail;
    if (pri != -1)
      userinfo.pri = pri;
    it2.set(userinfo);
    cout << usr << ' ' << userinfo << '\n';
  }
}; // class UserSystem

#endif
