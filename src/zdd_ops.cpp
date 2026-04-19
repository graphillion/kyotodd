#include "bdd.h"
#include "bdd_internal.h"
#include <stdexcept>

namespace kyotodd {


bddp bddpush(bddp f, bddvar v) {
    bddp_validate(f, "bddpush");
    if (f == bddnull) return bddnull;
    if (v < 1 || v > bdd_varcount) {
        throw std::invalid_argument("bddpush: var out of range");
    }
    // f == bddempty → ZDD::getnode_raw(v, bddempty, bddempty) → bddempty (zero-suppression)
    return bdd_gc_guard([&]() -> bddp { return ZDD::getnode_raw(v, bddempty, f); });
}

static bddp bddoffset_rec(bddp f, bddvar var);

bddp bddoffset(bddp f, bddvar var) {
    bddp_validate(f, "bddoffset");
    if (var < 1 || var > bdd_varcount) {
        throw std::invalid_argument("bddoffset: var out of range");
    }
    if (f == bddnull) return bddnull;
    // Terminal cases
    if (f & BDD_CONST_FLAG) return f;

    if (use_iter_1op(f)) {
        return bdd_gc_guard([&]() -> bddp { return bddoffset_iter(f, var); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bddoffset_rec(f, var); });
    }
}

bddp bddoffset(bddp f, bddvar var, BddExecMode mode) {
    bddp_validate(f, "bddoffset");
    if (var < 1 || var > bdd_varcount) {
        throw std::invalid_argument("bddoffset: var out of range");
    }
    if (f == bddnull) return bddnull;
    if (f & BDD_CONST_FLAG) return f;

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bddoffset_iter(f, var); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bddoffset_rec(f, var); });
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) {
            return bdd_gc_guard([&]() -> bddp { return bddoffset_iter(f, var); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bddoffset_rec(f, var); });
        }
    }
}

static bddp bddoffset_rec(bddp f, bddvar var) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f & BDD_CONST_FLAG) return f;

    // ZDD complement edge: only lo is toggled
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];
    bddvar v_level = var2level[var];

    // var is above f's top: var doesn't appear in f (ZDD suppression), return f as-is
    if (f_level < v_level) return f;

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_OFFSET, f, static_cast<bddp>(var));
    if (cached != bddnull) return cached;

    bddp f_lo = node_lo(f);
    bddp f_hi = node_hi(f);
    if (f_comp) { f_lo = bddnot(f_lo); }  // ZDD: only lo is complemented

    bddp result;
    if (f_var == var) {
        // lo branch = sets not containing var
        result = f_lo;
    } else {
        // f_level > v_level: var is below, recurse into both branches
        bddp lo = bddoffset_rec(f_lo, var);
        bddp hi = bddoffset_rec(f_hi, var);
        result = ZDD::getnode_raw(f_var, lo, hi);
    }

    bddwcache(BDD_OP_OFFSET, f, static_cast<bddp>(var), result);
    return result;
}

static bddp bddonset_rec(bddp f, bddvar var);

bddp bddonset(bddp f, bddvar var) {
    bddp_validate(f, "bddonset");
    if (var < 1 || var > bdd_varcount) {
        throw std::invalid_argument("bddonset: var out of range");
    }
    if (f == bddnull) return bddnull;
    // Terminal cases: no sets contain any variable
    if (f & BDD_CONST_FLAG) return bddempty;

    if (use_iter_1op(f)) {
        return bdd_gc_guard([&]() -> bddp { return bddonset_iter(f, var); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bddonset_rec(f, var); });
    }
}

bddp bddonset(bddp f, bddvar var, BddExecMode mode) {
    bddp_validate(f, "bddonset");
    if (var < 1 || var > bdd_varcount) {
        throw std::invalid_argument("bddonset: var out of range");
    }
    if (f == bddnull) return bddnull;
    if (f & BDD_CONST_FLAG) return bddempty;

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bddonset_iter(f, var); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bddonset_rec(f, var); });
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) {
            return bdd_gc_guard([&]() -> bddp { return bddonset_iter(f, var); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bddonset_rec(f, var); });
        }
    }
}

static bddp bddonset_rec(bddp f, bddvar var) {
    BDD_RecurGuard guard;
    // Terminal cases: no sets contain any variable
    if (f & BDD_CONST_FLAG) return bddempty;

    // ZDD complement edge: only lo is toggled
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];
    bddvar v_level = var2level[var];

    // var is above f's top: var doesn't appear in f (ZDD suppression)
    if (f_level < v_level) return bddempty;

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_ONSET, f, static_cast<bddp>(var));
    if (cached != bddnull) return cached;

    bddp f_lo = node_lo(f);
    bddp f_hi = node_hi(f);
    if (f_comp) { f_lo = bddnot(f_lo); }  // ZDD: only lo is complemented

    bddp result;
    if (f_var == var) {
        // hi branch = sets containing var (with var removed), re-attach var
        result = ZDD::getnode_raw(var, bddempty, f_hi);
    } else {
        // f_level > v_level: var is below, recurse into both branches
        bddp lo = bddonset_rec(f_lo, var);
        bddp hi = bddonset_rec(f_hi, var);
        result = ZDD::getnode_raw(f_var, lo, hi);
    }

    bddwcache(BDD_OP_ONSET, f, static_cast<bddp>(var), result);
    return result;
}

static bddp bddonset0_rec(bddp f, bddvar var);

bddp bddonset0(bddp f, bddvar var) {
    bddp_validate(f, "bddonset0");
    if (var < 1 || var > bdd_varcount) {
        throw std::invalid_argument("bddonset0: var out of range");
    }
    if (f == bddnull) return bddnull;
    // Terminal cases: no sets contain any variable
    if (f & BDD_CONST_FLAG) return bddempty;

    if (use_iter_1op(f)) {
        return bdd_gc_guard([&]() -> bddp { return bddonset0_iter(f, var); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bddonset0_rec(f, var); });
    }
}

bddp bddonset0(bddp f, bddvar var, BddExecMode mode) {
    bddp_validate(f, "bddonset0");
    if (var < 1 || var > bdd_varcount) {
        throw std::invalid_argument("bddonset0: var out of range");
    }
    if (f == bddnull) return bddnull;
    if (f & BDD_CONST_FLAG) return bddempty;

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bddonset0_iter(f, var); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bddonset0_rec(f, var); });
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) {
            return bdd_gc_guard([&]() -> bddp { return bddonset0_iter(f, var); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bddonset0_rec(f, var); });
        }
    }
}

static bddp bddonset0_rec(bddp f, bddvar var) {
    BDD_RecurGuard guard;
    // Terminal cases: no sets contain any variable
    if (f & BDD_CONST_FLAG) return bddempty;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];
    bddvar v_level = var2level[var];

    // var is above f's top: var doesn't appear in f (ZDD suppression)
    if (f_level < v_level) return bddempty;

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_ONSET0, f, static_cast<bddp>(var));
    if (cached != bddnull) return cached;

    bddp f_lo = node_lo(f);
    bddp f_hi = node_hi(f);
    if (f_comp) { f_lo = bddnot(f_lo); }  // ZDD: only lo is complemented

    bddp result;
    if (f_var == var) {
        // hi branch = sets containing var, with var already removed
        result = f_hi;
    } else {
        // f_level > v_level: var is below, recurse into both branches
        bddp lo = bddonset0_rec(f_lo, var);
        bddp hi = bddonset0_rec(f_hi, var);
        result = ZDD::getnode_raw(f_var, lo, hi);
    }

    bddwcache(BDD_OP_ONSET0, f, static_cast<bddp>(var), result);
    return result;
}

static bddp bddchange_rec(bddp f, bddvar var);

bddp bddchange(bddp f, bddvar var) {
    bddp_validate(f, "bddchange");
    if (var < 1) {
        throw std::invalid_argument("bddchange: var out of range");
    }
    if (var > 65536) {
        throw std::invalid_argument(
            "bddchange: auto-expansion beyond 2^16 variables is not supported");
    }
    if (f == bddnull) return bddnull;
    // Auto-expand variables if needed (same pattern as bdd_lshift_rec)
    while (bdd_varcount < var) {
        bddnewvar();
    }
    // Terminal cases
    if (f == bddempty) return bddempty;

    if (use_iter_1op(f)) {
        return bdd_gc_guard([&]() -> bddp { return bddchange_iter(f, var); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bddchange_rec(f, var); });
    }
}

bddp bddchange(bddp f, bddvar var, BddExecMode mode) {
    bddp_validate(f, "bddchange");
    if (var < 1) {
        throw std::invalid_argument("bddchange: var out of range");
    }
    if (var > 65536) {
        throw std::invalid_argument(
            "bddchange: auto-expansion beyond 2^16 variables is not supported");
    }
    if (f == bddnull) return bddnull;
    while (bdd_varcount < var) {
        bddnewvar();
    }
    if (f == bddempty) return bddempty;

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bddchange_iter(f, var); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bddchange_rec(f, var); });
    case BddExecMode::Auto:
    default:
        if (use_iter_1op(f)) {
            return bdd_gc_guard([&]() -> bddp { return bddchange_iter(f, var); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bddchange_rec(f, var); });
        }
    }
}

static bddp bddchange_rec(bddp f, bddvar var) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return ZDD::getnode_raw(var, bddempty, bddsingle);  // {{}} → {{var}}

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];
    bddvar v_level = var2level[var];

    // var is above f's top: all sets lack var, so add var to each
    if (f_level < v_level) return ZDD::getnode_raw(var, bddempty, f);

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_CHANGE, f, static_cast<bddp>(var));
    if (cached != bddnull) return cached;

    bddp f_lo = node_lo(f);
    bddp f_hi = node_hi(f);
    if (f_comp) { f_lo = bddnot(f_lo); }  // ZDD: only lo is complemented

    bddp result;
    if (f_var == var) {
        // Swap: sets without var get var, sets with var lose var
        result = ZDD::getnode_raw(var, f_hi, f_lo);
    } else {
        // f_level > v_level: var is below, recurse into both branches
        bddp lo = bddchange_rec(f_lo, var);
        bddp hi = bddchange_rec(f_hi, var);
        result = ZDD::getnode_raw(f_var, lo, hi);
    }

    bddwcache(BDD_OP_CHANGE, f, static_cast<bddp>(var), result);
    return result;
}

static bddp bddunion_rec(bddp f, bddp g);

bddp bddunion(bddp f, bddp g) {
    bddp_validate(f, "bddunion");
    bddp_validate(g, "bddunion");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return g;
    if (g == bddempty) return f;
    if (f == g) return f;

    // Normalize order (union is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    if (use_iter_2op(f, g)) {
        return bdd_gc_guard([&]() -> bddp { return bddunion_iter(f, g); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bddunion_rec(f, g); });
    }
}

bddp bddunion(bddp f, bddp g, BddExecMode mode) {
    bddp_validate(f, "bddunion");
    bddp_validate(g, "bddunion");
    if (f == bddnull || g == bddnull) return bddnull;
    if (f == bddempty) return g;
    if (g == bddempty) return f;
    if (f == g) return f;
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bddunion_iter(f, g); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bddunion_rec(f, g); });
    case BddExecMode::Auto:
    default:
        if (use_iter_2op(f, g)) {
            return bdd_gc_guard([&]() -> bddp { return bddunion_iter(f, g); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bddunion_rec(f, g); });
        }
    }
}

static bddp bddunion_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return g;
    if (g == bddempty) return f;
    if (f == g) return f;

    // Normalize order (union is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_UNION, f, g);
    if (cached != bddnull) return cached;

    // Determine top variable and cofactors
    bool f_const = (f & BDD_CONST_FLAG) != 0;
    bool g_const = (g & BDD_CONST_FLAG) != 0;
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = f_const ? 0 : node_var(f);
    bddvar g_var = g_const ? 0 : node_var(g);
    bddvar f_level = f_const ? 0 : var2level[f_var];
    bddvar g_level = g_const ? 0 : var2level[g_var];

    bddvar top_var;
    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        top_var = f_var;
        f_lo = node_lo(f); f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        g_lo = g; g_hi = bddempty;
    } else if (g_level > f_level) {
        top_var = g_var;
        f_lo = f; f_hi = bddempty;
        g_lo = node_lo(g); g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
    } else {
        top_var = f_var;
        f_lo = node_lo(f); f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        g_lo = node_lo(g); g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
    }

    bddp lo = bddunion_rec(f_lo, g_lo);
    bddp hi = bddunion_rec(f_hi, g_hi);
    bddp result = ZDD::getnode_raw(top_var, lo, hi);

    bddwcache(BDD_OP_UNION, f, g, result);
    return result;
}

static bddp bddintersec_rec(bddp f, bddp g);

bddp bddintersec(bddp f, bddp g) {
    bddp_validate(f, "bddintersec");
    bddp_validate(g, "bddintersec");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (g == bddempty) return bddempty;
    if (f == g) return f;

    // Normalize order (intersection is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    if (use_iter_2op(f, g)) {
        return bdd_gc_guard([&]() -> bddp { return bddintersec_iter(f, g); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bddintersec_rec(f, g); });
    }
}

bddp bddintersec(bddp f, bddp g, BddExecMode mode) {
    bddp_validate(f, "bddintersec");
    bddp_validate(g, "bddintersec");
    if (f == bddnull || g == bddnull) return bddnull;
    if (f == bddempty) return bddempty;
    if (g == bddempty) return bddempty;
    if (f == g) return f;
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bddintersec_iter(f, g); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bddintersec_rec(f, g); });
    case BddExecMode::Auto:
    default:
        if (use_iter_2op(f, g)) {
            return bdd_gc_guard([&]() -> bddp { return bddintersec_iter(f, g); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bddintersec_rec(f, g); });
        }
    }
}

static bddp bddintersec_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (g == bddempty) return bddempty;
    if (f == g) return f;

    // Normalize order (intersection is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_INTERSEC, f, g);
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
        // g has no sets containing f_var; only f's lo branch can match
        bddp f_lo = node_lo(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        result = bddintersec_rec(f_lo, g);
    } else if (g_level > f_level) {
        bddp g_lo = node_lo(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddintersec_rec(f, g_lo);
    } else {
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bddintersec_rec(f_lo, g_lo);
        bddp hi = bddintersec_rec(f_hi, g_hi);
        result = ZDD::getnode_raw(f_var, lo, hi);
    }

    bddwcache(BDD_OP_INTERSEC, f, g, result);
    return result;
}

static bddp bddsubtract_rec(bddp f, bddp g);

bddp bddsubtract(bddp f, bddp g) {
    bddp_validate(f, "bddsubtract");
    bddp_validate(g, "bddsubtract");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (g == bddempty) return f;
    if (f == g) return bddempty;

    if (use_iter_2op(f, g)) {
        return bdd_gc_guard([&]() -> bddp { return bddsubtract_iter(f, g); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bddsubtract_rec(f, g); });
    }
}

bddp bddsubtract(bddp f, bddp g, BddExecMode mode) {
    bddp_validate(f, "bddsubtract");
    bddp_validate(g, "bddsubtract");
    if (f == bddnull || g == bddnull) return bddnull;
    if (f == bddempty) return bddempty;
    if (g == bddempty) return f;
    if (f == g) return bddempty;

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bddsubtract_iter(f, g); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bddsubtract_rec(f, g); });
    case BddExecMode::Auto:
    default:
        if (use_iter_2op(f, g)) {
            return bdd_gc_guard([&]() -> bddp { return bddsubtract_iter(f, g); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bddsubtract_rec(f, g); });
        }
    }
}

static bddp bddsubtract_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (g == bddempty) return f;
    if (f == g) return bddempty;

    // Cache lookup (not commutative, no order normalization)
    bddp cached = bddrcache(BDD_OP_SUBTRACT, f, g);
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
        // g has no sets containing f_var; hi branch of f is untouched
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp lo = bddsubtract_rec(f_lo, g);
        result = ZDD::getnode_raw(f_var, lo, f_hi);
    } else if (g_level > f_level) {
        // f has no sets containing g_var; subtract from g's lo branch
        bddp g_lo = node_lo(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddsubtract_rec(f, g_lo);
    } else {
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bddsubtract_rec(f_lo, g_lo);
        bddp hi = bddsubtract_rec(f_hi, g_hi);
        result = ZDD::getnode_raw(f_var, lo, hi);
    }

    bddwcache(BDD_OP_SUBTRACT, f, g, result);
    return result;
}

typedef std::unordered_map<bddp, std::unordered_map<bddp, bool>> SubsetMemoMap;

static bool bddissubset_rec(bddp f, bddp g, SubsetMemoMap& memo) {
    // Terminal / identity cases
    if (f == bddempty) return true;
    if (g == bddempty) return false;  // f is non-empty
    if (f == g) return true;
    // f == bddsingle: {∅} ⊆ G iff ∅ ∈ G
    if (f == bddsingle) return bddhasempty(g);
    // g == bddsingle: f is non-trivial, can't be ⊆ {∅}
    if (g == bddsingle) return false;

    BDD_RecurGuard guard;

    // Memo lookup
    auto it_f = memo.find(f);
    if (it_f != memo.end()) {
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

    bool result;

    if (f_level > g_level) {
        // f has variable v, g doesn't → g has no sets containing v.
        // f_hi (non-empty by ZDD zero-suppression) can't be ⊆ ∅.
        result = false;
    } else if (g_level > f_level) {
        // g has variable v, f doesn't → check f ⊆ g_lo
        bddp g_lo = node_lo(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddissubset_rec(f, g_lo, memo);
    } else {
        // Same top variable
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        // Early termination: check hi first (often smaller)
        result = bddissubset_rec(f_hi, g_hi, memo)
              && bddissubset_rec(f_lo, g_lo, memo);
    }

    memo[f][g] = result;
    return result;
}

bool bddissubset(bddp f, bddp g) {
    bddp_validate(f, "bddissubset");
    bddp_validate(g, "bddissubset");
    if (f == bddnull || g == bddnull) return false;
    if (f == bddempty) return true;
    if (f == g) return true;
    if (use_iter_2op(f, g)) {
        return bddissubset_iter(f, g);
    }
    SubsetMemoMap memo;
    return bddissubset_rec(f, g, memo);
}

bool bddissubset(bddp f, bddp g, BddExecMode mode) {
    bddp_validate(f, "bddissubset");
    bddp_validate(g, "bddissubset");
    if (f == bddnull || g == bddnull) return false;
    if (f == bddempty) return true;
    if (f == g) return true;

    switch (mode) {
    case BddExecMode::Iterative:
        return bddissubset_iter(f, g);
    case BddExecMode::Recursive: {
        SubsetMemoMap memo;
        return bddissubset_rec(f, g, memo);
    }
    case BddExecMode::Auto:
    default:
        if (use_iter_2op(f, g)) {
            return bddissubset_iter(f, g);
        }
        SubsetMemoMap memo;
        return bddissubset_rec(f, g, memo);
    }
}

static bool bddisdisjoint_rec(bddp f, bddp g, SubsetMemoMap& memo) {
    // Terminal / identity cases
    if (f == bddempty || g == bddempty) return true;
    if (f == g) return false;  // non-empty and identical → share all sets
    // {∅} ∩ G: disjoint iff ∅ ∉ G
    if (f == bddsingle) return !bddhasempty(g);
    if (g == bddsingle) return !bddhasempty(f);

    // Canonical order: smaller first for memo efficiency
    if (f > g) { bddp t = f; f = g; g = t; }

    BDD_RecurGuard guard;

    // Memo lookup
    auto it_f = memo.find(f);
    if (it_f != memo.end()) {
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

    bool result;

    if (f_level > g_level) {
        // f has variable v, g doesn't.
        // ZDD: g has no sets containing v, so g_hi = 0 for var v.
        // f_hi ∩ g_hi = f_hi ∩ 0 = 0 (trivially disjoint).
        // Check: f_lo ∩ g disjoint?
        bddp f_lo = node_lo(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        result = bddisdisjoint_rec(f_lo, g, memo);
    } else if (g_level > f_level) {
        // g has variable v, f doesn't.
        bddp g_lo = node_lo(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddisdisjoint_rec(f, g_lo, memo);
    } else {
        // Same top variable
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        // Both hi and lo must be disjoint; early termination on first non-disjoint
        result = bddisdisjoint_rec(f_hi, g_hi, memo)
              && bddisdisjoint_rec(f_lo, g_lo, memo);
    }

    memo[f][g] = result;
    return result;
}

bool bddisdisjoint(bddp f, bddp g) {
    bddp_validate(f, "bddisdisjoint");
    bddp_validate(g, "bddisdisjoint");
    if (f == bddnull || g == bddnull) return false;
    if (f == bddempty || g == bddempty) return true;
    if (f == g) return false;
    if (use_iter_2op(f, g)) {
        return bddisdisjoint_iter(f, g);
    }
    SubsetMemoMap memo;
    return bddisdisjoint_rec(f, g, memo);
}

bool bddisdisjoint(bddp f, bddp g, BddExecMode mode) {
    bddp_validate(f, "bddisdisjoint");
    bddp_validate(g, "bddisdisjoint");
    if (f == bddnull || g == bddnull) return false;
    if (f == bddempty || g == bddempty) return true;
    if (f == g) return false;

    switch (mode) {
    case BddExecMode::Iterative:
        return bddisdisjoint_iter(f, g);
    case BddExecMode::Recursive: {
        SubsetMemoMap memo;
        return bddisdisjoint_rec(f, g, memo);
    }
    case BddExecMode::Auto:
    default:
        if (use_iter_2op(f, g)) {
            return bddisdisjoint_iter(f, g);
        }
        SubsetMemoMap memo;
        return bddisdisjoint_rec(f, g, memo);
    }
}

static bddp bdddiv_rec(bddp f, bddp g);

bddp bdddiv(bddp f, bddp g) {
    bddp_validate(f, "bdddiv");
    bddp_validate(g, "bdddiv");
    if (f == bddnull || g == bddnull) return bddnull;
    // Base cases
    if (g == bddsingle) return f;     // F / {∅} = F
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return bddempty;  // G contains non-empty sets

    if (use_iter_2op(f, g)) {
        return bdd_gc_guard([&]() -> bddp { return bdddiv_iter(f, g); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bdddiv_rec(f, g); });
    }
}

bddp bdddiv(bddp f, bddp g, BddExecMode mode) {
    bddp_validate(f, "bdddiv");
    bddp_validate(g, "bdddiv");
    if (f == bddnull || g == bddnull) return bddnull;
    if (g == bddsingle) return f;
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return bddempty;

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bdddiv_iter(f, g); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bdddiv_rec(f, g); });
    case BddExecMode::Auto:
    default:
        if (use_iter_2op(f, g)) {
            return bdd_gc_guard([&]() -> bddp { return bdddiv_iter(f, g); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bdddiv_rec(f, g); });
        }
    }
}

static bddp bdddiv_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Base cases
    if (g == bddsingle) return f;     // F / {∅} = F
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return bddempty;  // G contains non-empty sets

    // Cache lookup (not commutative)
    bddp cached = bddrcache(BDD_OP_DIV, f, g);
    if (cached != bddnull) return cached;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar g_var = node_var(g);
    bddvar f_level = var2level[f_var];
    bddvar g_level = var2level[g_var];

    bddp result;

    if (g_level > f_level) {
        // G contains sets with g_var but F doesn't
        result = bddempty;
    } else if (f_level > g_level) {
        // F has f_var, G doesn't
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp lo = bdddiv_rec(f_lo, g);
        bddp hi = bdddiv_rec(f_hi, g);
        result = ZDD::getnode_raw(f_var, lo, hi);
    } else {
        // Same top variable
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g);
        bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp q1 = bdddiv_rec(f_hi, g_hi);
        if (g_lo == bddempty) {
            result = q1;
        } else {
            bddp q0 = bdddiv_rec(f_lo, g_lo);
            // Use _rec for same-file call
            result = bddintersec_rec(q0, q1);
        }
    }

    bddwcache(BDD_OP_DIV, f, g, result);
    return result;
}

static bddp bddsymdiff_rec(bddp f, bddp g);

bddp bddsymdiff(bddp f, bddp g) {
    bddp_validate(f, "bddsymdiff");
    bddp_validate(g, "bddsymdiff");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return g;
    if (g == bddempty) return f;
    if (f == g) return bddempty;

    // Normalize order (symmetric difference is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    if (use_iter_2op(f, g)) {
        return bdd_gc_guard([&]() -> bddp { return bddsymdiff_iter(f, g); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bddsymdiff_rec(f, g); });
    }
}

bddp bddsymdiff(bddp f, bddp g, BddExecMode mode) {
    bddp_validate(f, "bddsymdiff");
    bddp_validate(g, "bddsymdiff");
    if (f == bddnull || g == bddnull) return bddnull;
    if (f == bddempty) return g;
    if (g == bddempty) return f;
    if (f == g) return bddempty;
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bddsymdiff_iter(f, g); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bddsymdiff_rec(f, g); });
    case BddExecMode::Auto:
    default:
        if (use_iter_2op(f, g)) {
            return bdd_gc_guard([&]() -> bddp { return bddsymdiff_iter(f, g); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bddsymdiff_rec(f, g); });
        }
    }
}

static bddp bddsymdiff_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty) return g;
    if (g == bddempty) return f;
    if (f == g) return bddempty;

    // Normalize order (symmetric difference is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_SYMDIFF, f, g);
    if (cached != bddnull) return cached;

    bool f_const = (f & BDD_CONST_FLAG) != 0;
    bool g_const = (g & BDD_CONST_FLAG) != 0;
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = f_const ? 0 : node_var(f);
    bddvar g_var = g_const ? 0 : node_var(g);
    bddvar f_level = f_const ? 0 : var2level[f_var];
    bddvar g_level = g_const ? 0 : var2level[g_var];

    bddvar top_var;
    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        top_var = f_var;
        f_lo = node_lo(f); f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        g_lo = g; g_hi = bddempty;
    } else if (g_level > f_level) {
        top_var = g_var;
        f_lo = f; f_hi = bddempty;
        g_lo = node_lo(g); g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
    } else {
        top_var = f_var;
        f_lo = node_lo(f); f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        g_lo = node_lo(g); g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
    }

    bddp lo = bddsymdiff_rec(f_lo, g_lo);
    bddp hi = bddsymdiff_rec(f_hi, g_hi);
    bddp result = ZDD::getnode_raw(top_var, lo, hi);

    bddwcache(BDD_OP_SYMDIFF, f, g, result);
    return result;
}

static bddp bddjoin_rec(bddp f, bddp g);

bddp bddjoin(bddp f, bddp g) {
    bddp_validate(f, "bddjoin");
    bddp_validate(g, "bddjoin");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return g;  // {∅} ⊔ G = G
    if (g == bddsingle) return f;  // F ⊔ {∅} = F

    // Normalize order (join is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    if (use_iter_2op(f, g)) {
        return bdd_gc_guard([&]() -> bddp { return bddjoin_iter(f, g); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bddjoin_rec(f, g); });
    }
}

bddp bddjoin(bddp f, bddp g, BddExecMode mode) {
    bddp_validate(f, "bddjoin");
    bddp_validate(g, "bddjoin");
    if (f == bddnull || g == bddnull) return bddnull;
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return g;
    if (g == bddsingle) return f;
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bddjoin_iter(f, g); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bddjoin_rec(f, g); });
    case BddExecMode::Auto:
    default:
        if (use_iter_2op(f, g)) {
            return bdd_gc_guard([&]() -> bddp { return bddjoin_iter(f, g); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bddjoin_rec(f, g); });
        }
    }
}

static bddp bddjoin_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return g;  // {∅} ⊔ G = G
    if (g == bddsingle) return f;  // F ⊔ {∅} = F

    // Normalize order (join is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_JOIN, f, g);
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
        // G has no sets with top_var, so A∪B keeps top_var iff A had it
        bddp lo = bddjoin_rec(f_lo, g);
        bddp hi = bddjoin_rec(f_hi, g);
        result = ZDD::getnode_raw(top_var, lo, hi);
    } else if (g_level > f_level) {
        bddvar top_var = g_var;
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bddjoin_rec(f, g_lo);
        bddp hi = bddjoin_rec(f, g_hi);
        result = ZDD::getnode_raw(top_var, lo, hi);
    } else {
        // Same top variable v
        // lo: A∈F_lo, B∈G_lo (neither has v)
        // hi: (F_lo ⊔ G_hi) ∪ (F_hi ⊔ G_lo) ∪ (F_hi ⊔ G_hi)
        bddvar top_var = f_var;
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo = bddjoin_rec(f_lo, g_lo);
        bddp hi_a = bddjoin_rec(f_lo, g_hi);
        bddp hi_b = bddjoin_rec(f_hi, g_lo);
        bddp hi_c = bddjoin_rec(f_hi, g_hi);
        // Use _rec for same-file calls
        bddp hi = bddunion_rec(bddunion_rec(hi_a, hi_b), hi_c);
        result = ZDD::getnode_raw(top_var, lo, hi);
    }

    bddwcache(BDD_OP_JOIN, f, g, result);
    return result;
}

static bddp bddproduct_rec(bddp f, bddp g);

bddp bddproduct(bddp f, bddp g) {
    bddp_validate(f, "bddproduct");
    bddp_validate(g, "bddproduct");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return g;
    if (g == bddsingle) return f;

    // Normalize order (product is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    if (use_iter_2op(f, g)) {
        return bdd_gc_guard([&]() -> bddp { return bddproduct_iter(f, g); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bddproduct_rec(f, g); });
    }
}

bddp bddproduct(bddp f, bddp g, BddExecMode mode) {
    bddp_validate(f, "bddproduct");
    bddp_validate(g, "bddproduct");
    if (f == bddnull || g == bddnull) return bddnull;
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return g;
    if (g == bddsingle) return f;
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bddproduct_iter(f, g); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bddproduct_rec(f, g); });
    case BddExecMode::Auto:
    default:
        if (use_iter_2op(f, g)) {
            return bdd_gc_guard([&]() -> bddp { return bddproduct_iter(f, g); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bddproduct_rec(f, g); });
        }
    }
}

static bddp bddproduct_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return g;
    if (g == bddsingle) return f;

    // Normalize order (product is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_PRODUCT, f, g);
    if (cached != bddnull) return cached;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = node_var(f);
    bddvar g_var = node_var(g);
    bddvar f_level = var2level[f_var];
    bddvar g_level = var2level[g_var];

    bddp result;

    if (f_level == g_level) {
        throw std::invalid_argument(
            "bddproduct: operands must have disjoint variable sets "
            "(variable " + std::to_string(f_var) + " appears in both)");
    }

    if (f_level > g_level) {
        // Decompose f; g has no sets with f_var (disjoint)
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp lo = bddproduct_rec(f_lo, g);
        bddp hi = bddproduct_rec(f_hi, g);
        result = ZDD::getnode_raw(f_var, lo, hi);
    } else {
        // Decompose g; f has no sets with g_var (disjoint)
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bddproduct_rec(f, g_lo);
        bddp hi = bddproduct_rec(f, g_hi);
        result = ZDD::getnode_raw(g_var, lo, hi);
    }

    bddwcache(BDD_OP_PRODUCT, f, g, result);
    return result;
}

static bddp bddmeet_rec(bddp f, bddp g);

bddp bddmeet(bddp f, bddp g) {
    bddp_validate(f, "bddmeet");
    bddp_validate(g, "bddmeet");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;  // ∅ ∩ B = ∅ for all B
    if (g == bddsingle) return bddsingle;

    // Normalize order (meet is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    if (use_iter_2op(f, g)) {
        return bdd_gc_guard([&]() -> bddp { return bddmeet_iter(f, g); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bddmeet_rec(f, g); });
    }
}

bddp bddmeet(bddp f, bddp g, BddExecMode mode) {
    bddp_validate(f, "bddmeet");
    bddp_validate(g, "bddmeet");
    if (f == bddnull || g == bddnull) return bddnull;
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;
    if (g == bddsingle) return bddsingle;
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bddmeet_iter(f, g); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bddmeet_rec(f, g); });
    case BddExecMode::Auto:
    default:
        if (use_iter_2op(f, g)) {
            return bdd_gc_guard([&]() -> bddp { return bddmeet_iter(f, g); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bddmeet_rec(f, g); });
        }
    }
}

static bddp bddmeet_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;  // ∅ ∩ B = ∅ for all B
    if (g == bddsingle) return bddsingle;

    // Normalize order (meet is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_MEET, f, g);
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
        // G has no sets with f_var; intersection removes f_var
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        // Use _rec for same-file calls
        result = bddunion_rec(bddmeet_rec(f_lo, g), bddmeet_rec(f_hi, g));
    } else if (g_level > f_level) {
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddunion_rec(bddmeet_rec(f, g_lo), bddmeet_rec(f, g_hi));
    } else {
        // Same top variable v
        // lo: (F_lo ⊓ G_lo) ∪ (F_lo ⊓ G_hi) ∪ (F_hi ⊓ G_lo)
        // hi: F_hi ⊓ G_hi
        bddvar top_var = f_var;
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo_a = bddmeet_rec(f_lo, g_lo);
        bddp lo_b = bddmeet_rec(f_lo, g_hi);
        bddp lo_c = bddmeet_rec(f_hi, g_lo);
        bddp lo = bddunion_rec(bddunion_rec(lo_a, lo_b), lo_c);
        bddp hi = bddmeet_rec(f_hi, g_hi);
        result = ZDD::getnode_raw(top_var, lo, hi);
    }

    bddwcache(BDD_OP_MEET, f, g, result);
    return result;
}

static bddp bdddelta_rec(bddp f, bddp g);

bddp bdddelta(bddp f, bddp g) {
    bddp_validate(f, "bdddelta");
    bddp_validate(g, "bdddelta");
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return g;  // ∅ ⊕ B = B for all B
    if (g == bddsingle) return f;

    // Normalize order (delta is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    if (use_iter_2op(f, g)) {
        return bdd_gc_guard([&]() -> bddp { return bdddelta_iter(f, g); });
    } else {
        return bdd_gc_guard([&]() -> bddp { return bdddelta_rec(f, g); });
    }
}

bddp bdddelta(bddp f, bddp g, BddExecMode mode) {
    bddp_validate(f, "bdddelta");
    bddp_validate(g, "bdddelta");
    if (f == bddnull || g == bddnull) return bddnull;
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return g;
    if (g == bddsingle) return f;
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    switch (mode) {
    case BddExecMode::Iterative:
        return bdd_gc_guard([&]() -> bddp { return bdddelta_iter(f, g); });
    case BddExecMode::Recursive:
        return bdd_gc_guard([&]() -> bddp { return bdddelta_rec(f, g); });
    case BddExecMode::Auto:
    default:
        if (use_iter_2op(f, g)) {
            return bdd_gc_guard([&]() -> bddp { return bdddelta_iter(f, g); });
        } else {
            return bdd_gc_guard([&]() -> bddp { return bdddelta_rec(f, g); });
        }
    }
}

static bddp bdddelta_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Terminal cases
    if (f == bddempty || g == bddempty) return bddempty;
    if (f == bddsingle) return g;  // ∅ ⊕ B = B for all B
    if (g == bddsingle) return f;

    // Normalize order (delta is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_DELTA, f, g);
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
        // G has no sets with f_var; A⊕B keeps f_var iff A had it
        bddp lo = bdddelta_rec(f_lo, g);
        bddp hi = bdddelta_rec(f_hi, g);
        result = ZDD::getnode_raw(top_var, lo, hi);
    } else if (g_level > f_level) {
        bddvar top_var = g_var;
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bdddelta_rec(f, g_lo);
        bddp hi = bdddelta_rec(f, g_hi);
        result = ZDD::getnode_raw(top_var, lo, hi);
    } else {
        // Same top variable v
        // v in A⊕B iff exactly one of A,B contains v
        // lo: (F_lo ⊞ G_lo) ∪ (F_hi ⊞ G_hi)  — v cancels or absent
        // hi: (F_lo ⊞ G_hi) ∪ (F_hi ⊞ G_lo)  — v in exactly one
        bddvar top_var = f_var;
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo_a = bdddelta_rec(f_lo, g_lo);
        bddp lo_b = bdddelta_rec(f_hi, g_hi);
        // Use _rec for same-file calls
        bddp lo = bddunion_rec(lo_a, lo_b);
        bddp hi_a = bdddelta_rec(f_lo, g_hi);
        bddp hi_b = bdddelta_rec(f_hi, g_lo);
        bddp hi = bddunion_rec(hi_a, hi_b);
        result = ZDD::getnode_raw(top_var, lo, hi);
    }

    bddwcache(BDD_OP_DELTA, f, g, result);
    return result;
}

bddp bddremainder(bddp f, bddp g) {
    bddp_validate(f, "bddremainder");
    bddp_validate(g, "bddremainder");
    if (f == bddnull || g == bddnull) return bddnull;
    if (f == bddempty) return bddempty;
    if (g == bddsingle) return bddempty;  // F/{∅}=F, {∅}⊔F=F, F\F=∅
    if (f == g) return bddempty;

    // F % G = F \ (G ⊔ (F / G))
    return bdd_gc_guard([&]() -> bddp {
        bddp cached = bddrcache(BDD_OP_REMAINDER, f, g);
        if (cached != bddnull) return cached;

        bddp q = bdddiv_rec(f, g);
        bddp j = bddjoin_rec(g, q);
        bddp result = bddsubtract_rec(f, j);

        bddwcache(BDD_OP_REMAINDER, f, g, result);
        return result;
    });
}

bddp bddlshiftz(bddp f, bddvar shift) {
    bddp_validate(f, "bddlshiftz");
    if (f == bddnull) return bddnull;
    if (f & BDD_CONST_FLAG) return f;
    if (shift == 0) return f;

    return bdd_gc_guard([&]() -> bddp {
        return bdd_lshift_rec(f, shift, BDD_OP_LSHIFTZ, ZDD::getnode_raw);
    });
}

bddp bddrshiftz(bddp f, bddvar shift) {
    bddp_validate(f, "bddrshiftz");
    if (f == bddnull) return bddnull;
    if (f & BDD_CONST_FLAG) return f;
    if (shift == 0) return f;

    return bdd_gc_guard([&]() -> bddp {
        return bdd_rshift_rec(f, shift, BDD_OP_RSHIFTZ, ZDD::getnode_raw);
    });
}

ZDD ZDD_Random(int lev, int density) {
    if (lev <= 0) {
        return (std::rand() % 100 < density) ? ZDD(1) : ZDD(0);
    }
    if (static_cast<bddvar>(lev) > bddvarused()) {
        throw std::invalid_argument("ZDD_Random: lev exceeds number of created variables");
    }
    bddvar v = bddvaroflev(lev);
    ZDD lo = ZDD_Random(lev - 1, density);
    ZDD hi = ZDD_Random(lev - 1, density).Change(v);
    return lo + hi;
}

} // namespace kyotodd
