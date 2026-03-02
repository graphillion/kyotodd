#include "bdd.h"
#include "bdd_internal.h"
#include <stdexcept>

bddp bddoffset(bddp f, bddvar var) {
    if (var < 1 || var > bdd_varcount) {
        throw std::invalid_argument("bddoffset: var out of range");
    }
    if (f == bddnull) return bddnull;
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
        bddp lo = bddoffset(f_lo, var);
        bddp hi = bddoffset(f_hi, var);
        result = getznode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_OFFSET, f, static_cast<bddp>(var), result);
    return result;
}

bddp bddonset(bddp f, bddvar var) {
    if (var < 1 || var > bdd_varcount) {
        throw std::invalid_argument("bddonset: var out of range");
    }
    if (f == bddnull) return bddnull;
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
        result = getznode(var, bddempty, f_hi);
    } else {
        // f_level > v_level: var is below, recurse into both branches
        bddp lo = bddonset(f_lo, var);
        bddp hi = bddonset(f_hi, var);
        result = getznode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_ONSET, f, static_cast<bddp>(var), result);
    return result;
}

bddp bddonset0(bddp f, bddvar var) {
    if (var < 1 || var > bdd_varcount) {
        throw std::invalid_argument("bddonset0: var out of range");
    }
    if (f == bddnull) return bddnull;
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
        bddp lo = bddonset0(f_lo, var);
        bddp hi = bddonset0(f_hi, var);
        result = getznode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_ONSET0, f, static_cast<bddp>(var), result);
    return result;
}

bddp bddchange(bddp f, bddvar var) {
    if (var < 1 || var > bdd_varcount) {
        throw std::invalid_argument("bddchange: var out of range");
    }
    if (f == bddnull) return bddnull;
    // Terminal cases
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return getznode(var, bddempty, bddsingle);  // {{}} → {{var}}

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];
    bddvar v_level = var2level[var];

    // var is above f's top: all sets lack var, so add var to each
    if (f_level < v_level) return getznode(var, bddempty, f);

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_CHANGE, f, static_cast<bddp>(var));
    if (cached != bddnull) return cached;

    bddp f_lo = node_lo(f);
    bddp f_hi = node_hi(f);
    if (f_comp) { f_lo = bddnot(f_lo); }  // ZDD: only lo is complemented

    bddp result;
    if (f_var == var) {
        // Swap: sets without var get var, sets with var lose var
        result = getznode(var, f_hi, f_lo);
    } else {
        // f_level > v_level: var is below, recurse into both branches
        bddp lo = bddchange(f_lo, var);
        bddp hi = bddchange(f_hi, var);
        result = getznode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_CHANGE, f, static_cast<bddp>(var), result);
    return result;
}

bddp bddunion(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
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

    bddp lo = bddunion(f_lo, g_lo);
    bddp hi = bddunion(f_hi, g_hi);
    bddp result = getznode(top_var, lo, hi);

    bddwcache(BDD_OP_UNION, f, g, result);
    return result;
}

bddp bddintersec(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
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
        result = bddintersec(f_lo, g);
    } else if (g_level > f_level) {
        bddp g_lo = node_lo(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddintersec(f, g_lo);
    } else {
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bddintersec(f_lo, g_lo);
        bddp hi = bddintersec(f_hi, g_hi);
        result = getznode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_INTERSEC, f, g, result);
    return result;
}

bddp bddsubtract(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
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
        bddp lo = bddsubtract(f_lo, g);
        result = getznode(f_var, lo, f_hi);
    } else if (g_level > f_level) {
        // f has no sets containing g_var; subtract from g's lo branch
        bddp g_lo = node_lo(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddsubtract(f, g_lo);
    } else {
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bddsubtract(f_lo, g_lo);
        bddp hi = bddsubtract(f_hi, g_hi);
        result = getznode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_SUBTRACT, f, g, result);
    return result;
}

bddp bdddiv(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
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
        bddp lo = bdddiv(f_lo, g);
        bddp hi = bdddiv(f_hi, g);
        result = getznode(f_var, lo, hi);
    } else {
        // Same top variable
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g);
        bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp q1 = bdddiv(f_hi, g_hi);
        if (g_lo == bddempty) {
            result = q1;
        } else {
            bddp q0 = bdddiv(f_lo, g_lo);
            result = bddintersec(q0, q1);
        }
    }

    bddwcache(BDD_OP_DIV, f, g, result);
    return result;
}

bddp bddsymdiff(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
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

    bddp lo = bddsymdiff(f_lo, g_lo);
    bddp hi = bddsymdiff(f_hi, g_hi);
    bddp result = getznode(top_var, lo, hi);

    bddwcache(BDD_OP_SYMDIFF, f, g, result);
    return result;
}

bddp bddjoin(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
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
        bddp lo = bddjoin(f_lo, g);
        bddp hi = bddjoin(f_hi, g);
        result = getznode(top_var, lo, hi);
    } else if (g_level > f_level) {
        bddvar top_var = g_var;
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bddjoin(f, g_lo);
        bddp hi = bddjoin(f, g_hi);
        result = getznode(top_var, lo, hi);
    } else {
        // Same top variable v
        // lo: A∈F_lo, B∈G_lo (neither has v)
        // hi: (F_lo ⊔ G_hi) ∪ (F_hi ⊔ G_lo) ∪ (F_hi ⊔ G_hi)
        bddvar top_var = f_var;
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo = bddjoin(f_lo, g_lo);
        bddp hi_a = bddjoin(f_lo, g_hi);
        bddp hi_b = bddjoin(f_hi, g_lo);
        bddp hi_c = bddjoin(f_hi, g_hi);
        bddp hi = bddunion(bddunion(hi_a, hi_b), hi_c);
        result = getznode(top_var, lo, hi);
    }

    bddwcache(BDD_OP_JOIN, f, g, result);
    return result;
}

bddp bddmeet(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
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
        result = bddunion(bddmeet(f_lo, g), bddmeet(f_hi, g));
    } else if (g_level > f_level) {
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddunion(bddmeet(f, g_lo), bddmeet(f, g_hi));
    } else {
        // Same top variable v
        // lo: (F_lo ⊓ G_lo) ∪ (F_lo ⊓ G_hi) ∪ (F_hi ⊓ G_lo)
        // hi: F_hi ⊓ G_hi
        bddvar top_var = f_var;
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo_a = bddmeet(f_lo, g_lo);
        bddp lo_b = bddmeet(f_lo, g_hi);
        bddp lo_c = bddmeet(f_hi, g_lo);
        bddp lo = bddunion(bddunion(lo_a, lo_b), lo_c);
        bddp hi = bddmeet(f_hi, g_hi);
        result = getznode(top_var, lo, hi);
    }

    bddwcache(BDD_OP_MEET, f, g, result);
    return result;
}

bddp bdddelta(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
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
        bddp lo = bdddelta(f_lo, g);
        bddp hi = bdddelta(f_hi, g);
        result = getznode(top_var, lo, hi);
    } else if (g_level > f_level) {
        bddvar top_var = g_var;
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bdddelta(f, g_lo);
        bddp hi = bdddelta(f, g_hi);
        result = getznode(top_var, lo, hi);
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

        bddp lo_a = bdddelta(f_lo, g_lo);
        bddp lo_b = bdddelta(f_hi, g_hi);
        bddp lo = bddunion(lo_a, lo_b);
        bddp hi_a = bdddelta(f_lo, g_hi);
        bddp hi_b = bdddelta(f_hi, g_lo);
        bddp hi = bddunion(hi_a, hi_b);
        result = getznode(top_var, lo, hi);
    }

    bddwcache(BDD_OP_DELTA, f, g, result);
    return result;
}

bddp bddremainder(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
    // F % G = F \ (G ⊔ (F / G))
    return bddsubtract(f, bddjoin(g, bdddiv(f, g)));
}
