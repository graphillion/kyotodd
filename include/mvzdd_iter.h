#ifndef KYOTODD_MVZDD_ITER_H
#define KYOTODD_MVZDD_ITER_H

#include "mvdd.h"
#include "zdd_rank_iter.h"
#include "zdd_random_iter.h"
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace kyotodd {

/**
 * @brief STL input iterator that enumerates MVDD assignments from an
 *        MVZDD family in structure order (same order as rank/unrank).
 *
 * Each dereference yields an assignment as a vector of values
 * (0-indexed: result[i] is the value of MVDD variable i+1).
 *
 * Structure order is inherited from the internal ZDD: empty
 * assignment (all zeros) first if present, then hi-edge before
 * lo-edge at each node.
 */
class MVZDDRankIterator {
public:
    typedef std::input_iterator_tag iterator_category;
    typedef std::vector<int> value_type;
    typedef std::ptrdiff_t difference_type;
    typedef const value_type* pointer;
    typedef const value_type& reference;

    MVZDDRankIterator(const MVZDD& mv);
    MVZDDRankIterator();  // end sentinel

    reference operator*() const;
    pointer operator->() const;
    MVZDDRankIterator& operator++();
    MVZDDRankIterator operator++(int);
    bool operator==(const MVZDDRankIterator& other) const;
    bool operator!=(const MVZDDRankIterator& other) const;

private:
    std::shared_ptr<MVDDVarTable> table_;
    ZddRankIterator inner_;
    ZddRankIterator end_;
    value_type current_;
    bool exhausted_;

    void refresh_current();
};

/**
 * @brief Range wrapper for MVZDDRankIterator, enabling range-based for.
 */
class MVZDDRankRange {
public:
    MVZDDRankRange(const MVZDD& mv);
    MVZDDRankIterator begin() const;
    MVZDDRankIterator end() const;
private:
    MVZDD mv_;
};

/**
 * @brief STL input iterator that enumerates MVDD assignments in
 *        uniformly random order without replacement.
 *
 * Thin wrapper around ZddRandomIterator that converts each set of
 * DD variables into the MVDD assignment vector.
 */
template<typename RNG>
class MVZDDRandomIterator {
public:
    typedef std::input_iterator_tag iterator_category;
    typedef std::vector<int> value_type;
    typedef std::ptrdiff_t difference_type;
    typedef const value_type* pointer;
    typedef const value_type& reference;

    MVZDDRandomIterator(const MVZDD& mv, RNG rng);
    MVZDDRandomIterator();  // end sentinel

    reference operator*() const { return current_; }
    pointer operator->() const { return &current_; }
    MVZDDRandomIterator& operator++() { advance(); return *this; }
    MVZDDRandomIterator operator++(int) {
        MVZDDRandomIterator tmp = *this; advance(); return tmp;
    }
    bool operator==(const MVZDDRandomIterator& other) const {
        return exhausted_ == other.exhausted_;
    }
    bool operator!=(const MVZDDRandomIterator& other) const {
        return !(*this == other);
    }

private:
    std::shared_ptr<MVDDVarTable> table_;
    ZddRandomIterator<RNG> inner_;
    ZddRandomIterator<RNG> end_;
    value_type current_;
    bool exhausted_;

    void advance();
    void refresh_current();
};

/** @brief Range wrapper for MVZDDRandomIterator. */
template<typename RNG>
class MVZDDRandomRange {
public:
    MVZDDRandomRange(const MVZDD& mv, RNG rng) : mv_(mv), rng_(rng) {}
    MVZDDRandomIterator<RNG> begin() const {
        return MVZDDRandomIterator<RNG>(mv_, rng_);
    }
    MVZDDRandomIterator<RNG> end() const {
        return MVZDDRandomIterator<RNG>();
    }
private:
    MVZDD mv_;
    RNG rng_;
};

// ---- MVZDDRandomIterator template implementations ----

template<typename RNG>
MVZDDRandomIterator<RNG>::MVZDDRandomIterator()
    : inner_(), end_(), exhausted_(true) {}

template<typename RNG>
MVZDDRandomIterator<RNG>::MVZDDRandomIterator(const MVZDD& mv, RNG rng)
    : table_(mv.var_table()),
      inner_(mv.to_zdd(), rng),
      end_(),
      exhausted_(false) {
    if (!table_) {
        throw std::logic_error("MVZDDRandomIterator: no var table");
    }
    if (inner_ == end_) {
        exhausted_ = true;
        return;
    }
    refresh_current();
}

template<typename RNG>
void MVZDDRandomIterator<RNG>::advance() {
    if (exhausted_) return;
    ++inner_;
    if (inner_ == end_) {
        exhausted_ = true;
        return;
    }
    refresh_current();
}

template<typename RNG>
void MVZDDRandomIterator<RNG>::refresh_current() {
    const std::vector<bddvar>& dd_vars = *inner_;
    bddvar n = table_->mvdd_var_count();
    current_.assign(n, 0);
    for (size_t j = 0; j < dd_vars.size(); ++j) {
        bddvar dv = dd_vars[j];
        bddvar mv = table_->mvdd_var_of(dv);
        if (mv == 0) {
            throw std::logic_error(
                "MVZDDRandomIterator: DD variable " + std::to_string(dv) +
                " is not registered in the var table");
        }
        int idx = table_->dd_var_index(dv);
        current_[mv - 1] = idx + 1;
    }
}

} // namespace kyotodd

#endif
