#ifndef KYOTODD_BDD_INTERNAL_H
#define KYOTODD_BDD_INTERNAL_H

#include "bdd_node.h"
#include <cstdint>

// Forward declarations for types used below
typedef uint64_t bddp;
typedef uint32_t bddvar;
static const bddp BDD_CONST_FLAG = UINT64_C(0x800000000000);
static const bddp BDD_COMP_FLAG  = UINT64_C(0x000000000001);

extern BddNode* bdd_nodes;

// --- Node write ---
// Node ID -> array index: node_id/2 - 1
inline void node_write(bddp node_id, bddvar var, bddp lo, bddp hi) {
    BddNode& n = bdd_nodes[node_id / 2 - 1];
    n.data[0] = (static_cast<uint64_t>(var) << BDD_NODE_VAR_SHIFT) | (lo >> BDD_NODE_LO_SPLIT);
    n.data[1] = ((lo & BDD_NODE_LO_LO_MASK) << BDD_NODE_LO_LO_SHIFT) | hi;
}

// --- Node reduced flag ---
inline void node_set_reduced(bddp node_id) {
    bdd_nodes[node_id / 2 - 1].data[0] |= BDD_NODE_REDUCED_FLAG;
}

inline bool node_is_reduced(bddp node_id) {
    return (bdd_nodes[node_id / 2 - 1].data[0] & BDD_NODE_REDUCED_FLAG) != 0;
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
    node_id &= ~BDD_COMP_FLAG;
    return static_cast<bddvar>(bdd_nodes[node_id / 2 - 1].data[0] >> BDD_NODE_VAR_SHIFT);
}

inline bddp node_lo(bddp node_id) {
    node_id &= ~BDD_COMP_FLAG;
    const BddNode& n = bdd_nodes[node_id / 2 - 1];
    return ((n.data[0] & BDD_NODE_LO_HI_MASK) << BDD_NODE_LO_SPLIT) | (n.data[1] >> BDD_NODE_LO_LO_SHIFT);
}

inline bddp node_hi(bddp node_id) {
    node_id &= ~BDD_COMP_FLAG;
    const BddNode& n = bdd_nodes[node_id / 2 - 1];
    return n.data[1] & BDD_NODE_HI_MASK;
}

#endif
