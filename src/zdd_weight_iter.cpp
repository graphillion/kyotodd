#include "bdd.h"
#include "bdd_internal.h"
#include "zdd_weight_iter.h"
#include <algorithm>
#include <stdexcept>

// ---- compute_min_dist ----

static long long compute_min_dist_rec(
    bddp f,
    const std::vector<int>& weights,
    std::unordered_map<bddp, long long>& memo)
{
    if (f == bddempty) return LLONG_MAX;
    if (f == bddsingle) return 0;

    BDD_RecurGuard guard;

    std::unordered_map<bddp, long long>::iterator it = memo.find(f);
    if (it != memo.end()) return it->second;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddvar var = node_var(f_raw);
    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);
    if (comp) lo = bddnot(lo);

    long long lo_dist = compute_min_dist_rec(lo, weights, memo);
    long long hi_dist = compute_min_dist_rec(hi, weights, memo);
    long long hi_total = (hi_dist == LLONG_MAX)
                             ? LLONG_MAX
                             : static_cast<long long>(weights[var]) + hi_dist;

    long long result = std::min(lo_dist, hi_total);
    memo[f] = result;
    return result;
}

long long ZddMinWeightIterator::compute_min_dist(bddp f) {
    return compute_min_dist_rec(f, state_->weights, state_->min_dist_memo);
}

// ---- trace_shortest_path ----

ZddMinWeightIterator::Path
ZddMinWeightIterator::trace_shortest_path(bddp f) {
    Path path;
    while (f != bddempty && f != bddsingle) {
        bool comp = (f & BDD_COMP_FLAG) != 0;
        bddp f_raw = f & ~BDD_COMP_FLAG;

        bddvar var = node_var(f_raw);
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);
        if (comp) lo = bddnot(lo);

        long long lo_dist = compute_min_dist(lo);
        long long hi_dist = compute_min_dist(hi);
        long long hi_total = (hi_dist == LLONG_MAX)
                                 ? LLONG_MAX
                                 : static_cast<long long>(state_->weights[var]) + hi_dist;

        if (lo_dist <= hi_total) {
            ZddPathStep step;
            step.node = f;
            step.choice = 0;
            path.push_back(step);
            f = lo;
        } else {
            ZddPathStep step;
            step.node = f;
            step.choice = 1;
            path.push_back(step);
            f = hi;
        }
    }
    if (f == bddempty) {
        // Should not happen if compute_min_dist was finite.
        return Path();
    }
    return path;
}

// ---- path_to_value ----

ZddMinWeightIterator::value_type
ZddMinWeightIterator::path_to_value(
    const Path& path,
    const std::vector<int>& weights)
{
    long long weight = 0;
    std::vector<bddvar> vars;
    for (size_t i = 0; i < path.size(); ++i) {
        if (path[i].choice == 1) {
            bddp f_raw = path[i].node & ~BDD_COMP_FLAG;
            bddvar var = node_var(f_raw);
            vars.push_back(var);
            weight += static_cast<long long>(weights[var]);
        }
    }
    std::sort(vars.begin(), vars.end());
    return value_type(weight, vars);
}

// ---- constructor ----

ZddMinWeightIterator::ZddMinWeightIterator(
    const ZDD& zdd,
    const std::vector<int>& weights)
    : state_(std::make_shared<State>()), exhausted_(false)
{
    state_->zdd = zdd;
    state_->weights = weights;

    bddp root = zdd.get_id();

    // Null ZDD is an error (consistent with min_weight/max_weight).
    if (root == bddnull) {
        throw std::invalid_argument(
            "ZddMinWeightIterator: null ZDD");
    }

    // Empty family: produce an end iterator.
    if (root == bddempty) {
        exhausted_ = true;
        return;
    }

    // Validate weights size.
    if (weights.size() <= static_cast<size_t>(bdd_varcount)) {
        throw std::invalid_argument(
            "ZddMinWeightIterator: weights.size() must be > var_used()");
    }

    // Check if any path to 1-terminal exists.
    long long dist = compute_min_dist(root);
    if (dist == LLONG_MAX) {
        // This shouldn't happen for a non-empty ZDD, but be safe.
        exhausted_ = true;
        return;
    }

    // When root is bddsingle, the path has zero steps (the empty set).
    // trace_shortest_path returns an empty path, which is valid.
    Path first = trace_shortest_path(root);

    state_->found_paths.push_back(first);
    current_ = path_to_value(first, state_->weights);
}

ZddMinWeightIterator::ZddMinWeightIterator()
    : state_(), exhausted_(true)
{
}

// ---- generate_candidates (Yen's core) ----

/// Get the child of node f for the given choice (0=lo, 1=hi),
/// respecting ZDD complement edge semantics.
static bddp zdd_child(bddp f, int choice) {
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;
    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);
    if (comp) lo = bddnot(lo);
    return (choice == 0) ? lo : hi;
}

void ZddMinWeightIterator::generate_candidates(const Path& last_path) {
    long long prefix_weight = 0;

    for (size_t i = 0; i < last_path.size(); ++i) {
        bddp spur_node = last_path[i].node;
        int alt_choice = 1 - last_path[i].choice;

        // Check if any previously found path shares the same prefix
        // [0..i-1] and makes the same alternative choice at position i.
        bool skip = false;
        for (size_t j = 0; j < state_->found_paths.size(); ++j) {
            const Path& prev = state_->found_paths[j];
            if (prev.size() <= i) continue;

            // Check prefix match [0..i-1].
            bool prefix_match = true;
            for (size_t k = 0; k < i; ++k) {
                if (prev[k] != last_path[k]) {
                    prefix_match = false;
                    break;
                }
            }
            if (!prefix_match) continue;

            // Same spur node and alternative choice already explored.
            if (prev[i].node == spur_node && prev[i].choice == alt_choice) {
                skip = true;
                break;
            }
        }
        if (skip) {
            // Accumulate weight of current step before moving on.
            if (last_path[i].choice == 1) {
                bddp f_raw = spur_node & ~BDD_COMP_FLAG;
                bddvar var = node_var(f_raw);
                prefix_weight += static_cast<long long>(state_->weights[var]);
            }
            continue;
        }

        // Compute the child for the alternative choice.
        bddp alt_child = zdd_child(spur_node, alt_choice);

        long long alt_edge_weight = 0;
        if (alt_choice == 1) {
            bddp f_raw = spur_node & ~BDD_COMP_FLAG;
            bddvar var = node_var(f_raw);
            alt_edge_weight = static_cast<long long>(state_->weights[var]);
        }

        long long suffix_dist = compute_min_dist(alt_child);
        if (suffix_dist == LLONG_MAX) {
            // Dead end: alt_child cannot reach 1-terminal.
            if (last_path[i].choice == 1) {
                bddp f_raw = spur_node & ~BDD_COMP_FLAG;
                bddvar var = node_var(f_raw);
                prefix_weight += static_cast<long long>(state_->weights[var]);
            }
            continue;
        }

        // Trace the shortest suffix from alt_child.
        Path suffix = trace_shortest_path(alt_child);

        // Build the full candidate path: prefix + spur step + suffix.
        Path candidate;
        candidate.reserve(i + 1 + suffix.size());
        for (size_t k = 0; k < i; ++k) {
            candidate.push_back(last_path[k]);
        }
        ZddPathStep spur_step;
        spur_step.node = spur_node;
        spur_step.choice = alt_choice;
        candidate.push_back(spur_step);
        for (size_t k = 0; k < suffix.size(); ++k) {
            candidate.push_back(suffix[k]);
        }

        long long candidate_weight = prefix_weight + alt_edge_weight + suffix_dist;

        WeightedPath wp;
        wp.weight = candidate_weight;
        wp.path = candidate;
        state_->candidates.push(wp);

        // Accumulate weight of current step.
        if (last_path[i].choice == 1) {
            bddp f_raw = spur_node & ~BDD_COMP_FLAG;
            bddvar var = node_var(f_raw);
            prefix_weight += static_cast<long long>(state_->weights[var]);
        }
    }
}

// ---- advance ----

void ZddMinWeightIterator::advance() {
    // Generate candidates from the most recently output path.
    generate_candidates(state_->found_paths.back());

    // Pop candidates until we find one not already in found_paths.
    while (!state_->candidates.empty()) {
        WeightedPath best = state_->candidates.top();
        state_->candidates.pop();

        // Check for duplicate.
        bool dup = false;
        for (size_t j = 0; j < state_->found_paths.size(); ++j) {
            if (state_->found_paths[j] == best.path) {
                dup = true;
                break;
            }
        }
        if (dup) continue;

        state_->found_paths.push_back(best.path);
        current_ = path_to_value(best.path, state_->weights);
        return;
    }

    exhausted_ = true;
}

// ---- iterator operators ----

ZddMinWeightIterator::reference
ZddMinWeightIterator::operator*() const {
    return current_;
}

ZddMinWeightIterator::pointer
ZddMinWeightIterator::operator->() const {
    return &current_;
}

ZddMinWeightIterator&
ZddMinWeightIterator::operator++() {
    advance();
    return *this;
}

ZddMinWeightIterator
ZddMinWeightIterator::operator++(int) {
    ZddMinWeightIterator tmp = *this;
    advance();
    return tmp;
}

bool ZddMinWeightIterator::operator==(
    const ZddMinWeightIterator& other) const
{
    bool this_end = exhausted_;
    bool other_end = other.exhausted_;
    return this_end == other_end;
}

bool ZddMinWeightIterator::operator!=(
    const ZddMinWeightIterator& other) const
{
    return !(*this == other);
}

// ---- ZddMinWeightRange ----

ZddMinWeightRange::ZddMinWeightRange(
    const ZDD& zdd,
    const std::vector<int>& weights)
    : zdd_(zdd), weights_(weights)
{
}

ZddMinWeightIterator ZddMinWeightRange::begin() const {
    return ZddMinWeightIterator(zdd_, weights_);
}

ZddMinWeightIterator ZddMinWeightRange::end() const {
    return ZddMinWeightIterator();
}
