#ifndef KYOTODD_BDD_H
#define KYOTODD_BDD_H

#include <cstdint>
#include "bdd_node.h"

// Terminal node constants (bit 47 = 1, remaining bits = constant value)
static const uint64_t bddfalse  = UINT64_C(0x800000000000);  // 0-terminal
static const uint64_t bddempty  = UINT64_C(0x800000000000);  // 0-terminal (ZDD alias)
static const uint64_t bddtrue   = UINT64_C(0x800000000001);  // 1-terminal
static const uint64_t bddsingle = UINT64_C(0x800000000001);  // 1-terminal (ZDD alias)
static const uint64_t bddnull   = UINT64_C(0x7FFFFFFFFFFF);  // error

extern BddNode* bdd_nodes;
extern uint64_t bdd_node_count;
extern uint64_t bdd_node_max;

// Variable-level mapping
// Level 0 = terminal, level i = var i (initial ordering: var i <-> level i)
extern uint32_t* var2level;  // var2level[var] = level
extern uint32_t* level2var;  // level2var[level] = var
extern uint32_t bdd_varcount;

// Per-variable unique table (open addressing, linear probing)
struct BddUniqueTable {
    uint64_t* slots;    // array of node IDs; 0 = empty
    uint64_t  capacity; // always power of 2
    uint64_t  count;    // occupied slots
};

extern BddUniqueTable* bdd_unique_tables;  // indexed by var (1-based)

class BDD {
public:
    uint64_t root;
};

class ZDD {
public:
    uint64_t root;
};

void BDD_Init(uint64_t node_count = 256, uint64_t node_max = UINT64_MAX);
uint32_t BDD_NewVar();
uint64_t BDD_UniqueTableLookup(uint32_t var, uint64_t lo, uint64_t hi);
void BDD_UniqueTableInsert(uint32_t var, uint64_t lo, uint64_t hi, uint64_t node_id);

#endif
