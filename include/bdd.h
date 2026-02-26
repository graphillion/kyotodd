#ifndef KYOTODD_BDD_H
#define KYOTODD_BDD_H

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

void bddinit(uint64_t node_count = 256, uint64_t node_max = UINT64_MAX);
inline void BDD_Init(uint64_t node_count = 256, uint64_t node_max = UINT64_MAX) { bddinit(node_count, node_max); }
bddvar bddnewvar();
inline bddvar BDD_NewVar() { return bddnewvar(); }
bddvar bddnewvaroflev(bddvar lev);
bddvar bddlevofvar(bddvar var);
bddvar bddvaroflev(bddvar lev);
bddvar bddvarused();
bddvar bddtop(bddp f);
inline bddp bddcopy(bddp f) { return f; }
void bddfree(bddp f);
uint64_t bddused();
uint64_t bddsize(bddp f);
uint64_t bddvsize(bddp* p, int lim);
uint64_t bddvsize(const std::vector<bddp>& v);

bddp BDD_UniqueTableLookup(bddvar var, bddp lo, bddp hi);
void BDD_UniqueTableInsert(bddvar var, bddp lo, bddp hi, bddp node_id);
bddp getnode(bddvar var, bddp lo, bddp hi);
bddp bddprime(bddvar v);
BDD BDD_ID(bddp p);
BDD BDDvar(bddvar v);

inline bddp bddnot(bddp p) { return p ^ BDD_COMP_FLAG; }
bddp bddand(bddp f, bddp g);
bddp bddor(bddp f, bddp g);
bddp bddxor(bddp f, bddp g);
bddp bddnand(bddp f, bddp g);
bddp bddnor(bddp f, bddp g);
bddp bddxnor(bddp f, bddp g);

bddp bddat0(bddp f, bddvar v);
bddp bddat1(bddp f, bddvar v);
bddp bddite(bddp f, bddp g, bddp h);

int bddimply(bddp f, bddp g);
bddp bddsupport(bddp f);
bddp bddexist(bddp f, bddp g);
bddp bdduniv(bddp f, bddp g);
bddp bddlshift(bddp f, bddvar shift);

bddp bddrcache(uint8_t op, bddp f, bddp g);
void bddwcache(uint8_t op, bddp f, bddp g, bddp result);
bddp bddrcache3(uint8_t op, bddp f, bddp g, bddp h);
void bddwcache3(uint8_t op, bddp f, bddp g, bddp h, bddp result);

inline BDD BDD::operator&(const BDD& other) const {
    BDD b(0);
    b.root = bddand(root, other.root);
    return b;
}

inline BDD& BDD::operator&=(const BDD& other) {
    root = bddand(root, other.root);
    return *this;
}

#endif
