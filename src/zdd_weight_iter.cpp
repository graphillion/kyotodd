#include "bdd.h"
#include "bdd_internal.h"
#include "zdd_weight_iter.h"
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace kyotodd {


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

// Iterative counterpart to compute_min_dist_rec. Stack-safe for deep ZDDs
// (level > BDD_RecurLimit). Memo is shared with the recursive variant; key
// is the bddp with complement bit retained (same as _rec).
static long long compute_min_dist_iter(
    bddp root_f,
    const std::vector<int>& weights,
    std::unordered_map<bddp, long long>& memo)
{
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddvar var;
        bddp hi;
        long long lo_dist;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root_f;
    init.var = 0;
    init.hi = 0;
    init.lo_dist = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    long long result = 0;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f == bddempty) {
                result = LLONG_MAX;
                stack.pop_back();
                break;
            }
            if (f == bddsingle) {
                result = 0;
                stack.pop_back();
                break;
            }
            std::unordered_map<bddp, long long>::iterator it = memo.find(f);
            if (it != memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp f_raw = f & ~BDD_COMP_FLAG;
            bddvar var = node_var(f_raw);
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            if (comp) lo = bddnot(lo);

            frame.var = var;
            frame.hi = hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = lo;
            child.var = 0;
            child.hi = 0;
            child.lo_dist = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_dist = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = frame.hi;
            child.var = 0;
            child.hi = 0;
            child.lo_dist = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            long long hi_dist = result;
            long long hi_total = (hi_dist == LLONG_MAX)
                                     ? LLONG_MAX
                                     : static_cast<long long>(weights[frame.var]) + hi_dist;
            long long r = std::min(frame.lo_dist, hi_total);
            memo[frame.f] = r;
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

long long ZddMinWeightIterator::compute_min_dist(bddp f) {
    if (use_iter_1op(f)) {
        return compute_min_dist_iter(f, state_->weights, state_->min_dist_memo);
    }
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

    bddp root = zdd.id();

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

    // Validate weights size against the ZDD's top variable.
    bddvar top = bddtop(root);
    if (top > 0 && weights.size() <= static_cast<size_t>(top)) {
        throw std::invalid_argument(
            "ZddMinWeightIterator: weights.size() must be > top variable");
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

// --- ZddMaxWeightIterator ---

static std::vector<int> negate_weights(const std::vector<int>& weights) {
    std::vector<int> neg(weights.size());
    for (size_t i = 0; i < weights.size(); ++i) {
        if (weights[i] == INT_MIN) {
            throw std::invalid_argument(
                "ZddMaxWeightIterator: weight value INT_MIN is not supported");
        }
        neg[i] = -weights[i];
    }
    return neg;
}

ZddMaxWeightIterator::ZddMaxWeightIterator(
    const ZDD& zdd, const std::vector<int>& weights)
    : inner_(zdd, negate_weights(weights)), current_(), exhausted_(false)
{
    ZddMinWeightIterator end;
    if (inner_ == end) {
        exhausted_ = true;
    } else {
        sync_current();
    }
}

ZddMaxWeightIterator::ZddMaxWeightIterator()
    : inner_(), current_(), exhausted_(true)
{
}

void ZddMaxWeightIterator::sync_current() {
    const auto& val = *inner_;
    current_ = {-val.first, val.second};
}

ZddMaxWeightIterator::reference ZddMaxWeightIterator::operator*() const {
    return current_;
}

ZddMaxWeightIterator::pointer ZddMaxWeightIterator::operator->() const {
    return &current_;
}

ZddMaxWeightIterator& ZddMaxWeightIterator::operator++() {
    ++inner_;
    ZddMinWeightIterator end;
    if (inner_ == end) {
        exhausted_ = true;
    } else {
        sync_current();
    }
    return *this;
}

ZddMaxWeightIterator ZddMaxWeightIterator::operator++(int) {
    ZddMaxWeightIterator old = *this;
    ++(*this);
    return old;
}

bool ZddMaxWeightIterator::operator==(const ZddMaxWeightIterator& other) const {
    if (exhausted_ && other.exhausted_) return true;
    if (exhausted_ != other.exhausted_) return false;
    return inner_ == other.inner_;
}

bool ZddMaxWeightIterator::operator!=(const ZddMaxWeightIterator& other) const {
    return !(*this == other);
}

// --- ZddMaxWeightRange ---

ZddMaxWeightRange::ZddMaxWeightRange(
    const ZDD& zdd, const std::vector<int>& weights)
    : zdd_(zdd), weights_(weights)
{
}

ZddMaxWeightIterator ZddMaxWeightRange::begin() const {
    return ZddMaxWeightIterator(zdd_, weights_);
}

ZddMaxWeightIterator ZddMaxWeightRange::end() const {
    return ZddMaxWeightIterator();
}

} // namespace kyotodd
