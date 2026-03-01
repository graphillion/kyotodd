#ifndef KYOTODD_BDD_INTERNAL_H
#define KYOTODD_BDD_INTERNAL_H

#include "bdd_types.h"
#include <stdexcept>

// --- Node index validation ---
// Node ID must be a valid non-terminal, even ID >= 2 with index < bdd_node_used.
inline uint64_t node_index(bddp node_id) {
    node_id &= ~BDD_COMP_FLAG;
    if (node_id < 2 || (node_id & BDD_CONST_FLAG)) {
        throw std::out_of_range("node access: invalid node ID");
    }
    uint64_t idx = node_id / 2 - 1;
    if (idx >= bdd_node_used) {
        throw std::out_of_range("node access: node ID out of bounds");
    }
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
    if (p & BDD_CONST_FLAG) return true;          // terminals are always reduced
    return node_is_reduced(p & ~BDD_COMP_FLAG);    // strip complement flag
}

// --- Node field extraction ---
// Node ID -> array index: node_id/2 - 1
// Complement flag is stripped internally so callers need not worry about it.
inline bddvar node_var(bddp node_id) {
    return static_cast<bddvar>(bdd_nodes[node_index(node_id)].data[0] >> BDD_NODE_VAR_SHIFT);
}

inline bddp node_lo(bddp node_id) {
    const BddNode& n = bdd_nodes[node_index(node_id)];
    return ((n.data[0] & BDD_NODE_LO_HI_MASK) << BDD_NODE_LO_SPLIT) | (n.data[1] >> BDD_NODE_LO_LO_SHIFT);
}

inline bddp node_hi(bddp node_id) {
    const BddNode& n = bdd_nodes[node_index(node_id)];
    return n.data[1] & BDD_NODE_HI_MASK;
}

#endif
