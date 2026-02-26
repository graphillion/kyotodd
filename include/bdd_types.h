#ifndef KYOTODD_BDD_TYPES_H
#define KYOTODD_BDD_TYPES_H

#include <cstdint>
#include <vector>
#include "bdd_node.h"

// DD node ID type (48-bit value stored in uint64_t)
typedef uint64_t bddp;

// Variable number type (31-bit value stored in uint32_t)
typedef uint32_t bddvar;

// Node ID encoding constants
static const bddp BDD_CONST_FLAG = UINT64_C(0x800000000000);  // bit 47: constant flag
static const bddp BDD_COMP_FLAG  = UINT64_C(0x000000000001);  // bit 0: complement flag

// Terminal node constants (bit 47 = 1, remaining bits = constant value)
static const bddp bddfalse  = BDD_CONST_FLAG | 0;  // 0-terminal
static const bddp bddempty  = BDD_CONST_FLAG | 0;  // 0-terminal (ZDD alias)
static const bddp bddtrue   = BDD_CONST_FLAG | 1;  // 1-terminal
static const bddp bddsingle = BDD_CONST_FLAG | 1;  // 1-terminal (ZDD alias)
static const bddp bddnull   = UINT64_C(0x7FFFFFFFFFFF);  // error

extern BddNode* bdd_nodes;
extern uint64_t bdd_node_count;  // allocated capacity
extern uint64_t bdd_node_used;   // number of nodes in use
extern uint64_t bdd_node_max;

// Variable-level mapping
// Level 0 = terminal, level i = var i (initial ordering: var i <-> level i)
extern bddvar* var2level;  // var2level[var] = level
extern bddvar* level2var;  // level2var[level] = var
extern bddvar bdd_varcount;

// Per-variable unique table (open addressing, linear probing)
struct BddUniqueTable {
    bddp*    slots;    // array of node IDs; 0 = empty
    uint64_t capacity; // always power of 2
    uint64_t count;    // occupied slots
};

extern BddUniqueTable* bdd_unique_tables;  // indexed by var (1-based)

// Operation cache (computed table) -- fixed-size, lossy
struct BddCacheEntry {
    uint64_t fop;     // bits [55:48] = op, bits [47:0] = f
    uint64_t g;       // bits [47:0] = g
    uint64_t h;       // bits [47:0] = h (0 for 2-operand ops)
    uint64_t result;  // cached result (bddnull = empty)
};

extern BddCacheEntry* bdd_cache;
extern uint64_t bdd_cache_size;  // number of entries (power of 2)

// Operation type constants
static const uint8_t BDD_OP_AND = 0;
static const uint8_t BDD_OP_XOR = 1;
static const uint8_t BDD_OP_AT0 = 2;
static const uint8_t BDD_OP_AT1 = 3;
static const uint8_t BDD_OP_ITE = 4;
static const uint8_t BDD_OP_IMPLY = 5;
static const uint8_t BDD_OP_EXIST = 6;
static const uint8_t BDD_OP_UNIV = 7;
static const uint8_t BDD_OP_LSHIFT = 8;
static const uint8_t BDD_OP_RSHIFT = 9;
static const uint8_t BDD_OP_COFACTOR = 10;
static const uint8_t BDD_OP_OFFSET = 11;
static const uint8_t BDD_OP_ONSET = 12;
static const uint8_t BDD_OP_ONSET0 = 13;
static const uint8_t BDD_OP_CHANGE = 14;
static const uint8_t BDD_OP_UNION = 15;
static const uint8_t BDD_OP_INTERSEC = 16;
static const uint8_t BDD_OP_SUBTRACT = 17;

class BDD {
public:
    bddp root;
    BDD(int val) : root(val < 0 ? bddnull : val == 0 ? bddfalse : bddtrue) {}
    BDD operator&(const BDD& other) const;
    BDD& operator&=(const BDD& other);
    static const BDD False;
    static const BDD True;
    static const BDD Null;
};

class ZDD {
public:
    bddp root;
    ZDD(int val) : root(val < 0 ? bddnull : val == 0 ? bddempty : bddsingle) {}
    static const ZDD Empty;
    static const ZDD Single;
    static const ZDD Null;
};

#endif
