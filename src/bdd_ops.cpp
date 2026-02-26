#include "bdd.h"
#include "bdd_internal.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_set>

bddp bddand(bddp f, bddp g) {
    // Terminal cases
    if (f == bddfalse || g == bddfalse) return bddfalse;
    if (f == bddtrue) return g;
    if (g == bddtrue) return f;
    if (f == g) return f;
    if (f == bddnot(g)) return bddfalse;

    // Normalize operand order (AND is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_AND, f, g);
    if (cached != bddnull) return cached;

    // Both f and g are non-terminal at this point
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = node_var(f);
    bddvar g_var = node_var(g);
    bddvar f_level = var2level[f_var];
    bddvar g_level = var2level[g_var];

    // Determine top variable (highest level) and cofactors
    bddvar top_var;
    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        top_var = f_var;
        f_lo = node_lo(f);
        f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = g;
        g_hi = g;
    } else if (g_level > f_level) {
        top_var = g_var;
        f_lo = f;
        f_hi = f;
        g_lo = node_lo(g);
        g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    } else {
        top_var = f_var;
        f_lo = node_lo(f);
        f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = node_lo(g);
        g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    }

    // Recurse
    bddp lo = bddand(f_lo, g_lo);
    bddp hi = bddand(f_hi, g_hi);

    bddp result = getnode(top_var, lo, hi);

    // Cache insert
    bddwcache(BDD_OP_AND, f, g, result);

    return result;
}

bddp bddor(bddp f, bddp g) {
    return bddnot(bddand(bddnot(f), bddnot(g)));
}

bddp bddnand(bddp f, bddp g) {
    return bddnot(bddand(f, g));
}

bddp bddnor(bddp f, bddp g) {
    return bddand(bddnot(f), bddnot(g));
}

bddp bddxor(bddp f, bddp g) {
    // Terminal cases
    if (f == bddfalse) return g;
    if (f == bddtrue) return bddnot(g);
    if (g == bddfalse) return f;
    if (g == bddtrue) return bddnot(f);
    if (f == g) return bddfalse;
    if (f == bddnot(g)) return bddtrue;

    // Normalize: XOR(~a, b) = ~XOR(a, b), so strip complements and track parity
    bool comp = false;
    if (f & BDD_COMP_FLAG) { f ^= BDD_COMP_FLAG; comp = !comp; }
    if (g & BDD_COMP_FLAG) { g ^= BDD_COMP_FLAG; comp = !comp; }

    // Commutative: normalize order
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_XOR, f, g);
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    // Both f and g are non-complement, non-terminal at this point
    bddvar f_var = node_var(f);
    bddvar g_var = node_var(g);
    bddvar f_level = var2level[f_var];
    bddvar g_level = var2level[g_var];

    // Determine top variable (highest level) and cofactors
    bddvar top_var;
    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        top_var = f_var;
        f_lo = node_lo(f);
        f_hi = node_hi(f);
        g_lo = g;
        g_hi = g;
    } else if (g_level > f_level) {
        top_var = g_var;
        f_lo = f;
        f_hi = f;
        g_lo = node_lo(g);
        g_hi = node_hi(g);
    } else {
        top_var = f_var;
        f_lo = node_lo(f);
        f_hi = node_hi(f);
        g_lo = node_lo(g);
        g_hi = node_hi(g);
    }

    // Recurse
    bddp lo = bddxor(f_lo, g_lo);
    bddp hi = bddxor(f_hi, g_hi);

    bddp result = getnode(top_var, lo, hi);

    // Cache insert (store result for normalized operands, without comp adjustment)
    bddwcache(BDD_OP_XOR, f, g, result);

    return comp ? bddnot(result) : result;
}

bddp bddxnor(bddp f, bddp g) {
    return bddnot(bddxor(f, g));
}

bddp bddite(bddp f, bddp g, bddp h) {
    // Terminal cases for f
    if (f == bddtrue) return g;
    if (f == bddfalse) return h;

    // g == h
    if (g == h) return g;

    // Reduction to 2-operand when g or h is terminal
    if (g == bddtrue) return bddor(f, h);
    if (g == bddfalse) return bddand(bddnot(f), h);
    if (h == bddfalse) return bddand(f, g);
    if (h == bddtrue) return bddor(bddnot(f), g);

    // All three are non-terminal at this point

    // Normalize: f must not be complemented
    if (f & BDD_COMP_FLAG) {
        f = bddnot(f);
        bddp tmp = g; g = h; h = tmp;
    }

    // Normalize: g must not be complemented
    bool comp = false;
    if (g & BDD_COMP_FLAG) {
        g = bddnot(g);
        h = bddnot(h);
        comp = true;
    }

    // Cache lookup
    bddp cached = bddrcache3(BDD_OP_ITE, f, g, h);
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    // Determine top variable (highest level among f, g, h)
    bddvar f_var = node_var(f);
    bddvar g_var = node_var(g);
    bddvar f_level = var2level[f_var];
    bddvar g_level = var2level[g_var];

    bddvar top_var = (f_level >= g_level) ? f_var : g_var;
    bddvar top_level = (f_level >= g_level) ? f_level : g_level;

    bool h_comp = (h & BDD_COMP_FLAG) != 0;
    bddvar h_var = node_var(h);
    bddvar h_level = var2level[h_var];
    if (h_level > top_level) { top_var = h_var; top_level = h_level; }

    // Cofactors for f (non-complemented)
    bddp f_lo, f_hi;
    if (f_level == top_level) {
        f_lo = node_lo(f);
        f_hi = node_hi(f);
    } else {
        f_lo = f; f_hi = f;
    }

    // Cofactors for g (non-complemented)
    bddp g_lo, g_hi;
    if (g_level == top_level) {
        g_lo = node_lo(g);
        g_hi = node_hi(g);
    } else {
        g_lo = g; g_hi = g;
    }

    // Cofactors for h (may be complemented)
    bddp h_lo, h_hi;
    if (h_level == top_level) {
        h_lo = node_lo(h);
        h_hi = node_hi(h);
        if (h_comp) { h_lo = bddnot(h_lo); h_hi = bddnot(h_hi); }
    } else {
        h_lo = h; h_hi = h;
    }

    // Recurse
    bddp lo = bddite(f_lo, g_lo, h_lo);
    bddp hi = bddite(f_hi, g_hi, h_hi);

    bddp result = getnode(top_var, lo, hi);

    // Cache write
    bddwcache3(BDD_OP_ITE, f, g, h, result);

    return comp ? bddnot(result) : result;
}

bddp bddat0(bddp f, bddvar v) {
    // Terminal case
    if (f & BDD_CONST_FLAG) return f;

    // Handle complement edge
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];
    bddvar v_level = var2level[v];

    // f's top variable is below v: v does not appear in f
    if (f_level < v_level) return f;

    // Cache lookup (use v as second operand)
    bddp cached = bddrcache(BDD_OP_AT0, f, static_cast<bddp>(v));
    if (cached != bddnull) return cached;

    bddp result;
    if (f_var == v) {
        // Substitute 0: take the low branch
        result = node_lo(f);
        if (f_comp) result = bddnot(result);
    } else {
        // f_level > v_level: recurse into both branches
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp lo = bddat0(f_lo, v);
        bddp hi = bddat0(f_hi, v);
        result = getnode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_AT0, f, static_cast<bddp>(v), result);
    return result;
}

bddp bddat1(bddp f, bddvar v) {
    // Terminal case
    if (f & BDD_CONST_FLAG) return f;

    // Handle complement edge
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];
    bddvar v_level = var2level[v];

    // f's top variable is below v: v does not appear in f
    if (f_level < v_level) return f;

    // Cache lookup (use v as second operand)
    bddp cached = bddrcache(BDD_OP_AT1, f, static_cast<bddp>(v));
    if (cached != bddnull) return cached;

    bddp result;
    if (f_var == v) {
        // Substitute 1: take the high branch
        result = node_hi(f);
        if (f_comp) result = bddnot(result);
    } else {
        // f_level > v_level: recurse into both branches
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp lo = bddat1(f_lo, v);
        bddp hi = bddat1(f_hi, v);
        result = getnode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_AT1, f, static_cast<bddp>(v), result);
    return result;
}

int bddimply(bddp f, bddp g) {
    // Terminal cases: is there an assignment where f=1 and g=0?
    if (f == bddfalse) return 1;   // f is never true
    if (g == bddtrue)  return 1;   // g is always true
    if (f == bddtrue && g == bddfalse) return 0;  // counterexample
    if (f == g) return 1;          // f implies f
    if (f == bddnot(g)) return 0;  // f=1 => g=0

    // Cache lookup (store result as bddtrue/bddfalse)
    bddp cached = bddrcache(BDD_OP_IMPLY, f, g);
    if (cached != bddnull) return cached == bddtrue ? 1 : 0;

    // Determine top variable and cofactors
    bool f_is_const = (f & BDD_CONST_FLAG) != 0;
    bool g_is_const = (g & BDD_CONST_FLAG) != 0;
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = f_is_const ? 0 : node_var(f);
    bddvar g_var = g_is_const ? 0 : node_var(g);
    bddvar f_level = f_is_const ? 0 : var2level[f_var];
    bddvar g_level = g_is_const ? 0 : var2level[g_var];

    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        f_lo = node_lo(f);
        f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = g;
        g_hi = g;
    } else if (g_level > f_level) {
        f_lo = f;
        f_hi = f;
        g_lo = node_lo(g);
        g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    } else {
        f_lo = node_lo(f);
        f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = node_lo(g);
        g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    }

    // Both branches must satisfy the implication
    int result = bddimply(f_lo, g_lo) && bddimply(f_hi, g_hi);

    bddwcache(BDD_OP_IMPLY, f, g, result ? bddtrue : bddfalse);
    return result;
}

static void bddsupport_collect(bddp f, std::unordered_set<bddvar>& vars,
                               std::unordered_set<bddp>& visited) {
    if (f & BDD_CONST_FLAG) return;
    bddp node = f & ~BDD_COMP_FLAG;
    if (!visited.insert(node).second) return;
    vars.insert(node_var(node));
    bddsupport_collect(node_lo(node), vars, visited);
    bddsupport_collect(node_hi(node), vars, visited);
}

bddp bddsupport(bddp f) {
    if (f & BDD_CONST_FLAG) return bddfalse;

    // Collect all variables appearing in f
    std::unordered_set<bddvar> var_set;
    std::unordered_set<bddp> visited;
    bddsupport_collect(f, var_set, visited);

    // Sort by level ascending (lowest level first)
    std::vector<bddvar> vars(var_set.begin(), var_set.end());
    std::sort(vars.begin(), vars.end(), [](bddvar a, bddvar b) {
        return var2level[a] < var2level[b];
    });

    // Build chain bottom-up: each node has lo=chain, hi=bddtrue
    bddp result = bddfalse;
    for (size_t i = 0; i < vars.size(); i++) {
        result = getnode(vars[i], result, bddtrue);
    }
    return result;
}

std::vector<bddvar> bddsupport_vec(bddp f) {
    std::vector<bddvar> vars;
    if (f & BDD_CONST_FLAG) return vars;

    std::unordered_set<bddvar> var_set;
    std::unordered_set<bddp> visited;
    bddsupport_collect(f, var_set, visited);

    vars.assign(var_set.begin(), var_set.end());
    std::sort(vars.begin(), vars.end(), [](bddvar a, bddvar b) {
        return var2level[a] > var2level[b];
    });
    return vars;
}

bddp bddexist(bddp f, bddp g) {
    // Terminal cases
    if (g == bddfalse) return f;    // no variables to quantify
    if (f == bddfalse) return bddfalse;
    if (f == bddtrue) return bddtrue;

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_EXIST, f, g);
    if (cached != bddnull) return cached;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];

    bddvar g_var = node_var(g);
    bddvar g_level = var2level[g_var];

    bddp result;

    if (g_level > f_level) {
        // g's top variable does not appear in f, skip it
        result = bddexist(f, node_lo(g));
    } else if (f_level > g_level) {
        // f's top variable is not being quantified, keep it
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp lo = bddexist(f_lo, g);
        bddp hi = bddexist(f_hi, g);
        result = getnode(f_var, lo, hi);
    } else {
        // Same variable: quantify it out
        // exist v. f = f|_{v=0} OR f|_{v=1}
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp g_rest = node_lo(g);
        bddp lo_r = bddexist(f_lo, g_rest);
        bddp hi_r = bddexist(f_hi, g_rest);
        result = bddor(lo_r, hi_r);
    }

    bddwcache(BDD_OP_EXIST, f, g, result);
    return result;
}

static bddp vars_to_cube(const std::vector<bddvar>& vars) {
    // Build cube BDD: sort by level ascending, chain with hi=bddtrue
    std::vector<bddvar> sorted(vars);
    std::sort(sorted.begin(), sorted.end(), [](bddvar a, bddvar b) {
        return var2level[a] < var2level[b];
    });
    bddp cube = bddfalse;
    for (size_t i = 0; i < sorted.size(); i++) {
        cube = getnode(sorted[i], cube, bddtrue);
    }
    return cube;
}

bddp bddexist(bddp f, const std::vector<bddvar>& vars) {
    return bddexist(f, vars_to_cube(vars));
}

bddp bdduniv(bddp f, bddp g) {
    // Terminal cases
    if (g == bddfalse) return f;
    if (f == bddfalse) return bddfalse;
    if (f == bddtrue) return bddtrue;

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_UNIV, f, g);
    if (cached != bddnull) return cached;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];

    bddvar g_var = node_var(g);
    bddvar g_level = var2level[g_var];

    bddp result;

    if (g_level > f_level) {
        // g's top variable does not appear in f, skip it
        result = bdduniv(f, node_lo(g));
    } else if (f_level > g_level) {
        // f's top variable is not being quantified, keep it
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp lo = bdduniv(f_lo, g);
        bddp hi = bdduniv(f_hi, g);
        result = getnode(f_var, lo, hi);
    } else {
        // Same variable: quantify it universally
        // forall v. f = f|_{v=0} AND f|_{v=1}
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp g_rest = node_lo(g);
        bddp lo_r = bdduniv(f_lo, g_rest);
        bddp hi_r = bdduniv(f_hi, g_rest);
        result = bddand(lo_r, hi_r);
    }

    bddwcache(BDD_OP_UNIV, f, g, result);
    return result;
}

bddp bdduniv(bddp f, const std::vector<bddvar>& vars) {
    return bdduniv(f, vars_to_cube(vars));
}

static bddp bddlshift_rec(bddp f, bddvar shift) {
    if (f & BDD_CONST_FLAG) return f;

    // Normalize complement for better cache hit rate
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp fn = f & ~BDD_COMP_FLAG;

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_LSHIFT, fn, static_cast<bddp>(shift));
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    bddvar v = node_var(fn);
    bddvar target_var = level2var[var2level[v] + shift];

    bddp lo = bddlshift_rec(node_lo(fn), shift);
    bddp hi = bddlshift_rec(node_hi(fn), shift);

    bddp result = getnode(target_var, lo, hi);

    bddwcache(BDD_OP_LSHIFT, fn, static_cast<bddp>(shift), result);
    return comp ? bddnot(result) : result;
}

bddp bddlshift(bddp f, bddvar shift) {
    if (f & BDD_CONST_FLAG) return f;
    if (shift == 0) return f;

    // Ensure variables exist up to the required level
    bddvar max_level = var2level[bddtop(f)] + shift;
    while (bdd_varcount < max_level) {
        bddnewvar();
    }

    return bddlshift_rec(f, shift);
}

static bddp bddrshift_rec(bddp f, bddvar shift) {
    if (f & BDD_CONST_FLAG) return f;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp fn = f & ~BDD_COMP_FLAG;

    bddp cached = bddrcache(BDD_OP_RSHIFT, fn, static_cast<bddp>(shift));
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    bddvar v = node_var(fn);
    bddvar target_var = level2var[var2level[v] - shift];

    bddp lo = bddrshift_rec(node_lo(fn), shift);
    bddp hi = bddrshift_rec(node_hi(fn), shift);

    bddp result = getnode(target_var, lo, hi);

    bddwcache(BDD_OP_RSHIFT, fn, static_cast<bddp>(shift), result);
    return comp ? bddnot(result) : result;
}

bddp bddrshift(bddp f, bddvar shift) {
    if (f & BDD_CONST_FLAG) return f;
    if (shift == 0) return f;

    // Pre-check: all variable levels must be greater than shift
    std::unordered_set<bddvar> var_set;
    std::unordered_set<bddp> visited;
    bddsupport_collect(f, var_set, visited);
    for (std::unordered_set<bddvar>::iterator it = var_set.begin();
         it != var_set.end(); ++it) {
        if (var2level[*it] <= shift) {
            throw std::invalid_argument("bddrshift: shift exceeds minimum variable level");
        }
    }

    return bddrshift_rec(f, shift);
}

bddp bddcofactor(bddp f, bddp g) {
    // Terminal cases
    if (f & BDD_CONST_FLAG) return f;   // f is constant
    if (g == bddfalse) return bddfalse; // care region is empty
    if (f == bddnot(g)) return bddfalse; // f=0 wherever g=1
    if (f == g) return bddtrue;          // f=1 wherever g=1
    if (g == bddtrue) return f;          // no don't care region

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_COFACTOR, f, g);
    if (cached != bddnull) return cached;

    // Determine top variable
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar g_var = node_var(g);
    bddvar f_level = var2level[f_var];
    bddvar g_level = var2level[g_var];

    bddvar top_var;
    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        top_var = f_var;
        f_lo = node_lo(f); f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = g; g_hi = g;
    } else if (g_level > f_level) {
        top_var = g_var;
        f_lo = f; f_hi = f;
        g_lo = node_lo(g); g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    } else {
        top_var = f_var;
        f_lo = node_lo(f); f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = node_lo(g); g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    }

    bddp result;

    if (g_lo == bddfalse && g_hi != bddfalse) {
        // Low branch is entirely don't care
        result = bddcofactor(f_hi, g_hi);
    } else if (g_hi == bddfalse && g_lo != bddfalse) {
        // High branch is entirely don't care
        result = bddcofactor(f_lo, g_lo);
    } else {
        bddp lo = bddcofactor(f_lo, g_lo);
        bddp hi = bddcofactor(f_hi, g_hi);
        result = getnode(top_var, lo, hi);
    }

    bddwcache(BDD_OP_COFACTOR, f, g, result);
    return result;
}

bddp bddoffset(bddp f, bddvar var) {
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
