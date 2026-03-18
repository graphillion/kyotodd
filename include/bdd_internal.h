#ifndef KYOTODD_BDD_INTERNAL_H
#define KYOTODD_BDD_INTERNAL_H

#include "bdd_types.h"
#include <stdexcept>
#include <functional>

// --- Debug assertion ---
#ifndef NDEBUG
#define BDD_DEBUG_ASSERT(cond) \
    do { if (!(cond)) throw std::logic_error("BDD assertion failed: " #cond); } while(0)
#else
#define BDD_DEBUG_ASSERT(cond) ((void)0)
#endif

// --- Recursion depth guard ---
struct BDD_RecurGuard {
    BDD_RecurGuard() {
        if (++BDD_RecurCount > BDD_RecurLimit) {
            --BDD_RecurCount;
            throw std::overflow_error("BDD recursion depth limit exceeded");
        }
    }
    ~BDD_RecurGuard() { --BDD_RecurCount; }
};

// --- GC guard ---
extern int bdd_gc_depth;
int bddgc();
bool bdd_should_gc();

struct BDD_GCDepthGuard {
    BDD_GCDepthGuard() { ++bdd_gc_depth; }
    ~BDD_GCDepthGuard() { --bdd_gc_depth; }
};

template<typename F>
bddp bdd_gc_guard(F func) {
    bool is_outermost = (bdd_gc_depth == 0);
    if (is_outermost && bdd_should_gc()) bddgc();
    for (int attempt = 0; ; attempt++) {
        try {
            BDD_GCDepthGuard depth_guard;
            bddp result = func();
            return result;
        } catch (std::overflow_error&) {
            if (is_outermost && attempt == 0) {
                bddgc();
                continue;
            }
            throw;
        }
    }
}

// --- Node index validation ---
// Node ID must be a valid non-terminal, even ID >= 2 with index < bdd_node_used.
inline uint64_t node_index(bddp node_id) {
    node_id &= ~BDD_COMP_FLAG;
    BDD_DEBUG_ASSERT(node_id >= 2 && !(node_id & BDD_CONST_FLAG));
    uint64_t idx = node_id / 2 - 1;
    BDD_DEBUG_ASSERT(idx < bdd_node_used);
    return idx;
}

// --- Node write ---
// Node ID -> array index: node_id/2 - 1
inline void node_write(bddp node_id, bddvar var, bddp lo, bddp hi) {
    BddNode& n = bdd_nodes[node_index(node_id)];
    n.data[0] = (static_cast<uint64_t>(var) << BDD_NODE_VAR_SHIFT) | (lo >> BDD_NODE_LO_SPLIT);
    n.data[1] = ((lo & BDD_NODE_LO_LO_MASK) << BDD_NODE_LO_LO_SHIFT) | hi;
}

// --- Node reduced flag ---
inline void node_set_reduced(bddp node_id) {
    bdd_nodes[node_index(node_id)].data[0] |= BDD_NODE_REDUCED_FLAG;
}

inline bool node_is_reduced(bddp node_id) {
    return (bdd_nodes[node_index(node_id)].data[0] & BDD_NODE_REDUCED_FLAG) != 0;
}

// Check if a bddp (node ID or terminal) is reduced
inline bool bddp_is_reduced(bddp p) {
    if (p == bddnull) return false;                // bddnull is not a real node
    if (p & BDD_CONST_FLAG) return true;           // terminals are always reduced
    return node_is_reduced(p & ~BDD_COMP_FLAG);    // strip complement flag
}

// --- Node field extraction ---
// Node ID -> array index: node_id/2 - 1
// Complement flag is stripped internally so callers need not worry about it.
inline bddvar node_var(bddp node_id) {
    bddvar v = static_cast<bddvar>(bdd_nodes[node_index(node_id)].data[0] >> BDD_NODE_VAR_SHIFT);
    if (v < 1 || v > bdd_varcount) {
        throw std::out_of_range("node_var: stored variable number out of range");
    }
    return v;
}

inline bddp node_lo(bddp node_id) {
    const BddNode& n = bdd_nodes[node_index(node_id)];
    return ((n.data[0] & BDD_NODE_LO_HI_MASK) << BDD_NODE_LO_SPLIT) | (n.data[1] >> BDD_NODE_LO_LO_SHIFT);
}

inline bddp node_hi(bddp node_id) {
    const BddNode& n = bdd_nodes[node_index(node_id)];
    return n.data[1] & BDD_NODE_HI_MASK;
}

// --- Common lshift/rshift recursive templates ---
// MakeNode: bddp(bddvar, bddp, bddp) — getnode or getznode

template<typename MakeNode>
bddp bdd_lshift_core(bddp f, bddvar shift, uint8_t op, MakeNode make_node) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) return f;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp fn = f & ~BDD_COMP_FLAG;

    bddp cached = bddrcache(op, fn, static_cast<bddp>(shift));
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    bddvar v = node_var(fn);
    uint64_t new_level64 = static_cast<uint64_t>(var2level[v]) + shift;
    if (new_level64 > static_cast<uint64_t>(UINT32_MAX)) {
        throw std::invalid_argument("bddlshift: shifted level exceeds maximum variable count");
    }
    bddvar new_level = static_cast<bddvar>(new_level64);
    while (bdd_varcount < new_level) {
        bddnewvar();
    }
    bddvar target_var = level2var[new_level];

    bddp lo = bdd_lshift_core(node_lo(fn), shift, op, make_node);
    bddp hi = bdd_lshift_core(node_hi(fn), shift, op, make_node);

    bddp result = make_node(target_var, lo, hi);

    bddwcache(op, fn, static_cast<bddp>(shift), result);
    return comp ? bddnot(result) : result;
}

template<typename MakeNode>
bddp bdd_rshift_core(bddp f, bddvar shift, uint8_t op, MakeNode make_node) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) return f;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp fn = f & ~BDD_COMP_FLAG;

    bddp cached = bddrcache(op, fn, static_cast<bddp>(shift));
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    bddvar v = node_var(fn);
    bddvar lev = var2level[v];
    if (lev <= shift) {
        throw std::invalid_argument("bddrshift: shifted level underflows");
    }
    bddvar target_var = level2var[lev - shift];

    bddp lo = bdd_rshift_core(node_lo(fn), shift, op, make_node);
    bddp hi = bdd_rshift_core(node_hi(fn), shift, op, make_node);

    bddp result = make_node(target_var, lo, hi);

    bddwcache(op, fn, static_cast<bddp>(shift), result);
    return comp ? bddnot(result) : result;
}

#endif
