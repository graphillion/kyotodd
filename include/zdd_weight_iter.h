#ifndef KYOTODD_ZDD_WEIGHT_ITER_H
#define KYOTODD_ZDD_WEIGHT_ITER_H

#include "bdd_types.h"
#include <vector>
#include <queue>
#include <memory>
#include <unordered_map>
#include <iterator>
#include <utility>
#include <climits>

/// A single step along a root-to-terminal path in the ZDD DAG.
struct ZddPathStep {
    bddp node;   ///< The bddp value (with complement bit) at this step.
    int choice;   ///< 0 = took lo edge, 1 = took hi edge.
    bool operator==(const ZddPathStep& o) const {
        return node == o.node && choice == o.choice;
    }
    bool operator!=(const ZddPathStep& o) const {
        return !(*this == o);
    }
};

/**
 * @brief STL input iterator that enumerates sets from a ZDD family
 *        in ascending order of weighted sum, using Yen's k-shortest
 *        path algorithm adapted to ZDD DAGs.
 *
 * Each dereference yields a pair (weight, set) where weight is the
 * sum of weights of elements in the set, and set is a sorted vector
 * of variable numbers.
 *
 * Usage:
 * @code
 *   ZDD f = ...;
 *   std::vector<int> weights = ...;
 *   ZddMinWeightRange range(f, weights);
 *   for (auto it = range.begin(); it != range.end(); ++it) {
 *       long long w = it->first;
 *       const std::vector<bddvar>& s = it->second;
 *       if (w > threshold) break;
 *   }
 * @endcode
 */
class ZddMinWeightIterator {
public:
    typedef std::input_iterator_tag iterator_category;
    typedef std::pair<long long, std::vector<bddvar> > value_type;
    typedef std::ptrdiff_t difference_type;
    typedef const value_type* pointer;
    typedef const value_type& reference;

    /// Construct a begin iterator for the given ZDD and weights.
    /// weights must be indexed by variable number with size > var_used().
    ZddMinWeightIterator(const ZDD& zdd, const std::vector<int>& weights);

    /// Construct an end (sentinel) iterator.
    ZddMinWeightIterator();

    reference operator*() const;
    pointer operator->() const;
    ZddMinWeightIterator& operator++();
    ZddMinWeightIterator operator++(int);
    bool operator==(const ZddMinWeightIterator& other) const;
    bool operator!=(const ZddMinWeightIterator& other) const;

private:
    typedef std::vector<ZddPathStep> Path;

    struct WeightedPath {
        long long weight;
        Path path;
        bool operator>(const WeightedPath& o) const {
            return weight > o.weight;
        }
    };

    struct State {
        ZDD zdd;
        std::vector<int> weights;
        std::unordered_map<bddp, long long> min_dist_memo;
        std::priority_queue<WeightedPath,
                            std::vector<WeightedPath>,
                            std::greater<WeightedPath> > candidates;
        std::vector<Path> found_paths;
        value_type current;
        bool exhausted;

        State() : zdd(0), exhausted(true) {}
    };

    std::shared_ptr<State> state_;

    long long compute_min_dist(bddp f);
    Path trace_shortest_path(bddp f);
    void generate_candidates(const Path& last_path);
    void advance();
    static value_type path_to_value(const Path& path,
                                     const std::vector<int>& weights);
};

/**
 * @brief Range wrapper for ZddMinWeightIterator, enabling range-based for.
 *
 * Usage:
 * @code
 *   for (auto& p : ZddMinWeightRange(f, weights)) { ... }
 * @endcode
 */
class ZddMinWeightRange {
public:
    ZddMinWeightRange(const ZDD& zdd, const std::vector<int>& weights);
    ZddMinWeightIterator begin() const;
    ZddMinWeightIterator end() const;
private:
    ZDD zdd_;
    std::vector<int> weights_;
};

#endif
