#include "bdd.h"
#include <cstdlib>
#include <cstring>
#include <stdexcept>

static const bddvar VAR_INITIAL_CAPACITY = 8192;
static const uint64_t UNIQUE_TABLE_INITIAL_CAPACITY = 1024;

static uint64_t next_power_of_2(uint64_t n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n |= n >> 32;
    return n + 1;
}

BddNode* bdd_nodes = nullptr;
uint64_t bdd_node_count = 0;
uint64_t bdd_node_used = 0;
uint64_t bdd_node_max = 0;

bddvar* var2level = nullptr;
bddvar* level2var = nullptr;
bddvar bdd_varcount = 0;
static bddvar var_capacity = 0;

BddUniqueTable* bdd_unique_tables = nullptr;

BddCacheEntry* bdd_cache = nullptr;
uint64_t bdd_cache_size = 0;

const BDD BDD::False(0);
const BDD BDD::True(1);
const BDD BDD::Null(-1);

const ZDD ZDD::Empty(0);
const ZDD ZDD::Single(1);
const ZDD ZDD::Null(-1);

// --- Node write ---
// Node ID -> array index: node_id/2 - 1
static inline void node_write(bddp node_id, bddvar var, bddp lo, bddp hi) {
    BddNode& n = bdd_nodes[node_id / 2 - 1];
    n.data[0] = (static_cast<uint64_t>(var) << BDD_NODE_VAR_SHIFT) | (lo >> BDD_NODE_LO_SPLIT);
    n.data[1] = ((lo & BDD_NODE_LO_LO_MASK) << BDD_NODE_LO_LO_SHIFT) | hi;
}

// --- Node reduced flag ---
static inline void node_set_reduced(bddp node_id) {
    bdd_nodes[node_id / 2 - 1].data[0] |= BDD_NODE_REDUCED_FLAG;
}

static inline bool node_is_reduced(bddp node_id) {
    return (bdd_nodes[node_id / 2 - 1].data[0] & BDD_NODE_REDUCED_FLAG) != 0;
}

// Check if a bddp (node ID or terminal) is reduced
static inline bool bddp_is_reduced(bddp p) {
    if (p & BDD_CONST_FLAG) return true;          // terminals are always reduced
    return node_is_reduced(p & ~BDD_COMP_FLAG);    // strip complement flag
}

// --- Node field extraction ---
// Node ID -> array index: node_id/2 - 1
static inline bddvar node_var(bddp node_id) {
    return static_cast<bddvar>(bdd_nodes[node_id / 2 - 1].data[0] >> BDD_NODE_VAR_SHIFT);
}

static inline bddp node_lo(bddp node_id) {
    const BddNode& n = bdd_nodes[node_id / 2 - 1];
    return ((n.data[0] & BDD_NODE_LO_HI_MASK) << BDD_NODE_LO_SPLIT) | (n.data[1] >> BDD_NODE_LO_LO_SHIFT);
}

static inline bddp node_hi(bddp node_id) {
    const BddNode& n = bdd_nodes[node_id / 2 - 1];
    return n.data[1] & BDD_NODE_HI_MASK;
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

// --- Cache hash function (splitmix64 style) ---
static inline uint64_t cache_hash(uint8_t op, bddp f, bddp g) {
    uint64_t k = f ^ (g * UINT64_C(0x9E3779B97F4A7C15));
    k ^= static_cast<uint64_t>(op) * UINT64_C(0x517CC1B727220A95);
    k ^= k >> 30;
    k *= UINT64_C(0xBF58476D1CE4E5B9);
    k ^= k >> 27;
    k *= UINT64_C(0x94D049BB133111EB);
    k ^= k >> 31;
    return k & (bdd_cache_size - 1);
}

// --- Cache API ---
bddp bddrcache(uint8_t op, bddp f, bddp g) {
    uint64_t idx = cache_hash(op, f, g);
    BddCacheEntry& e = bdd_cache[idx];
    uint64_t fop = (static_cast<uint64_t>(op) << 48) | f;
    if (e.fop == fop && e.g == g) {
        return e.result;
    }
    return bddnull;
}

void bddwcache(uint8_t op, bddp f, bddp g, bddp result) {
    uint64_t idx = cache_hash(op, f, g);
    BddCacheEntry& e = bdd_cache[idx];
    e.fop = (static_cast<uint64_t>(op) << 48) | f;
    e.g = g;
    e.result = result;
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
void bddinit(uint64_t node_count, uint64_t node_max) {
    // Free previous allocations if re-initializing
    std::free(bdd_nodes);
    std::free(var2level);
    std::free(level2var);
    // Free unique tables
    for (bddvar i = 1; i <= bdd_varcount; i++) {
        std::free(bdd_unique_tables[i].slots);
    }
    std::free(bdd_unique_tables);
    std::free(bdd_cache);

    // Reset variable state
    var2level = nullptr;
    level2var = nullptr;
    bdd_varcount = 0;
    var_capacity = 0;
    bdd_unique_tables = nullptr;
    bdd_node_used = 0;

    bdd_node_count = node_count;
    bdd_node_max = node_max;
    bdd_nodes = static_cast<BddNode*>(std::malloc(sizeof(BddNode) * node_count));

    // Initialize operation cache
    bdd_cache_size = next_power_of_2(node_count);
    bdd_cache = static_cast<BddCacheEntry*>(
        std::malloc(sizeof(BddCacheEntry) * bdd_cache_size));
    for (uint64_t i = 0; i < bdd_cache_size; i++) {
        bdd_cache[i].fop = 0;
        bdd_cache[i].g = 0;
        bdd_cache[i].result = bddnull;
    }
}

bddvar bddnewvar() {
    bdd_varcount++;
    bddvar var = bdd_varcount;
    if (var >= var_capacity) {
        bddvar old_capacity = var_capacity;
        var_capacity = (var_capacity == 0) ? VAR_INITIAL_CAPACITY : var_capacity * 2;
        var2level = static_cast<bddvar*>(std::realloc(var2level, sizeof(bddvar) * var_capacity));
        level2var = static_cast<bddvar*>(std::realloc(level2var, sizeof(bddvar) * var_capacity));
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

bddvar bddnewvaroflev(bddvar lev) {
    if (lev < 1 || lev > bdd_varcount + 1) {
        throw std::invalid_argument("bddnewvaroflev: lev out of range");
    }

    // Create a new variable (allocates arrays, unique table, etc.)
    bddvar new_var = bddnewvar();

    // Shift levels >= lev up by 1 to make room
    // new_var was assigned level == new_var (== bdd_varcount) by bddnewvar
    // We need to reassign: all vars currently at level >= lev get level + 1
    for (bddvar v = 1; v < new_var; v++) {
        if (var2level[v] >= lev) {
            var2level[v]++;
        }
    }

    // The new variable gets the requested level
    var2level[new_var] = lev;

    // Rebuild level2var from var2level
    for (bddvar v = 1; v <= bdd_varcount; v++) {
        level2var[var2level[v]] = v;
    }

    return new_var;
}

bddvar bddlevofvar(bddvar var) {
    if (var < 1 || var > bdd_varcount) {
        throw std::invalid_argument("bddlevofvar: var out of range");
    }
    return var2level[var];
}

bddvar bddvaroflev(bddvar lev) {
    if (lev < 1 || lev > bdd_varcount) {
        throw std::invalid_argument("bddvaroflev: lev out of range");
    }
    return level2var[lev];
}

bddvar bddvarused() {
    return bdd_varcount;
}

bddp BDD_UniqueTableLookup(bddvar var, bddp lo, bddp hi) {
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

void BDD_UniqueTableInsert(bddvar var, bddp lo, bddp hi, bddp node_id) {
    BddUniqueTable* t = &bdd_unique_tables[var];
    if ((t->count + 1) * 4 >= t->capacity * 3) {
        unique_table_resize(t);
    }
    unique_table_insert_raw(t->slots, t->capacity, lo, hi, node_id);
    t->count++;
}

static void node_array_grow() {
    uint64_t new_count = bdd_node_count * 2;
    if (new_count > bdd_node_max) {
        new_count = bdd_node_max;
    }
    bdd_nodes = static_cast<BddNode*>(std::realloc(bdd_nodes, sizeof(BddNode) * new_count));
    bdd_node_count = new_count;
}

bddp getnode(bddvar var, bddp lo, bddp hi) {
    if (lo == hi) return lo;  // reduction rule
    bddp found = BDD_UniqueTableLookup(var, lo, hi);
    if (found != 0) {
        return found;
    }
    if (bdd_node_used >= bdd_node_count) {
        node_array_grow();
    }
    bdd_node_used++;
    bddp node_id = bdd_node_used * 2;  // index = node_id/2 - 1 = bdd_node_used - 1
    node_write(node_id, var, lo, hi);
    if (bddp_is_reduced(lo) && bddp_is_reduced(hi)) {
        node_set_reduced(node_id);
    }
    BDD_UniqueTableInsert(var, lo, hi, node_id);
    return node_id;
}

bddp bddprime(bddvar v) {
    return getnode(v, bddfalse, bddtrue);
}

BDD BDD_ID(bddp p) {
    BDD b(0);
    b.root = p;
    return b;
}

BDD BDDvar(bddvar v) {
    return BDD_ID(bddprime(v));
}

bddp bddand(bddp f, bddp g) {
    // Terminal cases
    if (f == bddfalse || g == bddfalse) return bddfalse;
    if (f == bddtrue) return g;
    if (g == bddtrue) return f;
    if (f == g) return f;
    if (f == bddnot(g)) return bddfalse;

    // Normalize operand order (AND is commutative)
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_AND, f, g);
    if (cached != bddnull) return cached;

    // Both f and g are non-terminal at this point
    bddp f_raw = f & ~BDD_COMP_FLAG;
    bddp g_raw = g & ~BDD_COMP_FLAG;
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = node_var(f_raw);
    bddvar g_var = node_var(g_raw);
    bddvar f_level = var2level[f_var];
    bddvar g_level = var2level[g_var];

    // Determine top variable (highest level) and cofactors
    bddvar top_var;
    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        top_var = f_var;
        f_lo = node_lo(f_raw);
        f_hi = node_hi(f_raw);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = g;
        g_hi = g;
    } else if (g_level > f_level) {
        top_var = g_var;
        f_lo = f;
        f_hi = f;
        g_lo = node_lo(g_raw);
        g_hi = node_hi(g_raw);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    } else {
        top_var = f_var;
        f_lo = node_lo(f_raw);
        f_hi = node_hi(f_raw);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = node_lo(g_raw);
        g_hi = node_hi(g_raw);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    }

    // Recurse
    bddp lo = bddand(f_lo, g_lo);
    bddp hi = bddand(f_hi, g_hi);

    bddp result = getnode(top_var, lo, hi);

    // Cache insert
    bddwcache(BDD_OP_AND, f, g, result);

    return result;
}
