#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <algorithm>
#include <climits>
#include <stdexcept>
#include <unordered_map>

namespace kyotodd {


// ---- weight sum ----

static bigint::BigInt bddweightsum_rec(
    bddp f,
    const std::vector<int>& weights,
    CountMemoMap& sum_memo,
    CountMemoMap& count_memo)
{
    if (f == bddempty || f == bddsingle) return bigint::BigInt(0);

    BDD_RecurGuard guard;

    // get_sum is complement-invariant (∅ has weight 0), so strip complement
    bddp f_raw = f & ~BDD_COMP_FLAG;

    auto it = sum_memo.find(f_raw);
    if (it != sum_memo.end()) return it->second;

    bddvar var = node_var(f_raw);
    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);

    bigint::BigInt sum_lo = bddweightsum_rec(lo, weights, sum_memo, count_memo);
    bigint::BigInt sum_hi = bddweightsum_rec(hi, weights, sum_memo, count_memo);
    bigint::BigInt count_hi = bddexactcount_rec(hi, count_memo);

    bigint::BigInt result = sum_lo + sum_hi
                          + bigint::BigInt(weights[var]) * count_hi;
    sum_memo[f_raw] = result;
    return result;
}

static void bddweightsum_validate(bddp f, const std::vector<int>& weights,
                                   const char* name) {
    bddp_validate(f, name);
    if (f == bddnull) {
        throw std::invalid_argument(std::string(name) + ": null ZDD");
    }
    if (f == bddempty || f == bddsingle) return;
    bddvar top = bddtop(f);
    if (weights.size() <= static_cast<size_t>(top)) {
        throw std::invalid_argument(
            std::string(name) + ": weights.size() must be > top variable");
    }
}

bigint::BigInt bddweightsum(bddp f, const std::vector<int>& weights) {
    bddweightsum_validate(f, weights, "bddweightsum");
    if (f == bddempty || f == bddsingle)
        return bigint::BigInt(0);
    CountMemoMap sum_memo;
    CountMemoMap count_memo;
    if (use_iter_1op(f)) {
        return bddweightsum_iter(f, weights, sum_memo, count_memo);
    }
    return bddweightsum_rec(f, weights, sum_memo, count_memo);
}

bigint::BigInt bddweightsum(bddp f, const std::vector<int>& weights,
                             BddExecMode mode) {
    bddweightsum_validate(f, weights, "bddweightsum");
    if (f == bddempty || f == bddsingle)
        return bigint::BigInt(0);
    CountMemoMap sum_memo;
    CountMemoMap count_memo;
    switch (mode) {
    case BddExecMode::Iterative:
        return bddweightsum_iter(f, weights, sum_memo, count_memo);
    case BddExecMode::Recursive:
        return bddweightsum_rec(f, weights, sum_memo, count_memo);
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) {
            return bddweightsum_iter(f, weights, sum_memo, count_memo);
        }
        return bddweightsum_rec(f, weights, sum_memo, count_memo);
    }
}

// ---- min/max weight ----

static void bddweight_validate(bddp f, const std::vector<int>& weights,
                                const char* name) {
    bddp_validate(f, name);
    if (f == bddnull) {
        throw std::invalid_argument(
            std::string(name) + ": null ZDD");
    }
    if (f == bddempty) {
        throw std::invalid_argument(
            std::string(name) + ": empty family has no sets");
    }
    bddvar top = bddtop(f);
    if (top > 0 && weights.size() <= static_cast<size_t>(top)) {
        throw std::invalid_argument(
            std::string(name) + ": weights.size() must be > top variable");
    }
}

static long long bddminweight_rec(bddp f, const std::vector<int>& weights,
                                   std::unordered_map<bddp, long long>& memo) {
    if (f == bddempty) return LLONG_MAX;
    if (f == bddsingle) return 0;

    BDD_RecurGuard guard;

    auto it = memo.find(f);
    if (it != memo.end()) return it->second;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddvar var = node_var(f_raw);
    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);
    if (comp) lo = bddnot(lo);

    long long lo_val = bddminweight_rec(lo, weights, memo);
    long long hi_val = bddminweight_rec(hi, weights, memo);
    long long hi_total = static_cast<long long>(weights[var]) + hi_val;

    long long result = std::min(lo_val, hi_total);
    memo[f] = result;
    return result;
}

long long bddminweight(bddp f, const std::vector<int>& weights) {
    bddweight_validate(f, weights, "bddminweight");
    std::unordered_map<bddp, long long> memo;
    if (use_iter_1op(f)) {
        return bddminweight_iter(f, weights, memo);
    }
    return bddminweight_rec(f, weights, memo);
}

long long bddminweight(bddp f, const std::vector<int>& weights,
                       BddExecMode mode) {
    bddweight_validate(f, weights, "bddminweight");
    std::unordered_map<bddp, long long> memo;
    switch (mode) {
    case BddExecMode::Iterative:
        return bddminweight_iter(f, weights, memo);
    case BddExecMode::Recursive:
        return bddminweight_rec(f, weights, memo);
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) {
            return bddminweight_iter(f, weights, memo);
        }
        return bddminweight_rec(f, weights, memo);
    }
}

static long long bddmaxweight_rec(bddp f, const std::vector<int>& weights,
                                   std::unordered_map<bddp, long long>& memo) {
    if (f == bddempty) return LLONG_MIN;
    if (f == bddsingle) return 0;

    BDD_RecurGuard guard;

    auto it = memo.find(f);
    if (it != memo.end()) return it->second;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddvar var = node_var(f_raw);
    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);
    if (comp) lo = bddnot(lo);

    long long lo_val = bddmaxweight_rec(lo, weights, memo);
    long long hi_val = bddmaxweight_rec(hi, weights, memo);
    long long hi_total = static_cast<long long>(weights[var]) + hi_val;

    long long result = std::max(lo_val, hi_total);
    memo[f] = result;
    return result;
}

long long bddmaxweight(bddp f, const std::vector<int>& weights) {
    bddweight_validate(f, weights, "bddmaxweight");
    std::unordered_map<bddp, long long> memo;
    if (use_iter_1op(f)) {
        return bddmaxweight_iter(f, weights, memo);
    }
    return bddmaxweight_rec(f, weights, memo);
}

long long bddmaxweight(bddp f, const std::vector<int>& weights,
                       BddExecMode mode) {
    bddweight_validate(f, weights, "bddmaxweight");
    std::unordered_map<bddp, long long> memo;
    switch (mode) {
    case BddExecMode::Iterative:
        return bddmaxweight_iter(f, weights, memo);
    case BddExecMode::Recursive:
        return bddmaxweight_rec(f, weights, memo);
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) {
            return bddmaxweight_iter(f, weights, memo);
        }
        return bddmaxweight_rec(f, weights, memo);
    }
}

static void bddminweightset_trace(bddp f, const std::vector<int>& weights,
                                   std::unordered_map<bddp, long long>& memo,
                                   std::vector<bddvar>& result) {
    while (f != bddempty && f != bddsingle) {
        bool comp = (f & BDD_COMP_FLAG) != 0;
        bddp f_raw = f & ~BDD_COMP_FLAG;

        bddvar var = node_var(f_raw);
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);
        if (comp) lo = bddnot(lo);

        long long lo_val = memo[lo];
        long long hi_val = memo[hi];
        long long hi_total = static_cast<long long>(weights[var]) + hi_val;

        if (lo_val <= hi_total) {
            f = lo;
        } else {
            result.push_back(var);
            f = hi;
        }
    }
}

std::vector<bddvar> bddminweightset(bddp f, const std::vector<int>& weights) {
    bddweight_validate(f, weights, "bddminweightset");
    std::unordered_map<bddp, long long> memo;
    if (use_iter_1op(f)) {
        bddminweight_iter(f, weights, memo);
    } else {
        bddminweight_rec(f, weights, memo);
    }
    memo[bddempty] = LLONG_MAX;
    memo[bddsingle] = 0;
    std::vector<bddvar> result;
    bddminweightset_trace(f, weights, memo, result);
    std::sort(result.begin(), result.end());
    return result;
}

static void bddmaxweightset_trace(bddp f, const std::vector<int>& weights,
                                   std::unordered_map<bddp, long long>& memo,
                                   std::vector<bddvar>& result) {
    while (f != bddempty && f != bddsingle) {
        bool comp = (f & BDD_COMP_FLAG) != 0;
        bddp f_raw = f & ~BDD_COMP_FLAG;

        bddvar var = node_var(f_raw);
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);
        if (comp) lo = bddnot(lo);

        long long lo_val = memo[lo];
        long long hi_val = memo[hi];
        long long hi_total = static_cast<long long>(weights[var]) + hi_val;

        if (lo_val >= hi_total) {
            f = lo;
        } else {
            result.push_back(var);
            f = hi;
        }
    }
}

std::vector<bddvar> bddmaxweightset(bddp f, const std::vector<int>& weights) {
    bddweight_validate(f, weights, "bddmaxweightset");
    std::unordered_map<bddp, long long> memo;
    if (use_iter_1op(f)) {
        bddmaxweight_iter(f, weights, memo);
    } else {
        bddmaxweight_rec(f, weights, memo);
    }
    memo[bddempty] = LLONG_MIN;
    memo[bddsingle] = 0;
    std::vector<bddvar> result;
    bddmaxweightset_trace(f, weights, memo, result);
    std::sort(result.begin(), result.end());
    return result;
}

// --- BkTrk-IntervalMemo (cost-bound) ---

struct CostBoundResult {
    bddp h;
    long long aw;
    long long rb;
};

static long long costbound_safe_add(long long a, long long b) {
    if (a == LLONG_MIN || b == LLONG_MIN) return LLONG_MIN;
    if (a == LLONG_MAX || b == LLONG_MAX) return LLONG_MAX;
    if (b > 0 && a > LLONG_MAX - b) return LLONG_MAX;
    if (b < 0 && a < LLONG_MIN - b) return LLONG_MIN;
    return a + b;
}

static CostBoundResult bddcostbound_rec(
    bddp f, const std::vector<int>& weights, long long b,
    CostBoundMemo& memo)
{
    BDD_RecurGuard guard;

    // Terminal cases
    if (f == bddempty) {
        return {bddempty, LLONG_MIN, LLONG_MAX};
    }
    if (f == bddsingle) {
        if (b >= 0) return {bddsingle, 0, LLONG_MAX};
        else        return {bddempty, LLONG_MIN, 0};
    }

    // Interval-memo lookup
    bddp cached_h;
    long long cached_aw, cached_rb;
    if (memo.lookup(f, b, cached_h, cached_aw, cached_rb)) {
        return {cached_h, cached_aw, cached_rb};
    }

    // Decompose f = <x, f0, f1> with ZDD complement edge handling
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;
    bddvar var = node_var(f_raw);
    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);
    if (comp) lo = bddnot(lo);  // ZDD: only lo is toggled

    // Recurse on children
    long long cx = static_cast<long long>(weights[var]);
    CostBoundResult r0 = bddcostbound_rec(lo, weights, b, memo);
    CostBoundResult r1 = bddcostbound_rec(hi, weights, b - cx, memo);

    // Construct result node
    bddp h = ZDD::getnode_raw(var, r0.h, r1.h);

    // Compute interval bounds
    long long aw = std::max(r0.aw, costbound_safe_add(r1.aw, cx));
    long long rb = std::min(r0.rb, costbound_safe_add(r1.rb, cx));

    // Store in interval-memo
    memo.insert(f, aw, rb, h);
    return {h, aw, rb};
}

static void bddcostbound_le_validate(bddp f, const std::vector<int>& weights,
                                      CostBoundMemo& memo, const char* name) {
    bddp_validate(f, name);
    if (f == bddnull) {
        throw std::invalid_argument(std::string(name) + ": null ZDD");
    }
    if (weights.size() <= static_cast<size_t>(bddvarused())) {
        throw std::invalid_argument(
            std::string(name) + ": weights.size() must be > bddvarused()");
    }
    memo.bind_weights(weights);
}

bddp bddcostbound_le(bddp f, const std::vector<int>& weights, long long b,
                      CostBoundMemo& memo) {
    bddcostbound_le_validate(f, weights, memo, "bddcostbound_le");

    // Terminal fast-paths
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return b >= 0 ? bddsingle : bddempty;

    if (use_iter_1op(f)) {
        return bdd_gc_guard([&]() -> bddp {
            memo.invalidate_if_stale();
            return bddcostbound_iter(f, weights, b, memo);
        });
    }
    return bdd_gc_guard([&]() -> bddp {
        // Invalidate memo if GC ran (e.g. at bdd_gc_guard entry)
        memo.invalidate_if_stale();
        return bddcostbound_rec(f, weights, b, memo).h;
    });
}

bddp bddcostbound_le(bddp f, const std::vector<int>& weights, long long b,
                      CostBoundMemo& memo, BddExecMode mode) {
    bddcostbound_le_validate(f, weights, memo, "bddcostbound_le");

    if (f == bddempty) return bddempty;
    if (f == bddsingle) return b >= 0 ? bddsingle : bddempty;

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp {
            memo.invalidate_if_stale();
            return bddcostbound_iter(f, weights, b, memo);
        });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp {
            memo.invalidate_if_stale();
            return bddcostbound_rec(f, weights, b, memo).h;
        });
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) {
            return bdd_gc_guard([&]() -> bddp {
                memo.invalidate_if_stale();
                return bddcostbound_iter(f, weights, b, memo);
            });
        }
        return bdd_gc_guard([&]() -> bddp {
            memo.invalidate_if_stale();
            return bddcostbound_rec(f, weights, b, memo).h;
        });
    }
}

static std::vector<int> costbound_negate_weights(const std::vector<int>& weights) {
    std::vector<int> neg(weights.size());
    for (size_t i = 0; i < weights.size(); ++i) {
        if (weights[i] == INT_MIN) {
            throw std::invalid_argument(
                "bddcostbound_ge: weight value INT_MIN is not supported");
        }
        neg[i] = -weights[i];
    }
    return neg;
}

bddp bddcostbound_ge(bddp f, const std::vector<int>& weights, long long b,
                      CostBoundMemo& memo) {
    if (b == LLONG_MIN) {
        throw std::invalid_argument(
            "bddcostbound_ge: bound value LLONG_MIN is not supported");
    }
    std::vector<int> neg = costbound_negate_weights(weights);
    // Use a local memo for the negated-weight call to avoid conflicting
    // with the user's memo which is bound to the original weights.
    CostBoundMemo local_memo;
    (void)memo;  // user memo is not usable for negated-weight le call
    return bddcostbound_le(f, neg, -b, local_memo);
}

bddp bddcostbound_ge(bddp f, const std::vector<int>& weights, long long b,
                      CostBoundMemo& memo, BddExecMode mode) {
    if (b == LLONG_MIN) {
        throw std::invalid_argument(
            "bddcostbound_ge: bound value LLONG_MIN is not supported");
    }
    std::vector<int> neg = costbound_negate_weights(weights);
    CostBoundMemo local_memo;
    (void)memo;
    return bddcostbound_le(f, neg, -b, local_memo, mode);
}

// ---------------------------------------------------------------
// get_k_lightest / get_k_heaviest
// ---------------------------------------------------------------

bddp bddgetklightest(bddp f, const bigint::BigInt& k,
                      const std::vector<int>& weights, int strict) {
    bddp_validate(f, "bddgetklightest");
    if (f == bddnull) {
        throw std::invalid_argument("bddgetklightest: null ZDD");
    }
    if (k < bigint::BigInt(0)) {
        throw std::invalid_argument("bddgetklightest: k must be >= 0");
    }
    if (weights.size() <= static_cast<size_t>(bddvarused())) {
        throw std::invalid_argument(
            "bddgetklightest: weights.size() must be > bddvarused()");
    }

    // Terminal fast-paths
    if (k.is_zero() || f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;

    CountMemoMap count_memo;
    bigint::BigInt total = bddexactcount(f, count_memo);
    if (k >= total) return f;

    return bdd_gc_guard([&]() -> bddp {
        CostBoundMemo cost_memo;
        // Re-populate count_memo if GC caused a retry
        count_memo.clear();
        bigint::BigInt total_inner = bddexactcount(f, count_memo);

        long long lo_bound = bddminweight(f, weights);
        // Guard against LLONG_MIN underflow
        if (lo_bound > LLONG_MIN) {
            lo_bound -= 1;
        }
        long long hi_bound = bddmaxweight(f, weights);

        bddp left_zbdd = bddempty;
        bigint::BigInt left_card(0);
        bddp right_zbdd = f;

        // Binary search on cost bound
        while (hi_bound - lo_bound > 1) {
            long long mid = lo_bound + (hi_bound - lo_bound) / 2;
            bddp mid_zbdd = bddcostbound_le(f, weights, mid, cost_memo);
            bigint::BigInt mid_card = bddexactcount(mid_zbdd, count_memo);

            if (mid_card == k) {
                return mid_zbdd;  // exact match
            } else if (mid_card < k) {
                lo_bound = mid;
                left_zbdd = mid_zbdd;
                left_card = mid_card;
            } else {
                hi_bound = mid;
                right_zbdd = mid_zbdd;
            }
        }

        // hi_bound - lo_bound == 1
        if (strict < 0) {
            return left_zbdd;
        } else if (strict > 0) {
            return right_zbdd;
        } else {
            // strict == 0: exactly k sets
            bddp delta = bddsubtract(right_zbdd, left_zbdd);
            bigint::BigInt need = k - left_card;
            bddp delta_first = bddgetksets(delta, need, count_memo);
            return bddunion(left_zbdd, delta_first);
        }
    });
}

bddp bddgetklightest(bddp f, int64_t k,
                      const std::vector<int>& weights, int strict) {
    if (k < 0) {
        throw std::invalid_argument("bddgetklightest: k must be >= 0");
    }
    return bddgetklightest(f, bigint::BigInt(k), weights, strict);
}

bddp bddgetkheaviest(bddp f, const bigint::BigInt& k,
                      const std::vector<int>& weights, int strict) {
    bddp_validate(f, "bddgetkheaviest");
    if (f == bddnull) {
        throw std::invalid_argument("bddgetkheaviest: null ZDD");
    }
    if (k < bigint::BigInt(0)) {
        throw std::invalid_argument("bddgetkheaviest: k must be >= 0");
    }
    if (weights.size() <= static_cast<size_t>(bddvarused())) {
        throw std::invalid_argument(
            "bddgetkheaviest: weights.size() must be > bddvarused()");
    }

    // Terminal fast-paths
    if (k.is_zero() || f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;

    CountMemoMap count_memo;
    bigint::BigInt total = bddexactcount(f, count_memo);
    if (k >= total) return f;

    bigint::BigInt complement_k = total - k;
    return bdd_gc_guard([&]() -> bddp {
        bddp lightest = bddgetklightest(f, complement_k, weights, -strict);
        return bddsubtract(f, lightest);
    });
}

bddp bddgetkheaviest(bddp f, int64_t k,
                      const std::vector<int>& weights, int strict) {
    if (k < 0) {
        throw std::invalid_argument("bddgetkheaviest: k must be >= 0");
    }
    return bddgetkheaviest(f, bigint::BigInt(k), weights, strict);
}

} // namespace kyotodd
