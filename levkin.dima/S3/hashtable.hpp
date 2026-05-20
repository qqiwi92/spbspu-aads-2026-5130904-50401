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
    void add(Key k, Value v);
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
