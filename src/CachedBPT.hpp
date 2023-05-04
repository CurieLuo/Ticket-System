#ifndef _SJTU_CACHED_BPLUSTREE_HPP_
#define _SJTU_CACHED_BPLUSTREE_HPP_

#include "BPT.hpp"
#include "Hashmap.hpp"

/**
 * @brief B+ tree with LRU cache
 */
template <class Key, class T, size_t MEM_CAP = 1 << 18>
class CachedBPT : public BPT<Key, T> {
private:
  using Node = typename CachedBPT<Key, T>::Node;
  static constexpr size_t LRU_CAP = MEM_CAP / sizeof(Node);
  // adjust LRU_CAP according to sizeof(node)

  using Cache = Hashmap<int, Node, LRU_CAP>;
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

public:
  explicit CachedBPT(string filename, bool retrieve = true)
      : BPT<Key, T>(filename, retrieve) {}
  virtual ~CachedBPT() override { flush(); }
  virtual void clear() override {
    flush();
    BPT<Key, T>::clear();
  }
};

#endif
