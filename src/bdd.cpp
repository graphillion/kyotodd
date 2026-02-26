#include "bdd.h"
#include <cstdlib>
#include <cstring>

static const uint32_t VAR_INITIAL_CAPACITY = 8192;
static const uint64_t UNIQUE_TABLE_INITIAL_CAPACITY = 1024;

BddNode* bdd_nodes = nullptr;
uint64_t bdd_node_count = 0;
uint64_t bdd_node_max = 0;

uint32_t* var2level = nullptr;
uint32_t* level2var = nullptr;
uint32_t bdd_varcount = 0;
static uint32_t var_capacity = 0;

BddUniqueTable* bdd_unique_tables = nullptr;

// --- Node field extraction ---
// Node ID -> array index: node_id/2 - 1
static inline bddp node_lo(bddp node_id) {
    const BddNode& n = bdd_nodes[node_id / 2 - 1];
    return ((n.data[0] & UINT64_C(0xFFFFFFFF)) << 16) | (n.data[1] >> 48);
}

static inline bddp node_hi(bddp node_id) {
    const BddNode& n = bdd_nodes[node_id / 2 - 1];
    return n.data[1] & UINT64_C(0x0000FFFFFFFFFFFF);
}

// --- Hash function (splitmix64 style) ---
static inline uint64_t unique_table_hash(bddp lo, bddp hi, uint64_t capacity) {
    uint64_t k = lo ^ (hi * UINT64_C(0x9E3779B97F4A7C15));
    k ^= k >> 30;
    k *= UINT64_C(0xBF58476D1CE4E5B9);
    k ^= k >> 27;
    k *= UINT64_C(0x94D049BB133111EB);
    k ^= k >> 31;
    return k & (capacity - 1);
}

// --- Unique table helpers ---
static void unique_table_init(BddUniqueTable* t) {
    t->capacity = UNIQUE_TABLE_INITIAL_CAPACITY;
    t->count = 0;
    t->slots = static_cast<bddp*>(std::calloc(t->capacity, sizeof(bddp)));
}

static void unique_table_insert_raw(bddp* slots, uint64_t capacity,
                                    bddp lo, bddp hi, bddp node_id) {
    uint64_t idx = unique_table_hash(lo, hi, capacity);
    while (slots[idx] != 0) {
        idx = (idx + 1) & (capacity - 1);
    }
    slots[idx] = node_id;
}

static void unique_table_resize(BddUniqueTable* t) {
    uint64_t old_capacity = t->capacity;
    bddp* old_slots = t->slots;
    uint64_t new_capacity = old_capacity * 2;
    bddp* new_slots = static_cast<bddp*>(std::calloc(new_capacity, sizeof(bddp)));
    for (uint64_t i = 0; i < old_capacity; i++) {
        if (old_slots[i] != 0) {
            bddp nid = old_slots[i];
            unique_table_insert_raw(new_slots, new_capacity, node_lo(nid), node_hi(nid), nid);
        }
    }
    t->slots = new_slots;
    t->capacity = new_capacity;
    std::free(old_slots);
}

// --- Public API ---
void BDD_Init(uint64_t node_count, uint64_t node_max) {
    bdd_node_count = node_count;
    bdd_node_max = node_max;
    bdd_nodes = static_cast<BddNode*>(std::malloc(sizeof(BddNode) * node_count));
}

uint32_t BDD_NewVar() {
    bdd_varcount++;
    uint32_t var = bdd_varcount;
    if (var >= var_capacity) {
        uint32_t old_capacity = var_capacity;
        var_capacity = (var_capacity == 0) ? VAR_INITIAL_CAPACITY : var_capacity * 2;
        var2level = static_cast<uint32_t*>(std::realloc(var2level, sizeof(uint32_t) * var_capacity));
        level2var = static_cast<uint32_t*>(std::realloc(level2var, sizeof(uint32_t) * var_capacity));
        bdd_unique_tables = static_cast<BddUniqueTable*>(
            std::realloc(bdd_unique_tables, sizeof(BddUniqueTable) * var_capacity));
        std::memset(bdd_unique_tables + old_capacity, 0,
                    sizeof(BddUniqueTable) * (var_capacity - old_capacity));
    }
    var2level[var] = var;  // var i <-> level i
    level2var[var] = var;
    unique_table_init(&bdd_unique_tables[var]);
    return var;
}

bddp BDD_UniqueTableLookup(uint32_t var, bddp lo, bddp hi) {
    BddUniqueTable* t = &bdd_unique_tables[var];
    uint64_t idx = unique_table_hash(lo, hi, t->capacity);
    while (t->slots[idx] != 0) {
        bddp nid = t->slots[idx];
        if (node_lo(nid) == lo && node_hi(nid) == hi) {
            return nid;
        }
        idx = (idx + 1) & (t->capacity - 1);
    }
    return 0;
}

void BDD_UniqueTableInsert(uint32_t var, bddp lo, bddp hi, bddp node_id) {
    BddUniqueTable* t = &bdd_unique_tables[var];
    if ((t->count + 1) * 4 >= t->capacity * 3) {
        unique_table_resize(t);
    }
    unique_table_insert_raw(t->slots, t->capacity, lo, hi, node_id);
    t->count++;
}
