#include "bdd.h"
#include "bdd_internal.h"

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
