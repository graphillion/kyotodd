#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <map>
#include <stdexcept>
#include <unordered_map>

static bddp bdddisjoin_rec(bddp f, bddp g);

bddp bdddisjoin(bddp f, bddp g) {
    bddp_validate(f, "bdddisjoin");
    bddp_validate(g, "bdddisjoin");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return g;
    if (g == bddsingle) return f;

    // Normalize order (disjoint join is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    return bdd_gc_guard([&]() -> bddp { return bdddisjoin_rec(f, g); });
}

static bddp bdddisjoin_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return g;
    if (g == bddsingle) return f;

    // Normalize order (disjoint join is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_DISJOIN, f, g);
    if (cached != bddnull) return cached;

    bool f_const = (f & BDD_CONST_FLAG) != 0;
    bool g_const = (g & BDD_CONST_FLAG) != 0;
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = f_const ? 0 : node_var(f);
    bddvar g_var = g_const ? 0 : node_var(g);
    bddvar f_level = f_const ? 0 : var2level[f_var];
    bddvar g_level = g_const ? 0 : var2level[g_var];

    bddp result;

    if (f_level > g_level) {
        bddvar top_var = f_var;
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp lo = bdddisjoin_rec(f_lo, g);
        bddp hi = bdddisjoin_rec(f_hi, g);
        result = ZDD::getnode_raw(top_var, lo, hi);
    } else if (g_level > f_level) {
        bddvar top_var = g_var;
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bdddisjoin_rec(f, g_lo);
        bddp hi = bdddisjoin_rec(f, g_hi);
        result = ZDD::getnode_raw(top_var, lo, hi);
    } else {
        // Same top variable v
        // Both A and B having v means A ∩ B ⊇ {v} ≠ ∅, so that pair is excluded
        bddvar top_var = f_var;
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo = bdddisjoin_rec(f_lo, g_lo);
        bddp hi_a = bdddisjoin_rec(f_lo, g_hi);
        bddp hi_b = bdddisjoin_rec(f_hi, g_lo);
        // Cross-file call: use public wrapper
        bddp hi = bddunion(hi_a, hi_b);
        result = ZDD::getnode_raw(top_var, lo, hi);
    }

    bddwcache(BDD_OP_DISJOIN, f, g, result);
    return result;
}

static bddp bddjointjoin_rec(bddp f, bddp g);

bddp bddjointjoin(bddp f, bddp g) {
    bddp_validate(f, "bddjointjoin");
    bddp_validate(g, "bddjointjoin");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return bddempty;  // ∅ ∩ B = ∅ for all B
    if (g == bddsingle) return bddempty;

    // Normalize order (joint join is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    return bdd_gc_guard([&]() -> bddp { return bddjointjoin_rec(f, g); });
}

static bddp bddjointjoin_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return bddempty;  // ∅ ∩ B = ∅ for all B
    if (g == bddsingle) return bddempty;

    // Normalize order (joint join is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_JOINTJOIN, f, g);
    if (cached != bddnull) return cached;

    bool f_const = (f & BDD_CONST_FLAG) != 0;
    bool g_const = (g & BDD_CONST_FLAG) != 0;
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = f_const ? 0 : node_var(f);
    bddvar g_var = g_const ? 0 : node_var(g);
    bddvar f_level = f_const ? 0 : var2level[f_var];
    bddvar g_level = g_const ? 0 : var2level[g_var];

    bddp result;

    if (f_level > g_level) {
        bddvar top_var = f_var;
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp lo = bddjointjoin_rec(f_lo, g);
        bddp hi = bddjointjoin_rec(f_hi, g);
        result = ZDD::getnode_raw(top_var, lo, hi);
    } else if (g_level > f_level) {
        bddvar top_var = g_var;
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bddjointjoin_rec(f, g_lo);
        bddp hi = bddjointjoin_rec(f, g_hi);
        result = ZDD::getnode_raw(top_var, lo, hi);
    } else {
        // Same top variable v
        // Both having v: A ∩ B ⊇ {v} ≠ ∅, always qualifies → regular join
        bddvar top_var = f_var;
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo = bddjointjoin_rec(f_lo, g_lo);
        bddp hi_a = bddjointjoin_rec(f_lo, g_hi);
        bddp hi_b = bddjointjoin_rec(f_hi, g_lo);
        // Cross-file calls: use public wrappers
        bddp hi_c = bddjoin(f_hi, g_hi);  // always qualifies
        bddp hi = bddunion(bddunion(hi_a, hi_b), hi_c);
        result = ZDD::getnode_raw(top_var, lo, hi);
    }

    bddwcache(BDD_OP_JOINTJOIN, f, g, result);
    return result;
}

static bddp bddrestrict_rec(bddp f, bddp g);

bddp bddrestrict(bddp f, bddp g) {
    bddp_validate(f, "bddrestrict");
    bddp_validate(g, "bddrestrict");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (g == bddempty) return bddempty;
    if (g == bddsingle) return f;  // ∅ ⊆ every set A
    if (f == g) return f;

    return bdd_gc_guard([&]() -> bddp { return bddrestrict_rec(f, g); });
}

static bddp bddrestrict_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (g == bddempty) return bddempty;
    if (g == bddsingle) return f;  // ∅ ⊆ every set A
    if (f == g) return f;

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_RESTRICT, f, g);
    if (cached != bddnull) return cached;

    bool f_const = (f & BDD_CONST_FLAG) != 0;
    bool g_const = (g & BDD_CONST_FLAG) != 0;
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = f_const ? 0 : node_var(f);
    bddvar g_var = g_const ? 0 : node_var(g);
    bddvar f_level = f_const ? 0 : var2level[f_var];
    bddvar g_level = g_const ? 0 : var2level[g_var];

    bddp result;

    if (f_level > g_level) {
        // G has no f_var: B⊆A works for both branches of F
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp lo = bddrestrict_rec(f_lo, g);
        bddp hi = bddrestrict_rec(f_hi, g);
        result = ZDD::getnode_raw(f_var, lo, hi);
    } else if (g_level > f_level) {
        // F has no g_var: B with g_var can't be ⊆ A, skip to G_lo
        bddp g_lo = node_lo(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddrestrict_rec(f, g_lo);
    } else {
        // Same top variable v
        // F_lo: only B from G_lo can be ⊆ A (since A has no v)
        // F_hi: B from G_lo (no v, B⊆S) or B from G_hi (with v, T⊆S)
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo = bddrestrict_rec(f_lo, g_lo);
        // Cross-file call: use public wrapper
        bddp hi = bddrestrict_rec(f_hi, bddunion(g_lo, g_hi));
        result = ZDD::getnode_raw(f_var, lo, hi);
    }

    bddwcache(BDD_OP_RESTRICT, f, g, result);
    return result;
}

static bddp bddpermit_rec(bddp f, bddp g);

bddp bddpermit(bddp f, bddp g) {
    bddp_validate(f, "bddpermit");
    bddp_validate(g, "bddpermit");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (g == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;  // ∅ ⊆ any B in G
    if (f == g) return f;

    return bdd_gc_guard([&]() -> bddp { return bddpermit_rec(f, g); });
}

static bddp bddpermit_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (g == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;  // ∅ ⊆ any B in G
    if (f == g) return f;

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_PERMIT, f, g);
    if (cached != bddnull) return cached;

    bool f_const = (f & BDD_CONST_FLAG) != 0;
    bool g_const = (g & BDD_CONST_FLAG) != 0;
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = f_const ? 0 : node_var(f);
    bddvar g_var = g_const ? 0 : node_var(g);
    bddvar f_level = f_const ? 0 : var2level[f_var];
    bddvar g_level = g_const ? 0 : var2level[g_var];

    bddp result;

    if (f_level > g_level) {
        // G has no f_var: A with f_var can't be ⊆ any B
        bddp f_lo = node_lo(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        result = bddpermit_rec(f_lo, g);
    } else if (g_level > f_level) {
        // F has no g_var: A⊆(T∪{g_var}) iff A⊆T (since g_var∉A)
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        // Cross-file call: use public wrapper
        result = bddpermit_rec(f, bddunion(g_lo, g_hi));
    } else {
        // Same top variable v
        // F_lo (no v): A⊆B for B∈G_lo or A⊆T for T∈G_hi (v irrelevant)
        // F_hi (had v): S∪{v}⊆B requires v∈B, so only B=T∪{v}, T∈G_hi
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        // Cross-file call: use public wrapper
        bddp lo = bddpermit_rec(f_lo, bddunion(g_lo, g_hi));
        bddp hi = bddpermit_rec(f_hi, g_hi);
        result = ZDD::getnode_raw(f_var, lo, hi);
    }

    bddwcache(BDD_OP_PERMIT, f, g, result);
    return result;
}

static bddp bddnonsup_rec(bddp f, bddp g);

bddp bddnonsup(bddp f, bddp g) {
    bddp_validate(f, "bddnonsup");
    bddp_validate(g, "bddnonsup");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (g == bddempty) return f;           // no B exists → all A qualify
    if (g == bddsingle) return bddempty;   // ∅ ⊆ every A → none qualify
    if (f == g) return bddempty;           // A ⊆ A → none qualify

    return bdd_gc_guard([&]() -> bddp { return bddnonsup_rec(f, g); });
}

static bddp bddnonsup_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (g == bddempty) return f;           // no B exists → all A qualify
    if (g == bddsingle) return bddempty;   // ∅ ⊆ every A → none qualify
    if (f == g) return bddempty;           // A ⊆ A → none qualify

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_NONSUP, f, g);
    if (cached != bddnull) return cached;

    bool f_const = (f & BDD_CONST_FLAG) != 0;
    bool g_const = (g & BDD_CONST_FLAG) != 0;
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = f_const ? 0 : node_var(f);
    bddvar g_var = g_const ? 0 : node_var(g);
    bddvar f_level = f_const ? 0 : var2level[f_var];
    bddvar g_level = g_const ? 0 : var2level[g_var];

    bddp result;

    if (f_level > g_level) {
        // G has no f_var: B⊄A check is same for both branches
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp lo = bddnonsup_rec(f_lo, g);
        bddp hi = bddnonsup_rec(f_hi, g);
        result = ZDD::getnode_raw(f_var, lo, hi);
    } else if (g_level > f_level) {
        // F has no g_var: B with g_var can't be ⊆ A, skip to G_lo
        bddp g_lo = node_lo(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddnonsup_rec(f, g_lo);
    } else {
        // Same top variable v
        // F_lo: only B∈G_lo can be ⊆ A (B with v can't be ⊆ A without v)
        // F_hi: B∈G_lo or T∈G_hi can be ⊆ S (same logic as restrict)
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo = bddnonsup_rec(f_lo, g_lo);
        // Cross-file call: use public wrapper
        bddp hi = bddnonsup_rec(f_hi, bddunion(g_lo, g_hi));
        result = ZDD::getnode_raw(f_var, lo, hi);
    }

    bddwcache(BDD_OP_NONSUP, f, g, result);
    return result;
}

static bddp bddnonsub_rec(bddp f, bddp g);

bddp bddnonsub(bddp f, bddp g) {
    bddp_validate(f, "bddnonsub");
    bddp_validate(g, "bddnonsub");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (g == bddempty) return f;           // no B exists → all A qualify
    if (f == bddsingle) return bddempty;   // ∅ ⊆ every B → condition fails
    if (f == g) return bddempty;           // A ⊆ A → condition fails

    return bdd_gc_guard([&]() -> bddp { return bddnonsub_rec(f, g); });
}

static bddp bddnonsub_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (g == bddempty) return f;           // no B exists → all A qualify
    if (f == bddsingle) return bddempty;   // ∅ ⊆ every B → condition fails
    if (f == g) return bddempty;           // A ⊆ A → condition fails

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_NONSUB, f, g);
    if (cached != bddnull) return cached;

    bool f_const = (f & BDD_CONST_FLAG) != 0;
    bool g_const = (g & BDD_CONST_FLAG) != 0;
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = f_const ? 0 : node_var(f);
    bddvar g_var = g_const ? 0 : node_var(g);
    bddvar f_level = f_const ? 0 : var2level[f_var];
    bddvar g_level = g_const ? 0 : var2level[g_var];

    bddp result;

    if (f_level > g_level) {
        // G has no f_var: A with f_var can't be ⊆ any B → F_hi all qualify
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp lo = bddnonsub_rec(f_lo, g);
        result = ZDD::getnode_raw(f_var, lo, f_hi);
    } else if (g_level > f_level) {
        // F has no g_var: A⊆(T∪{g_var}) iff A⊆T → union G branches
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        // Cross-file call: use public wrapper
        result = bddnonsub_rec(f, bddunion(g_lo, g_hi));
    } else {
        // Same top variable v
        // F_lo: A⊆B for B∈G_lo or A⊆T for T∈G_hi → check both
        // F_hi: S∪{v}⊆B requires v∈B → only T∈G_hi matters
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        // Cross-file call: use public wrapper
        bddp lo = bddnonsub_rec(f_lo, bddunion(g_lo, g_hi));
        bddp hi = bddnonsub_rec(f_hi, g_hi);
        result = ZDD::getnode_raw(f_var, lo, hi);
    }

    bddwcache(BDD_OP_NONSUB, f, g, result);
    return result;
}

static bddp bddmaximal_rec(bddp f);

bddp bddmaximal(bddp f) {
    bddp_validate(f, "bddmaximal");
    if (f == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;

    return bdd_gc_guard([&]() -> bddp { return bddmaximal_rec(f); });
}

static bddp bddmaximal_rec(bddp f) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;

    // Cache lookup (unary: g=0)
    bddp cached = bddrcache(BDD_OP_MAXIMAL, f, 0);
    if (cached != bddnull) return cached;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddp f_lo = node_lo(f);
    bddp f_hi = node_hi(f);
    if (f_comp) { f_lo = bddnot(f_lo); }

    // hi: S∪{v} is maximal in F iff S is maximal in F₁
    bddp hi = bddmaximal_rec(f_hi);
    // lo: S∈F₀ is maximal in F iff S is maximal in F₀ AND S is not ⊆ any T∈F₁
    // bddnonsub is cross-file: use public wrapper
    bddp lo = bddnonsub(bddmaximal_rec(f_lo), f_hi);
    bddp result = ZDD::getnode_raw(f_var, lo, hi);

    bddwcache(BDD_OP_MAXIMAL, f, 0, result);
    return result;
}

static bddp bddminimal_rec(bddp f);

bddp bddminimal(bddp f) {
    bddp_validate(f, "bddminimal");
    if (f == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;

    return bdd_gc_guard([&]() -> bddp { return bddminimal_rec(f); });
}

static bddp bddminimal_rec(bddp f) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;

    // Cache lookup (unary: g=0)
    bddp cached = bddrcache(BDD_OP_MINIMAL, f, 0);
    if (cached != bddnull) return cached;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddp f_lo = node_lo(f);
    bddp f_hi = node_hi(f);
    if (f_comp) { f_lo = bddnot(f_lo); }

    // lo: S∈F₀ is minimal in F iff S is minimal in F₀
    // (no T∪{v} from F₁ can be ⊊ S since v∉S)
    bddp lo = bddminimal_rec(f_lo);
    // hi: S∪{v} is minimal in F iff S is minimal in F₁ AND no S'∈F₀ with S'⊆S
    // bddnonsup is same-file: use _rec
    bddp hi = bddnonsup_rec(bddminimal_rec(f_hi), f_lo);
    bddp result = ZDD::getnode_raw(f_var, lo, hi);

    bddwcache(BDD_OP_MINIMAL, f, 0, result);
    return result;
}

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

static bddp bddminhit_rec(bddp f);

bddp bddminhit(bddp f) {
    bddp_validate(f, "bddminhit");
    if (f == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return bddsingle;  // no constraints → {∅}
    if (f == bddsingle) return bddempty;  // ∅ ∈ F → impossible to hit

    // Early termination: if ∅ ∈ F, impossible to hit
    if (bddhasempty(f)) return bddempty;

    return bdd_gc_guard([&]() -> bddp { return bddminhit_rec(f); });
}

static bddp bddminhit_rec(bddp f) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return bddsingle;  // no constraints → {∅}
    if (f == bddsingle) return bddempty;  // ∅ ∈ F → impossible to hit

    // Early termination: if ∅ ∈ F, impossible to hit
    if (bddhasempty(f)) return bddempty;

    // Cache lookup (unary: g=0)
    bddp cached = bddrcache(BDD_OP_MINHIT, f, 0);
    if (cached != bddnull) return cached;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddp f_lo = node_lo(f);
    bddp f_hi = node_hi(f);
    if (f_comp) { f_lo = bddnot(f_lo); }

    // F₀ = offset(F, v) = f_lo, F₁ = onset0(F, v) = f_hi
    // P = minhit(F₁ ∪ F₀): hitting sets not using v
    // bddunion is cross-file: use public wrapper
    bddp P = bddminhit_rec(bddunion(f_hi, f_lo));
    // Q = minhit(F₀): hitting sets of F₀ (will add v)
    bddp Q = bddminhit_rec(f_lo);
    // Filter: keep T∈Q only if no S∈P with S⊆T
    // bddnonsup is same-file: use _rec
    bddp Q_filt = bddnonsup_rec(Q, P);
    bddp result = ZDD::getnode_raw(f_var, P, Q_filt);

    bddwcache(BDD_OP_MINHIT, f, 0, result);
    return result;
}

static bddp bddclosure_rec(bddp f);

bddp bddclosure(bddp f) {
    bddp_validate(f, "bddclosure");
    if (f == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;

    return bdd_gc_guard([&]() -> bddp { return bddclosure_rec(f); });
}

static bddp bddclosure_rec(bddp f) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;

    // Cache lookup (unary: g=0)
    bddp cached = bddrcache(BDD_OP_CLOSURE, f, 0);
    if (cached != bddnull) return cached;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddp f_lo = node_lo(f);
    bddp f_hi = node_hi(f);
    if (f_comp) { f_lo = bddnot(f_lo); }

    // F₀ = offset(F, v), F₁ = onset0(F, v)
    bddp C0 = bddclosure_rec(f_lo);
    bddp C1 = bddclosure_rec(f_hi);
    // lo: closure(F₀) ∪ closure(F₁)
    // bddunion is cross-file: use public wrapper
    bddp lo = bddunion(C0, C1);
    // hi: closure(F₁)
    bddp hi = C1;
    bddp result = ZDD::getnode_raw(f_var, lo, hi);

    bddwcache(BDD_OP_CLOSURE, f, 0, result);
    return result;
}

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

uint64_t bddlit(bddp f) {
    bddp_validate(f, "bddlit");
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

    uint64_t l0 = bddlit(lo);
    uint64_t l1 = bddlit(hi);
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

static const uint64_t BDDLEN_MAX = (UINT64_C(1) << 39) - 1;

uint64_t bddlen(bddp f) {
    bddp_validate(f, "bddlen");
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

    uint64_t len0 = bddlen(lo);
    uint64_t len1 = bddlen(hi);

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
    return bddcount_rec(f, memo);
}

static bigint::BigInt bddexactcount_rec(
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
    return bddexactcount_rec(f, memo);
}

bigint::BigInt bddexactcount(bddp f, CountMemoMap& memo) {
    bddp_validate(f, "bddexactcount");
    if (f == bddnull) return bigint::BigInt(0);
    return bddexactcount_rec(f, memo);
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

bigint::BigInt bddweightsum(bddp f, const std::vector<int>& weights) {
    bddp_validate(f, "bddweightsum");
    if (f == bddnull || f == bddempty || f == bddsingle)
        return bigint::BigInt(0);
    bddvar top = bddtop(f);
    if (weights.size() <= static_cast<size_t>(top)) {
        throw std::invalid_argument(
            "bddweightsum: weights.size() must be > top variable");
    }
    CountMemoMap sum_memo;
    CountMemoMap count_memo;
    return bddweightsum_rec(f, weights, sum_memo, count_memo);
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
    return bddminweight_rec(f, weights, memo);
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
    return bddmaxweight_rec(f, weights, memo);
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
    bddminweight_rec(f, weights, memo);
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
    bddmaxweight_rec(f, weights, memo);
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

bddp bddcostbound_le(bddp f, const std::vector<int>& weights, long long b,
                      CostBoundMemo& memo) {
    bddp_validate(f, "bddcostbound_le");
    if (f == bddnull) {
        throw std::invalid_argument("bddcostbound_le: null ZDD");
    }
    if (weights.size() <= static_cast<size_t>(bddvarused())) {
        throw std::invalid_argument(
            "bddcostbound_le: weights.size() must be > bddvarused()");
    }

    // Validate memo is used with consistent weights
    memo.bind_weights(weights);

    // Terminal fast-paths
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return b >= 0 ? bddsingle : bddempty;

    return bdd_gc_guard([&]() -> bddp {
        // Invalidate memo if GC ran (e.g. at bdd_gc_guard entry)
        memo.invalidate_if_stale();
        return bddcostbound_rec(f, weights, b, memo).h;
    });
}

static std::vector<int> costbound_negate_weights(const std::vector<int>& weights) {
    std::vector<int> neg(weights.size());
    for (size_t i = 0; i < weights.size(); ++i) {
        neg[i] = -weights[i];
    }
    return neg;
}

bddp bddcostbound_ge(bddp f, const std::vector<int>& weights, long long b,
                      CostBoundMemo& memo) {
    std::vector<int> neg = costbound_negate_weights(weights);
    return bddcostbound_le(f, neg, -b, memo);
}

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

ZDD ZDD_LCM_A(char* /*filename*/, int /*threshold*/) {
    throw std::logic_error("ZDD_LCM_A: not implemented");
}

ZDD ZDD_LCM_C(char* /*filename*/, int /*threshold*/) {
    throw std::logic_error("ZDD_LCM_C: not implemented");
}

ZDD ZDD_LCM_M(char* /*filename*/, int /*threshold*/) {
    throw std::logic_error("ZDD_LCM_M: not implemented");
}
