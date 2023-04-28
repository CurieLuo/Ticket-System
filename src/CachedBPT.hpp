#ifndef _SJTU_CACHED_BPLUSTREE_HPP_
#define _SJTU_CACHED_BPLUSTREE_HPP_

#include "BPT.hpp"
#include "Hashmap.hpp"

// #define _NO_CACHE_ //! switch on/off cache

// #define _CHECK_HASHMAP_
#ifdef _CHECK_HASHMAP_
#include <map> //!!!!!!!!!! debug
#endif

#ifndef _NO_CACHE_
/**
 * @brief B+ tree with LRU cache
 */
template <class Key, class T> class CachedBPT : public BPT<Key, T> {
private:
  using Node = typename CachedBPT<Key, T>::Node;
  static constexpr size_t LRU_CAP = (1 << 18) / sizeof(Node);
  // adjust LRU_CAP according to sizeof(node)

#ifdef _CHECK_HASHMAP_
  using Cache = std::map<int, Node>; //!!!!!!!!!! debug
#else
  using Cache = Hashmap<int, Node, LRU_CAP>;
#endif
  Cache cache;

  /**
   * @brief writes the data of an iterator into the disk
   */
  void flush(typename Cache::iterator it) {
    BPT<Key, T>::write(it->second, it->first);
  }
  /**
   * @brief flushes and clears the cache
   */
  void flush() {
    for (auto it = cache.begin(), end = cache.end(); it != end; ++it)
      flush(it);
    cache.clear();
  }
  /**
   * @brief insert into LRU cache
   */
  void cache_insert(int pos, const Node &x) {
    if (cache.size() == LRU_CAP)
      flush(--cache.end()), cache.erase(--cache.end());
    cache.insert(make_pair(pos, x));
  }

protected:
  virtual void read(Node &x, int pos) override {
    auto it = cache.find(pos);
    if (it == cache.end()) { // cache miss
      BPT<Key, T>::read(x, pos);
      cache_insert(pos, x);
    } else { // cache hit
      x = it->second;
    }
  }
  virtual void read(Node &x) override { CachedBPT<Key, T>::read(x, x.pos); }
  virtual void write(Node &x, int pos) override {
    auto it = cache.find(pos);
    if (it == cache.end()) { // cache miss
      cache_insert(pos, x);
    } else { // cache hit
      it->second = x;
    }
  }
  virtual void write(Node &x) override { CachedBPT<Key, T>::write(x, x.pos); }
  // TODO caching the values and recycling
  /*
  virtual void read_value(T &x, int pos) override {
    read_(BPT<Key, T>::value_file, x, pos);
  }
  virtual void write_value(T &x, int pos) override {
    write_(BPT<Key, T>::value_file, x, pos);
  }
  virtual void read_value(const T &x, int pos) override {
    read_(BPT<Key, T>::value_file, x, pos);
  }
  virtual void write_value(const T &x, int pos) override {
    write_(BPT<Key, T>::value_file, x, pos);
  }
  */

public:
  explicit CachedBPT(string filename, bool retrieve = true)
      : BPT<Key, T>(filename, retrieve) {}
  virtual ~CachedBPT() override { flush(); }
  virtual void clear() override {
    flush();
    BPT<Key, T>::clear();
  }
};

#else
#define CachedBPT BPT //!!!!!!!!!! debug
#endif

#endif
