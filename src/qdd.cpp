#include "bdd.h"
#include "bdd_internal.h"
#include <stdexcept>
#include <unordered_map>

namespace kyotodd {


const QDD QDD::False(0);
const QDD QDD::True(1);
const QDD QDD::Null(-1);

// QDD::node is removed; use QDD::getnode instead.

// --- QDD to BDD conversion ---

static bddp qdd_to_bdd_rec(bddp f, std::unordered_map<bddp, bddp>& memo) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) return f;

    bddp base = f & ~BDD_COMP_FLAG;
    bool comp = (f & BDD_COMP_FLAG) != 0;

    auto it = memo.find(base);
    if (it != memo.end())
        return comp ? bddnot(it->second) : it->second;

    bddp lo = node_lo(base);
    bddp hi = node_hi(base);

    bddp rlo = qdd_to_bdd_rec(lo, memo);
    bddp rhi = qdd_to_bdd_rec(hi, memo);

    bddp result = BDD::getnode_raw(node_var(base), rlo, rhi);  // jump rule applied
    memo[base] = result;
    return comp ? bddnot(result) : result;
}

BDD QDD::to_bdd() const {
    if (root == bddnull) return BDD::Null;
    bddp result = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        return qdd_to_bdd_rec(root, memo);
    });
    return BDD_ID(result);
}

// --- QDD to ZDD conversion ---

static bddp qdd_to_zdd_rec(bddp f, std::unordered_map<bddp, bddp>& memo) {
    BDD_RecurGuard guard;
    if (f == bddfalse) return bddempty;
    if (f == bddtrue) return bddsingle;

    auto it = memo.find(f);
    if (it != memo.end())
        return it->second;

    // Resolve BDD complement to get explicit lo/hi
    bddp base = f & ~BDD_COMP_FLAG;
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp lo = node_lo(base);
    bddp hi = node_hi(base);
    if (comp) {
        lo = bddnot(lo);
        hi = bddnot(hi);
    }

    bddp zlo = qdd_to_zdd_rec(lo, memo);
    bddp zhi = qdd_to_zdd_rec(hi, memo);

    bddp result = ZDD::getnode_raw(node_var(base), zlo, zhi);
    memo[f] = result;
    return result;
}

ZDD QDD::to_zdd() const {
    if (root == bddnull) return ZDD::Null;
    bddp result = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        return qdd_to_zdd_rec(root, memo);
    });
    return ZDD_ID(result);
}

// --- Helpers ---

// Fill skipped levels with identity (lo==hi) nodes (for BDD conversion).
// BDD skipped levels mean "don't care" — the function is independent of
// the variable, so both branches lead to the same child.
static bddp qdd_fill_levels(bddp child, bddvar child_level, bddvar target_level) {
    if (target_level > bdd_varcount) {
        throw std::invalid_argument(
            "qdd_fill_levels: target_level exceeds bdd_varcount");
    }
    bddp current = child;
    for (bddvar lev = child_level + 1; lev <= target_level; ++lev) {
        bddvar var = level2var[lev];
        current = QDD::getnode_raw(var, current, current);
    }
    return current;
}

// Fill skipped levels with zero-suppression nodes (for ZDD conversion).
// ZDD zero-suppressed levels mean "variable not in set", so in BDD terms:
// lo (var=0) = continue, hi (var=1) = false.
static bddp qdd_fill_levels_zdd(bddp child, bddvar child_level, bddvar target_level) {
    if (target_level > bdd_varcount) {
        throw std::invalid_argument(
            "qdd_fill_levels_zdd: target_level exceeds bdd_varcount");
    }
    bddp current = child;
    // Build false column incrementally alongside current
    bddp false_col = bddfalse;
    for (bddvar lev = 1; lev <= child_level; ++lev) {
        false_col = QDD::getnode_raw(level2var[lev], false_col, false_col);
    }
    // Now false_col and current are both at child_level
    for (bddvar lev = child_level + 1; lev <= target_level; ++lev) {
        bddvar var = level2var[lev];
        current = QDD::getnode_raw(var, current, false_col);
        false_col = QDD::getnode_raw(var, false_col, false_col);
    }
    return current;
}

// --- BDD to QDD conversion ---

static bddp bdd_to_qdd_rec(bddp f, std::unordered_map<bddp, bddp>& memo) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) return f;

    bddp base = f & ~BDD_COMP_FLAG;
    bool comp = (f & BDD_COMP_FLAG) != 0;

    auto it = memo.find(base);
    if (it != memo.end())
        return comp ? bddnot(it->second) : it->second;

    bddp lo = node_lo(base);
    bddp hi = node_hi(base);

    bddp qlo = bdd_to_qdd_rec(lo, memo);
    bddp qhi = bdd_to_qdd_rec(hi, memo);

    bddvar node_level = var2level[node_var(base)];

    // Fill skipped levels between children and this node
    bddp filled_lo = qdd_fill_levels(qlo, bddp_level(qlo), node_level - 1);
    bddp filled_hi = qdd_fill_levels(qhi, bddp_level(qhi), node_level - 1);

    bddp result = QDD::getnode_raw(node_var(base), filled_lo, filled_hi);
    memo[base] = result;
    return comp ? bddnot(result) : result;
}

QDD BDD::to_qdd() const {
    if (root == bddnull) return QDD::Null;
    bddp result = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        bddp q = bdd_to_qdd_rec(root, memo);
        // Fill levels above the root up to the top level
        bddvar q_level = bddp_level(q);
        return qdd_fill_levels(q, q_level, bdd_varcount);
    });
    return QDD_ID(result);
}

// --- ZDD to QDD conversion ---

static bddp zdd_to_qdd_rec(bddp f, std::unordered_map<bddp, bddp>& memo) {
    BDD_RecurGuard guard;
    if (f == bddempty) return bddfalse;
    if (f == bddtrue) return bddtrue;  // bddsingle == bddtrue

    auto it = memo.find(f);
    if (it != memo.end())
        return it->second;

    // ZDD complement: only lo is toggled
    bddp base = f & ~BDD_COMP_FLAG;
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp lo = node_lo(base);
    bddp hi = node_hi(base);
    if (comp) {
        lo = bddnot(lo);  // ZDD: only lo affected
    }

    bddp qlo = zdd_to_qdd_rec(lo, memo);
    bddp qhi = zdd_to_qdd_rec(hi, memo);

    bddvar node_level = var2level[node_var(base)];

    // ZDD: skipped levels are zero-suppressed (hi=false in BDD terms)
    bddp filled_lo = qdd_fill_levels_zdd(qlo, bddp_level(qlo), node_level - 1);
    bddp filled_hi = qdd_fill_levels_zdd(qhi, bddp_level(qhi), node_level - 1);

    bddp result = QDD::getnode_raw(node_var(base), filled_lo, filled_hi);
    memo[f] = result;  // complement-inclusive key
    return result;
}

QDD ZDD::to_qdd() const {
    if (root == bddnull) return QDD::Null;
    bddp result = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        bddp q = zdd_to_qdd_rec(root, memo);
        bddvar q_level = bddp_level(q);
        return qdd_fill_levels_zdd(q, q_level, bdd_varcount);
    });
    return QDD_ID(result);
}


// ============================================================
// Direct ZDD <-> BDD conversion (without QDD intermediate)
// ============================================================

// Fill ZDD-suppressed levels as BDD nodes: (var, current, bddfalse)
// Meaning: "variable=1 → false" (ZDD zero-suppression in BDD terms)
static bddp fill_zdd_levels_as_bdd(bddp child, bddvar child_level, bddvar target_level) {
    bddp current = child;
    for (bddvar lev = child_level + 1; lev <= target_level; ++lev) {
        bddvar var = level2var[lev];
        current = BDD::getnode_raw(var, current, bddfalse);
    }
    return current;
}

// Fill BDD don't-care levels as ZDD nodes: (var, current, current)
// Meaning: "variable present or absent, same result"
static bddp fill_bdd_levels_as_zdd(bddp child, bddvar child_level, bddvar target_level) {
    bddp current = child;
    for (bddvar lev = child_level + 1; lev <= target_level; ++lev) {
        bddvar var = level2var[lev];
        current = ZDD::getnode_raw(var, current, current);
    }
    return current;
}

static bddp zdd_to_bdd_rec(bddp f, std::unordered_map<bddp, bddp>& memo) {
    BDD_RecurGuard guard;
    if (f == bddempty) return bddfalse;
    if (f == bddsingle) return bddtrue;

    auto it = memo.find(f);
    if (it != memo.end()) return it->second;

    // ZDD complement: only lo toggled
    bddp base = f & ~BDD_COMP_FLAG;
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp lo = node_lo(base);
    bddp hi = node_hi(base);
    if (comp) lo = bddnot(lo);

    // Use SOURCE (ZDD) child levels, not target (BDD) levels,
    // because BDD jump rule may collapse nodes and change levels.
    bddvar lo_src_level = bddp_level(lo);
    bddvar hi_src_level = bddp_level(hi);

    bddp bdd_lo = zdd_to_bdd_rec(lo, memo);
    bddp bdd_hi = zdd_to_bdd_rec(hi, memo);

    bddvar node_level = var2level[node_var(base)];

    // Fill ZDD-suppressed levels with BDD "hi=false" nodes
    bdd_lo = fill_zdd_levels_as_bdd(bdd_lo, lo_src_level, node_level - 1);
    bdd_hi = fill_zdd_levels_as_bdd(bdd_hi, hi_src_level, node_level - 1);

    bddp result = BDD::getnode_raw(node_var(base), bdd_lo, bdd_hi);
    memo[f] = result;
    return result;
}

BDD ZDD::to_bdd(int n) const {
    if (root == bddnull) return BDD::Null;
    bddvar nv = (n <= 0) ? bddvarused() : static_cast<bddvar>(n);
    bddp result = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        bddvar root_src_level = bddp_level(root);
        bddp bdd_root = zdd_to_bdd_rec(root, memo);
        return fill_zdd_levels_as_bdd(bdd_root, root_src_level, nv);
    });
    return BDD_ID(result);
}

static bddp bdd_to_zdd_rec(bddp f, std::unordered_map<bddp, bddp>& memo) {
    BDD_RecurGuard guard;
    if (f == bddfalse) return bddempty;
    if (f == bddtrue) return bddsingle;

    auto it = memo.find(f);
    if (it != memo.end()) return it->second;

    // BDD complement: both lo and hi toggled
    bddp base = f & ~BDD_COMP_FLAG;
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp lo = node_lo(base);
    bddp hi = node_hi(base);
    if (comp) {
        lo = bddnot(lo);
        hi = bddnot(hi);
    }

    // Use SOURCE (BDD) child levels, not target (ZDD) levels,
    // because ZDD zero-suppression may collapse nodes and change levels.
    bddvar lo_src_level = bddp_level(lo);
    bddvar hi_src_level = bddp_level(hi);

    bddp zdd_lo = bdd_to_zdd_rec(lo, memo);
    bddp zdd_hi = bdd_to_zdd_rec(hi, memo);

    bddvar node_level = var2level[node_var(base)];

    // Fill BDD don't-care levels with ZDD "both branches same" nodes
    zdd_lo = fill_bdd_levels_as_zdd(zdd_lo, lo_src_level, node_level - 1);
    zdd_hi = fill_bdd_levels_as_zdd(zdd_hi, hi_src_level, node_level - 1);

    bddp result = ZDD::getnode_raw(node_var(base), zdd_lo, zdd_hi);
    memo[f] = result;
    return result;
}

ZDD BDD::to_zdd(int n) const {
    if (root == bddnull) return ZDD::Null;
    bddvar nv = (n <= 0) ? bddvarused() : static_cast<bddvar>(n);
    bddp result = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        bddvar root_src_level = bddp_level(root);
        bddp zdd_root = bdd_to_zdd_rec(root, memo);
        return fill_bdd_levels_as_zdd(zdd_root, root_src_level, nv);
    });
    return ZDD_ID(result);
}

QDD QDD_ID(bddp p) {
    if (p != bddnull && !(p & BDD_CONST_FLAG)) {
        if (!bddp_is_reduced(p)) {
            throw std::invalid_argument("QDD_ID: node is not reduced");
        }
    }
    QDD q(0);
    q.root = p;
    return q;
}

QDD QDD::cache_get(uint8_t op, const QDD& f, const QDD& g) {
    return QDD_ID(bddrcache(op, f.id(), g.id()));
}

void QDD::cache_put(uint8_t op, const QDD& f, const QDD& g, const QDD& result) {
    bddwcache(op, f.id(), g.id(), result.id());
}

} // namespace kyotodd
