#ifndef HASH
#define HASH

#include "vector.hpp"
#include <algorithm>
#include <boost/uuid/detail/sha1.hpp>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace levkin {
  template < class T > struct Sha1Hasher {
    size_t operator()(const T& key) const
    {
      boost::uuids::detail::sha1 sha1;
      sha1.process_bytes(&key, sizeof(T));
      unsigned int digest[5];
      sha1.get_digest(digest);

      if (sizeof(size_t) >= 8) {
        return (static_cast< size_t >(digest[0]) << 32) | digest[1];
      }
      return static_cast< size_t >(digest[0]);
    }
  };

  template <> struct Sha1Hasher< std::string > {
    size_t operator()(const std::string& key) const
    {
      boost::uuids::detail::sha1 sha1;
      sha1.process_bytes(key.data(), key.size());
      unsigned int digest[5];
      sha1.get_digest(digest);

      if (sizeof(size_t) >= 8) {
        return (static_cast< size_t >(digest[0]) << 32) | digest[1];
      }
      return static_cast< size_t >(digest[0]);
    }
  };

  template <> struct Sha1Hasher< std::pair< std::string, std::string > > {
    size_t operator()(const std::pair< std::string, std::string >& key) const
    {
      boost::uuids::detail::sha1 sha1;
      sha1.process_bytes(key.first.data(), key.first.size());
      char separator = '\0';
      sha1.process_bytes(&separator, 1);
      sha1.process_bytes(key.second.data(), key.second.size());
      unsigned int digest[5];
      sha1.get_digest(digest);

      if (sizeof(size_t) >= 8) {
        return (static_cast< size_t >(digest[0]) << 32) | digest[1];
      }
      return static_cast< size_t >(digest[0]);
    }
  };

  template < class T > struct Equal {
    bool operator()(const T& a, const T& b) const { return a == b; }
  };

  template < class Key, class Value > struct NodeHashTable {
    Key key_;
    Value value_;
    bool is_valid_;

    NodeHashTable() : is_valid_(false) {}
    NodeHashTable(const Key& k, const Value& v, bool valid)
        : key_(k), value_(v), is_valid_(valid)
    {
    }
  };

  template <
      class Key,
      class Value,
      class Hash = Sha1Hasher< Key >,
      class EqualTo = Equal< Key > >
  class HashTable;

  template < class Key, class Value, class Hash, class EqualTo, bool IsConst >
  class HashTableIterator
  {
    template < class K, class V, class H, class E > friend class HashTable;
    template < class K, class V, class H, class E, bool C >
    friend class HashTableIterator;

  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = NodeHashTable< Key, Value >;
    using difference_type = std::ptrdiff_t;
    using pointer = typename std::
        conditional< IsConst, const value_type*, value_type* >::type;
    using reference = typename std::
        conditional< IsConst, const value_type&, value_type& >::type;

    HashTableIterator() noexcept : table_(nullptr), index_(0) {}

    HashTableIterator(
        const HashTable< Key, Value, Hash, EqualTo >* table,
        size_t index) noexcept
        : table_(const_cast< HashTable< Key, Value, Hash, EqualTo >* >(table)),
          index_(index)
    {
    }

    template < bool OtherConst >
    HashTableIterator(
        const HashTableIterator< Key, Value, Hash, EqualTo, OtherConst >& other,
        typename std::enable_if< IsConst && !OtherConst >::type* =
            nullptr) noexcept
        : table_(other.table_), index_(other.index_)
    {
    }

    typename std::conditional< IsConst, const Key&, Key& >::type
    key() const noexcept
    {
      return table_->pool_[index_].key_;
    }

    typename std::conditional< IsConst, const Value&, Value& >::type
    value() const noexcept
    {
      return table_->pool_[index_].value_;
    }

    reference operator*() const noexcept { return table_->pool_[index_]; }
    pointer operator->() const noexcept { return &(table_->pool_[index_]); }

    HashTableIterator& operator++()
    {
      index_ = table_->findNextValid(index_ + 1);
      return *this;
    }

    HashTableIterator operator++(int)
    {
      HashTableIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator==(const HashTableIterator& rhs) const noexcept
    {
      return table_ == rhs.table_ && index_ == rhs.index_;
    }
    bool operator!=(const HashTableIterator& rhs) const noexcept
    {
      return !(*this == rhs);
    }

  private:
    HashTable< Key, Value, Hash, EqualTo >* table_;
    size_t index_;
  };

  template < class Key, class Value, class Hash, class EqualTo > class HashTable
  {
    template < class K, class V, class H, class E, bool C >
    friend class HashTableIterator;

  public:
    using iterator = HashTableIterator< Key, Value, Hash, EqualTo, false >;
    using const_iterator = HashTableIterator< Key, Value, Hash, EqualTo, true >;

    HashTable() : HashTable(11, 4) {}

    HashTable(size_t num_buckets, size_t bucket_capacity)
        : num_buckets_(num_buckets == 0 ? 11 : num_buckets),
          bucket_capacity_(bucket_capacity == 0 ? 4 : bucket_capacity),
          count_valid_(0)
    {
      size_t total_slots = (num_buckets_ + 1) * bucket_capacity_;
      for (size_t i = 0; i < total_slots; ++i) {
        pool_.pushBack(NodeHashTable< Key, Value >{});
      }
    }

    HashTable(const HashTable& other)
        : pool_(other.pool_), num_buckets_(other.num_buckets_),
          bucket_capacity_(other.bucket_capacity_),
          count_valid_(other.count_valid_)
    {
    }

    HashTable(HashTable&& other) noexcept
        : num_buckets_(0), bucket_capacity_(0), count_valid_(0)
    {
      swap(other);
    }

    HashTable& operator=(const HashTable& other)
    {
      if (this != &other) {
        HashTable tmp(other);
        swap(tmp);
      }
      return *this;
    }

    HashTable& operator=(HashTable&& other) noexcept
    {
      if (this != &other) {
        HashTable tmp(std::move(other));
        swap(tmp);
      }
      return *this;
    }

    void swap(HashTable& other) noexcept
    {
      std::swap(pool_, other.pool_);

      std::swap(num_buckets_, other.num_buckets_);
      std::swap(bucket_capacity_, other.bucket_capacity_);
      std::swap(count_valid_, other.count_valid_);
    }

    void add(const Key& key, const Value& value)
    {
      size_t slot = findSlotOrTarget(key, true);
      if (pool_[slot].is_valid_) {
        throw std::logic_error("Key already exists.");
      }
      pool_[slot].key_ = key;
      pool_[slot].value_ = value;
      pool_[slot].is_valid_ = true;
      count_valid_++;
    }

    void add(const Key& key, Value&& value)
    {
      size_t slot = findSlotOrTarget(key, true);
      if (pool_[slot].is_valid_) {
        throw std::logic_error("Key already exists.");
      }
      pool_[slot].key_ = key;
      pool_[slot].value_ = std::move(value);
      pool_[slot].is_valid_ = true;
      count_valid_++;
    }

    Value drop(const Key& key)
    {
      size_t slot = findSlotOrTarget(key, false);
      if (slot == pool_.getSize() || !pool_[slot].is_valid_) {
        throw std::logic_error("No such key");
      }
      Value removed_value = std::move(pool_[slot].value_);
      pool_[slot].is_valid_ = false;
      count_valid_--;
      return removed_value;
    }

    bool has(const Key& key) const noexcept
    {
      size_t slot = findSlotOrTarget(key, false);
      return (slot != pool_.getSize() && pool_[slot].is_valid_);
    }

    Value& get(const Key& key)
    {
      size_t slot = findSlotOrTarget(key, false);
      if (slot == pool_.getSize() || !pool_[slot].is_valid_) {
        throw std::logic_error("No such key");
      }
      return pool_[slot].value_;
    }

    const Value& get(const Key& key) const
    {
      size_t slot = findSlotOrTarget(key, false);
      if (slot == pool_.getSize() || !pool_[slot].is_valid_) {
        throw std::logic_error("No such key");
      }
      return pool_[slot].value_;
    }

    Value& operator[](const Key& key)
    {
      size_t slot = findSlotOrTarget(key, false);
      if (slot != pool_.getSize() && pool_[slot].is_valid_) {
        return pool_[slot].value_;
      }
      add(key, Value{});
      return get(key);
    }

    const Value& operator[](const Key& key) const { return get(key); }

    void rehash(size_t new_num_buckets, size_t new_bucket_capacity)
    {
      HashTable< Key, Value, Hash, EqualTo > new_table(
          new_num_buckets, new_bucket_capacity);
      for (size_t i = 0; i < pool_.getSize(); ++i) {
        if (pool_[i].is_valid_) {
          new_table.add(pool_[i].key_, std::move(pool_[i].value_));
        }
      }
      this->swap(new_table);
    }

    void clear() noexcept
    {
      for (size_t i = 0; i < pool_.getSize(); ++i) {
        pool_[i].is_valid_ = false;
      }
      count_valid_ = 0;
    }

    size_t size() const noexcept { return pool_.getSize(); }
    size_t count() const noexcept { return num_buckets_; }
    size_t countValid() const noexcept { return count_valid_; }
    size_t capacity() const noexcept { return bucket_capacity_; }

    iterator begin() noexcept { return iterator(this, findNextValid(0)); }
    iterator end() noexcept { return iterator(this, pool_.getSize()); }

    const_iterator begin() const noexcept
    {
      return const_iterator(this, findNextValid(0));
    }
    const_iterator end() const noexcept
    {
      return const_iterator(this, pool_.getSize());
    }

    const_iterator cbegin() const noexcept
    {
      return const_iterator(this, findNextValid(0));
    }
    const_iterator cend() const noexcept
    {
      return const_iterator(this, pool_.getSize());
    }

  private:
    size_t findNextValid(size_t index) const noexcept
    {
      while (index < pool_.getSize() && !pool_[index].is_valid_) {
        index++;
      }
      return index;
    }

    size_t findSlotOrTarget(const Key& key, bool mode_insert) const
    {
      Hash hasher;
      EqualTo equal;

      size_t bucket_idx = hasher(key) % num_buckets_;
      size_t home_start = bucket_idx * bucket_capacity_;
      size_t first_free = pool_.getSize();

      for (size_t i = 0; i < bucket_capacity_; ++i) {
        size_t curr = home_start + i;
        if (pool_[curr].is_valid_) {
          if (equal(pool_[curr].key_, key)) {
            return curr;
          }
        } else if (first_free == pool_.getSize()) {
          first_free = curr;
        }
      }

      size_t reserve_start = num_buckets_ * bucket_capacity_;
      for (size_t i = 0; i < bucket_capacity_; ++i) {
        size_t curr = reserve_start + i;
        if (pool_[curr].is_valid_) {
          if (equal(pool_[curr].key_, key)) {
            return curr;
          }
        } else if (first_free == pool_.getSize()) {
          first_free = curr;
        }
      }

      if (mode_insert) {
        if (first_free != pool_.getSize()) {
          return first_free;
        }
        throw std::logic_error(
            "HashTable overflow: home and reserve buckets are full.");
      }

      return pool_.getSize();
    }

    stuff::Vector< NodeHashTable< Key, Value > > pool_;
    size_t num_buckets_;
    size_t bucket_capacity_;
    size_t count_valid_;
  };
}

#endif
