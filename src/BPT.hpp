#ifndef _SJTU_BPLUSTREE_HPP_
#define _SJTU_BPLUSTREE_HPP_

#include "vector.hpp"
#include <fstream>
#include <iostream>
#include <string>

#ifdef DEBUG
#include <cassert>
#define SHOW(x) (cerr << #x "=" << (x) << endl, (x))
#else
#define SHOW(x) (x)
#endif

using sjtu::vector;
using std::cerr;
using std::cin;
using std::cout;
using std::endl;
using std::fstream;
using std::ios;
using std::make_pair;
using std::pair;
using std::string;
using Ptr = char *;

/**
 * @brief for debugging
 */
template <class T1, class T2>
std::ostream &operator<<(std::ostream &os, pair<T1, T2> x) {
  return os << '(' << x.first << ',' << x.second << ')';
}

/**
 * @brief functions for reading from/writing into files (flow: continuous)
 */
template <class T> inline void read_flow_(fstream &fs, T &x) {
  fs.read((Ptr)&x, sizeof(x));
}
template <class T, class... Args>
inline void read_flow_(fstream &fs, T &x, Args &...args) {
  read_flow_(fs, x), read_flow_(fs, args...);
}
template <class T> inline void write_flow_(fstream &fs, T &x) {
  fs.write((Ptr)&x, sizeof(x));
}
template <class T, class... Args>
inline void write_flow_(fstream &fs, T &x, Args &...args) {
  write_flow_(fs, x), write_flow_(fs, args...);
}
template <class T1> inline void read_(fstream &fs, T1 &x, int pos) {
  fs.seekg(pos);
  fs.read((Ptr)&x, sizeof(x));
}
template <class T1> inline void write_(fstream &fs, T1 &x, int pos) {
  fs.seekp(pos);
  fs.write((Ptr)&x, sizeof(x));
}

/**
 * @brief B+ tree
 * ! assumes Key and T have fixed size
 */
template <class Key, class T> class BPT {

  using Data = pair<Key, int>;
  static constexpr size_t szmax = std::max(4000 / (int)(sizeof(Data)) - 1, 4),
                          szmin = szmax >> 1;
  friend class iterator;

public:
  using key_type = Key;
  using mapped_type = T;
  using value_type = std::pair<const Key, T>;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;

protected:
  /**
   * @brief pair<key, pos> only stores pos for both a leaf and an internal
    node to minimize the size of a node
   * @param key max of subtree;
   * @param pos that of a child or a value
   */
  struct Node {
    size_t size = 0;          // degree
    int pos;                  // where the node is stored in node_file
    int prev = -1, next = -1; // position of adjacent siblings in node_file
    bool leaf;
    Data data[szmax + 1];
    explicit Node(int pos = -1, bool leaf = true) : pos(pos), leaf(leaf) {}
    Data &operator[](size_t idx) { return data[idx]; }
    const Data &operator[](size_t idx) const { return data[idx]; }
    Key &max_key() { return data[size - 1].first; }
    const Key &max_key() const { return data[size - 1].first; }
    bool operator==(const Node &rhs) const { return pos == rhs.pos; }
    bool operator!=(const Node &rhs) const { return pos != rhs.pos; }
    operator int() const { return pos; }

    /**
     * @brief find the index of key x in data[]
     */
    size_t lower_bound(const Key &x) const {
      int left = 0, right = size - 1, mid, ret = size;
      while (left <= right) {
        mid = (left + right) >> 1;
        if (data[mid].first < x)
          left = mid + 1;
        else
          ret = mid, right = mid - 1;
      }
      return ret;
    }
    size_t upper_bound(const Key &x) const {
      int left = 0, right = size - 1, mid, ret = size;
      while (left <= right) {
        mid = (left + right) >> 1;
        if (x < data[mid].first)
          ret = mid, right = mid - 1;
        else
          left = mid + 1;
      }
      return ret;
    }

    /**
     * @brief insert x at data[idx]
     */
    void insert(const Data &x, size_t idx) {
      for (size_t i = size++; i > idx; i--) {
        data[i] = data[i - 1];
      }
      data[idx] = x;
    }
    /**
     * @brief erase data[idx]
     */
    void erase(size_t idx) {
      --size;
      for (size_t i = idx; i < size; i++) {
        data[i] = data[i + 1];
      }
    }

  } root, null;
  int beg_pos, end_pos; // position of the first/last leaf node

  fstream tree_file, node_file, value_file;
  string tree_filename, node_filename, value_filename;
  sjtu::vector<int> node_pool, value_pool;
  // TODO: recycling (value)
  /**
   * @brief find a new position in the file
   * ! must write before allocating another
   */
  virtual int new_node() {
    if (!node_pool.empty()) {
      int p = node_pool.back();
      node_pool.pop_back();
      return p;
    }
    node_file.seekp(0, ios::end);
    int pos = node_file.tellp();
    write_(node_file, null, pos);
    return pos;
  }
  virtual int new_value() {
    if (!value_pool.empty()) {
      int p = value_pool.back();
      value_pool.pop_back();
      return p;
    }
    value_file.seekp(0, ios::end);
    int pos = value_file.tellp();
    // write_(value_file, T(), pos); //! add this if cache is used
    return pos;
  }
  virtual void delete_node(int pos) { node_pool.push_back(pos); }
  virtual void delete_value(int pos) { value_pool.push_back(pos); }

  virtual void read(Node &x, int pos) { read_(node_file, x, pos); }
  virtual void write(Node &x, int pos) { write_(node_file, x, pos); }
  virtual void read(Node &x) { read(x, x.pos); }
  virtual void write(Node &x) { write(x, x.pos); }
  virtual void read_value(T &x, int pos) { read_(value_file, x, pos); }
  virtual void write_value(T &x, int pos) { write_(value_file, x, pos); }
  virtual void read_value(const T &x, int pos) { read_(value_file, x, pos); }
  virtual void write_value(const T &x, int pos) { write_(value_file, x, pos); }
  // #define Z assert(root.max_key());
  //$ use this to find out which line first violated this property

private:
  /**
   * @brief create a file
   */
  inline void create_file(fstream &fs, const string &name) {
    fs.open(name, ios::out);
    fs.close();
  }

  /**
   * @brief splits an oversized node
   *
   * @param u the node to be split
   * @param p parent of u
   * @param idx_u index of u among children of p
   */
  void split(Node &u, Node &p, size_t idx_u) {
    Node v(new_node(), u.leaf);
    v.size = u.size >> 1, u.size -= v.size;
    v.next = u.next, v.prev = u, u.next = v;
    if (u == end_pos)
      end_pos = v; // update end_pos
    // copy half of u to v
    // Variation: std::copy(u.data + u.size, u.data + u.size + v.size, v.data);
    for (size_t i = 0; i < v.size; i++) {
      v[i] = u[i + u.size];
    }
    write(u), write(v); // allocate space for v
    if (u == root) {
      Node temp =
          Node(new_node(),
               false); // must not modify root here, because u refers to root
      temp.size = 2;
      temp[0] = Data(u.max_key(), u);
      temp[1] = Data(v.max_key(), v);
      root = temp;
      write(root);
    } else {
      if (v.next != -1) {
        Node nxt;
        read(nxt, v.next);
        nxt.prev = v;
        write(nxt);
        //                write(v.pos, v.next + ((Ptr)&v.prev - (Ptr)&v));
        //! write v.next.prev=v
      }
      p[idx_u].first = u.max_key(); // update the indexing key for u
      p.insert(Data(v.max_key(), v), idx_u + 1);
    }
  }
  /**
   * @brief inserts a node recursively, then adjusts by calling split()
   * makes sure every change is ultimately written into the disk
   */
  void BP_insert(const Key &key, int val, Node &u, Node &p, size_t idx_u = 0) {
    size_t idx_s = u.lower_bound(key); // index of s (see below)
    if (idx_s < u.size && u[idx_s].first == key) {
      throw "Error in insert(): element already exists";
    }
    if (idx_s == u.size && p != null) {
      p[idx_u].first = key;
    }
    if (u.leaf) {
      u.insert(Data(key, val), idx_s);
    } else {
      if (idx_s == u.size)
        --idx_s;
      Node s;
      read(s, u[idx_s].second);
      BP_insert(key, val, s, u, idx_s);
    }
    if (u.size > szmax) {
      split(u, p, idx_u);
    } else {
      write(u);
    }
  }

  /**
   * @brief makes sure neither u nor v is undersized
   */
  void merge(Node &u, Node &v, Node &p, size_t idx_u) {
    if (u.size <= szmin && v.size <= szmin) { // merge u and v
      if (v == end_pos)
        end_pos = u; // update end_pos
      // copy v to u
      // Variation: std::copy(v.data, v.data + v.size, u.data + u.size);
      for (size_t i = 0; i < v.size; i++) {
        u[u.size + i] = v[i];
      }
      u.size += v.size;
      u.next = v.next;
      if (u.next != -1) {
        Node nxt;
        read(nxt, u.next);
        nxt.prev = u;
        write(nxt);
        // write(u.pos, u.next + ((Ptr)&u.prev - (Ptr)&u));
        // write u.next.prev=u
      }
      write(u), delete_node(v);
      if (p == root && p.size == 2) { // delete root
        delete_node(root);
        root = u;
      } else {
        p[idx_u].first = u.max_key(); // update the indexing key for u
        p.erase(idx_u + 1);
      }
    } else { // lend a child if either u or v has size > szmin
      if (u.size > szmin) {
        // Variation: v.insert(u[--u.size], 0);
        v.insert(u[u.size - 1], 0), u.erase(u.size - 1);
      } else /*if (v.size > szmin)*/ {
        u.insert(v[0], u.size), v.erase(0);
      }
      p[idx_u].first = u.max_key();
      // update the indexing key for u (v is not affected)
      write(u), write(v);
    }
  }
  /**
   * @brief erases a node recursively, then adjusts by calling merge()
   * makes sure every change is ultimately written into the disk
   */
  void BP_erase(const Key &key, Node &u, Node &p, size_t idx_u = 0) {
    size_t idx_s = u.lower_bound(key);
    if (idx_s == u.size || (u.leaf && u[idx_s].first != key)) {
      throw "Error in erase(): element does not exist";
    }
    if (u.leaf) {
      u.erase(idx_s);
    } else {
      Node s;
      read(s, u[idx_s].second);
      BP_erase(key, s, u, idx_s);
    }
    p[idx_u].first = u.max_key();
    if (u != root && u.size < szmin) {
      Node v;
      if (idx_u > 0) { // v must be a brother of u
        read(v, u.prev);
        merge(v, u, p, idx_u - 1);
      } else {
        read(v, u.next);
        merge(u, v, p, idx_u);
      }
    } else {
      write(u);
    }
  }

public:
  class iterator {
    friend class BPT;
    BPT *tr = nullptr;
    Node node;
    size_t idx = 0;
    iterator(BPT *tr_, Node node_, size_t idx_)
        : tr(tr_), node(node_), idx(idx_) {
      if (idx == node.size)
        node = tr->null, idx = 0; // this = end()
    }

    void move_prev() {
      if (node == tr->null) { // this == end()
        tr->read(node, tr->end_pos);
        idx = node.size - 1;
      } else if (idx-- == 0) {
        tr->read(node, node.prev);
        idx = 0;
      }
    }
    void move_next() {
      if (++idx == node.size) {
        if (node.next != -1)
          tr->read(node, node.next);
        else
          node = tr->null; // this = end()
        idx = 0;
      }
    }

  public:
    iterator() {}
    T value() const {
      T val;
      tr->read_value(val, node[idx].second);
      return val;
    }
    void set(const T &val) const { tr->write_value(val, node[idx].second); }
    int handle() const { return node[idx].second; }
    const Key &key() const { return node[idx].first; }
    value_type operator*() const { return {key(), value()}; }
    iterator &operator++() {
      move_next();
      return *this;
    }
    const iterator operator++(int) {
      iterator ret = *this;
      move_next();
      return ret;
    }
    iterator &operator--() {
      move_prev();
      return *this;
    }
    const iterator operator--(int) {
      iterator ret = *this;
      move_prev();
      return ret;
    }
    bool operator==(const iterator &rhs) const {
      // TODO: support insert/erase
      return tr == rhs.tr && node == rhs.node && idx == rhs.idx;
    }
    bool operator!=(const iterator &rhs) const { return !(*this == rhs); }
    operator bool() const { return node.pos != -1; }
  }; // class iterator

  iterator begin() {
    Node p;
    read(p, beg_pos);
    return {this, p, 0};
  }
  iterator end() { return {this, null, 0}; }
  iterator cbegin() { return begin(); }
  iterator cend() { return end(); }

  /**
   * @brief Construct a new BPT object
   * @param retrieve = false: ignore old data (for debugging)
   */
  explicit BPT(string filename, bool retrieve = true) {
    filename = "./bin/BPT_" + filename; //!!
    tree_filename = filename + "_tree.bin",
    node_filename = filename + "_node.bin",
    value_filename = filename + "_value.bin";
    auto mode = ios::in | ios::out | ios::binary;
    if (!retrieve)
      mode |= ios::trunc;
    tree_file.open(tree_filename, mode);
    if (!tree_file) {
      create_file(tree_file, tree_filename);
      create_file(node_file, node_filename);
      create_file(value_file, value_filename);
      tree_file.open(tree_filename, mode);
    } else if (retrieve) {
      tree_file.seekg(0);
      read_flow_(tree_file, root.pos, beg_pos, end_pos);
    }
    node_file.open(node_filename, mode);
    value_file.open(value_filename, mode);
    if (root.pos == -1) {
      beg_pos = end_pos = root.pos = new_node();
      write(root);
    } else if (retrieve) {
      read(root);
    }
  }

  virtual ~BPT() {
    tree_file.seekp(0);
    write_flow_(tree_file, root.pos, beg_pos, end_pos);
    tree_file.close();
    node_file.close();
    value_file.close();
  }
  virtual void clear() {
    auto mode = ios::in | ios::out | ios::binary | ios::trunc;
    tree_file.close();
    node_file.close();
    value_file.close();
    tree_file.open(tree_filename, mode);
    node_file.open(node_filename, mode);
    value_file.open(value_filename, mode);
    beg_pos = end_pos = root.pos = new_node();
    write(root);
  }

  /**
   * @brief finds key
   * @return iterator pointing to key, end() if not found
   */
  iterator find(const Key &key) {
    Node u = root;
    while (1) {
      size_t idx = u.lower_bound(key);
      if (idx >= u.size || (u.leaf && u[idx].first != key))
        return end();
      else if (u.leaf) {
        return iterator(this, u, idx);
      }
      read(u, u[idx].second);
    }
  }
  /**
   * @return value of given (existing) key
   */
  T get(const Key &key) {
    auto ret = find(key);
    if (!ret) {
      throw "Error in get(): element does not exist";
    }
    return ret.value();
  }
  T get_default(const Key &key) {
    auto ret = find(key);
    if (!ret)
      return T();
    return ret.value();
  }
  /**
   * @brief checks if element exists
   */
  bool count(const Key &key) { return find(key); }
  /**
   * @brief finds lower bound of key
   * @return iterator pointing to the lower bound of key, end() if not found
   */
  iterator lower_bound(const Key &key) {
    Node u = root;
    while (1) {
      size_t idx = u.lower_bound(key);
      if (u.leaf) {
        return iterator(this, u, idx);
      } else if (idx == u.size) {
        return end(); // u==root must hold
      }
      read(u, u[idx].second);
    }
  }
  iterator upper_bound(const Key &key) {
    Node u = root;
    while (1) {
      size_t idx = u.upper_bound(key);
      if (u.leaf) {
        return iterator(this, u, idx);
      } else if (idx == u.size) {
        return end(); // u==root must hold
      }
      read(u, u[idx].second);
    }
  }

  /**
   * @brief sets the value of an existing element
   */
  void set(const Key &key, const T &value) {
    Node u = root;
    while (1) {
      size_t idx = u.lower_bound(key);
      if (idx >= u.size || (u.leaf && u[idx].first != key))
        throw "Error in set(): element does not exist";
      if (u.leaf) {
        write_value(value, u[idx].second);
        return;
      }
      read(u, u[idx].second);
    }
  }
  void set(const value_type &x) { return set(x.first, x.second); }

  /**
   * @brief get/set value through the handle of an iterator
   */
  T get_by_handle(int handle) {
    T val;
    read_value(val, handle);
    return val;
  }
  void set_by_handle(int handle, const T &val) { write_value(val, handle); }

  /**
   * @brief inserts a new element. throws error if element already exists
   * @return handle to the new node
   */
  int insert(const Key &key, const T &value) {
    int val_pos = new_value();
    BP_insert(key, val_pos, root, null);
    write_value(value, val_pos);
    return val_pos;
  }
  int insert(const value_type &x) { return insert(x.first, x.second); }

  /**
   * @brief erases a new element. throws error if element does not exist
   */
  void erase(const Key &key) { // TODO:return/throw
    BP_erase(key, root, null);
  }

  bool empty() const { return !root.size; }

  void debug(const Node &u, int dep) {
    if (u.leaf) {
      cerr << "leaf size=" << u.size << ", dep=" << dep << ": ";
      for (size_t i = 0; i < u.size; i++)
        cerr << u[i].first << ' ';
      cerr << endl;
      return;
    }
    cerr << "branch size=" << u.size << ", dep=" << dep << ": ";
    for (size_t i = 0; i < u.size; i++)
      cerr << u[i].first << ' ';
    cerr << endl;
    for (size_t i = 0; i < u.size; i++) {
      Node s;
      read(s, u[i].second);
      debug(s, dep + 1);
    }
  }
  void debug(const string &msg = "") {
    cerr << msg << endl;
    debug(root, 0);
    cerr << endl;
  }
}; // Class BPT

#endif
