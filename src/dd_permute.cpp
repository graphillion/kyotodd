// Variable substitution (permutation / renaming) for BDD and ZDD.
//
// bddpermute(f, from, to) replaces variable from[i] by to[i] throughout f.
// Both DD types use the same recursion scheme: substitute in the two
// children first, then rebuild the node for the new variable. When the new
// variable's level is above both children the node can be created directly;
// otherwise the order must be restored with ITE (BDD) or with
// union / change (ZDD). This is the same scheme as bddswap / bddswapz, which
// are the two-variable special case, and as CUDD's Cudd_bddPermute.
//
// Non-injective mappings are allowed: f(x1, x2) with {1, 2} -> {3, 3}
// gives f(x3, x3). For ZDDs this means duplicate elements collapse and sets
// that become equal are merged.
//
// Operation-cache key: the cache stores (op, f, g) -> result, so a mapping
// must be reduced to one bddp value. Normalized mappings are interned in a
// global table and numbered consecutively; the number is used as the cache
// key so that repeated calls with the same mapping share cache entries.
//
// The iterative counterparts live in src/dd_permute_iter.cpp.

#include "bdd.h"
#include "bdd_internal.h"
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace kyotodd {

// ---------------------------------------------------------------------------
// Shared helpers: validation, normalization and interning of mappings
// ---------------------------------------------------------------------------

namespace {

// Interned mappings. Key: (from, to) pairs with from != to, sorted by from,
// flattened as [from0, to0, from1, to1, ...]. Value: cache key (>= 1).
std::map<std::vector<bddvar>, bddp>& perm_id_table() {
    static std::map<std::vector<bddvar>, bddp> table;
    return table;
}

bddp intern_perm(const std::vector<bddvar>& key) {
    std::map<std::vector<bddvar>, bddp>& table = perm_id_table();
    std::map<std::vector<bddvar>, bddp>::iterator it = table.find(key);
    if (it != table.end()) return it->second;
    bddp id = static_cast<bddp>(table.size()) + 1;
    table.insert(std::make_pair(key, id));
    return id;
}

// Validate (from, to) and build the full substitution table.
// Returns false when the mapping is the identity (nothing to do).
// On success: perm[v] is the image of v for v in 1..bdd_varcount,
// min_level the lowest level among changed variables, perm_id the cache key.
bool prepare_perm(const std::vector<bddvar>& from,
                  const std::vector<bddvar>& to,
                  const char* name,
                  std::vector<bddvar>& perm,
                  bddvar& min_level,
                  bddp& perm_id) {
    if (from.size() != to.size()) {
        throw std::invalid_argument(std::string(name) +
            ": from and to must have the same length");
    }
    perm.assign(static_cast<size_t>(bdd_varcount) + 1, 0);
    for (bddvar v = 1; v <= bdd_varcount; ++v) perm[v] = v;

    std::vector<bool> seen(static_cast<size_t>(bdd_varcount) + 1, false);
    for (size_t i = 0; i < from.size(); ++i) {
        bddvar a = from[i];
        bddvar b = to[i];
        if (a < 1 || a > bdd_varcount || b < 1 || b > bdd_varcount) {
            throw std::invalid_argument(std::string(name) +
                ": variable out of range");
        }
        if (seen[a]) {
            throw std::invalid_argument(std::string(name) +
                ": duplicate variable in from");
        }
        seen[a] = true;
        perm[a] = b;
    }

    std::vector<bddvar> key;
    min_level = 0;
    for (bddvar v = 1; v <= bdd_varcount; ++v) {
        if (perm[v] == v) continue;
        key.push_back(v);
        key.push_back(perm[v]);
        bddvar lev = var2level[v];
        if (min_level == 0 || lev < min_level) min_level = lev;
    }
    if (key.empty()) return false;  // identity
    perm_id = intern_perm(key);
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// BDD
// ---------------------------------------------------------------------------

// Rebuild a BDD node for variable w with already-substituted children.
// Direct node creation is valid only when w sits above both children;
// otherwise ITE restores the variable order (and handles the case where the
// children already depend on w, which arises for non-injective mappings).
static bddp permute_make_bdd(bddvar w, bddp r_lo, bddp r_hi) {
    bddvar wl = var2level[w];
    if (wl > bddp_level(r_lo) && wl > bddp_level(r_hi)) {
        return BDD::getnode_raw(w, r_lo, r_hi);
    }
    return bddite(bddprime(w), r_hi, r_lo);
}

static bddp bddpermute_rec(bddp f, const std::vector<bddvar>& perm,
                           bddvar min_level, bddp perm_id) {
    BDD_RecurGuard guard;

    if (f & BDD_CONST_FLAG) return f;

    // Factor out complement: permute(~f) = ~permute(f)
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp fn = f & ~static_cast<bddp>(BDD_COMP_FLAG);

    bddvar f_var = node_var(fn);
    // Entirely below every changed variable: f is unchanged.
    if (var2level[f_var] < min_level) return f;

    bddp cached = bddrcache(BDD_OP_PERMUTE, fn, perm_id);
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    // fn is non-complemented, so node_lo is non-complemented as stored.
    bddp r_lo = bddpermute_rec(node_lo(fn), perm, min_level, perm_id);
    bddp r_hi = bddpermute_rec(node_hi(fn), perm, min_level, perm_id);
    bddp result = permute_make_bdd(perm[f_var], r_lo, r_hi);

    bddwcache(BDD_OP_PERMUTE, fn, perm_id, result);
    return comp ? bddnot(result) : result;
}

bddp bddpermute(bddp f, const std::vector<bddvar>& from,
                const std::vector<bddvar>& to) {
    return bddpermute(f, from, to, BddExecMode::Auto);
}

bddp bddpermute(bddp f, const std::vector<bddvar>& from,
                const std::vector<bddvar>& to, BddExecMode mode) {
    bddp_validate(f, "bddpermute");
    std::vector<bddvar> perm;
    bddvar min_level = 0;
    bddp perm_id = 0;
    bool changed = prepare_perm(from, to, "bddpermute", perm, min_level, perm_id);
    if (f == bddnull) return bddnull;
    if (!changed) return f;
    if (f & BDD_CONST_FLAG) return f;

    if (select_iter_1op(f, mode)) {
        return bdd_gc_guard([&]() -> bddp {
            return bddpermute_iter(f, perm, min_level, perm_id);
        });
    } else {
        return bdd_gc_guard([&]() -> bddp {
            return bddpermute_rec(f, perm, min_level, perm_id);
        });
    }
}

// ---------------------------------------------------------------------------
// ZDD
// ---------------------------------------------------------------------------

// { S ∪ {w} : S ∈ g }. When no set of g contains w this is bddchange;
// otherwise the sets already containing w are kept as they are.
static bddp zdd_attach(bddp g, bddvar w) {
    bddp g_off = bddoffset(g, w);
    if (g_off == g) return bddchange(g, w);
    return bddunion(bddchange(g_off, w), bddonset(g, w));
}

// Rebuild a ZDD node for variable w: result = r_lo ∪ (r_hi with w added to
// every set). Direct node creation is valid only when w sits above both
// children.
static bddp permute_make_zdd(bddvar w, bddp r_lo, bddp r_hi) {
    bddvar wl = var2level[w];
    if (wl > bddp_level(r_lo) && wl > bddp_level(r_hi)) {
        return ZDD::getnode_raw(w, r_lo, r_hi);
    }
    return bddunion(r_lo, zdd_attach(r_hi, w));
}

static bddp bddpermutez_rec(bddp f, const std::vector<bddvar>& perm,
                            bddvar min_level, bddp perm_id) {
    BDD_RecurGuard guard;

    if (f & BDD_CONST_FLAG) return f;

    // Factor out complement. A ZDD complement edge toggles membership of
    // the empty set, and the empty set is mapped to itself, so
    // permute(~f) = ~permute(f).
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp fn = f & ~static_cast<bddp>(BDD_COMP_FLAG);

    bddvar f_var = node_var(fn);
    if (var2level[f_var] < min_level) return f;

    bddp cached = bddrcache(BDD_OP_PERMUTEZ, fn, perm_id);
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    bddp r_lo = bddpermutez_rec(node_lo(fn), perm, min_level, perm_id);
    bddp r_hi = bddpermutez_rec(node_hi(fn), perm, min_level, perm_id);
    bddp result = permute_make_zdd(perm[f_var], r_lo, r_hi);

    bddwcache(BDD_OP_PERMUTEZ, fn, perm_id, result);
    return comp ? bddnot(result) : result;
}

bddp bddpermutez(bddp f, const std::vector<bddvar>& from,
                 const std::vector<bddvar>& to) {
    return bddpermutez(f, from, to, BddExecMode::Auto);
}

bddp bddpermutez(bddp f, const std::vector<bddvar>& from,
                 const std::vector<bddvar>& to, BddExecMode mode) {
    bddp_validate(f, "bddpermutez");
    std::vector<bddvar> perm;
    bddvar min_level = 0;
    bddp perm_id = 0;
    bool changed = prepare_perm(from, to, "bddpermutez", perm, min_level, perm_id);
    if (f == bddnull) return bddnull;
    if (!changed) return f;
    if (f & BDD_CONST_FLAG) return f;

    if (select_iter_1op(f, mode)) {
        return bdd_gc_guard([&]() -> bddp {
            return bddpermutez_iter(f, perm, min_level, perm_id);
        });
    } else {
        return bdd_gc_guard([&]() -> bddp {
            return bddpermutez_rec(f, perm, min_level, perm_id);
        });
    }
}

}  // namespace kyotodd
