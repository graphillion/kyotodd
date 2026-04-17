#ifndef KYOTODD_ZDD_RANDOM_ITER_H
#define KYOTODD_ZDD_RANDOM_ITER_H

#include "bdd_types.h"
#include <vector>
#include <memory>
#include <iterator>

namespace kyotodd {

/**
 * @brief STL input iterator that enumerates sets from a ZDD family
 *        in uniformly random order without replacement.
 *
 * Uses a hybrid strategy:
 * - When fewer than half the sets have been sampled, uses rejection
 *   sampling from the original ZDD.
 * - When half or more have been sampled, computes the remaining
 *   family (f - sampled) and samples directly from it.
 *
 * Each dereference yields a sorted vector of variable numbers.
 *
 * Usage:
 * @code
 *   ZDD f = ...;
 *   std::mt19937_64 rng(42);
 *   ZddRandomRange<std::mt19937_64> range(f, rng);
 *   for (auto it = range.begin(); it != range.end(); ++it) {
 *       const std::vector<bddvar>& s = *it;
 *       // process set s
 *   }
 * @endcode
 *
 * @tparam RNG A uniform random bit generator (e.g. std::mt19937_64).
 */
template<typename RNG>
class ZddRandomIterator {
public:
    typedef std::input_iterator_tag iterator_category;
    typedef std::vector<bddvar> value_type;
    typedef std::ptrdiff_t difference_type;
    typedef const value_type* pointer;
    typedef const value_type& reference;

    /// Construct a begin iterator for the given ZDD.
    /// @param zdd The ZDD family to sample from.
    /// @param rng A random number generator (copied into the iterator).
    ZddRandomIterator(const ZDD& zdd, RNG rng);

    /// Construct an end (sentinel) iterator.
    ZddRandomIterator();

    reference operator*() const;
    pointer operator->() const;
    ZddRandomIterator& operator++();
    ZddRandomIterator operator++(int);
    bool operator==(const ZddRandomIterator& other) const;
    bool operator!=(const ZddRandomIterator& other) const;

private:
    struct State {
        ZDD original;               ///< Original ZDD (GC protection).
        ZDD sampled;                ///< ZDD of already-sampled sets.
        ZDD remaining;              ///< f - sampled (lazily computed).
        ZddCountMemo f_memo;        ///< Memo for original (computed once).
        bigint::BigInt total;       ///< |f|.
        bigint::BigInt count_sampled; ///< Number of sets returned so far.
        bool direct_mode;           ///< Whether we switched to direct sampling.
        RNG rng;

        State(const ZDD& z, RNG r)
            : original(z), sampled(0), remaining(0),
              f_memo(z), total(0), count_sampled(0),
              direct_mode(false), rng(r) {}
    };

    std::shared_ptr<State> state_;
    value_type current_;
    bool exhausted_;

    void advance();

    /// Check if a set is a member of a ZDD family.
    static bool contains(const ZDD& family,
                         const std::vector<bddvar>& s);
};

/**
 * @brief Range wrapper for ZddRandomIterator, enabling range-based for.
 *
 * Usage:
 * @code
 *   std::mt19937_64 rng(42);
 *   for (auto& s : ZddRandomRange<std::mt19937_64>(f, rng)) { ... }
 * @endcode
 */
template<typename RNG>
class ZddRandomRange {
public:
    ZddRandomRange(const ZDD& zdd, RNG rng);
    ZddRandomIterator<RNG> begin() const;
    ZddRandomIterator<RNG> end() const;
private:
    ZDD zdd_;
    RNG rng_;
};

// ---- Template implementations ----

template<typename RNG>
bool ZddRandomIterator<RNG>::contains(
    const ZDD& family,
    const std::vector<bddvar>& s)
{
    ZDD single = ZDD::single_set(s);
    return (family & single).id() != bddempty;
}

template<typename RNG>
ZddRandomIterator<RNG>::ZddRandomIterator(const ZDD& zdd, RNG rng)
    : state_(std::make_shared<State>(zdd, rng)), exhausted_(false)
{
    bddp root = zdd.id();

    if (root == bddnull) {
        throw std::invalid_argument(
            "ZddRandomIterator: null ZDD");
    }

    if (root == bddempty) {
        exhausted_ = true;
        return;
    }

    // Compute total count and populate memo.
    state_->total = state_->original.exact_count(state_->f_memo);

    // Sample the first set (no membership check needed).
    current_ = state_->original.uniform_sample(
        state_->rng, state_->f_memo);
    state_->sampled = state_->sampled + ZDD::single_set(current_);
    state_->count_sampled = bigint::BigInt(1);
}

template<typename RNG>
ZddRandomIterator<RNG>::ZddRandomIterator()
    : state_(), exhausted_(true)
{
}

template<typename RNG>
void ZddRandomIterator<RNG>::advance() {
    if (state_->count_sampled >= state_->total) {
        exhausted_ = true;
        return;
    }

    // Threshold: switch to direct mode when sampled >= total / 2.
    if (state_->count_sampled + state_->count_sampled < state_->total) {
        // Rejection sampling from original ZDD.
        while (true) {
            std::vector<bddvar> s =
                state_->original.uniform_sample(
                    state_->rng, state_->f_memo);
            if (!contains(state_->sampled, s)) {
                state_->sampled =
                    state_->sampled + ZDD::single_set(s);
                state_->count_sampled += bigint::BigInt(1);
                current_ = s;
                return;
            }
        }
    } else {
        // Direct sampling from remaining = f - sampled.
        if (!state_->direct_mode) {
            state_->remaining =
                state_->original - state_->sampled;
            state_->direct_mode = true;
        }

        ZddCountMemo remaining_memo(state_->remaining);
        std::vector<bddvar> s =
            state_->remaining.uniform_sample(
                state_->rng, remaining_memo);

        ZDD single = ZDD::single_set(s);
        state_->sampled = state_->sampled + single;
        state_->remaining = state_->remaining - single;
        state_->count_sampled += bigint::BigInt(1);
        current_ = s;
    }
}

// ---- Iterator operators ----

template<typename RNG>
typename ZddRandomIterator<RNG>::reference
ZddRandomIterator<RNG>::operator*() const {
    return current_;
}

template<typename RNG>
typename ZddRandomIterator<RNG>::pointer
ZddRandomIterator<RNG>::operator->() const {
    return &current_;
}

template<typename RNG>
ZddRandomIterator<RNG>&
ZddRandomIterator<RNG>::operator++() {
    advance();
    return *this;
}

template<typename RNG>
ZddRandomIterator<RNG>
ZddRandomIterator<RNG>::operator++(int) {
    ZddRandomIterator tmp = *this;
    advance();
    return tmp;
}

template<typename RNG>
bool ZddRandomIterator<RNG>::operator==(
    const ZddRandomIterator& other) const
{
    bool this_end = exhausted_;
    bool other_end = other.exhausted_;
    return this_end == other_end;
}

template<typename RNG>
bool ZddRandomIterator<RNG>::operator!=(
    const ZddRandomIterator& other) const
{
    return !(*this == other);
}

// ---- ZddRandomRange ----

template<typename RNG>
ZddRandomRange<RNG>::ZddRandomRange(const ZDD& zdd, RNG rng)
    : zdd_(zdd), rng_(rng)
{
}

template<typename RNG>
ZddRandomIterator<RNG> ZddRandomRange<RNG>::begin() const {
    return ZddRandomIterator<RNG>(zdd_, rng_);
}

template<typename RNG>
ZddRandomIterator<RNG> ZddRandomRange<RNG>::end() const {
    return ZddRandomIterator<RNG>();
}

} // namespace kyotodd

#endif
