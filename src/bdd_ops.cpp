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

bddp bddchange(bddp f, bddvar var) {
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
    // F % G = F \ (G ⊔ (F / G))
    return bddsubtract(f, bddjoin(g, bdddiv(f, g)));
}

bddp bdddisjoin(bddp f, bddp g) {
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
        bddp lo = bdddisjoin(f_lo, g);
        bddp hi = bdddisjoin(f_hi, g);
        result = getznode(top_var, lo, hi);
    } else if (g_level > f_level) {
        bddvar top_var = g_var;
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bdddisjoin(f, g_lo);
        bddp hi = bdddisjoin(f, g_hi);
        result = getznode(top_var, lo, hi);
    } else {
        // Same top variable v
        // Both A and B having v means A ∩ B ⊇ {v} ≠ ∅, so that pair is excluded
        bddvar top_var = f_var;
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo = bdddisjoin(f_lo, g_lo);
        bddp hi_a = bdddisjoin(f_lo, g_hi);
        bddp hi_b = bdddisjoin(f_hi, g_lo);
        bddp hi = bddunion(hi_a, hi_b);
        result = getznode(top_var, lo, hi);
    }

    bddwcache(BDD_OP_DISJOIN, f, g, result);
    return result;
}

bddp bddjointjoin(bddp f, bddp g) {
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
        bddp lo = bddjointjoin(f_lo, g);
        bddp hi = bddjointjoin(f_hi, g);
        result = getznode(top_var, lo, hi);
    } else if (g_level > f_level) {
        bddvar top_var = g_var;
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        bddp lo = bddjointjoin(f, g_lo);
        bddp hi = bddjointjoin(f, g_hi);
        result = getznode(top_var, lo, hi);
    } else {
        // Same top variable v
        // Both having v: A ∩ B ⊇ {v} ≠ ∅, always qualifies → regular join
        bddvar top_var = f_var;
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo = bddjointjoin(f_lo, g_lo);
        bddp hi_a = bddjointjoin(f_lo, g_hi);
        bddp hi_b = bddjointjoin(f_hi, g_lo);
        bddp hi_c = bddjoin(f_hi, g_hi);  // always qualifies
        bddp hi = bddunion(bddunion(hi_a, hi_b), hi_c);
        result = getznode(top_var, lo, hi);
    }

    bddwcache(BDD_OP_JOINTJOIN, f, g, result);
    return result;
}

bddp bddrestrict(bddp f, bddp g) {
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
        bddp lo = bddrestrict(f_lo, g);
        bddp hi = bddrestrict(f_hi, g);
        result = getznode(f_var, lo, hi);
    } else if (g_level > f_level) {
        // F has no g_var: B with g_var can't be ⊆ A, skip to G_lo
        bddp g_lo = node_lo(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddrestrict(f, g_lo);
    } else {
        // Same top variable v
        // F_lo: only B from G_lo can be ⊆ A (since A has no v)
        // F_hi: B from G_lo (no v, B⊆S) or B from G_hi (with v, T⊆S)
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo = bddrestrict(f_lo, g_lo);
        bddp hi = bddrestrict(f_hi, bddunion(g_lo, g_hi));
        result = getznode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_RESTRICT, f, g, result);
    return result;
}

bddp bddpermit(bddp f, bddp g) {
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
        result = bddpermit(f_lo, g);
    } else if (g_level > f_level) {
        // F has no g_var: A⊆(T∪{g_var}) iff A⊆T (since g_var∉A)
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddpermit(f, bddunion(g_lo, g_hi));
    } else {
        // Same top variable v
        // F_lo (no v): A⊆B for B∈G_lo or A⊆T for T∈G_hi (v irrelevant)
        // F_hi (had v): S∪{v}⊆B requires v∈B, so only B=T∪{v}, T∈G_hi
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo = bddpermit(f_lo, bddunion(g_lo, g_hi));
        bddp hi = bddpermit(f_hi, g_hi);
        result = getznode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_PERMIT, f, g, result);
    return result;
}

bddp bddnonsup(bddp f, bddp g) {
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
        bddp lo = bddnonsup(f_lo, g);
        bddp hi = bddnonsup(f_hi, g);
        result = getznode(f_var, lo, hi);
    } else if (g_level > f_level) {
        // F has no g_var: B with g_var can't be ⊆ A, skip to G_lo
        bddp g_lo = node_lo(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddnonsup(f, g_lo);
    } else {
        // Same top variable v
        // F_lo: only B∈G_lo can be ⊆ A (B with v can't be ⊆ A without v)
        // F_hi: B∈G_lo or T∈G_hi can be ⊆ S (same logic as restrict)
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo = bddnonsup(f_lo, g_lo);
        bddp hi = bddnonsup(f_hi, bddunion(g_lo, g_hi));
        result = getznode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_NONSUP, f, g, result);
    return result;
}

bddp bddnonsub(bddp f, bddp g) {
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
        bddp lo = bddnonsub(f_lo, g);
        result = getznode(f_var, lo, f_hi);
    } else if (g_level > f_level) {
        // F has no g_var: A⊆(T∪{g_var}) iff A⊆T → union G branches
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }
        result = bddnonsub(f, bddunion(g_lo, g_hi));
    } else {
        // Same top variable v
        // F_lo: A⊆B for B∈G_lo or A⊆T for T∈G_hi → check both
        // F_hi: S∪{v}⊆B requires v∈B → only T∈G_hi matters
        bddp f_lo = node_lo(f); bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }
        bddp g_lo = node_lo(g); bddp g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); }

        bddp lo = bddnonsub(f_lo, bddunion(g_lo, g_hi));
        bddp hi = bddnonsub(f_hi, g_hi);
        result = getznode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_NONSUB, f, g, result);
    return result;
}
