#ifndef _SJTU_HASHMAP_HPP_
#define _SJTU_HASHMAP_HPP_
#include <utility>

using std::make_pair;
using std::pair;

/**
 * @brief linked hash map with LRU caching
 */
template <class Key, class T, size_t LRU_CAP_ = size_t(-1),
          class Hash = std::hash<Key>, class Equal = std::equal_to<Key>>
class Hashmap {
public:
  static constexpr size_t LRU_CAP = LRU_CAP_;
  using key_type = Key;
  using mapped_type = T;
  using value_type = pair<Key, T>;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using reference = value_type &;
  using const_reference = const value_type &;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;

private:
  friend class iterator;
  static const Hash hash_;
  static const Equal eq;
  static constexpr double load = 0.8; // maximum load factor
  static constexpr size_t primes[] = {
      53,     97,      193,     389,     769,      1543,     3079,
      6151,   12289,   24593,   49157,   98317,    196613,   393241,
      786433, 1572869, 3145739, 6291469, 12582917, 25165843, 50331653};
  int pnum;
  size_t BNUM, sz, maxsz; // BNUM=primes[pnum] is the number of buckets (the
                          // capacity of the hash table)
  // size: limited for caching purposes
  /**
   * @brief hash function
   */
  int hash(const Key &key) { return hash_(key) % BNUM; }
  struct Node;
  struct ListNode;
  using NodePtr = Node *;
  using Iter = ListNode *;
  /**
   * @brief  list node, stores data
   */
  struct ListNode {
    Iter prev = nullptr, next = nullptr;
    NodePtr hash_node;
    value_type data;
    ListNode() {}
    ListNode(const Key &k, const T &v) : data(k, v) {}
  } *head;
  /**
   * @brief hash node
   */
  struct Node {
    NodePtr next = nullptr;
    Iter iter = nullptr;
    Node(Iter it) : iter(it) {}
    const Key &key() const { return iter->data.first; }
  };
  NodePtr *table; // table[i]: Node*
  /**
   * @brief inserts a list node
   */
  void list_insert(Iter node) {
    node->next = head->next;
    node->prev = head;
    head->next = node->next->prev = node;
  }
  /**
   * @brief removes a node from the list without deleting it
   */
  void list_remove(Iter node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
  }
  /**
   * @brief inserts a hash node
   */
  void table_insert(NodePtr node) {
    size_t idx = hash(node->key());
    node->next = table[idx];
    table[idx] = node;
  }
  /**
   * @brief deletes a hash node
   */
  void table_erase(NodePtr node) {
    size_t idx = hash(node->key());
    if (table[idx] == node) {
      table[idx] = node->next;
    } else {
      auto p = table[idx];
      while (p->next != node) {
        p = p->next;
      }
      p->next = node->next;
    }
    delete node;
  }
  /**
   * @brief expands the capacity of the hash table to limit the load factor
   */
  void expand() {
    NodePtr *p = table;
    table = new NodePtr[BNUM = primes[++pnum]];
    memset(table, 0, sizeof(*table) * BNUM);
    maxsz = BNUM * load;
    for (Iter it = head->next; it != head; it = it->next) {
      table_insert(it->hash_node);
    }
    if (p) {
      delete p;
    }
  }

public:
  class iterator {
    friend class Hashmap;
    Iter iter;
    iterator(Iter it_) : iter(it_) {}

  public:
    iterator() : iter(nullptr) {}
    /**
     * @brief ++iter
     */
    iterator &operator++() {
      iter = iter->next;
      return *this;
    }
    /**
     * @brief iter++
     */
    const iterator operator++(int) {
      auto ret = *this;
      iter = iter->next;
      return ret;
    }
    /**
     * @brief --iter
     */
    iterator &operator--() {
      iter = iter->prev;
      return *this;
    }
    /**
     * @brief iter--
     */
    const iterator operator--(int) {
      auto ret = *this;
      iter = iter->prev;
      return ret;
    }
    bool operator==(const iterator &rhs) const { return iter == rhs.iter; }
    bool operator!=(const iterator &rhs) const { return iter != rhs.iter; }
    value_type &operator*() { return iter->data; }
    value_type *operator->() { return &(iter->data); }
  }; // class iterator
  iterator end_;
  Hashmap() : pnum(-1), BNUM(0), sz(0), maxsz(0), table(nullptr) {
    head = new ListNode();
    head->next = head->prev = head;
    end_ = iterator(head);
  }
  // (TODO) copy constructor

  /**
   * @brief clears the hashmap
   * ! remember to flush the cache before clearing it
   */
  void clear() {
    for (Iter it = head->next, nxt; it != head; it = nxt) {
      nxt = it->next;
      delete it->hash_node;
      delete it;
    }
    head->prev = head->next = head;
    if (table) {
      delete[] table;
      table = nullptr;
    }
    sz = 0, pnum = -1, BNUM = 0, maxsz = 0;
  }
  ~Hashmap() {
    clear();
    delete head;
  }
  /**
   * @brief inserts a key at the front of the list
   */
  void insert(const Key &key, const T &val) {
    if (++sz > maxsz) {
      expand();
    }
    Iter it = new ListNode(key, val);
    NodePtr ptr = new Node(it);
    it->hash_node = ptr;
    list_insert(it);
    table_insert(ptr);
  }
  void insert(const value_type &x) { insert(x.first, x.second); }
  /**
   * @brief finds a key and moves it to the front of the list
   */
  iterator find(const Key &key) {
    if (!table)
      return end();
    size_t idx = hash(key);
    NodePtr p = table[idx];
    while (p) {
      if (eq(p->key(), key)) {
        list_remove(p->iter), list_insert(p->iter); // LRU: move to the front
        return p->iter;
      }
      p = p->next;
    }
    return end();
  }
  bool count(const Key &key) { return find(key) != end_; }
  /**
   * @brief erases a key at the given position
   * to erase a given key, first call find(key) to check if the key exists
   */
  void erase(iterator it) {
    Iter p = it.iter;
    table_erase(p->hash_node);
    list_remove(p);
    delete p;
    sz--;
  }
  iterator begin() { return head->next; }
  iterator end() { return end_; }
  size_t size() const { return sz; }
  bool empty() const { return !sz; }
}; // class Hashmap

#endif
