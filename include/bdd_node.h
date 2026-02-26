#ifndef KYOTODD_BDD_NODE_H
#define KYOTODD_BDD_NODE_H

#include <cstdint>

// BDD node (16 bytes)
//
// Bit layout (user-specified order: var, reduced, lo, hi):
//   data[0] bits [63:33] : var     (31 bits) - variable number (1, 2, 3, ...)
//   data[0] bit  [32]    : reduced (1 bit)   - ignored in current version
//   data[0] bits [31:0]  : lo_hi   (32 bits) - upper 32 bits of 0-arc
//   data[1] bits [63:48] : lo_lo   (16 bits) - lower 16 bits of 0-arc
//   data[1] bits [47:0]  : hi      (48 bits) - 1-arc
//
// 0-arc = (lo_hi << 16) | lo_lo   (48 bits, even: 2, 4, 6, ...)
// 1-arc = hi                      (48 bits, even: 2, 4, 6, ...)
// Node IDs are even natural numbers: 2, 4, 6, ...
struct BddNode {
    uint64_t data[2];
};

static_assert(sizeof(BddNode) == 16, "BddNode must be 16 bytes");

#endif
