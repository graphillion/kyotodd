#ifndef KYOTODD_BDD_H
#define KYOTODD_BDD_H

#include <cstdint>
#include "bdd_node.h"

// DD node ID type (48-bit value stored in uint64_t)
typedef uint64_t bddp;

// Variable number type (31-bit value stored in uint32_t)
typedef uint32_t bddvar;

// Terminal node constants (bit 47 = 1, remaining bits = constant value)
static const bddp bddfalse  = UINT64_C(0x800000000000);  // 0-terminal
static const bddp bddempty  = UINT64_C(0x800000000000);  // 0-terminal (ZDD alias)
static const bddp bddtrue   = UINT64_C(0x800000000001);  // 1-terminal
static const bddp bddsingle = UINT64_C(0x800000000001);  // 1-terminal (ZDD alias)
static const bddp bddnull   = UINT64_C(0x7FFFFFFFFFFF);  // error

extern BddNode* bdd_nodes;
extern uint64_t bdd_node_count;
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

class BDD {
public:
    bddp root;
    BDD(int val) : root(val < 0 ? bddnull : val == 0 ? bddfalse : bddtrue) {}
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

void BDD_Init(uint64_t node_count = 256, uint64_t node_max = UINT64_MAX);
bddvar BDD_NewVar();
bddp BDD_UniqueTableLookup(bddvar var, bddp lo, bddp hi);
void BDD_UniqueTableInsert(bddvar var, bddp lo, bddp hi, bddp node_id);

#endif
