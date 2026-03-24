#ifndef KYOTODD_ZDD_RANK_ITER_H
#define KYOTODD_ZDD_RANK_ITER_H

#include "bdd_types.h"
#include <vector>
#include <memory>
#include <iterator>

/**
 * @brief STL input iterator that enumerates sets from a ZDD family
 *        in structure order (the same order as rank/unrank).
 *
 * Structure order: empty set (if present) first, then at each node
 * hi-edge sets before lo-edge sets (depth-first, hi-first traversal).
 *
 * Each dereference yields a sorted vector of variable numbers
 * representing a set in the family.
 *
 * Usage:
 * @code
 *   ZDD f = ...;
 *   ZddRankRange range(f);
 *   for (auto it = range.begin(); it != range.end(); ++it) {
 *       const std::vector<bddvar>& s = *it;
 *       // process set s
 *   }
 * @endcode
 */
class ZddRankIterator {
public:
    typedef std::input_iterator_tag iterator_category;
    typedef std::vector<bddvar> value_type;
    typedef std::ptrdiff_t difference_type;
    typedef const value_type* pointer;
    typedef const value_type& reference;

    /// Construct a begin iterator for the given ZDD.
    ZddRankIterator(const ZDD& zdd);

    /// Construct an end (sentinel) iterator.
    ZddRankIterator();

    reference operator*() const;
    pointer operator->() const;
    ZddRankIterator& operator++();
    ZddRankIterator operator++(int);
    bool operator==(const ZddRankIterator& other) const;
    bool operator!=(const ZddRankIterator& other) const;

private:
    struct StackFrame {
        bddvar var;  ///< Variable number at this node.
        bddp lo;     ///< Resolved lo child (to explore after hi).
        int phase;   ///< 0 = exploring hi subtree, 1 = hi done.
    };

    struct State {
        ZDD zdd;                          ///< Original ZDD (GC protection).
        bddp effective_root;              ///< Root after empty set removal.
        std::vector<StackFrame> stack;    ///< DFS stack.
        std::vector<bddvar> current_set;  ///< Variables in current path.
        bool has_empty;                   ///< Whether empty set is in family.
        bool empty_yielded;               ///< Whether empty set has been yielded.

        State() : zdd(0), effective_root(0),
                  has_empty(false), empty_yielded(false) {}
    };

    std::shared_ptr<State> state_;
    value_type current_;
    bool exhausted_;

    bool descend(bddp f);
    void advance();
};

/**
 * @brief Range wrapper for ZddRankIterator, enabling range-based for.
 *
 * Usage:
 * @code
 *   for (auto& s : ZddRankRange(f)) { ... }
 * @endcode
 */
class ZddRankRange {
public:
    ZddRankRange(const ZDD& zdd);
    ZddRankIterator begin() const;
    ZddRankIterator end() const;
private:
    ZDD zdd_;
};

#endif
