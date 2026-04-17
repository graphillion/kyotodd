#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

namespace kyotodd {


// ---------------------------------------------------------------
// Rank / Unrank
// ---------------------------------------------------------------

bigint::BigInt bddexactrank(bddp f, const std::vector<bddvar>& s,
                            CountMemoMap& memo) {
    bddp_validate(f, "bddexactrank");
    if (f == bddnull) {
        throw std::invalid_argument("bddexactrank: null ZDD");
    }

    // Normalize input: sort by level descending, deduplicate
    std::vector<bddvar> sorted_s(s);
    std::sort(sorted_s.begin(), sorted_s.end());
    sorted_s.erase(std::unique(sorted_s.begin(), sorted_s.end()),
                   sorted_s.end());
    // Sort by level descending (root has highest level)
    std::sort(sorted_s.begin(), sorted_s.end(),
              [](bddvar a, bddvar b) { return var2level[a] > var2level[b]; });

    // Terminal cases
    if (f == bddempty) return bigint::BigInt(-1);
    if (f == bddsingle) {
        return sorted_s.empty() ? bigint::BigInt(0) : bigint::BigInt(-1);
    }
    if (sorted_s.empty()) {
        return bddhasempty(f) ? bigint::BigInt(0) : bigint::BigInt(-1);
    }

    bigint::BigInt rank(0);

    // Handle empty set membership
    if (bddhasempty(f)) {
        rank = bigint::BigInt(1);  // ∅ occupies index 0
        f = bddnot(f);            // remove ∅ from family
    }

    size_t s_idx = 0;

    while (!(f & BDD_CONST_FLAG)) {
        bool comp = (f & BDD_COMP_FLAG) != 0;
        bddp f_raw = f & ~BDD_COMP_FLAG;

        bddvar var = node_var(f_raw);
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);
        if (comp) lo = bddnot(lo);  // ZDD complement: lo only

        // Check if any remaining target variables have higher level than
        // the current node. Such variables are zero-suppressed (hi == bddempty),
        // so if s contains them, s is not in the family.
        while (s_idx < sorted_s.size() &&
               var2level[sorted_s[s_idx]] > var2level[var]) {
            return bigint::BigInt(-1);
        }

        if (s_idx < sorted_s.size() && sorted_s[s_idx] == var) {
            // var is in s: follow hi-edge
            s_idx++;
            f = hi;
        } else {
            // var is not in s: follow lo-edge, add count of hi subtree
            rank += bddexactcount_rec(hi, memo);
            f = lo;
        }
    }

    // Loop ended at a terminal
    if (f == bddsingle && s_idx == sorted_s.size()) {
        return rank;
    }
    return bigint::BigInt(-1);
}

bigint::BigInt bddexactrank(bddp f, const std::vector<bddvar>& s) {
    CountMemoMap memo;
    return bddexactrank(f, s, memo);
}

int64_t bddrank(bddp f, const std::vector<bddvar>& s) {
    CountMemoMap memo;
    bigint::BigInt result = bddexactrank(f, s, memo);
    if (result == bigint::BigInt(-1)) return -1;
    if (result > bigint::BigInt(INT64_MAX)) {
        throw std::overflow_error(
            "bddrank: rank exceeds int64_t range; use bddexactrank instead");
    }
    return std::stoll(result.to_string());
}

std::vector<bddvar> bddexactunrank(bddp f, const bigint::BigInt& order,
                                   CountMemoMap& memo) {
    bddp_validate(f, "bddexactunrank");
    if (f == bddnull) {
        throw std::invalid_argument("bddexactunrank: null ZDD");
    }

    bigint::BigInt total = bddexactcount_rec(f, memo);
    if (order < bigint::BigInt(0) || order >= total) {
        throw std::out_of_range("bddexactunrank: order out of range");
    }

    std::vector<bddvar> result;
    bigint::BigInt remaining = order;

    // Handle empty set membership
    if (bddhasempty(f)) {
        if (remaining.is_zero()) {
            return result;  // ∅
        }
        remaining -= bigint::BigInt(1);
        f = bddnot(f);  // remove ∅ from family
    }

    while (!(f & BDD_CONST_FLAG)) {
        bool comp = (f & BDD_COMP_FLAG) != 0;
        bddp f_raw = f & ~BDD_COMP_FLAG;

        bddvar var = node_var(f_raw);
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);
        if (comp) lo = bddnot(lo);

        bigint::BigInt count_hi = bddexactcount_rec(hi, memo);
        if (remaining < count_hi) {
            result.push_back(var);  // select var, follow hi-edge
            f = hi;
        } else {
            remaining -= count_hi;  // follow lo-edge
            f = lo;
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

std::vector<bddvar> bddexactunrank(bddp f, const bigint::BigInt& order) {
    CountMemoMap memo;
    return bddexactunrank(f, order, memo);
}

std::vector<bddvar> bddunrank(bddp f, int64_t order) {
    CountMemoMap memo;
    return bddexactunrank(f, bigint::BigInt(order), memo);
}

// ---------------------------------------------------------------
// get_k_sets
// ---------------------------------------------------------------

// Core recursive function: pure hi-before-lo ordering, no ∅ special handling.
// ∅ is handled only at the outermost level (in the public wrapper).
static bddp bddgetksets_core(bddp f, const bigint::BigInt& k,
                              CountMemoMap& memo) {
    BDD_RecurGuard guard;

    // Base cases
    if (k.is_zero() || f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;

    // k >= count(f) → return f unchanged
    bigint::BigInt total = bddexactcount_rec(f, memo);
    if (k >= total) return f;

    // Decompose with ZDD complement edge handling
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddvar var = node_var(f_raw);
    bddp raw_lo = node_lo(f_raw);
    bddp f_hi = node_hi(f_raw);
    bddp f_lo = comp ? bddnot(raw_lo) : raw_lo;  // ZDD complement: lo only

    // Count hi-branch sets (hi first in structure order)
    bigint::BigInt card_hi = bddexactcount_rec(f_hi, memo);

    if (k <= card_hi) {
        // Take only from hi
        bddp g_hi = bddgetksets_core(f_hi, k, memo);
        return ZDD::getnode_raw(var, bddempty, g_hi);
    } else {
        // Take all hi + (k - card_hi) sets from lo
        bddp g_lo = bddgetksets_core(f_lo, k - card_hi, memo);
        return ZDD::getnode_raw(var, g_lo, f_hi);
    }
}

bddp bddgetksets(bddp f, const bigint::BigInt& k, CountMemoMap& memo) {
    bddp_validate(f, "bddgetksets");
    if (f == bddnull) {
        throw std::invalid_argument("bddgetksets: null ZDD");
    }
    if (k < bigint::BigInt(0)) {
        throw std::invalid_argument("bddgetksets: k must be >= 0");
    }

    // Terminal fast-paths
    if (k.is_zero() || f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;

    return bdd_gc_guard([&]() -> bddp {
        bigint::BigInt total = bddexactcount_rec(f, memo);
        if (k >= total) return f;

        // ∅ gets index 0 in structure order (handled only at top level)
        if (bddhasempty(f)) {
            if (k == bigint::BigInt(1)) return bddsingle;
            bddp g = bddgetksets_core(bddnot(f), k - bigint::BigInt(1), memo);
            return bddnot(g);
        }

        return bddgetksets_core(f, k, memo);
    });
}

bddp bddgetksets(bddp f, const bigint::BigInt& k) {
    CountMemoMap memo;
    return bddgetksets(f, k, memo);
}

bddp bddgetksets(bddp f, int64_t k) {
    if (k < 0) {
        throw std::invalid_argument("bddgetksets: k must be >= 0");
    }
    return bddgetksets(f, bigint::BigInt(k));
}

// ============================================================
// supersets_of
// ============================================================

struct BddpIdxPairHash {
    std::size_t operator()(const std::pair<bddp, size_t>& p) const {
        std::size_t h = std::hash<bddp>()(p.first);
        h ^= std::hash<size_t>()(p.second) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};
typedef std::unordered_map<std::pair<bddp, size_t>, bddp, BddpIdxPairHash> BddpIdxMemo;

static bddp bddsupersets_of_rec(
    bddp f, size_t idx,
    const std::vector<bddvar>& ss, BddpIdxMemo& memo) {
    if (f == bddempty) return bddempty;
    if (idx == ss.size()) return f;
    if (f == bddsingle) return bddempty;

    BDD_RecurGuard guard;

    auto key = std::make_pair(f, idx);
    auto it = memo.find(key);
    if (it != memo.end()) return it->second;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddvar f_var = node_var(f_raw);
    bddvar f_level = var2level[f_var];
    bddvar s_var = ss[idx];
    bddvar s_level = var2level[s_var];

    bddp f_lo = node_lo(f_raw);
    bddp f_hi = node_hi(f_raw);
    if (comp) f_lo = bddnot(f_lo);

    bddp result;

    if (s_level > f_level) {
        result = bddempty;
    } else if (s_level == f_level) {
        bddp hi = bddsupersets_of_rec(f_hi, idx + 1, ss, memo);
        result = ZDD::getnode_raw(f_var, bddempty, hi);
    } else {
        bddp lo = bddsupersets_of_rec(f_lo, idx, ss, memo);
        bddp hi = bddsupersets_of_rec(f_hi, idx, ss, memo);
        result = ZDD::getnode_raw(f_var, lo, hi);
    }

    memo[key] = result;
    return result;
}

bddp bddsupersets_of(bddp f, const std::vector<bddvar>& s) {
    bddp_validate(f, "bddsupersets_of");
    if (f == bddnull) return bddnull;
    if (f == bddempty) return bddempty;
    if (s.empty()) return f;

    std::vector<bddvar> sorted_s(s);
    std::sort(sorted_s.begin(), sorted_s.end());
    sorted_s.erase(std::unique(sorted_s.begin(), sorted_s.end()),
                   sorted_s.end());
    for (bddvar v : sorted_s) {
        if (v < 1 || v > bdd_varcount)
            throw std::invalid_argument("bddsupersets_of: variable out of range");
    }
    std::sort(sorted_s.begin(), sorted_s.end(),
              [](bddvar a, bddvar b) { return var2level[a] > var2level[b]; });

    return bdd_gc_guard([&]() -> bddp {
        BddpIdxMemo memo;
        return bddsupersets_of_rec(f, 0, sorted_s, memo);
    });
}

// ============================================================
// subsets_of
// ============================================================

static bddp bddsubsets_of_rec(
    bddp f, size_t idx,
    const std::vector<bddvar>& ss, BddpIdxMemo& memo) {
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;
    if (idx == ss.size()) {
        return bddhasempty(f) ? bddsingle : bddempty;
    }

    BDD_RecurGuard guard;

    auto key = std::make_pair(f, idx);
    auto it = memo.find(key);
    if (it != memo.end()) return it->second;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddvar f_var = node_var(f_raw);
    bddvar f_level = var2level[f_var];
    bddvar s_var = ss[idx];
    bddvar s_level = var2level[s_var];

    bddp f_lo = node_lo(f_raw);
    bddp f_hi = node_hi(f_raw);
    if (comp) f_lo = bddnot(f_lo);

    bddp result;

    if (s_level > f_level) {
        result = bddsubsets_of_rec(f, idx + 1, ss, memo);
    } else if (s_level == f_level) {
        bddp lo = bddsubsets_of_rec(f_lo, idx + 1, ss, memo);
        bddp hi = bddsubsets_of_rec(f_hi, idx + 1, ss, memo);
        result = ZDD::getnode_raw(f_var, lo, hi);
    } else {
        result = bddsubsets_of_rec(f_lo, idx, ss, memo);
    }

    memo[key] = result;
    return result;
}

bddp bddsubsets_of(bddp f, const std::vector<bddvar>& s) {
    bddp_validate(f, "bddsubsets_of");
    if (f == bddnull) return bddnull;
    if (f == bddempty) return bddempty;

    std::vector<bddvar> sorted_s(s);
    std::sort(sorted_s.begin(), sorted_s.end());
    sorted_s.erase(std::unique(sorted_s.begin(), sorted_s.end()),
                   sorted_s.end());
    for (bddvar v : sorted_s) {
        if (v < 1 || v > bdd_varcount)
            throw std::invalid_argument("bddsubsets_of: variable out of range");
    }
    std::sort(sorted_s.begin(), sorted_s.end(),
              [](bddvar a, bddvar b) { return var2level[a] > var2level[b]; });

    return bdd_gc_guard([&]() -> bddp {
        BddpIdxMemo memo;
        return bddsubsets_of_rec(f, 0, sorted_s, memo);
    });
}

// ============================================================
// project
// ============================================================

static bddp bddproject_rec(
    bddp f, const std::unordered_set<bddvar>& proj_vars,
    std::unordered_map<bddp, bddp>& memo) {
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;

    BDD_RecurGuard guard;

    auto it = memo.find(f);
    if (it != memo.end()) return it->second;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddvar f_var = node_var(f_raw);
    bddp f_lo = node_lo(f_raw);
    bddp f_hi = node_hi(f_raw);
    if (comp) f_lo = bddnot(f_lo);

    bddp result;

    if (proj_vars.count(f_var)) {
        bddp lo_proj = bddproject_rec(f_lo, proj_vars, memo);
        bddp hi_proj = bddproject_rec(f_hi, proj_vars, memo);
        result = bddunion(lo_proj, hi_proj);
    } else {
        bddp lo_proj = bddproject_rec(f_lo, proj_vars, memo);
        bddp hi_proj = bddproject_rec(f_hi, proj_vars, memo);
        result = ZDD::getnode_raw(f_var, lo_proj, hi_proj);
    }

    memo[f] = result;
    return result;
}

bddp bddproject(bddp f, const std::vector<bddvar>& vars) {
    bddp_validate(f, "bddproject");
    if (f == bddnull) return bddnull;
    if (f == bddempty) return bddempty;
    if (vars.empty()) return f;

    for (bddvar v : vars) {
        if (v < 1 || v > bdd_varcount)
            throw std::invalid_argument("bddproject: variable out of range");
    }

    std::unordered_set<bddvar> var_set(vars.begin(), vars.end());
    return bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        return bddproject_rec(f, var_set, memo);
    });
}

ZDD ZDD_LCM_A(char* /*filename*/, int /*threshold*/) {
    throw std::logic_error("ZDD_LCM_A: not implemented");
}

ZDD ZDD_LCM_C(char* /*filename*/, int /*threshold*/) {
    throw std::logic_error("ZDD_LCM_C: not implemented");
}

ZDD ZDD_LCM_M(char* /*filename*/, int /*threshold*/) {
    throw std::logic_error("ZDD_LCM_M: not implemented");
}

} // namespace kyotodd
