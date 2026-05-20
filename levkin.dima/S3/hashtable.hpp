#ifndef HASH
#define HASH

#include "list.hpp"
#include "vector.hpp"
#include <boost/uuid/detail/sha1.hpp>
#include <cstddef>

namespace levkin {
  constexpr size_t bucketSize = 3;
  template < class Key, class Value, class Hash, class Equal > class HashTable
  {
  public:
    explicit HashTable(size_t initial_slots = 16)
        : size_(initial_slots), used_(0)
    {
      if (initial_slots == 0)
        throw std::invalid_argument("slots must be > 0");

      data_.resize(initial_slots);
    }

    void add(Key k, Value v)
    {
      size_t ind = getBucketIndex(k);
      Bucket& bucket = data_[ind];

      for (size_t i = 0; i < bucket.filled; ++i) {
        if (eq_fn(bucket.cells[i].first, k)) {
          bucket.cells[i].second = v;
          return;
        }
      }
      for (auto& it : bucket.overflow_) {
        if (eq_fn(it.first, k)) {
          it.second = v;
          return;
        }
      }

      if (bucket.filled >= bucketSize) {
        bucket.overflow_.pushFront(std::make_pair(k, v));
      } else {
        bucket.cells[bucket.filled++] = std::make_pair(k, v);
      }
      used_++;
    };

    Value drop(Key k);

    bool has(const Key& k) const
    {
      size_t ind = getBucketIndex(k);
      const Bucket& bucket = data_[ind];

      for (size_t i = 0; i < bucket.filled; ++i) {
        if (eq_fn(bucket.cells[i].first, k)) {
          return true;
        }
      }
      for (const auto& it : bucket.overflow_) {
        if (eq_fn(it.first, k)) {
          return true;
        }
      }
      return false;
    };

    void rehash(size_t slots);

  private:
    using Cell = std::pair< Key, Value >;
    struct Bucket {
      size_t filled;
      Cell pair[bucketSize];
      List< Cell > overflow_;
    };
    stuff::Vector< Bucket > data_;
  };

}

#endif
