#include "bdd.h"
#include <cinttypes>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <istream>
#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

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
// Complement flag is stripped internally so callers need not worry about it.
static inline bddvar node_var(bddp node_id) {
    node_id &= ~BDD_COMP_FLAG;
    return static_cast<bddvar>(bdd_nodes[node_id / 2 - 1].data[0] >> BDD_NODE_VAR_SHIFT);
}

static inline bddp node_lo(bddp node_id) {
    node_id &= ~BDD_COMP_FLAG;
    const BddNode& n = bdd_nodes[node_id / 2 - 1];
    return ((n.data[0] & BDD_NODE_LO_HI_MASK) << BDD_NODE_LO_SPLIT) | (n.data[1] >> BDD_NODE_LO_LO_SHIFT);
}

static inline bddp node_hi(bddp node_id) {
    node_id &= ~BDD_COMP_FLAG;
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
    uint64_t idx = cache_hash(op, f, g);
    BddCacheEntry& e = bdd_cache[idx];
    uint64_t fop = (static_cast<uint64_t>(op) << 48) | f;
    if (e.fop == fop && e.g == g && e.h == 0) {
        return e.result;
    }
    return bddnull;
}

void bddwcache(uint8_t op, bddp f, bddp g, bddp result) {
    uint64_t idx = cache_hash(op, f, g);
    BddCacheEntry& e = bdd_cache[idx];
    e.fop = (static_cast<uint64_t>(op) << 48) | f;
    e.g = g;
    e.h = 0;
    e.result = result;
}

// --- Cache API (3-operand) ---
bddp bddrcache3(uint8_t op, bddp f, bddp g, bddp h) {
    uint64_t idx = cache_hash3(op, f, g, h);
    BddCacheEntry& e = bdd_cache[idx];
    uint64_t fop = (static_cast<uint64_t>(op) << 48) | f;
    if (e.fop == fop && e.g == g && e.h == h) {
        return e.result;
    }
    return bddnull;
}

void bddwcache3(uint8_t op, bddp f, bddp g, bddp h, bddp result) {
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
        bdd_cache[i].h = 0;
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
    if (f & BDD_CONST_FLAG) return;
    bddp node = f & ~BDD_COMP_FLAG;
    if (!visited.insert(node).second) return;
    bddsize_traverse(node_lo(node), visited);
    bddsize_traverse(node_hi(node), visited);
}

uint64_t bddsize(bddp f) {
    std::unordered_set<bddp> visited;
    bddsize_traverse(f, visited);
    return visited.size();
}

uint64_t bddvsize(bddp* p, int lim) {
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
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = node_var(f);
    bddvar g_var = node_var(g);
    bddvar f_level = var2level[f_var];
    bddvar g_level = var2level[g_var];

    // Determine top variable (highest level) and cofactors
    bddvar top_var;
    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        top_var = f_var;
        f_lo = node_lo(f);
        f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = g;
        g_hi = g;
    } else if (g_level > f_level) {
        top_var = g_var;
        f_lo = f;
        f_hi = f;
        g_lo = node_lo(g);
        g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    } else {
        top_var = f_var;
        f_lo = node_lo(f);
        f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = node_lo(g);
        g_hi = node_hi(g);
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

bddp bddor(bddp f, bddp g) {
    return bddnot(bddand(bddnot(f), bddnot(g)));
}

bddp bddnand(bddp f, bddp g) {
    return bddnot(bddand(f, g));
}

bddp bddnor(bddp f, bddp g) {
    return bddand(bddnot(f), bddnot(g));
}

bddp bddxor(bddp f, bddp g) {
    // Terminal cases
    if (f == bddfalse) return g;
    if (f == bddtrue) return bddnot(g);
    if (g == bddfalse) return f;
    if (g == bddtrue) return bddnot(f);
    if (f == g) return bddfalse;
    if (f == bddnot(g)) return bddtrue;

    // Normalize: XOR(~a, b) = ~XOR(a, b), so strip complements and track parity
    bool comp = false;
    if (f & BDD_COMP_FLAG) { f ^= BDD_COMP_FLAG; comp = !comp; }
    if (g & BDD_COMP_FLAG) { g ^= BDD_COMP_FLAG; comp = !comp; }

    // Commutative: normalize order
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_XOR, f, g);
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    // Both f and g are non-complement, non-terminal at this point
    bddvar f_var = node_var(f);
    bddvar g_var = node_var(g);
    bddvar f_level = var2level[f_var];
    bddvar g_level = var2level[g_var];

    // Determine top variable (highest level) and cofactors
    bddvar top_var;
    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        top_var = f_var;
        f_lo = node_lo(f);
        f_hi = node_hi(f);
        g_lo = g;
        g_hi = g;
    } else if (g_level > f_level) {
        top_var = g_var;
        f_lo = f;
        f_hi = f;
        g_lo = node_lo(g);
        g_hi = node_hi(g);
    } else {
        top_var = f_var;
        f_lo = node_lo(f);
        f_hi = node_hi(f);
        g_lo = node_lo(g);
        g_hi = node_hi(g);
    }

    // Recurse
    bddp lo = bddxor(f_lo, g_lo);
    bddp hi = bddxor(f_hi, g_hi);

    bddp result = getnode(top_var, lo, hi);

    // Cache insert (store result for normalized operands, without comp adjustment)
    bddwcache(BDD_OP_XOR, f, g, result);

    return comp ? bddnot(result) : result;
}

bddp bddxnor(bddp f, bddp g) {
    return bddnot(bddxor(f, g));
}

bddp bddite(bddp f, bddp g, bddp h) {
    // Terminal cases for f
    if (f == bddtrue) return g;
    if (f == bddfalse) return h;

    // g == h
    if (g == h) return g;

    // Reduction to 2-operand when g or h is terminal
    if (g == bddtrue) return bddor(f, h);
    if (g == bddfalse) return bddand(bddnot(f), h);
    if (h == bddfalse) return bddand(f, g);
    if (h == bddtrue) return bddor(bddnot(f), g);

    // All three are non-terminal at this point

    // Normalize: f must not be complemented
    if (f & BDD_COMP_FLAG) {
        f = bddnot(f);
        bddp tmp = g; g = h; h = tmp;
    }

    // Normalize: g must not be complemented
    bool comp = false;
    if (g & BDD_COMP_FLAG) {
        g = bddnot(g);
        h = bddnot(h);
        comp = true;
    }

    // Cache lookup
    bddp cached = bddrcache3(BDD_OP_ITE, f, g, h);
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    // Determine top variable (highest level among f, g, h)
    bddvar f_var = node_var(f);
    bddvar g_var = node_var(g);
    bddvar f_level = var2level[f_var];
    bddvar g_level = var2level[g_var];

    bddvar top_var = (f_level >= g_level) ? f_var : g_var;
    bddvar top_level = (f_level >= g_level) ? f_level : g_level;

    bool h_comp = (h & BDD_COMP_FLAG) != 0;
    bddvar h_var = node_var(h);
    bddvar h_level = var2level[h_var];
    if (h_level > top_level) { top_var = h_var; top_level = h_level; }

    // Cofactors for f (non-complemented)
    bddp f_lo, f_hi;
    if (f_level == top_level) {
        f_lo = node_lo(f);
        f_hi = node_hi(f);
    } else {
        f_lo = f; f_hi = f;
    }

    // Cofactors for g (non-complemented)
    bddp g_lo, g_hi;
    if (g_level == top_level) {
        g_lo = node_lo(g);
        g_hi = node_hi(g);
    } else {
        g_lo = g; g_hi = g;
    }

    // Cofactors for h (may be complemented)
    bddp h_lo, h_hi;
    if (h_level == top_level) {
        h_lo = node_lo(h);
        h_hi = node_hi(h);
        if (h_comp) { h_lo = bddnot(h_lo); h_hi = bddnot(h_hi); }
    } else {
        h_lo = h; h_hi = h;
    }

    // Recurse
    bddp lo = bddite(f_lo, g_lo, h_lo);
    bddp hi = bddite(f_hi, g_hi, h_hi);

    bddp result = getnode(top_var, lo, hi);

    // Cache write
    bddwcache3(BDD_OP_ITE, f, g, h, result);

    return comp ? bddnot(result) : result;
}

bddp bddat0(bddp f, bddvar v) {
    // Terminal case
    if (f & BDD_CONST_FLAG) return f;

    // Handle complement edge
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];
    bddvar v_level = var2level[v];

    // f's top variable is below v: v does not appear in f
    if (f_level < v_level) return f;

    // Cache lookup (use v as second operand)
    bddp cached = bddrcache(BDD_OP_AT0, f, static_cast<bddp>(v));
    if (cached != bddnull) return cached;

    bddp result;
    if (f_var == v) {
        // Substitute 0: take the low branch
        result = node_lo(f);
        if (f_comp) result = bddnot(result);
    } else {
        // f_level > v_level: recurse into both branches
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp lo = bddat0(f_lo, v);
        bddp hi = bddat0(f_hi, v);
        result = getnode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_AT0, f, static_cast<bddp>(v), result);
    return result;
}

bddp bddat1(bddp f, bddvar v) {
    // Terminal case
    if (f & BDD_CONST_FLAG) return f;

    // Handle complement edge
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];
    bddvar v_level = var2level[v];

    // f's top variable is below v: v does not appear in f
    if (f_level < v_level) return f;

    // Cache lookup (use v as second operand)
    bddp cached = bddrcache(BDD_OP_AT1, f, static_cast<bddp>(v));
    if (cached != bddnull) return cached;

    bddp result;
    if (f_var == v) {
        // Substitute 1: take the high branch
        result = node_hi(f);
        if (f_comp) result = bddnot(result);
    } else {
        // f_level > v_level: recurse into both branches
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp lo = bddat1(f_lo, v);
        bddp hi = bddat1(f_hi, v);
        result = getnode(f_var, lo, hi);
    }

    bddwcache(BDD_OP_AT1, f, static_cast<bddp>(v), result);
    return result;
}

int bddimply(bddp f, bddp g) {
    // Terminal cases: is there an assignment where f=1 and g=0?
    if (f == bddfalse) return 1;   // f is never true
    if (g == bddtrue)  return 1;   // g is always true
    if (f == bddtrue && g == bddfalse) return 0;  // counterexample
    if (f == g) return 1;          // f implies f
    if (f == bddnot(g)) return 0;  // f=1 => g=0

    // Cache lookup (store result as bddtrue/bddfalse)
    bddp cached = bddrcache(BDD_OP_IMPLY, f, g);
    if (cached != bddnull) return cached == bddtrue ? 1 : 0;

    // Determine top variable and cofactors
    bool f_is_const = (f & BDD_CONST_FLAG) != 0;
    bool g_is_const = (g & BDD_CONST_FLAG) != 0;
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;

    bddvar f_var = f_is_const ? 0 : node_var(f);
    bddvar g_var = g_is_const ? 0 : node_var(g);
    bddvar f_level = f_is_const ? 0 : var2level[f_var];
    bddvar g_level = g_is_const ? 0 : var2level[g_var];

    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        f_lo = node_lo(f);
        f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = g;
        g_hi = g;
    } else if (g_level > f_level) {
        f_lo = f;
        f_hi = f;
        g_lo = node_lo(g);
        g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    } else {
        f_lo = node_lo(f);
        f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = node_lo(g);
        g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    }

    // Both branches must satisfy the implication
    int result = bddimply(f_lo, g_lo) && bddimply(f_hi, g_hi);

    bddwcache(BDD_OP_IMPLY, f, g, result ? bddtrue : bddfalse);
    return result;
}

static void bddsupport_collect(bddp f, std::unordered_set<bddvar>& vars,
                               std::unordered_set<bddp>& visited) {
    if (f & BDD_CONST_FLAG) return;
    bddp node = f & ~BDD_COMP_FLAG;
    if (!visited.insert(node).second) return;
    vars.insert(node_var(node));
    bddsupport_collect(node_lo(node), vars, visited);
    bddsupport_collect(node_hi(node), vars, visited);
}

bddp bddsupport(bddp f) {
    if (f & BDD_CONST_FLAG) return bddfalse;

    // Collect all variables appearing in f
    std::unordered_set<bddvar> var_set;
    std::unordered_set<bddp> visited;
    bddsupport_collect(f, var_set, visited);

    // Sort by level ascending (lowest level first)
    std::vector<bddvar> vars(var_set.begin(), var_set.end());
    std::sort(vars.begin(), vars.end(), [](bddvar a, bddvar b) {
        return var2level[a] < var2level[b];
    });

    // Build chain bottom-up: each node has lo=chain, hi=bddtrue
    bddp result = bddfalse;
    for (size_t i = 0; i < vars.size(); i++) {
        result = getnode(vars[i], result, bddtrue);
    }
    return result;
}

std::vector<bddvar> bddsupport_vec(bddp f) {
    std::vector<bddvar> vars;
    if (f & BDD_CONST_FLAG) return vars;

    std::unordered_set<bddvar> var_set;
    std::unordered_set<bddp> visited;
    bddsupport_collect(f, var_set, visited);

    vars.assign(var_set.begin(), var_set.end());
    std::sort(vars.begin(), vars.end(), [](bddvar a, bddvar b) {
        return var2level[a] > var2level[b];
    });
    return vars;
}

bddp bddexist(bddp f, bddp g) {
    // Terminal cases
    if (g == bddfalse) return f;    // no variables to quantify
    if (f == bddfalse) return bddfalse;
    if (f == bddtrue) return bddtrue;

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_EXIST, f, g);
    if (cached != bddnull) return cached;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];

    bddvar g_var = node_var(g);
    bddvar g_level = var2level[g_var];

    bddp result;

    if (g_level > f_level) {
        // g's top variable does not appear in f, skip it
        result = bddexist(f, node_lo(g));
    } else if (f_level > g_level) {
        // f's top variable is not being quantified, keep it
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp lo = bddexist(f_lo, g);
        bddp hi = bddexist(f_hi, g);
        result = getnode(f_var, lo, hi);
    } else {
        // Same variable: quantify it out
        // exist v. f = f|_{v=0} OR f|_{v=1}
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp g_rest = node_lo(g);
        bddp lo_r = bddexist(f_lo, g_rest);
        bddp hi_r = bddexist(f_hi, g_rest);
        result = bddor(lo_r, hi_r);
    }

    bddwcache(BDD_OP_EXIST, f, g, result);
    return result;
}

static bddp vars_to_cube(const std::vector<bddvar>& vars) {
    // Build cube BDD: sort by level ascending, chain with hi=bddtrue
    std::vector<bddvar> sorted(vars);
    std::sort(sorted.begin(), sorted.end(), [](bddvar a, bddvar b) {
        return var2level[a] < var2level[b];
    });
    bddp cube = bddfalse;
    for (size_t i = 0; i < sorted.size(); i++) {
        cube = getnode(sorted[i], cube, bddtrue);
    }
    return cube;
}

bddp bddexist(bddp f, const std::vector<bddvar>& vars) {
    return bddexist(f, vars_to_cube(vars));
}

bddp bdduniv(bddp f, bddp g) {
    // Terminal cases
    if (g == bddfalse) return f;
    if (f == bddfalse) return bddfalse;
    if (f == bddtrue) return bddtrue;

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_UNIV, f, g);
    if (cached != bddnull) return cached;

    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];

    bddvar g_var = node_var(g);
    bddvar g_level = var2level[g_var];

    bddp result;

    if (g_level > f_level) {
        // g's top variable does not appear in f, skip it
        result = bdduniv(f, node_lo(g));
    } else if (f_level > g_level) {
        // f's top variable is not being quantified, keep it
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp lo = bdduniv(f_lo, g);
        bddp hi = bdduniv(f_hi, g);
        result = getnode(f_var, lo, hi);
    } else {
        // Same variable: quantify it universally
        // forall v. f = f|_{v=0} AND f|_{v=1}
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        bddp g_rest = node_lo(g);
        bddp lo_r = bdduniv(f_lo, g_rest);
        bddp hi_r = bdduniv(f_hi, g_rest);
        result = bddand(lo_r, hi_r);
    }

    bddwcache(BDD_OP_UNIV, f, g, result);
    return result;
}

bddp bdduniv(bddp f, const std::vector<bddvar>& vars) {
    return bdduniv(f, vars_to_cube(vars));
}

static bddp bddlshift_rec(bddp f, bddvar shift) {
    if (f & BDD_CONST_FLAG) return f;

    // Normalize complement for better cache hit rate
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp fn = f & ~BDD_COMP_FLAG;

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_LSHIFT, fn, static_cast<bddp>(shift));
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    bddvar v = node_var(fn);
    bddvar target_var = level2var[var2level[v] + shift];

    bddp lo = bddlshift_rec(node_lo(fn), shift);
    bddp hi = bddlshift_rec(node_hi(fn), shift);

    bddp result = getnode(target_var, lo, hi);

    bddwcache(BDD_OP_LSHIFT, fn, static_cast<bddp>(shift), result);
    return comp ? bddnot(result) : result;
}

bddp bddlshift(bddp f, bddvar shift) {
    if (f & BDD_CONST_FLAG) return f;
    if (shift == 0) return f;

    // Ensure variables exist up to the required level
    bddvar max_level = var2level[bddtop(f)] + shift;
    while (bdd_varcount < max_level) {
        bddnewvar();
    }

    return bddlshift_rec(f, shift);
}

static bddp bddrshift_rec(bddp f, bddvar shift) {
    if (f & BDD_CONST_FLAG) return f;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp fn = f & ~BDD_COMP_FLAG;

    bddp cached = bddrcache(BDD_OP_RSHIFT, fn, static_cast<bddp>(shift));
    if (cached != bddnull) return comp ? bddnot(cached) : cached;

    bddvar v = node_var(fn);
    bddvar target_var = level2var[var2level[v] - shift];

    bddp lo = bddrshift_rec(node_lo(fn), shift);
    bddp hi = bddrshift_rec(node_hi(fn), shift);

    bddp result = getnode(target_var, lo, hi);

    bddwcache(BDD_OP_RSHIFT, fn, static_cast<bddp>(shift), result);
    return comp ? bddnot(result) : result;
}

bddp bddrshift(bddp f, bddvar shift) {
    if (f & BDD_CONST_FLAG) return f;
    if (shift == 0) return f;

    // Pre-check: all variable levels must be greater than shift
    std::unordered_set<bddvar> var_set;
    std::unordered_set<bddp> visited;
    bddsupport_collect(f, var_set, visited);
    for (std::unordered_set<bddvar>::iterator it = var_set.begin();
         it != var_set.end(); ++it) {
        if (var2level[*it] <= shift) {
            throw std::invalid_argument("bddrshift: shift exceeds minimum variable level");
        }
    }

    return bddrshift_rec(f, shift);
}

bddp bddcofactor(bddp f, bddp g) {
    // Terminal cases
    if (f & BDD_CONST_FLAG) return f;   // f is constant
    if (g == bddfalse) return bddfalse; // care region is empty
    if (f == bddnot(g)) return bddfalse; // f=0 wherever g=1
    if (f == g) return bddtrue;          // f=1 wherever g=1
    if (g == bddtrue) return f;          // no don't care region

    // Cache lookup
    bddp cached = bddrcache(BDD_OP_COFACTOR, f, g);
    if (cached != bddnull) return cached;

    // Determine top variable
    bool f_comp = (f & BDD_COMP_FLAG) != 0;
    bool g_comp = (g & BDD_COMP_FLAG) != 0;
    bddvar f_var = node_var(f);
    bddvar g_var = node_var(g);
    bddvar f_level = var2level[f_var];
    bddvar g_level = var2level[g_var];

    bddvar top_var;
    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        top_var = f_var;
        f_lo = node_lo(f); f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = g; g_hi = g;
    } else if (g_level > f_level) {
        top_var = g_var;
        f_lo = f; f_hi = f;
        g_lo = node_lo(g); g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    } else {
        top_var = f_var;
        f_lo = node_lo(f); f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
        g_lo = node_lo(g); g_hi = node_hi(g);
        if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
    }

    bddp result;

    if (g_lo == bddfalse && g_hi != bddfalse) {
        // Low branch is entirely don't care
        result = bddcofactor(f_hi, g_hi);
    } else if (g_hi == bddfalse && g_lo != bddfalse) {
        // High branch is entirely don't care
        result = bddcofactor(f_lo, g_lo);
    } else {
        bddp lo = bddcofactor(f_lo, g_lo);
        bddp hi = bddcofactor(f_hi, g_hi);
        result = getnode(top_var, lo, hi);
    }

    bddwcache(BDD_OP_COFACTOR, f, g, result);
    return result;
}

int bddisbdd(bddp f) {
    (void)f;
    throw std::logic_error("bddisbdd: not supported");
}

int bddiszbdd(bddp f) {
    (void)f;
    throw std::logic_error("bddiszbdd: not supported");
}
