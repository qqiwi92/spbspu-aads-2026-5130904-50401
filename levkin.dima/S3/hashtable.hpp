#ifndef HASH
#define HASH

#include "list.hpp"
#include "vector.hpp"
#include <boost/uuid/detail/sha1.hpp>
#include <cstddef>
#include <stdexcept>
#include <utility>

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
    };

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
    };

    void rehash(size_t slots)
    {
      HashTable temp(slots);

      for (size_t i = 0; i < size_; ++i) {
        for (size_t j = 0; j < data_[i].filled; ++j) {
          temp.add(data_[i].cells[j].first, data_[i].cells[j].second);
        }
        for (const auto& it : data_[i].overflow_) {
          temp.add(it.first, it.second);
        }
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
    using Cell = std::pair< Key, Value >;

    size_t size_ = 0;
    size_t used_ = 0;

    struct Bucket {
      size_t filled = 0;
      Cell cells[bucketSize];
      List< Cell > overflow_;
    };

    stuff::Vector< Bucket > data_;

    Hash hash_fn;
    Equal eq_fn;

    size_t getBucketIndex(const Key& k) const { return hash_fn(k) % size_; }
  };
}

#endif
