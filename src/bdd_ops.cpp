#include "bdd.h"
#include "bdd_internal.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_set>

static bddp bddand_rec(bddp f, bddp g);

bddp bddand(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddfalse || g == bddfalse) return bddfalse;
    if (f == bddtrue) return g;
    if (g == bddtrue) return f;
    if (f == g) return f;
    if (f == bddnot(g)) return bddfalse;

    // Normalize operand order (AND is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    return bdd_gc_guard([&]() -> bddp { return bddand_rec(f, g); });
}

static bddp bddand_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
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
    bddp lo = bddand_rec(f_lo, g_lo);
    bddp hi = bddand_rec(f_hi, g_hi);

    bddp result = BDD::getnode_raw(top_var, lo, hi);

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

static bddp bddxor_rec(bddp f, bddp g);

bddp bddxor(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f == bddfalse) return g;
    if (f == bddtrue) return bddnot(g);
    if (g == bddfalse) return f;
    if (g == bddtrue) return bddnot(f);
    if (f == g) return bddfalse;
    if (f == bddnot(g)) return bddtrue;

    return bdd_gc_guard([&]() -> bddp { return bddxor_rec(f, g); });
}

static bddp bddxor_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
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
    bddp lo = bddxor_rec(f_lo, g_lo);
    bddp hi = bddxor_rec(f_hi, g_hi);

    bddp result = BDD::getnode_raw(top_var, lo, hi);

    // Cache insert (store result for normalized operands, without comp adjustment)
    bddwcache(BDD_OP_XOR, f, g, result);

    return comp ? bddnot(result) : result;
}

bddp bddxnor(bddp f, bddp g) {
    return bddnot(bddxor(f, g));
}

static bddp bddite_rec(bddp f, bddp g, bddp h);

bddp bddite(bddp f, bddp g, bddp h) {
    if (f == bddnull || g == bddnull || h == bddnull) return bddnull;
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

    return bdd_gc_guard([&]() -> bddp { return bddite_rec(f, g, h); });
}

static bddp bddite_rec(bddp f, bddp g, bddp h) {
    BDD_RecurGuard guard;
    // Terminal cases for f
    if (f == bddtrue) return g;
    if (f == bddfalse) return h;

    // g == h
    if (g == h) return g;

    // Reduction to 2-operand when g or h is terminal
    // Use _rec versions for same-file calls
    if (g == bddtrue) return bddnot(bddand_rec(bddnot(f), bddnot(h)));
    if (g == bddfalse) return bddand_rec(bddnot(f), h);
    if (h == bddfalse) return bddand_rec(f, g);
    if (h == bddtrue) return bddnot(bddand_rec(f, bddnot(g)));

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

    // Post-normalization terminal check
    if (g == h) return comp ? bddnot(g) : g;

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
    bddp lo = bddite_rec(f_lo, g_lo, h_lo);
    bddp hi = bddite_rec(f_hi, g_hi, h_hi);

    bddp result = BDD::getnode_raw(top_var, lo, hi);

    // Cache write
    bddwcache3(BDD_OP_ITE, f, g, h, result);

    return comp ? bddnot(result) : result;
}

static bddp bddat0_rec(bddp f, bddvar v);

bddp bddat0(bddp f, bddvar v) {
    if (v < 1 || v > bdd_varcount) {
        throw std::invalid_argument("bddat0: var out of range");
    }
    if (f == bddnull) return bddnull;
    // Terminal case
    if (f & BDD_CONST_FLAG) return f;

    return bdd_gc_guard([&]() -> bddp { return bddat0_rec(f, v); });
}

static bddp bddat0_rec(bddp f, bddvar v) {
    BDD_RecurGuard guard;
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
        bddp lo = bddat0_rec(f_lo, v);
        bddp hi = bddat0_rec(f_hi, v);
        result = BDD::getnode_raw(f_var, lo, hi);
    }

    bddwcache(BDD_OP_AT0, f, static_cast<bddp>(v), result);
    return result;
}

static bddp bddat1_rec(bddp f, bddvar v);

bddp bddat1(bddp f, bddvar v) {
    if (v < 1 || v > bdd_varcount) {
        throw std::invalid_argument("bddat1: var out of range");
    }
    if (f == bddnull) return bddnull;
    // Terminal case
    if (f & BDD_CONST_FLAG) return f;

    return bdd_gc_guard([&]() -> bddp { return bddat1_rec(f, v); });
}

static bddp bddat1_rec(bddp f, bddvar v) {
    BDD_RecurGuard guard;
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
        bddp lo = bddat1_rec(f_lo, v);
        bddp hi = bddat1_rec(f_hi, v);
        result = BDD::getnode_raw(f_var, lo, hi);
    }

    bddwcache(BDD_OP_AT1, f, static_cast<bddp>(v), result);
    return result;
}

int bddimply(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return -1;
    // Terminal cases: is there an assignment where f=1 and g=0?
    if (f == bddfalse) return 1;   // f is never true
    if (g == bddtrue)  return 1;   // g is always true
    if (f == bddtrue && g == bddfalse) return 0;  // counterexample
    if (f == g) return 1;          // f implies f
    if (f == bddnot(g)) return 0;  // f=1 => g=0

    BDD_RecurGuard guard;

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
    int r_lo = bddimply(f_lo, g_lo);
    if (r_lo != 1) {
        if (r_lo == 0) bddwcache(BDD_OP_IMPLY, f, g, bddfalse);
        return r_lo;  // propagate 0 or -1
    }
    int result = bddimply(f_hi, g_hi);

    if (result >= 0) bddwcache(BDD_OP_IMPLY, f, g, result ? bddtrue : bddfalse);
    return result;
}

static void bddsupport_collect(bddp f, std::unordered_set<bddvar>& vars,
                               std::unordered_set<bddp>& visited) {
    std::vector<bddp> stack;
    stack.push_back(f);
    while (!stack.empty()) {
        bddp cur = stack.back();
        stack.pop_back();
        if (cur & BDD_CONST_FLAG) continue;
        bddp node = cur & ~BDD_COMP_FLAG;
        if (!visited.insert(node).second) continue;
        vars.insert(node_var(node));
        stack.push_back(node_lo(node));
        stack.push_back(node_hi(node));
    }
}

bddp bddsupport(bddp f) {
    if (f == bddnull) return bddnull;
    if (f & BDD_CONST_FLAG) return bddfalse;

    return bdd_gc_guard([&]() -> bddp {
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
            result = BDD::getnode_raw(vars[i], result, bddtrue);
        }
        return result;
    });
}

std::vector<bddvar> bddsupport_vec(bddp f) {
    std::vector<bddvar> vars;
    if (f == bddnull) return vars;
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

static bddp bddexist_rec(bddp f, bddp g);

bddp bddexist(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
    // Cube represents a variable set; complement is meaningless — strip it.
    g = g & ~BDD_COMP_FLAG;
    // Terminal cases
    if (g == bddfalse) return f;    // no variables to quantify
    if (f == bddfalse) return bddfalse;
    if (f == bddtrue) return bddtrue;

    return bdd_gc_guard([&]() -> bddp { return bddexist_rec(f, g); });
}

static bddp bddexist_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Cube represents a variable set; complement is meaningless — strip it.
    g = g & ~BDD_COMP_FLAG;
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
        result = bddexist_rec(f, node_lo(g));
    } else if (f_level > g_level) {
        // f's top variable is not being quantified, keep it
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp lo = bddexist_rec(f_lo, g);
        bddp hi = bddexist_rec(f_hi, g);
        result = BDD::getnode_raw(f_var, lo, hi);
    } else {
        // Same variable: quantify it out
        // exist v. f = f|_{v=0} OR f|_{v=1}
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp g_rest = node_lo(g);
        bddp lo_r = bddexist_rec(f_lo, g_rest);
        bddp hi_r = bddexist_rec(f_hi, g_rest);
        // bddor = ~bddand(~a, ~b), use _rec for same-file call
        result = bddnot(bddand_rec(bddnot(lo_r), bddnot(hi_r)));
    }

    bddwcache(BDD_OP_EXIST, f, g, result);
    return result;
}

static bddp vars_to_cube(const std::vector<bddvar>& vars) {
    for (size_t i = 0; i < vars.size(); i++) {
        if (vars[i] < 1 || vars[i] > bdd_varcount) {
            throw std::invalid_argument("bddexist/bdduniv: var out of range");
        }
    }
    // Build cube BDD: sort by level ascending, chain with hi=bddtrue
    std::vector<bddvar> sorted(vars);
    std::sort(sorted.begin(), sorted.end(), [](bddvar a, bddvar b) {
        return var2level[a] < var2level[b];
    });
    bddp cube = bddfalse;
    for (size_t i = 0; i < sorted.size(); i++) {
        cube = BDD::getnode_raw(sorted[i], cube, bddtrue);
    }
    return cube;
}

bddp bddexist(bddp f, const std::vector<bddvar>& vars) {
    if (f == bddnull) return bddnull;
    return bdd_gc_guard([&]() -> bddp {
        bddp cube = vars_to_cube(vars);
        return bddexist_rec(f, cube);
    });
}

bddp bddexistvar(bddp f, bddvar v) {
    return bddexist(f, bddprime(v));
}

static bddp bdduniv_rec(bddp f, bddp g);

bddp bdduniv(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
    // Cube represents a variable set; complement is meaningless — strip it.
    g = g & ~BDD_COMP_FLAG;
    // Terminal cases
    if (g == bddfalse) return f;
    if (f == bddfalse) return bddfalse;
    if (f == bddtrue) return bddtrue;

    return bdd_gc_guard([&]() -> bddp { return bdduniv_rec(f, g); });
}

static bddp bdduniv_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
    // Cube represents a variable set; complement is meaningless — strip it.
    g = g & ~BDD_COMP_FLAG;
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
        result = bdduniv_rec(f, node_lo(g));
    } else if (f_level > g_level) {
        // f's top variable is not being quantified, keep it
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp lo = bdduniv_rec(f_lo, g);
        bddp hi = bdduniv_rec(f_hi, g);
        result = BDD::getnode_raw(f_var, lo, hi);
    } else {
        // Same variable: quantify it universally
        // forall v. f = f|_{v=0} AND f|_{v=1}
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp g_rest = node_lo(g);
        bddp lo_r = bdduniv_rec(f_lo, g_rest);
        bddp hi_r = bdduniv_rec(f_hi, g_rest);
        // Use _rec for same-file call
        result = bddand_rec(lo_r, hi_r);
    }

    bddwcache(BDD_OP_UNIV, f, g, result);
    return result;
}

bddp bdduniv(bddp f, const std::vector<bddvar>& vars) {
    if (f == bddnull) return bddnull;
    return bdd_gc_guard([&]() -> bddp {
        bddp cube = vars_to_cube(vars);
        return bdduniv_rec(f, cube);
    });
}

bddp bddunivvar(bddp f, bddvar v) {
    return bdduniv(f, bddprime(v));
}

bddp bddlshiftb(bddp f, bddvar shift) {
    if (f == bddnull) return bddnull;
    if (f & BDD_CONST_FLAG) return f;
    if (shift == 0) return f;

    return bdd_gc_guard([&]() -> bddp {
        return bdd_lshift_core(f, shift, BDD_OP_LSHIFTB, BDD::getnode_raw);
    });
}

bddp bddrshiftb(bddp f, bddvar shift) {
    if (f == bddnull) return bddnull;
    if (f & BDD_CONST_FLAG) return f;
    if (shift == 0) return f;

    return bdd_gc_guard([&]() -> bddp {
        return bdd_rshift_core(f, shift, BDD_OP_RSHIFTB, BDD::getnode_raw);
    });
}

bddp bddlshift(bddp f, bddvar shift) {
    (void)f; (void)shift;
    throw std::logic_error("bddlshift is no longer supported. Use bddlshiftb (BDD) or bddlshiftz (ZDD).");
}

bddp bddrshift(bddp f, bddvar shift) {
    (void)f; (void)shift;
    throw std::logic_error("bddrshift is no longer supported. Use bddrshiftb (BDD) or bddrshiftz (ZDD).");
}

static bddp bddcofactor_rec(bddp f, bddp g);

bddp bddcofactor(bddp f, bddp g) {
    if (f == bddnull || g == bddnull) return bddnull;
    // Terminal cases
    if (f & BDD_CONST_FLAG) return f;   // f is constant
    if (g == bddfalse) return bddfalse; // care region is empty
    if (f == bddnot(g)) return bddfalse; // f=0 wherever g=1
    if (f == g) return bddtrue;          // f=1 wherever g=1
    if (g == bddtrue) return f;          // no don't care region

    return bdd_gc_guard([&]() -> bddp { return bddcofactor_rec(f, g); });
}

static bddp bddcofactor_rec(bddp f, bddp g) {
    BDD_RecurGuard guard;
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
        result = bddcofactor_rec(f_hi, g_hi);
    } else if (g_hi == bddfalse && g_lo != bddfalse) {
        // High branch is entirely don't care
        result = bddcofactor_rec(f_lo, g_lo);
    } else {
        bddp lo = bddcofactor_rec(f_lo, g_lo);
        bddp hi = bddcofactor_rec(f_hi, g_hi);
        result = BDD::getnode_raw(top_var, lo, hi);
    }

    bddwcache(BDD_OP_COFACTOR, f, g, result);
    return result;
}

// --- bddswap ---

static bddp bddswap_rec(bddp f, bddvar v1, bddvar v2,
                         bddvar lev1, bddvar lev2) {
    BDD_RecurGuard guard;

    // Terminal
    if (f & BDD_CONST_FLAG) return f;

    // Factor out complement: swap(~f, v1, v2) = ~swap(f, v1, v2)
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp fn = f & ~static_cast<bddp>(BDD_COMP_FLAG);

    bddvar f_var = node_var(fn);
    bddvar f_level = var2level[f_var];

    // Below both swap levels: f doesn't depend on v1 or v2
    if (f_level < lev2) {
        return f;
    }

    // Cache lookup (use non-complemented fn)
    bddp cache_key = (static_cast<bddp>(v1) << 31) | static_cast<bddp>(v2);
    bddp cached = bddrcache(BDD_OP_SWAP, fn, cache_key);
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    // fn is non-complemented, so node_lo is guaranteed non-complemented
    bddp f_lo = node_lo(fn);
    bddp f_hi = node_hi(fn);

    bddp result;

    if (f_var == v1) {
        // At the higher swap variable (level lev1)
        // Children may contain v2. Recurse, then reconstruct with v2.
        bddp r_lo = bddswap_rec(f_lo, v1, v2, lev1, lev2);
        bddp r_hi = bddswap_rec(f_hi, v1, v2, lev1, lev2);
        // What was controlled by v1 is now controlled by v2.
        // Use ITE to handle ordering correctly (children may contain
        // v1 nodes from v2->v1 relabeling at lev1 > lev2).
        result = bddite_rec(bddprime(v2), r_hi, r_lo);
    } else if (f_var == v2) {
        // At the lower swap variable (level lev2)
        // Children are below lev2, hence below lev1 too.
        // Neither v1 nor v2 appears in children. Just relabel v2 -> v1.
        result = BDD::getnode_raw(v1, f_lo, f_hi);
    } else if (f_level > lev1) {
        // Above both swap levels
        // After swap, children have max level <= lev1 < f_level. Safe.
        bddp r_lo = bddswap_rec(f_lo, v1, v2, lev1, lev2);
        bddp r_hi = bddswap_rec(f_hi, v1, v2, lev1, lev2);
        result = BDD::getnode_raw(f_var, r_lo, r_hi);
    } else {
        // Between swap levels: lev2 < f_level < lev1
        // Children may contain v2; after swap, v2->v1 at lev1 > f_level.
        // Must use ITE for correct ordering.
        bddp r_lo = bddswap_rec(f_lo, v1, v2, lev1, lev2);
        bddp r_hi = bddswap_rec(f_hi, v1, v2, lev1, lev2);
        result = bddite_rec(bddprime(f_var), r_hi, r_lo);
    }

    // Cache write (result is for non-complemented fn)
    bddwcache(BDD_OP_SWAP, fn, cache_key, result);

    return comp ? bddnot(result) : result;
}

bddp bddswap(bddp f, bddvar v1, bddvar v2) {
    if (f == bddnull) return bddnull;
    if (v1 == v2) return f;
    if (v1 < 1 || v1 > bdd_varcount || v2 < 1 || v2 > bdd_varcount) {
        throw std::invalid_argument("bddswap: variable out of range");
    }
    if (f & BDD_CONST_FLAG) return f;

    // Normalize: ensure lev1 > lev2 (v1 at higher level)
    bddvar lev1 = var2level[v1];
    bddvar lev2 = var2level[v2];
    if (lev1 < lev2) {
        bddvar tmp;
        tmp = v1; v1 = v2; v2 = tmp;
        tmp = lev1; lev1 = lev2; lev2 = tmp;
    }

    return bdd_gc_guard([&]() -> bddp {
        return bddswap_rec(f, v1, v2, lev1, lev2);
    });
}

// --- bddsmooth ---

static bddp bddsmooth_rec(bddp f, bddvar v) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) return f;

    bddvar t = node_var(f);
    bddvar t_level = var2level[t];
    bddvar v_level = var2level[v];

    if (t_level < v_level) return f;

    bddp cached = bddrcache(BDD_OP_SMOOTH, f, static_cast<bddp>(v));
    if (cached != bddnull) return cached;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_lo = node_lo(f);
    bddp f_hi = node_hi(f);
    if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }

    bddp result;
    if (t == v) {
        // Quantify: f|_{v=0} | f|_{v=1}
        result = bddnot(bddand_rec(bddnot(f_lo), bddnot(f_hi)));
    } else {
        bddp lo = bddsmooth_rec(f_lo, v);
        bddp hi = bddsmooth_rec(f_hi, v);
        result = BDD::getnode_raw(t, lo, hi);
    }

    bddwcache(BDD_OP_SMOOTH, f, static_cast<bddp>(v), result);
    return result;
}

bddp bddsmooth(bddp f, bddvar v) {
    if (f == bddnull) return bddnull;
    if (v < 1 || v > bdd_varcount) {
        throw std::invalid_argument("bddsmooth: var out of range");
    }
    if (f & BDD_CONST_FLAG) return f;

    return bdd_gc_guard([&]() -> bddp { return bddsmooth_rec(f, v); });
}

// --- bddspread ---

static bddp bddspread_rec(bddp f, int k) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) return f;
    if (k == 0) return f;

    bddp cached = bddrcache(BDD_OP_SPREAD, f, static_cast<bddp>(k));
    if (cached != bddnull) return cached;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar t = node_var(f);
    bddp f_lo = node_lo(f);
    bddp f_hi = node_hi(f);
    if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }

    bddp f0_spread_k = bddspread_rec(f_lo, k);
    bddp f1_spread_k = bddspread_rec(f_hi, k);
    bddp f0_spread_k1 = bddspread_rec(f_lo, k - 1);
    bddp f1_spread_k1 = bddspread_rec(f_hi, k - 1);

    // lo = f0.Spread(k) | f1.Spread(k-1)
    bddp lo = bddnot(bddand_rec(bddnot(f0_spread_k), bddnot(f1_spread_k1)));
    // hi = f1.Spread(k) | f0.Spread(k-1)
    bddp hi = bddnot(bddand_rec(bddnot(f1_spread_k), bddnot(f0_spread_k1)));

    bddp result = BDD::getnode_raw(t, lo, hi);

    bddwcache(BDD_OP_SPREAD, f, static_cast<bddp>(k), result);
    return result;
}

bddp bddspread(bddp f, int k) {
    if (f == bddnull) return bddnull;
    if (k < 0) {
        throw std::invalid_argument("bddspread: k must be >= 0");
    }
    if (k == 0) return f;
    if (f & BDD_CONST_FLAG) return f;

    return bdd_gc_guard([&]() -> bddp { return bddspread_rec(f, k); });
}

// --- BDD satisfiability counting ---

static double bddcount_bdd_rec(
    bddp f, bddvar n, std::unordered_map<bddp, double>& memo) {
    if (f == bddfalse) return 0.0;
    if (f == bddtrue) return 1.0;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    auto it = memo.find(f_raw);
    double count;
    if (it != memo.end()) {
        count = it->second;
    } else {
        bddvar var = node_var(f_raw);
        if (var > n) {
            throw std::invalid_argument(
                "bddcount: BDD contains variable > n");
        }
        bddvar f_level = var2level[var];

        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);

        bddvar lo_level = (lo & BDD_CONST_FLAG) ? 0
            : var2level[node_var(lo)];
        bddvar hi_level = (hi & BDD_CONST_FLAG) ? 0
            : var2level[node_var(hi & ~BDD_COMP_FLAG)];

        double lo_count = bddcount_bdd_rec(lo, n, memo);
        double hi_count = bddcount_bdd_rec(hi, n, memo);

        count = ldexp(lo_count, f_level - 1 - lo_level)
              + ldexp(hi_count, f_level - 1 - hi_level);

        memo[f_raw] = count;
    }

    if (comp) {
        bddvar f_level = var2level[node_var(f_raw)];
        count = ldexp(1.0, f_level) - count;
    }

    return count;
}

double bddcount(bddp f, bddvar n) {
    if (f == bddnull) return 0.0;
    if (f == bddfalse) return 0.0;
    if (f == bddtrue) return ldexp(1.0, n);

    bddp f_raw = f & ~BDD_COMP_FLAG;
    bddvar top_level = var2level[node_var(f_raw)];

    std::unordered_map<bddp, double> memo;
    double inner = bddcount_bdd_rec(f, n, memo);
    return ldexp(inner, n - top_level);
}

static bigint::BigInt bddexactcount_bdd_rec(
    bddp f, bddvar n,
    std::unordered_map<bddp, bigint::BigInt>& memo) {
    if (f == bddfalse) return bigint::BigInt(0);
    if (f == bddtrue) return bigint::BigInt(1);

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    auto it = memo.find(f_raw);
    bigint::BigInt count;
    if (it != memo.end()) {
        count = it->second;
    } else {
        bddvar var = node_var(f_raw);
        if (var > n) {
            throw std::invalid_argument(
                "bddexactcount: BDD contains variable > n");
        }
        bddvar f_level = var2level[var];

        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);

        bddvar lo_level = (lo & BDD_CONST_FLAG) ? 0
            : var2level[node_var(lo)];
        bddvar hi_level = (hi & BDD_CONST_FLAG) ? 0
            : var2level[node_var(hi & ~BDD_COMP_FLAG)];

        bigint::BigInt lo_count = bddexactcount_bdd_rec(lo, n, memo);
        bigint::BigInt hi_count = bddexactcount_bdd_rec(hi, n, memo);

        count = (lo_count << static_cast<std::size_t>(f_level - 1 - lo_level))
              + (hi_count << static_cast<std::size_t>(f_level - 1 - hi_level));

        memo[f_raw] = count;
    }

    if (comp) {
        bddvar f_level = var2level[node_var(f_raw)];
        count = (bigint::BigInt(1) << static_cast<std::size_t>(f_level))
              - count;
    }

    return count;
}

bigint::BigInt bddexactcount(bddp f, bddvar n) {
    if (f == bddnull) return bigint::BigInt(0);
    if (f == bddfalse) return bigint::BigInt(0);
    if (f == bddtrue) return bigint::BigInt(1) << static_cast<std::size_t>(n);

    bddp f_raw = f & ~BDD_COMP_FLAG;
    bddvar top_level = var2level[node_var(f_raw)];

    std::unordered_map<bddp, bigint::BigInt> memo;
    bigint::BigInt inner = bddexactcount_bdd_rec(f, n, memo);
    return inner << static_cast<std::size_t>(n - top_level);
}

bigint::BigInt bddexactcount(bddp f, bddvar n, CountMemoMap& memo) {
    if (f == bddnull) return bigint::BigInt(0);
    if (f == bddfalse) return bigint::BigInt(0);
    if (f == bddtrue) return bigint::BigInt(1) << static_cast<std::size_t>(n);

    bddp f_raw = f & ~BDD_COMP_FLAG;
    bddvar top_level = var2level[node_var(f_raw)];

    bigint::BigInt inner = bddexactcount_bdd_rec(f, n, memo);
    return inner << static_cast<std::size_t>(n - top_level);
}
