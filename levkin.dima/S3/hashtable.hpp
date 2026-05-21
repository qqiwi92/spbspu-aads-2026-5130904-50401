#ifndef HASH
#define HASH

#include "list.hpp"
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

      const Cell* operator->() const { return &(operator*()); }

      ConstIterator& operator++()
      {
        advance();
        return *this;
      }

      ConstIterator operator++(int)
      {
        ConstIterator tmp = *this;
        advance();
        return tmp;
      }

      bool operator==(const ConstIterator& other) const
      {
        if (table_ != other.table_ || bucket_ind != other.bucket_ind)
          return false;
        if (table_ == nullptr || bucket_ind >= table_->size_)
          return true;
        if (in_overflow != other.in_overflow)
          return false;
        if (in_overflow)
          return overflow_it == other.overflow_it;
        return cell_ind == other.cell_ind;
      }

      bool operator!=(const ConstIterator& other) const
      {
        return !(*this == other);
      }

    private:
      const HashTable* table_;
      size_t bucket_ind;
      size_t cell_ind;
      typename List< Cell >::const_iterator overflow_it;
      bool in_overflow;

      void satisfy()
      {
        while (bucket_ind < table_->size_) {
          const Bucket& bucket = table_->data_[bucket_ind];

          if (!in_overflow) {
            if (cell_ind < bucket.filled) {
              return;
            }
            in_overflow = true;
            overflow_it = bucket.overflow_.cbegin();
          }

          if (in_overflow) {
            if (overflow_it != bucket.overflow_.cend()) {
              return;
            }
            in_overflow = false;
            cell_ind = 0;
            bucket_ind++;
          }
        }
      }

      void advance()
      {
        if (!in_overflow) {
          cell_ind++;
        } else {
          overflow_it++;
        }
        satisfy();
      }
    };

    using iterator = Iterator;
    using const_iterator = ConstIterator;

    iterator begin() { return iterator(this, 0); }
    iterator end() { return iterator(this, size_); }

    const_iterator begin() const { return const_iterator(this, 0); }
    const_iterator end() const { return const_iterator(this, size_); }
    const_iterator cbegin() const { return const_iterator(this, 0); }
    const_iterator cend() const { return const_iterator(this, size_); }


    explicit HashTable(size_t initial_slots = 16)
        : size_(initial_slots), used_(0)
    {
      if (initial_slots == 0)
        throw std::invalid_argument("slots must be > 0");

      data_.resize(initial_slots);
    }

    HashTable(const HashTable& other)
        : hash_fn(other.hash_fn), eq_fn(other.eq_fn), size_(other.size_),
          used_(other.used_), data_(other.data_)
    {
    }

    HashTable(HashTable&& other) noexcept
        : hash_fn(std::move(other.hash_fn)), eq_fn(std::move(other.eq_fn)),
          size_(other.size_), used_(other.used_), data_(std::move(other.data_))
    {
      other.size_ = 0;
      other.used_ = 0;
    }

    HashTable& operator=(const HashTable& other)
    {
      if (this != &other) {
        HashTable temp(other);
        *this = std::move(temp);
      }
      return *this;
    }

    HashTable& operator=(HashTable&& other) noexcept
    {
      if (this != &other) {
        hash_fn = std::move(other.hash_fn);
        eq_fn = std::move(other.eq_fn);
        size_ = other.size_;
        used_ = other.used_;
        data_ = std::move(other.data_);

        other.size_ = 0;
        other.used_ = 0;
      }
      return *this;
    }

    ~HashTable() = default;

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
    }

    Value drop(Key k)
    {
      size_t ind = getBucketIndex(k);
      Bucket& original_bucket = data_[ind];

      Bucket tempBucket = original_bucket;
      bool found = false;
      Value removed_value{};

      for (size_t i = 0; i < tempBucket.filled; ++i) {
        if (eq_fn(tempBucket.cells[i].first, k)) {
          removed_value = tempBucket.cells[i].second;

          for (size_t j = i; j < tempBucket.filled - 1; ++j) {
            tempBucket.cells[j] = tempBucket.cells[j + 1];
          }
          tempBucket.filled--;

          if (!tempBucket.overflow_.empty()) {
            tempBucket.cells[tempBucket.filled++] =
                tempBucket.overflow_.front();
            tempBucket.overflow_.popFront();
          }

          found = true;
          break;
        }
      }

      if (!found) {
        for (auto it = tempBucket.overflow_.begin();
             it != tempBucket.overflow_.end(); ++it) {
          if (eq_fn((*it).first, k)) {
            removed_value = (*it).second;
            tempBucket.overflow_.erase(it);
            found = true;
            break;
          }
        }
      }

      if (!found) {
        throw std::runtime_error("key not found");
      }

      original_bucket = std::move(tempBucket);
      used_--;

      return removed_value;
    }

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
    }

    void rehash(size_t slots)
    {
      HashTable temp(slots);

      for (auto it = begin(); it != end(); ++it) {
        temp.add((*it).first, (*it).second);
      }
      *this = std::move(temp);
    }

    size_t get_overflow_count() const
    {
      size_t total = 0;

      for (size_t i = 0; i < size_; ++i) {
        total += data_[i].overflow_.size();
      }
      return total;
    }

    double get_average_elements_per_bucket() const
    {
      if (size_ == 0)
        return 0.0;
      return static_cast< double >(used_) / size_;
    }

  private:
    size_t size_ = 0;
    size_t used_ = 0;

    stuff::Vector< Bucket > data_;

    Hash hash_fn;
    Equal eq_fn;

    size_t getBucketIndex(const Key& k) const { return hash_fn(k) % size_; }
  };
}

#endif
