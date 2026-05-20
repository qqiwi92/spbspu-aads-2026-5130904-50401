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
    bool has(Key k);
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
