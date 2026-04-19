#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <unordered_map>
#include <unordered_set>

namespace kyotodd {


// Check if ∅ ∈ F (the empty set is a member of the ZDD family)
bool bddhasempty(bddp f) {
    if (f == bddnull) return false;
    while (true) {
        if (f == bddempty) return false;
        if (f == bddsingle) return true;
        bool comp = (f & BDD_COMP_FLAG) != 0;
        f = node_lo(f);
        if (comp) f = bddnot(f);
    }
}

// Check if set s is a member of the ZDD family
bool bddcontains(bddp f, const std::vector<bddvar>& s) {
    bddp_validate(f, "bddcontains");
    if (f == bddnull) return false;

    // Sort by level descending, deduplicate
    std::vector<bddvar> sorted_s(s);
    std::sort(sorted_s.begin(), sorted_s.end());
    sorted_s.erase(std::unique(sorted_s.begin(), sorted_s.end()),
                   sorted_s.end());
    std::sort(sorted_s.begin(), sorted_s.end(),
              [](bddvar a, bddvar b) {
                  return var2level[a] > var2level[b];
              });

    size_t s_idx = 0;

    while (!(f & BDD_CONST_FLAG)) {
        bool comp = (f & BDD_COMP_FLAG) != 0;
        bddp f_raw = f & ~BDD_COMP_FLAG;

        bddvar var = node_var(f_raw);
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);
        if (comp) lo = bddnot(lo);  // ZDD complement: lo only

        // Variables in s with higher level than current node are
        // zero-suppressed (hi == bddempty), so s is not in the family.
        if (s_idx < sorted_s.size() &&
            var2level[sorted_s[s_idx]] > var2level[var]) {
            return false;
        }

        if (s_idx < sorted_s.size() && sorted_s[s_idx] == var) {
            s_idx++;
            f = hi;  // var is in s: follow hi-edge
        } else {
            f = lo;  // var is not in s: follow lo-edge
        }
    }

    // At terminal: success iff we consumed all elements and reached bddsingle
    return (f == bddsingle && s_idx == sorted_s.size());
}

// Filter to sets of exactly k elements
static bddp bddchoose_rec(bddp f, int k);

bddp bddchoose(bddp f, int k) {
    bddp_validate(f, "bddchoose");
    if (f == bddnull) return bddnull;
    if (f == bddempty) return bddempty;
    if (k < 0) return bddempty;
    if (f == bddsingle) return (k == 0) ? bddsingle : bddempty;

    // Complement toggles ∅ membership only (size 0).
    // For k > 0, complement has no effect. For k == 0, resolve directly.
    bool comp = (f & BDD_COMP_FLAG) != 0;
    if (comp) {
        bddp f_raw = f & ~BDD_COMP_FLAG;
        if (k == 0) {
            return bddhasempty(f_raw) ? bddempty : bddsingle;
        }
        f = f_raw;
    } else if (k == 0) {
        return bddhasempty(f) ? bddsingle : bddempty;
    }

    // k > 0, f has no complement bit
    if (use_iter_1op(f)) {
        return bdd_gc_guard([&]() -> bddp { return bddchoose_iter(f, k); });
    }
    return bdd_gc_guard([&]() -> bddp { return bddchoose_rec(f, k); });
}

bddp bddchoose(bddp f, int k, BddExecMode mode) {
    bddp_validate(f, "bddchoose");
    if (f == bddnull) return bddnull;
    if (f == bddempty) return bddempty;
    if (k < 0) return bddempty;
    if (f == bddsingle) return (k == 0) ? bddsingle : bddempty;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    if (comp) {
        bddp f_raw = f & ~BDD_COMP_FLAG;
        if (k == 0) {
            return bddhasempty(f_raw) ? bddempty : bddsingle;
        }
        f = f_raw;
    } else if (k == 0) {
        return bddhasempty(f) ? bddsingle : bddempty;
    }

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bddchoose_iter(f, k); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bddchoose_rec(f, k); });
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) {
            return bdd_gc_guard([&]() -> bddp { return bddchoose_iter(f, k); });
        }
        return bdd_gc_guard([&]() -> bddp { return bddchoose_rec(f, k); });
    }
}

static bddp bddchoose_rec(bddp f, int k) {
    BDD_RecurGuard guard;

    if (f == bddempty) return bddempty;
    if (k < 0) return bddempty;
    if (f == bddsingle) return (k == 0) ? bddsingle : bddempty;
    if (k == 0) return bddhasempty(f) ? bddsingle : bddempty;

    // For k > 0, complement only affects ∅ (size 0), so strip it.
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddp cached = bddrcache(BDD_OP_CHOOSE, f_raw, static_cast<bddp>(k));
    if (cached != bddnull) return cached;

    bddvar var = node_var(f_raw);
    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);
    if (comp) lo = bddnot(lo);  // ZDD complement: lo only

    bddp r0 = bddchoose_rec(lo, k);      // sets not containing var
    bddp r1 = bddchoose_rec(hi, k - 1);  // sets containing var

    bddp result = ZDD::getnode_raw(var, r0, r1);

    bddwcache(BDD_OP_CHOOSE, f_raw, static_cast<bddp>(k), result);
    return result;
}

// Set size distribution (profile / f-vector)
typedef std::unordered_map<bddp, std::vector<bigint::BigInt>> ProfileMemoMap;

static std::vector<bigint::BigInt> bddprofile_rec(
    bddp f, ProfileMemoMap& memo) {
    if (f == bddempty) return {};
    if (f == bddsingle) return {bigint::BigInt(1)};

    BDD_RecurGuard guard;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    auto it = memo.find(f_raw);
    std::vector<bigint::BigInt> result;
    if (it != memo.end()) {
        result = it->second;
    } else {
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);

        auto prof_lo = bddprofile_rec(lo, memo);
        auto prof_hi = bddprofile_rec(hi, memo);

        // hi-result shifted by +1 (each set in hi includes the current variable)
        size_t max_len = std::max(prof_lo.size(), prof_hi.size() + 1);
        result.resize(max_len);

        for (size_t i = 0; i < prof_lo.size(); i++) {
            result[i] += prof_lo[i];
        }
        for (size_t i = 0; i < prof_hi.size(); i++) {
            result[i + 1] += prof_hi[i];
        }

        memo[f_raw] = result;
    }

    // Complement toggles empty set membership → adjust profile[0]
    if (comp) {
        if (result.empty()) {
            result.push_back(bigint::BigInt(1));
        } else if (bddhasempty(f_raw)) {
            result[0] -= bigint::BigInt(1);
            while (!result.empty() && result.back().is_zero()) result.pop_back();
        } else {
            result[0] += bigint::BigInt(1);
        }
    }

    return result;
}

std::vector<bigint::BigInt> bddprofile(bddp f) {
    bddp_validate(f, "bddprofile");
    if (f == bddnull) return {};
    ProfileMemoMap memo;
    if (use_iter_1op(f)) {
        return bddprofile_iter(f, memo);
    }
    return bddprofile_rec(f, memo);
}

std::vector<bigint::BigInt> bddprofile(bddp f, BddExecMode mode) {
    bddp_validate(f, "bddprofile");
    if (f == bddnull) return {};
    ProfileMemoMap memo;
    switch (mode) {
    case BddExecMode::Iterative:
        return bddprofile_iter(f, memo);
    case BddExecMode::Recursive:
        return bddprofile_rec(f, memo);
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) return bddprofile_iter(f, memo);
        return bddprofile_rec(f, memo);
    }
}

std::vector<double> bddprofile_double(bddp f) {
    bddp_validate(f, "bddprofile_double");
    if (f == bddnull) return {};
    auto bigint_result = bddprofile(f);
    std::vector<double> result;
    result.reserve(bigint_result.size());
    for (auto& v : bigint_result) {
        result.push_back(bigint_to_double(v));
    }
    return result;
}

// ---- element frequency ----

typedef std::unordered_map<bddp, std::vector<bigint::BigInt>> ElmFreqMemoMap;

static std::vector<bigint::BigInt> bddelmfreq_rec(
    bddp f, ElmFreqMemoMap& freq_memo, CountMemoMap& count_memo) {
    // Terminal: no variables → empty frequency
    if (f == bddempty || f == bddsingle) return {};

    BDD_RecurGuard guard;

    // Complement doesn't affect element frequency
    // (only toggles empty set membership, ∅ has no elements)
    bddp f_raw = f & ~BDD_COMP_FLAG;

    auto it = freq_memo.find(f_raw);
    if (it != freq_memo.end()) return it->second;

    bddvar var = node_var(f_raw);
    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);

    auto freq_lo = bddelmfreq_rec(lo, freq_memo, count_memo);
    auto freq_hi = bddelmfreq_rec(hi, freq_memo, count_memo);

    // Result vector: needs to cover variable var at minimum
    size_t result_size = static_cast<size_t>(var) + 1;
    if (freq_lo.size() > result_size) result_size = freq_lo.size();
    if (freq_hi.size() > result_size) result_size = freq_hi.size();

    std::vector<bigint::BigInt> result(result_size);

    for (size_t i = 0; i < freq_lo.size(); i++) {
        result[i] += freq_lo[i];
    }
    for (size_t i = 0; i < freq_hi.size(); i++) {
        result[i] += freq_hi[i];
    }

    // Variable var: every set in hi subtree contains var
    result[var] += bddexactcount(hi, count_memo);

    freq_memo[f_raw] = result;
    return result;
}

std::vector<bigint::BigInt> bddelmfreq(bddp f) {
    bddp_validate(f, "bddelmfreq");
    if (f == bddnull) return {};
    ElmFreqMemoMap freq_memo;
    CountMemoMap count_memo;
    if (use_iter_1op(f)) {
        return bddelmfreq_iter(f, freq_memo, count_memo);
    }
    return bddelmfreq_rec(f, freq_memo, count_memo);
}

std::vector<bigint::BigInt> bddelmfreq(bddp f, BddExecMode mode) {
    bddp_validate(f, "bddelmfreq");
    if (f == bddnull) return {};
    ElmFreqMemoMap freq_memo;
    CountMemoMap count_memo;
    switch (mode) {
    case BddExecMode::Iterative:
        return bddelmfreq_iter(f, freq_memo, count_memo);
    case BddExecMode::Recursive:
        return bddelmfreq_rec(f, freq_memo, count_memo);
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) return bddelmfreq_iter(f, freq_memo, count_memo);
        return bddelmfreq_rec(f, freq_memo, count_memo);
    }
}

bddp bddflatten(bddp f) {
    bddp_validate(f, "bddflatten");
    if (f == bddnull) return bddnull;
    if (f == bddempty) return bddempty;
    auto vars = bddsupport_vec(f);
    if (vars.empty()) return bddsingle;  // non-empty family with no variables → {∅}
    return bdd_gc_guard([&]() -> bddp {
        bddp result = bddsingle;
        for (bddvar v : vars) {
            result = bddchange(result, v);
        }
        return result;
    });
}

bddp bddcoalesce(bddp f, bddvar v1, bddvar v2) {
    bddp_validate(f, "bddcoalesce");
    if (f == bddnull) return bddnull;
    if (f == bddempty) return bddempty;
    // Terminal: {∅} has no variables → unchanged
    if (f == bddsingle) return bddsingle;
    if (v1 == v2) return f;
    if (v1 < 1 || v1 > bdd_varcount) {
        throw std::invalid_argument("bddcoalesce: v1 out of range");
    }
    if (v2 < 1 || v2 > bdd_varcount) {
        throw std::invalid_argument("bddcoalesce: v2 out of range");
    }

    return bdd_gc_guard([&]() -> bddp {
        // A = sets not containing v2 (unchanged)
        bddp a = bddoffset(f, v2);
        // B = sets containing v2, with v2 removed
        bddp b = bddonset0(f, v2);

        // Ensure v1 is present in every set of B:
        // - sets already having v1: keep as is
        // - sets without v1: add v1
        bddp b_has_v1 = bddonset(b, v1);
        bddp b_no_v1 = bddoffset(b, v1);
        bddp b_merged = bddunion(b_has_v1, bddchange(b_no_v1, v1));

        return bddunion(a, b_merged);
    });
}

// ---- cardinality ----

// NOTE: bddcard is obsolete. Use bddcount (double) or bddexactcount (BigInt)
// instead. No _iter version is provided for bddcard; deep ZDDs should be
// counted via those functions, which have matching _iter implementations.

static const uint64_t BDDCARD_MAX = (UINT64_C(1) << 39) - 1;

uint64_t bddcard(bddp f) {
    bddp_validate(f, "bddcard");
    if (f == bddnull) return 0;
    // Terminal cases
    if (f == bddempty) return 0;
    if (f == bddsingle) return 1;

    BDD_RecurGuard guard;

    // Handle complement edge (ZDD: complement toggles empty set membership)
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    // Cache lookup: use result field to store the count
    // bddnull (0x7FFFFFFFFFFF) > BDDCARD_MAX (2^39-1), so no collision
    bddp cached = bddrcache(BDD_OP_CARD, f_raw, 0);
    uint64_t count;
    if (cached != bddnull) {
        count = cached;
    } else {
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);

        uint64_t c0 = bddcard(lo);
        uint64_t c1 = bddcard(hi);

        if (c0 > BDDCARD_MAX - c1) {
            count = BDDCARD_MAX;
        } else {
            count = c0 + c1;
        }

        bddwcache(BDD_OP_CARD, f_raw, 0, count);
    }

    // Complement edge toggles empty set: if ∅ was in the family, remove it; if not, add it.
    if (comp) {
        // Check if ∅ is in the family of f_raw by following the lo-chain to terminal
        // Instead, use the relationship: card(~f) = card(f) ± 1
        // ∅ ∈ family(f_raw) iff the lo-only path reaches bddsingle
        // We can determine this: complement flips ∅ membership
        // If ∅ ∈ family(f_raw): card(~f_raw) = count - 1
        // If ∅ ∉ family(f_raw): card(~f_raw) = count + 1
        // To check ∅ membership: follow lo edges to terminal
        // Use bddhasempty which correctly handles complement edges
        if (bddhasempty(f_raw)) {
            // ∅ was in the family, complement removes it
            count = count - 1;
        } else {
            // ∅ was not in the family, complement adds it
            if (count >= BDDCARD_MAX) {
                count = BDDCARD_MAX;
            } else {
                count = count + 1;
            }
        }
    }

    return count;
}

static const uint64_t BDDLIT_MAX = (UINT64_C(1) << 39) - 1;

static uint64_t bddlit_rec(bddp f) {
    if (f == bddnull) return 0;
    // Terminal cases
    if (f == bddempty) return 0;
    if (f == bddsingle) return 0;  // {∅} — empty set has 0 literals

    BDD_RecurGuard guard;

    // Complement edge: toggles ∅ membership. ∅ has 0 literals, so lit is unchanged.
    bddp f_raw = f & ~BDD_COMP_FLAG;

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_LIT, f_raw, 0);
    if (cached != bddnull) {
        return cached;
    }

    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);

    uint64_t l0 = bddlit_rec(lo);
    uint64_t l1 = bddlit_rec(hi);
    uint64_t c1 = bddcard(hi);

    // lit(f) = lit(f0) + lit(f1) + card(f1), with overflow check
    uint64_t sum = l0;
    if (l1 > BDDLIT_MAX - sum) {
        sum = BDDLIT_MAX;
    } else {
        sum += l1;
    }
    if (c1 > BDDLIT_MAX - sum) {
        sum = BDDLIT_MAX;
    } else {
        sum += c1;
    }
    if (sum > BDDLIT_MAX) sum = BDDLIT_MAX;

    bddwcache(BDD_OP_LIT, f_raw, 0, sum);
    return sum;
}

uint64_t bddlit(bddp f) {
    bddp_validate(f, "bddlit");
    if (use_iter_1op(f)) return bddlit_iter(f);
    return bddlit_rec(f);
}

static const uint64_t BDDLEN_MAX = (UINT64_C(1) << 39) - 1;

static uint64_t bddlen_rec(bddp f) {
    if (f == bddnull) return 0;
    if (f == bddempty) return 0;
    if (f == bddsingle) return 0;

    BDD_RecurGuard guard;

    // Complement edge toggles ∅ membership; ∅ has size 0,
    // so max element size is unchanged.
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddp cached = bddrcache(BDD_OP_LEN, f_raw, 0);
    if (cached != bddnull) {
        return cached;
    }

    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);

    uint64_t len0 = bddlen_rec(lo);
    uint64_t len1 = bddlen_rec(hi);

    // len(f1) + 1 with overflow check
    if (len1 >= BDDLEN_MAX) {
        len1 = BDDLEN_MAX;
    } else {
        len1 += 1;
    }

    uint64_t result = len0 > len1 ? len0 : len1;

    bddwcache(BDD_OP_LEN, f_raw, 0, result);
    return result;
}

uint64_t bddlen(bddp f) {
    bddp_validate(f, "bddlen");
    if (use_iter_1op(f)) return bddlen_iter(f);
    return bddlen_rec(f);
}

static uint64_t bddminsize_rec(bddp f) {
    if (f == bddempty) return UINT64_MAX;
    if (f == bddsingle) return 0;

    BDD_RecurGuard guard;

    bddp cached = bddrcache(BDD_OP_MINSIZE, f, 0);
    if (cached != bddnull) {
        return cached;
    }

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);
    if (comp) lo = bddnot(lo);

    uint64_t min0 = bddminsize_rec(lo);
    uint64_t min1 = bddminsize_rec(hi);

    if (min1 != UINT64_MAX) {
        if (min1 >= BDDLEN_MAX) {
            min1 = BDDLEN_MAX;
        } else {
            min1 += 1;
        }
    }

    uint64_t result = (min0 < min1) ? min0 : min1;

    bddwcache(BDD_OP_MINSIZE, f, 0, result);
    return result;
}

uint64_t bddminsize(bddp f) {
    bddp_validate(f, "bddminsize");
    if (f == bddnull) return 0;
    if (f == bddempty) return 0;
    if (f == bddsingle) return 0;

    if (bddhasempty(f)) return 0;

    uint64_t r = use_iter_1op(f) ? bddminsize_iter(f) : bddminsize_rec(f);
    return (r == UINT64_MAX) ? 0 : r;
}

uint64_t bddminsize(bddp f, BddExecMode mode) {
    bddp_validate(f, "bddminsize");
    if (f == bddnull) return 0;
    if (f == bddempty) return 0;
    if (f == bddsingle) return 0;

    if (bddhasempty(f)) return 0;

    uint64_t r;
    switch (mode) {
    case BddExecMode::Iterative: r = bddminsize_iter(f); break;
    case BddExecMode::Recursive: r = bddminsize_rec(f); break;
    case BddExecMode::Auto:
    default:
        r = use_iter_1op(f) ? bddminsize_iter(f) : bddminsize_rec(f);
        break;
    }
    return (r == UINT64_MAX) ? 0 : r;
}

uint64_t bddmaxsize(bddp f) {
    bddp_validate(f, "bddmaxsize");
    return bddlen(f);
}

static double bddcount_rec(
    bddp f, std::unordered_map<bddp, double>& memo) {
    if (f == bddempty) return 0.0;
    if (f == bddsingle) return 1.0;

    BDD_RecurGuard guard;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    auto it = memo.find(f_raw);
    double count;
    if (it != memo.end()) {
        count = it->second;
    } else {
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);

        count = bddcount_rec(lo, memo) + bddcount_rec(hi, memo);

        memo[f_raw] = count;
    }

    if (comp) {
        if (bddhasempty(f_raw)) {
            count -= 1.0;
        } else {
            count += 1.0;
        }
    }

    return count;
}

double bddcount(bddp f) {
    bddp_validate(f, "bddcount");
    if (f == bddnull) return 0.0;
    std::unordered_map<bddp, double> memo;
    if (use_iter_1op(f)) return bddcount_iter(f, memo);
    return bddcount_rec(f, memo);
}

double bddcount(bddp f, BddExecMode mode) {
    bddp_validate(f, "bddcount");
    if (f == bddnull) return 0.0;
    std::unordered_map<bddp, double> memo;
    switch (mode) {
    case BddExecMode::Iterative: return bddcount_iter(f, memo);
    case BddExecMode::Recursive: return bddcount_rec(f, memo);
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) return bddcount_iter(f, memo);
        return bddcount_rec(f, memo);
    }
}

// Non-static: declared in bdd_internal.h for cross-file use
bigint::BigInt bddexactcount_rec(
    bddp f, std::unordered_map<bddp, bigint::BigInt>& memo) {
    // Terminal cases
    if (f == bddempty) return bigint::BigInt(0);
    if (f == bddsingle) return bigint::BigInt(1);

    BDD_RecurGuard guard;

    // Handle complement edge (ZDD: complement toggles empty set membership)
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    // Memo lookup
    auto it = memo.find(f_raw);
    bigint::BigInt count;
    if (it != memo.end()) {
        count = it->second;
    } else {
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);

        bigint::BigInt c0 = bddexactcount_rec(lo, memo);
        bigint::BigInt c1 = bddexactcount_rec(hi, memo);

        count = c0 + c1;

        memo[f_raw] = count;
    }

    // Complement edge toggles empty set membership
    if (comp) {
        if (bddhasempty(f_raw)) {
            // ∅ was in the family, complement removes it
            count -= bigint::BigInt(1);
        } else {
            // ∅ was not in the family, complement adds it
            count += bigint::BigInt(1);
        }
    }

    return count;
}

bigint::BigInt bddexactcount(bddp f) {
    bddp_validate(f, "bddexactcount");
    if (f == bddnull) return bigint::BigInt(0);
    std::unordered_map<bddp, bigint::BigInt> memo;
    if (use_iter_1op(f)) return bddexactcount_iter(f, memo);
    return bddexactcount_rec(f, memo);
}

bigint::BigInt bddexactcount(bddp f, BddExecMode mode) {
    bddp_validate(f, "bddexactcount");
    if (f == bddnull) return bigint::BigInt(0);
    std::unordered_map<bddp, bigint::BigInt> memo;
    switch (mode) {
    case BddExecMode::Iterative: return bddexactcount_iter(f, memo);
    case BddExecMode::Recursive: return bddexactcount_rec(f, memo);
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) return bddexactcount_iter(f, memo);
        return bddexactcount_rec(f, memo);
    }
}

bigint::BigInt bddexactcount(bddp f, CountMemoMap& memo) {
    bddp_validate(f, "bddexactcount");
    if (f == bddnull) return bigint::BigInt(0);
    if (use_iter_1op(f)) return bddexactcount_iter(f, memo);
    return bddexactcount_rec(f, memo);
}

bigint::BigInt bddexactcount(bddp f, CountMemoMap& memo, BddExecMode mode) {
    bddp_validate(f, "bddexactcount");
    if (f == bddnull) return bigint::BigInt(0);
    switch (mode) {
    case BddExecMode::Iterative: return bddexactcount_iter(f, memo);
    case BddExecMode::Recursive: return bddexactcount_rec(f, memo);
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) return bddexactcount_iter(f, memo);
        return bddexactcount_rec(f, memo);
    }
}

// ---------------------------------------------------------------------------
// count_intersec: count |F ∩ G| without building the intersection ZDD
// ---------------------------------------------------------------------------

typedef std::unordered_map<bddp,
    std::unordered_map<bddp, bigint::BigInt>> PairCountMemoMap;

static bigint::BigInt bddcountintersec_rec(
    bddp f, bddp g,
    PairCountMemoMap& pair_memo, CountMemoMap& count_memo) {
    // Terminal cases
    if (f == bddempty || g == bddempty) return bigint::BigInt(0);
    if (f == bddsingle && g == bddsingle) return bigint::BigInt(1);
    if (f == bddsingle) return bddhasempty(g) ? bigint::BigInt(1)
                                              : bigint::BigInt(0);
    if (g == bddsingle) return bddhasempty(f) ? bigint::BigInt(1)
                                              : bigint::BigInt(0);
    if (f == g) return bddexactcount_rec(f, count_memo);

    // Canonical order (intersection is commutative)
    if (f > g) { bddp t = f; f = g; g = t; }

    BDD_RecurGuard guard;

    // Pair memo lookup
    auto it_f = pair_memo.find(f);
    if (it_f != pair_memo.end()) {
        auto it_g = it_f->second.find(g);
        if (it_g != it_f->second.end()) return it_g->second;
    }

    bool f_const = (f & BDD_CONST_FLAG) != 0;
    bool g_const = (g & BDD_CONST_FLAG) != 0;
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = f_const ? 0 : node_var(f);
    bddvar g_var = g_const ? 0 : node_var(g);
    bddvar f_level = f_const ? 0 : var2level[f_var];
    bddvar g_level = g_const ? 0 : var2level[g_var];

    bigint::BigInt result;

    if (f_level > g_level) {
        // g has no sets containing f_var; only f's lo branch can match
        bddp f_lo = node_lo(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        result = bddcountintersec_rec(f_lo, g, pair_memo, count_memo);
    } else if (g_level > f_level) {
        bddp g_lo = node_lo(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddcountintersec_rec(f, g_lo, pair_memo, count_memo);
    } else {
        // Same top variable
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddcountintersec_rec(f_lo, g_lo, pair_memo, count_memo)
               + bddcountintersec_rec(f_hi, g_hi, pair_memo, count_memo);
    }

    pair_memo[f][g] = result;
    return result;
}

bigint::BigInt bddcountintersec(bddp f, bddp g) {
    bddp_validate(f, "bddcountintersec");
    bddp_validate(g, "bddcountintersec");
    if (f == bddnull || g == bddnull) return bigint::BigInt(0);
    if (f == bddempty || g == bddempty) return bigint::BigInt(0);
    if (f == g) return bddexactcount(f);
    PairCountMemoMap pair_memo;
    CountMemoMap count_memo;
    if (use_iter_2op(f, g)) {
        return bddcountintersec_iter(f, g, pair_memo, count_memo);
    }
    return bddcountintersec_rec(f, g, pair_memo, count_memo);
}

bigint::BigInt bddcountintersec(bddp f, bddp g, BddExecMode mode) {
    bddp_validate(f, "bddcountintersec");
    bddp_validate(g, "bddcountintersec");
    if (f == bddnull || g == bddnull) return bigint::BigInt(0);
    if (f == bddempty || g == bddempty) return bigint::BigInt(0);
    if (f == g) return bddexactcount(f, mode);
    PairCountMemoMap pair_memo;
    CountMemoMap count_memo;
    switch (mode) {
    case BddExecMode::Iterative:
        return bddcountintersec_iter(f, g, pair_memo, count_memo);
    case BddExecMode::Recursive:
        return bddcountintersec_rec(f, g, pair_memo, count_memo);
    case BddExecMode::Auto:
    default:
        if (use_iter_2op(f, g)) {
            return bddcountintersec_iter(f, g, pair_memo, count_memo);
        }
        return bddcountintersec_rec(f, g, pair_memo, count_memo);
    }
}

double bddjaccardindex(bddp f, bddp g) {
    bddp_validate(f, "bddjaccardindex");
    bddp_validate(g, "bddjaccardindex");
    if (f == bddnull || g == bddnull) return 0.0;
    if (f == bddempty && g == bddempty) return 1.0;
    if (f == bddempty || g == bddempty) return 0.0;
    if (f == g) return 1.0;

    CountMemoMap count_memo;
    bigint::BigInt count_f = bddexactcount_rec(f, count_memo);
    bigint::BigInt count_g = bddexactcount_rec(g, count_memo);

    PairCountMemoMap pair_memo;
    bigint::BigInt count_fg = bddcountintersec_rec(
        f, g, pair_memo, count_memo);

    bigint::BigInt count_union = count_f + count_g - count_fg;
    if (count_union == bigint::BigInt(0)) return 1.0;

    return bigint_ratio_to_double(count_fg, count_union);
}

bigint::BigInt bddhammingdist(bddp f, bddp g) {
    bddp_validate(f, "bddhammingdist");
    bddp_validate(g, "bddhammingdist");
    if (f == bddnull || g == bddnull) return bigint::BigInt(0);
    if (f == g) return bigint::BigInt(0);
    if (f == bddempty) return bddexactcount(g);
    if (g == bddempty) return bddexactcount(f);

    CountMemoMap count_memo;
    bigint::BigInt count_f = bddexactcount_rec(f, count_memo);
    bigint::BigInt count_g = bddexactcount_rec(g, count_memo);

    PairCountMemoMap pair_memo;
    bigint::BigInt count_fg = bddcountintersec_rec(
        f, g, pair_memo, count_memo);

    // |F △ G| = |F| + |G| - 2|F ∩ G|
    return count_f + count_g - count_fg - count_fg;
}

double bddoverlapcoeff(bddp f, bddp g) {
    bddp_validate(f, "bddoverlapcoeff");
    bddp_validate(g, "bddoverlapcoeff");
    if (f == bddnull || g == bddnull) return 0.0;
    if (f == bddempty && g == bddempty) return 1.0;
    if (f == bddempty || g == bddempty) return 0.0;
    if (f == g) return 1.0;

    CountMemoMap count_memo;
    bigint::BigInt count_f = bddexactcount_rec(f, count_memo);
    bigint::BigInt count_g = bddexactcount_rec(g, count_memo);

    bigint::BigInt min_count = (count_f < count_g) ? count_f : count_g;
    if (min_count == bigint::BigInt(0)) return 0.0;

    PairCountMemoMap pair_memo;
    bigint::BigInt count_fg = bddcountintersec_rec(
        f, g, pair_memo, count_memo);

    return bigint_ratio_to_double(count_fg, min_count);
}

// Legacy compatibility wrapper: returns cardinality as uppercase hex string.
char *bddcardmp16(bddp f, char *s) {
    std::string hex = bddexactcount(f).to_hex_upper();
    if (s == NULL) {
        char *buf = static_cast<char *>(std::malloc(hex.size() + 1));
        if (buf == NULL) return NULL;
        std::memcpy(buf, hex.c_str(), hex.size() + 1);
        return buf;
    }
    std::memcpy(s, hex.c_str(), hex.size() + 1);
    return s;
}

} // namespace kyotodd
