#include "bdd.h"
#include "bdd_internal.h"
#include <cstdlib>
#include <cstring>
#include <new>
#include <stdexcept>
#include <unordered_set>

static const bddvar VAR_INITIAL_CAPACITY = 8192;
static const uint64_t UNIQUE_TABLE_INITIAL_CAPACITY = 1024;

static uint64_t next_power_of_2(uint64_t n) {
    if (n == 0) return 1;
    if (n > (UINT64_C(1) << 63)) return UINT64_C(1) << 63;
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

static inline uint64_t cache_hash3(uint8_t op, bddp f, bddp g, bddp h) {
    uint64_t k = f ^ (g * UINT64_C(0x9E3779B97F4A7C15));
    k ^= h * UINT64_C(0x517CC1B727220A95);
    k ^= static_cast<uint64_t>(op) * UINT64_C(0x9E3779B97F4A7C55);
    k ^= k >> 30;
    k *= UINT64_C(0xBF58476D1CE4E5B9);
    k ^= k >> 27;
    k *= UINT64_C(0x94D049BB133111EB);
    k ^= k >> 31;
    return k & (bdd_cache_size - 1);
}

// --- Cache API (2-operand) ---
bddp bddrcache(uint8_t op, bddp f, bddp g) {
    if (bdd_cache_size == 0 || !bdd_cache) return bddnull;
    uint64_t idx = cache_hash(op, f, g);
    BddCacheEntry& e = bdd_cache[idx];
    uint64_t fop = (static_cast<uint64_t>(op) << 48) | f;
    if (e.fop == fop && e.g == g && e.h == 0) {
        return e.result;
    }
    return bddnull;
}

void bddwcache(uint8_t op, bddp f, bddp g, bddp result) {
    if (bdd_cache_size == 0 || !bdd_cache) return;
    uint64_t idx = cache_hash(op, f, g);
    BddCacheEntry& e = bdd_cache[idx];
    e.fop = (static_cast<uint64_t>(op) << 48) | f;
    e.g = g;
    e.h = 0;
    e.result = result;
}

// --- Cache API (3-operand) ---
bddp bddrcache3(uint8_t op, bddp f, bddp g, bddp h) {
    if (bdd_cache_size == 0 || !bdd_cache) return bddnull;
    uint64_t idx = cache_hash3(op, f, g, h);
    BddCacheEntry& e = bdd_cache[idx];
    uint64_t fop = (static_cast<uint64_t>(op) << 48) | f;
    if (e.fop == fop && e.g == g && e.h == h) {
        return e.result;
    }
    return bddnull;
}

void bddwcache3(uint8_t op, bddp f, bddp g, bddp h, bddp result) {
    if (bdd_cache_size == 0 || !bdd_cache) return;
    uint64_t idx = cache_hash3(op, f, g, h);
    BddCacheEntry& e = bdd_cache[idx];
    e.fop = (static_cast<uint64_t>(op) << 48) | f;
    e.g = g;
    e.h = h;
    e.result = result;
}

// --- Unique table helpers ---
static void unique_table_init(BddUniqueTable* t) {
    t->capacity = UNIQUE_TABLE_INITIAL_CAPACITY;
    t->count = 0;
    t->slots = static_cast<bddp*>(std::calloc(t->capacity, sizeof(bddp)));
    if (!t->slots) {
        throw std::bad_alloc();
    }
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
    if (old_capacity > UINT64_MAX / 2) {
        throw std::overflow_error("unique_table_resize: capacity overflow");
    }
    uint64_t new_capacity = old_capacity * 2;
    bddp* new_slots = static_cast<bddp*>(std::calloc(new_capacity, sizeof(bddp)));
    if (!new_slots) {
        throw std::bad_alloc();
    }
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

    if (node_count == 0) node_count = 1;
    if (node_max == 0) node_max = 1;
    // Cap node_max so that node_id (= (node_used+1)*2) stays below BDD_CONST_FLAG
    static const uint64_t NODE_ID_MAX = (BDD_CONST_FLAG >> 1) - 1;  // 0x3FFFFFFFFFFF
    if (node_max > NODE_ID_MAX) node_max = NODE_ID_MAX;
    if (node_count > node_max) node_count = node_max;
    bdd_node_count = node_count;
    bdd_node_max = node_max;
    // Guard against size_t overflow in malloc argument
    if (node_count > SIZE_MAX / sizeof(BddNode)) {
        throw std::overflow_error("bddinit: node allocation size overflow");
    }
    bdd_nodes = static_cast<BddNode*>(std::malloc(sizeof(BddNode) * node_count));
    if (!bdd_nodes) {
        throw std::bad_alloc();
    }

    // Initialize operation cache
    bdd_cache_size = next_power_of_2(node_count);
    // Guard against size_t overflow in malloc argument
    if (bdd_cache_size > SIZE_MAX / sizeof(BddCacheEntry)) {
        std::free(bdd_nodes);
        bdd_nodes = nullptr;
        throw std::overflow_error("bddinit: cache allocation size overflow");
    }
    bdd_cache = static_cast<BddCacheEntry*>(
        std::malloc(sizeof(BddCacheEntry) * bdd_cache_size));
    if (!bdd_cache) {
        std::free(bdd_nodes);
        bdd_nodes = nullptr;
        throw std::bad_alloc();
    }
    for (uint64_t i = 0; i < bdd_cache_size; i++) {
        bdd_cache[i].fop = 0;
        bdd_cache[i].g = 0;
        bdd_cache[i].h = 0;
        bdd_cache[i].result = bddnull;
    }
}

bddvar bddnewvar() {
    bdd_varcount++;
    bddvar var = bdd_varcount;
    if (var >= var_capacity) {
        bddvar old_capacity = var_capacity;
        bddvar new_capacity;
        if (var_capacity == 0) {
            new_capacity = VAR_INITIAL_CAPACITY;
        } else if (var_capacity > UINT32_MAX / 2) {
            // Prevent uint32_t overflow in capacity doubling
            throw std::overflow_error("bddnewvar: variable capacity overflow");
        } else {
            new_capacity = var_capacity * 2;
        }
        // Guard against size_t overflow in realloc arguments
        if (static_cast<uint64_t>(new_capacity) > SIZE_MAX / sizeof(BddUniqueTable)) {
            bdd_varcount--;
            throw std::overflow_error("bddnewvar: allocation size overflow");
        }
        bddvar* new_v2l = static_cast<bddvar*>(std::realloc(var2level, sizeof(bddvar) * new_capacity));
        if (!new_v2l) { bdd_varcount--; throw std::bad_alloc(); }
        var2level = new_v2l;
        bddvar* new_l2v = static_cast<bddvar*>(std::realloc(level2var, sizeof(bddvar) * new_capacity));
        if (!new_l2v) { bdd_varcount--; throw std::bad_alloc(); }
        level2var = new_l2v;
        BddUniqueTable* new_ut = static_cast<BddUniqueTable*>(
            std::realloc(bdd_unique_tables, sizeof(BddUniqueTable) * new_capacity));
        if (!new_ut) { bdd_varcount--; throw std::bad_alloc(); }
        bdd_unique_tables = new_ut;
        var_capacity = new_capacity;
        // Zero-initialize newly allocated regions
        std::memset(var2level + old_capacity, 0,
                    sizeof(bddvar) * (new_capacity - old_capacity));
        std::memset(level2var + old_capacity, 0,
                    sizeof(bddvar) * (new_capacity - old_capacity));
        std::memset(bdd_unique_tables + old_capacity, 0,
                    sizeof(BddUniqueTable) * (new_capacity - old_capacity));
        if (old_capacity == 0) {
            // Initialize index 0 (terminal level) to safe values
            var2level[0] = 0;
            level2var[0] = 0;
        }
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

bddvar bddtop(bddp f) {
    if (f == bddnull) {
        throw std::invalid_argument("bddtop: bddnull");
    }
    if (f & BDD_CONST_FLAG) {
        return 0;
    }
    return node_var(f);
}

void bddfree(bddp) {
    // Currently a no-op; reserved for future garbage collection
}

uint64_t bddused() {
    return bdd_node_used;
}

static void bddsize_traverse(bddp f, std::unordered_set<bddp>& visited) {
    if (f == bddnull) return;
    if (f & BDD_CONST_FLAG) return;
    bddp node = f & ~BDD_COMP_FLAG;
    if (!visited.insert(node).second) return;
    bddsize_traverse(node_lo(node), visited);
    bddsize_traverse(node_hi(node), visited);
}

uint64_t bddsize(bddp f) {
    if (f == bddnull) return 0;
    std::unordered_set<bddp> visited;
    bddsize_traverse(f, visited);
    return visited.size();
}

uint64_t bddvsize(bddp* p, int lim) {
    if (lim <= 0 || !p) return 0;
    std::unordered_set<bddp> visited;
    for (int i = 0; i < lim; i++) {
        bddsize_traverse(p[i], visited);
    }
    return visited.size();
}

uint64_t bddvsize(const std::vector<bddp>& v) {
    std::unordered_set<bddp> visited;
    for (size_t i = 0; i < v.size(); i++) {
        bddsize_traverse(v[i], visited);
    }
    return visited.size();
}

bddp BDD_UniqueTableLookup(bddvar var, bddp lo, bddp hi) {
    if (var < 1 || var > bdd_varcount || !bdd_unique_tables) return 0;
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
    if (var < 1 || var > bdd_varcount || !bdd_unique_tables) {
        throw std::out_of_range("BDD_UniqueTableInsert: var out of range");
    }
    BddUniqueTable* t = &bdd_unique_tables[var];
    if ((t->count + 1) * 4 >= t->capacity * 3) {
        unique_table_resize(t);
    }
    unique_table_insert_raw(t->slots, t->capacity, lo, hi, node_id);
    t->count++;
}

static void node_array_grow() {
    if (bdd_node_count >= bdd_node_max) return;
    uint64_t new_count;
    if (bdd_node_count > UINT64_MAX / 2) {
        new_count = bdd_node_max;
    } else {
        new_count = bdd_node_count * 2;
        if (new_count > bdd_node_max) {
            new_count = bdd_node_max;
        }
    }
    // Guard against size_t overflow in realloc argument
    if (new_count > SIZE_MAX / sizeof(BddNode)) {
        throw std::overflow_error("node_array_grow: allocation size overflow");
    }
    BddNode* p = static_cast<BddNode*>(std::realloc(bdd_nodes, sizeof(BddNode) * new_count));
    if (!p) throw std::bad_alloc();
    bdd_nodes = p;
    bdd_node_count = new_count;
}

bddp getnode(bddvar var, bddp lo, bddp hi) {
    if (lo == hi) return lo;  // reduction rule

    // Complement edge normalization: lo must not be complemented.
    // If lo is complemented, negate both edges and complement the result.
    bool comp = (lo & BDD_COMP_FLAG) != 0;
    if (comp) {
        lo = bddnot(lo);
        hi = bddnot(hi);
    }

    bddp found = BDD_UniqueTableLookup(var, lo, hi);
    if (found != 0) {
        return comp ? bddnot(found) : found;
    }
    if (bdd_node_used >= bdd_node_count) {
        node_array_grow();
        if (bdd_node_used >= bdd_node_count) {
            return bddnull;
        }
    }
    bdd_node_used++;
    bddp node_id = bdd_node_used * 2;  // index = node_id/2 - 1 = bdd_node_used - 1
    node_write(node_id, var, lo, hi);
    if (bddp_is_reduced(lo) && bddp_is_reduced(hi)) {
        node_set_reduced(node_id);
    }
    BDD_UniqueTableInsert(var, lo, hi, node_id);
    return comp ? bddnot(node_id) : node_id;
}

bddp getznode(bddvar var, bddp lo, bddp hi) {
    if (hi == bddempty) return lo;  // ZDD zero-suppression rule

    // Complement edge normalization: lo must not be complemented.
    // For ZDD: ~(x, lo, hi) = (x, ~lo, hi) -- only lo is toggled.
    bool comp = (lo & BDD_COMP_FLAG) != 0;
    if (comp) {
        lo = bddnot(lo);
    }

    bddp found = BDD_UniqueTableLookup(var, lo, hi);
    if (found != 0) {
        return comp ? bddnot(found) : found;
    }
    if (bdd_node_used >= bdd_node_count) {
        node_array_grow();
        if (bdd_node_used >= bdd_node_count) {
            return bddnull;
        }
    }
    bdd_node_used++;
    bddp node_id = bdd_node_used * 2;
    node_write(node_id, var, lo, hi);
    if (bddp_is_reduced(lo) && bddp_is_reduced(hi)) {
        node_set_reduced(node_id);
    }
    BDD_UniqueTableInsert(var, lo, hi, node_id);
    return comp ? bddnot(node_id) : node_id;
}

bddp bddprime(bddvar v) {
    if (v < 1 || v > bdd_varcount) {
        throw std::invalid_argument("bddprime: var out of range");
    }
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

// Obsolete: BDD and ZDD share the same node table, so node-level type
// distinction is not possible. Retained for API compatibility.
int bddisbdd(bddp f) {
    (void)f;
    throw std::logic_error("bddisbdd: obsolete — BDD/ZDD share the same node table");
}

// Obsolete: see bddisbdd above.
int bddiszbdd(bddp f) {
    (void)f;
    throw std::logic_error("bddiszbdd: obsolete — BDD/ZDD share the same node table");
}

