#pragma once

#include "index.h"

#include "Rcpp.h"

#include <memory>

#ifdef VROOM_LOG
#include "spdlog/spdlog.h"
#endif

namespace vroom {

class index_collection : public std::enable_shared_from_this<index_collection> {

public:
  index_collection(
      Rcpp::List in,
      const char* delim,
      const char quote,
      const bool trim_ws,
      const bool escape_double,
      const bool escape_backslash,
      const bool has_header,
      const size_t skip,
      const size_t n_max,
      const char comment,
      const size_t num_threads,
      const bool progress);

  const string get(size_t row, size_t col) const;

  size_t num_columns() const { return columns_; }

  size_t num_rows() const { return rows_; }

  std::vector<std::string> filenames() const {
    std::vector<std::string> out;
    for (const auto& index : indexes_) {
      out.push_back(index->filename());
    }
    return out;
  }

  std::vector<size_t> row_sizes() const {
    std::vector<size_t> out;
    for (const auto& index : indexes_) {
      out.push_back(index->num_rows());
    }
    return out;
  }

  class column {

  public:
    class base_iterator {
    public:
      virtual void next() = 0;
      virtual void prev() = 0;
      virtual void advance(ptrdiff_t n) = 0;
      virtual bool equal_to(const base_iterator& it) const = 0;
      virtual ptrdiff_t distance_to(const base_iterator& it) const = 0;
      virtual string value() const = 0;
      virtual base_iterator* clone() const = 0;
      virtual string at(ptrdiff_t n) const = 0;
      virtual ~base_iterator() {
        SPDLOG_TRACE("{0:x}: base_iterator dtor", (size_t)this);
      }
    };

    class iterator {
      base_iterator* it_;

    public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = string;
      using pointer = string*;
      using reference = string&;

      iterator(base_iterator* it) : it_(it) {
        SPDLOG_TRACE("{0:x}: iterator ctor", (size_t)this);
      }

      iterator& operator=(const iterator& other) = delete;
      //{
      // SPDLOG_TRACE("{0:x}: iterator assignment", (size_t)this);

      // base_iterator* original = it_;
      // it_ = other.it_->clone();
      // delete original;

      // return *this;
      //}

      iterator(const iterator& other) : it_(other.it_->clone()) {
        SPDLOG_TRACE("{0:x}: iterator cctor", (size_t)this);
      }

      iterator operator++(int) { /* postfix */
        iterator copy(*this);
        it_->next();
        return copy;
      }

      iterator& operator++() /* prefix */ {
        it_->next();
        return *this;
      }

      iterator operator--(int) { /* postfix */
        iterator copy(*this);
        it_->prev();
        return copy;
      }

      iterator& operator--() /* prefix */ {
        it_->prev();
        return *this;
      }

      bool operator!=(const iterator& other) const {
        return !it_->equal_to(*other.it_);
      }

      bool operator==(const iterator& other) const {
        return it_->equal_to(*other.it_);
      }

      string operator*() const { return it_->value(); }

      iterator& operator+=(ptrdiff_t n) {
        it_->advance(n);
        return *this;
      }
      iterator operator+(ptrdiff_t n) const {

        SPDLOG_TRACE("{0:x}: iterator operator+({1})", (size_t)this, n);

        iterator copy(*this);
        copy.it_->advance(n);
        return copy;
      }

      iterator operator-(ptrdiff_t n) const {
        iterator copy(*this);
        copy.it_->advance(-n);
        return copy;
      }

      ptrdiff_t operator-(const iterator& other) const {
        return -it_->distance_to(*other.it_);
      }

      string operator[](ptrdiff_t n) const { return it_->at(n); }

      ~iterator() {
        SPDLOG_TRACE("{0:x}: iterator dtor", (size_t)this);
        delete it_;
      }
    };

    class full_iterator : public base_iterator {
      size_t i_;
      std::shared_ptr<const index_collection> idx_;
      size_t column_;
      size_t start_;
      size_t end_;
      index::column::iterator it_;
      index::column::iterator it_end_;
      index::column::iterator it_start_;

    public:
      full_iterator(std::shared_ptr<const index_collection> idx, size_t column);
      void next();
      void prev();
      void advance(ptrdiff_t n);
      inline bool equal_to(const base_iterator& other) const {
        return i_ == static_cast<const full_iterator&>(other).i_ &&
               it_ == static_cast<const full_iterator&>(other).it_;
      }
      ptrdiff_t distance_to(const base_iterator& it) const;
      string value() const;
      full_iterator* clone() const;
      string at(ptrdiff_t n) const;
      virtual ~full_iterator() {
        SPDLOG_TRACE("{0:x}: full_iterator dtor", (size_t)this);
      }
    };

    class subset_iterator : public base_iterator {
      size_t i_;
      iterator it_;
      std::shared_ptr<std::vector<size_t> > indexes_;

    public:
      subset_iterator(
          const iterator& it,
          const std::shared_ptr<std::vector<size_t> >& indexes)
          : i_(0), it_(it), indexes_(indexes) {
        SPDLOG_TRACE("{0:x}: subset_iterator ctor", (size_t)this);
      }
      void next() { ++i_; }
      void prev() { --i_; }
      void advance(ptrdiff_t n) { i_ += n; }
      bool equal_to(const base_iterator& other) const {
        auto other_ = static_cast<const subset_iterator&>(other);
        return i_ == other_.i_;
      };
      ptrdiff_t distance_to(const base_iterator& that) const {
        auto that_ = static_cast<const subset_iterator&>(that);
        return that_.i_ - i_;
      };
      string value() const { return *(it_ + (*indexes_)[i_]); };
      subset_iterator* clone() const {
        SPDLOG_TRACE("{0:x}: subset_iterator clone", (size_t)this);
        auto copy = new index_collection::column::subset_iterator(*this);
        return copy;
      };

      string at(ptrdiff_t n) const { return it_[(*indexes_)[n]]; }

      virtual ~subset_iterator() {
        SPDLOG_TRACE("{0:x}: subset_iterator dtor", (size_t)this);
      }
    };

    iterator begin() const { return begin_; }
    iterator end() const { return end_; }

    column slice(size_t start, size_t end) const {
      return {begin_ + start, begin_ + end};
    }

    column subset(const std::shared_ptr<std::vector<size_t> >& idx) const {
      auto begin = new subset_iterator(begin_, idx);
      auto end = new subset_iterator(begin_, idx);
      end->advance(idx->size());
      return {begin, end};
    }

    size_t size() const { return end_ - begin_; }
    string operator[](size_t i) const { return begin_[i]; }

    column() = delete;
    column(base_iterator* begin, base_iterator* end)
        : begin_(begin), end_(end) {
      SPDLOG_TRACE("{0:x}: column ctor 1", (size_t)this);
    };

    column(const iterator& begin, const iterator& end)
        : begin_(begin), end_(end) {
      SPDLOG_TRACE("{0:x}: column ctor 2", (size_t)this);
    };

  private:
    const iterator begin_;
    const iterator end_;
  };

  column get_column(size_t num) const {
    SPDLOG_TRACE("{0:x}: get_column()", (size_t)this);
    auto end = new column::full_iterator(shared_from_this(), num);
    end->advance(rows_);
    return {new column::full_iterator(shared_from_this(), num), end};
  }

  index::row row(size_t row) const {

    for (const auto& idx : indexes_) {

      auto sz = idx->num_rows();
      if (row < sz) {
        return idx->get_row(row);
      }
      row -= sz;
    }
    /* should never get here */
    return indexes_[0]->get_header();
  }

  index::row get_header() const { return indexes_[0]->get_header(); }

private:
  std::vector<std::shared_ptr<index> > indexes_;

  size_t rows_;
  size_t columns_;
}; // namespace vroom
} // namespace vroom
