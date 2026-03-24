#ifndef KYOTODD_BDD_INTERNAL_H
#define KYOTODD_BDD_INTERNAL_H

#include "bdd_types.h"
#include <stdexcept>
#include <functional>

/**
 * @brief Debug assertion macro.
 *
 * In debug builds (NDEBUG not defined), throws std::logic_error if the
 * condition is false. In release builds, expands to a no-op.
 */
#ifndef NDEBUG
#define BDD_DEBUG_ASSERT(cond) \
    do { if (!(cond)) throw std::logic_error("BDD assertion failed: " #cond); } while(0)
#else
#define BDD_DEBUG_ASSERT(cond) ((void)0)
#endif

/**
 * @brief RAII guard that tracks recursion depth.
 *
 * Increments BDD_RecurCount on construction, decrements on destruction.
 * Throws std::overflow_error if the recursion limit is exceeded.
 */
struct BDD_RecurGuard {
    BDD_RecurGuard() {
        if (++BDD_RecurCount > BDD_RecurLimit) {
            --BDD_RecurCount;
            throw std::overflow_error("BDD recursion depth limit exceeded");
        }
    }
    ~BDD_RecurGuard() { --BDD_RecurCount; }
};

/** @brief Current GC nesting depth. GC is suppressed when > 0. */
extern int bdd_gc_depth;
/** @brief GC generation counter. Incremented each time GC actually runs. */
extern uint64_t bdd_gc_generation;
/** @brief Run garbage collection. No-op if bdd_gc_depth > 0. */
int bddgc();
/**
 * @brief Check whether GC should be triggered.
 * @return true if the node table usage exceeds the GC threshold.
 */
bool bdd_should_gc();

/**
 * @brief RAII guard that increments/decrements bdd_gc_depth.
 *
 * While bdd_gc_depth > 0, GC invocations are suppressed to prevent
 * collecting nodes that are in use by a recursive operation.
 */
struct BDD_GCDepthGuard {
    BDD_GCDepthGuard() { ++bdd_gc_depth; }
    ~BDD_GCDepthGuard() { --bdd_gc_depth; }
};

/**
 * @brief Execute a function with GC safety.
 *
 * At the outermost level (bdd_gc_depth == 0): pre-checks GC, increments
 * depth, executes the lambda, catches std::overflow_error for retry
 * after GC. At depth > 0: just increments/decrements depth.
 *
 * @tparam F Callable returning bddp.
 * @param func The operation to execute.
 * @return The bddp result of the operation.
 * @throws std::overflow_error if GC retry also fails.
 */
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

/**
 * @brief Validate that a bddp value is a well-formed node reference.
 *
 * Returns true if f is bddnull, a constant (terminal), or a valid
 * non-terminal node ID that falls within the allocated node array.
 * Does NOT check whether the node is live (not garbage-collected).
 *
 * @param f The node ID to validate.
 * @return true if the ID is structurally valid.
 */
inline bool bddp_is_valid(bddp f) {
    if (f == bddnull) return true;
    if (f & BDD_CONST_FLAG) return true;
    bddp fn = f & ~BDD_COMP_FLAG;
    if (fn < 2) return false;
    uint64_t idx = fn / 2 - 1;
    return idx < bdd_node_used;
}

/**
 * @brief Throw std::invalid_argument if f is not a valid bddp.
 *
 * @param f    The node ID to validate.
 * @param func Name of the calling function (for error message).
 */
inline void bddp_validate(bddp f, const char* func) {
    if (!bddp_is_valid(f)) {
        throw std::invalid_argument(
            std::string(func) + ": invalid node ID");
    }
}

/**
 * @brief Convert a node ID to its array index.
 *
 * Strips the complement flag and computes the index into bdd_nodes[].
 * Node ID 2 maps to index 0, node ID 4 to index 1, etc.
 *
 * @param node_id The node ID (complement flag is stripped internally).
 * @return The array index.
 */
inline uint64_t node_index(bddp node_id) {
    node_id &= ~BDD_COMP_FLAG;
    BDD_DEBUG_ASSERT(node_id >= 2 && !(node_id & BDD_CONST_FLAG));
    uint64_t idx = node_id / 2 - 1;
    BDD_DEBUG_ASSERT(idx < bdd_node_used);
    return idx;
}

/**
 * @brief Write variable, lo, and hi into a node's data fields.
 *
 * Encodes the (var, lo, hi) triple into the node at bdd_nodes[node_id/2 - 1].
 *
 * @param node_id The node ID to write to (must be even, non-terminal).
 * @param var     Variable number to store.
 * @param lo      Low (0-edge) child node ID.
 * @param hi      High (1-edge) child node ID.
 */
inline void node_write(bddp node_id, bddvar var, bddp lo, bddp hi) {
    BddNode& n = bdd_nodes[node_index(node_id)];
    n.data[0] = (static_cast<uint64_t>(var) << BDD_NODE_VAR_SHIFT) | (lo >> BDD_NODE_LO_SPLIT);
    n.data[1] = ((lo & BDD_NODE_LO_LO_MASK) << BDD_NODE_LO_LO_SHIFT) | hi;
}

/**
 * @brief Set the reduced flag on a node.
 * @param node_id The node ID (complement flag is stripped internally).
 */
inline void node_set_reduced(bddp node_id) {
    bdd_nodes[node_index(node_id)].data[0] |= BDD_NODE_REDUCED_FLAG;
}

/**
 * @brief Check if the reduced flag is set on a node.
 * @param node_id The node ID (complement flag is stripped internally).
 * @return true if the node is marked as reduced.
 */
inline bool node_is_reduced(bddp node_id) {
    return (bdd_nodes[node_index(node_id)].data[0] & BDD_NODE_REDUCED_FLAG) != 0;
}

/**
 * @brief Check if a bddp value (node ID or terminal) is reduced.
 *
 * Terminal nodes are always reduced. Null returns false.
 * For regular nodes, checks the reduced flag.
 *
 * @param p A bddp value.
 * @return true if the value represents a reduced node.
 */
inline bool bddp_is_reduced(bddp p) {
    if (p == bddnull) return false;                // bddnull is not a real node
    if (p & BDD_CONST_FLAG) return true;           // terminals are always reduced
    return node_is_reduced(p & ~BDD_COMP_FLAG);    // strip complement flag
}

/**
 * @brief Extract the variable number from a node.
 *
 * Complement flag is stripped internally so callers need not worry about it.
 *
 * @param node_id The node ID.
 * @return The variable number stored in the node.
 * @throws std::out_of_range if the stored variable is invalid.
 */
inline bddvar node_var(bddp node_id) {
    bddvar v = static_cast<bddvar>(bdd_nodes[node_index(node_id)].data[0] >> BDD_NODE_VAR_SHIFT);
    if (v < 1 || v > bdd_varcount) {
        throw std::out_of_range("node_var: stored variable number out of range");
    }
    return v;
}

/**
 * @brief Extract the raw lo (0-edge) child from a node.
 *
 * Returns the stored lo child without complement edge resolution.
 *
 * @param node_id The node ID (complement flag is stripped internally).
 * @return The lo child node ID.
 */
inline bddp node_lo(bddp node_id) {
    const BddNode& n = bdd_nodes[node_index(node_id)];
    return ((n.data[0] & BDD_NODE_LO_HI_MASK) << BDD_NODE_LO_SPLIT) | (n.data[1] >> BDD_NODE_LO_LO_SHIFT);
}

/**
 * @brief Extract the raw hi (1-edge) child from a node.
 *
 * Returns the stored hi child without complement edge resolution.
 *
 * @param node_id The node ID (complement flag is stripped internally).
 * @return The hi child node ID.
 */
inline bddp node_hi(bddp node_id) {
    const BddNode& n = bdd_nodes[node_index(node_id)];
    return n.data[1] & BDD_NODE_HI_MASK;
}

/**
 * @brief Core implementation of left shift for BDD/ZDD.
 *
 * Recursively shifts all variable levels upward by the given amount.
 * Creates new variables if the shifted level exceeds the current maximum.
 *
 * @tparam MakeNode Callable with signature bddp(bddvar, bddp, bddp)
 *                  (e.g., BDD::getnode or ZDD::getnode).
 * @param f         The root node ID.
 * @param shift     Number of levels to shift upward.
 * @param op        Cache operation code.
 * @param make_node Node creation function.
 * @return The shifted node ID.
 * @throws std::invalid_argument if the shifted level overflows.
 */
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

/**
 * @brief Core implementation of right shift for BDD/ZDD.
 *
 * Recursively shifts all variable levels downward by the given amount.
 *
 * @tparam MakeNode Callable with signature bddp(bddvar, bddp, bddp)
 *                  (e.g., BDD::getnode or ZDD::getnode).
 * @param f         The root node ID.
 * @param shift     Number of levels to shift downward.
 * @param op        Cache operation code.
 * @param make_node Node creation function.
 * @return The shifted node ID.
 * @throws std::invalid_argument if the shifted level underflows.
 */
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
