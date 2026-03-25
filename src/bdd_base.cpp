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

// --- Garbage collection state ---
// Leaked singleton to avoid static init/destruction order issues
static std::unordered_set<bddp*>& gc_roots() {
    static auto* instance = new std::unordered_set<bddp*>();
    return *instance;
}
int BDD_RecurCount = 0;
int bdd_gc_depth = 0;
uint64_t bdd_gc_generation = 0;
static bddp bdd_free_list = 0;
static uint64_t bdd_free_count = 0;
static const double BDD_GC_THRESHOLD_DEFAULT = 0.9;
static double bdd_gc_threshold = BDD_GC_THRESHOLD_DEFAULT;

// Sentinel for 2-operand cache entries' h field.
// Must differ from any valid bddp used as h in 3-operand entries.
// No valid bddp equals 0 (nodes start at 2, constants have bit 47 set).
static const uint64_t BDD_CACHE_H_UNUSED = 0;

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
    if (e.fop == fop && e.g == g && e.h == BDD_CACHE_H_UNUSED) {
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
    e.h = BDD_CACHE_H_UNUSED;
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
void bddfinal() {
    // Check if any GC root points to a non-terminal node or to a
    // non-standard terminal (MTBDD/MTZDD terminal with index >= 2).
    // Standard terminals: bddfalse (index 0) and bddtrue (index 1)
    // are safe because they are static constants. MTBDD/MTZDD terminals
    // with index >= 2 will be invalidated by mtbdd_clear_all_terminal_tables.
    for (bddp* p : gc_roots()) {
        bddp v = *p;
        if (v == bddnull) continue;
        if (v & BDD_CONST_FLAG) {
            uint64_t idx = v & ~BDD_CONST_FLAG;
            if (idx >= 2) {
                throw std::runtime_error(
                    "bddfinal(): cannot finalize while MTBDD/MTZDD objects "
                    "with non-standard terminal values exist. "
                    "Delete all such objects first.");
            }
            continue;  // bddfalse (idx=0) and bddtrue (idx=1) are safe
        }
        throw std::runtime_error(
            "bddfinal(): cannot finalize while BDD/ZDD objects "
            "referencing non-terminal nodes exist. "
            "Delete all such objects first.");
    }
    mtbdd_clear_all_terminal_tables();
    std::free(bdd_nodes);
    bdd_nodes = nullptr;
    std::free(var2level);
    var2level = nullptr;
    std::free(level2var);
    level2var = nullptr;
    for (bddvar i = 1; i <= bdd_varcount; i++) {
        std::free(bdd_unique_tables[i].slots);
    }
    std::free(bdd_unique_tables);
    bdd_unique_tables = nullptr;
    std::free(bdd_cache);
    bdd_cache = nullptr;
    bdd_cache_size = 0;

    bdd_varcount = 0;
    var_capacity = 0;
    bdd_node_count = 0;
    bdd_node_used = 0;
    bdd_node_max = 0;

    BDD_RecurCount = 0;
    bdd_gc_depth = 0;
    bdd_gc_generation = 0;
    bdd_free_list = 0;
    bdd_free_count = 0;
    bdd_gc_threshold = BDD_GC_THRESHOLD_DEFAULT;
    gc_roots().clear();
}

int bddinit(uint64_t node_count, uint64_t node_max) {
    bddfinal();

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
    return 0;
}

static void bdd_cache_clear() {
    if (bdd_cache_size == 0 || !bdd_cache) return;
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
        // Attempt all three reallocs before committing any, so that a
        // partial failure does not leave arrays with mismatched sizes.
        bddvar* new_v2l = static_cast<bddvar*>(std::realloc(var2level, sizeof(bddvar) * new_capacity));
        if (!new_v2l) { bdd_varcount--; throw std::bad_alloc(); }
        bddvar* new_l2v = static_cast<bddvar*>(std::realloc(level2var, sizeof(bddvar) * new_capacity));
        if (!new_l2v) { var2level = new_v2l; bdd_varcount--; throw std::bad_alloc(); }
        BddUniqueTable* new_ut = static_cast<BddUniqueTable*>(
            std::realloc(bdd_unique_tables, sizeof(BddUniqueTable) * new_capacity));
        if (!new_ut) { var2level = new_v2l; level2var = new_l2v; bdd_varcount--; throw std::bad_alloc(); }
        var2level = new_v2l;
        level2var = new_l2v;
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
    try {
        unique_table_init(&bdd_unique_tables[var]);
    } catch (...) {
        var2level[var] = 0;
        level2var[var] = 0;
        bdd_varcount--;
        throw;
    }
    return var;
}

std::vector<bddvar> bddnewvar(int n) {
    if (n < 0) {
        throw std::invalid_argument("bddnewvar: n must be non-negative");
    }
    std::vector<bddvar> vars;
    vars.reserve(n);
    for (int i = 0; i < n; i++) {
        vars.push_back(bddnewvar());
    }
    return vars;
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

    // Invalidate cache: level mapping changed, so cached results may be wrong
    bdd_cache_clear();

    return new_var;
}

bddvar bddlevofvar(bddvar var) {
    if (var == 0) return 0;
    if (var > bdd_varcount) {
        throw std::invalid_argument("bddlevofvar: var out of range");
    }
    return var2level[var];
}

bddvar bddvaroflev(bddvar lev) {
    if (lev == 0) return 0;
    if (lev > bdd_varcount) {
        throw std::invalid_argument("bddvaroflev: lev out of range");
    }
    return level2var[lev];
}

bddvar bddvarused() {
    return bdd_varcount;
}

bddvar bddtoplev() {
    return bdd_varcount;
}

bddvar bddtop(bddp f) {
    bddp_validate(f, "bddtop");
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

uint64_t bddlive() {
    return bdd_node_used - bdd_free_count;
}

// --- Garbage collection ---

void bddgc_protect(bddp* p) {
    gc_roots().insert(p);
}

void bddgc_unprotect(bddp* p) {
    gc_roots().erase(p);
}

uint64_t bddgc_rootcount() {
    return gc_roots().size();
}

void bddgc_setthreshold(double threshold) {
    if (threshold < 0.0) threshold = 0.0;
    if (threshold > 1.0) threshold = 1.0;
    bdd_gc_threshold = threshold;
}

double bddgc_getthreshold() {
    return bdd_gc_threshold;
}

static void bddgc_mark(bddp f, uint64_t* marks) {
    std::vector<bddp> stack;
    stack.push_back(f);
    while (!stack.empty()) {
        bddp cur = stack.back();
        stack.pop_back();
        if (cur == bddnull) continue;
        if (cur & BDD_CONST_FLAG) continue;
        bddp node_id = cur & ~BDD_COMP_FLAG;
        uint64_t idx = node_id / 2 - 1;
        if (idx >= bdd_node_used) continue;
        uint64_t word = idx / 64;
        uint64_t bit = static_cast<uint64_t>(1) << (idx % 64);
        if (marks[word] & bit) continue;
        marks[word] |= bit;
        stack.push_back(node_lo(node_id));
        stack.push_back(node_hi(node_id));
    }
}

int bddgc() {
    if (bdd_gc_depth > 0) return 0;
    if (bdd_node_used == 0) return 0;

    // 1. Allocate mark bit vector
    uint64_t mark_words = (bdd_node_used + 63) / 64;
    uint64_t* marks = static_cast<uint64_t*>(std::calloc(mark_words, sizeof(uint64_t)));
    if (!marks) return 1;

    // 2. Mark reachable nodes from all registered roots
    for (bddp* ptr : gc_roots()) {
        bddgc_mark(*ptr, marks);
    }

    // 3. Clear all unique tables
    for (bddvar v = 1; v <= bdd_varcount; v++) {
        BddUniqueTable& t = bdd_unique_tables[v];
        std::memset(t.slots, 0, t.capacity * sizeof(bddp));
        t.count = 0;
    }

    // 4. Sweep: rebuild unique tables and free list
    bdd_free_list = 0;
    bdd_free_count = 0;
    for (uint64_t i = 0; i < bdd_node_used; i++) {
        bddp node_id = (i + 1) * 2;
        if (marks[i / 64] & (static_cast<uint64_t>(1) << (i % 64))) {
            // Live node: re-insert into unique table (reduced nodes only;
            // unreduced nodes are not stored in the unique table)
            if (node_is_reduced(node_id)) {
                bddvar var = node_var(node_id);
                bddp lo = node_lo(node_id);
                bddp hi = node_hi(node_id);
                BDD_UniqueTableInsert(var, lo, hi, node_id);
            }
        } else {
            // Dead node: add to free list
            bdd_nodes[i].data[0] = bdd_free_list;
            bdd_free_list = node_id;
            bdd_free_count++;
        }
    }

    // 5. Clear operation cache
    bdd_cache_clear();

    // 6. Increment GC generation counter
    ++bdd_gc_generation;

    // 7. Free mark array
    std::free(marks);

    return (bdd_free_count == 0) ? 1 : 0;
}

bool bdd_should_gc() {
    if (bdd_node_count < bdd_node_max) return false;
    uint64_t live = bdd_node_used - bdd_free_count;
    return live > static_cast<uint64_t>(bdd_node_max * bdd_gc_threshold);
}

static void bddsize_traverse(bddp f, std::unordered_set<bddp>& visited) {
    std::vector<bddp> stack;
    stack.push_back(f);
    while (!stack.empty()) {
        bddp cur = stack.back();
        stack.pop_back();
        if (cur == bddnull) continue;
        if (cur & BDD_CONST_FLAG) continue;
        bddp node = cur & ~BDD_COMP_FLAG;
        if (!visited.insert(node).second) continue;
        stack.push_back(node_lo(node));
        stack.push_back(node_hi(node));
    }
}

static void bddplainsize_traverse(bddp f, bool is_zdd,
                                   std::unordered_set<bddp>& visited) {
    std::vector<bddp> stack;
    stack.push_back(f);
    while (!stack.empty()) {
        bddp cur = stack.back();
        stack.pop_back();
        if (cur == bddnull) continue;
        if (cur & BDD_CONST_FLAG) continue;
        // Key includes complement bit: different complement = different plain node
        if (!visited.insert(cur).second) continue;

        bddp base = cur & ~BDD_COMP_FLAG;
        bool comp = cur & BDD_COMP_FLAG;

        bddp lo = node_lo(base);
        bddp hi = node_hi(base);

        if (comp) {
            lo ^= BDD_COMP_FLAG;
            if (!is_zdd) {
                hi ^= BDD_COMP_FLAG;
            }
        }

        stack.push_back(lo);
        stack.push_back(hi);
    }
}

uint64_t bddplainsize(bddp f, bool is_zdd) {
    bddp_validate(f, "bddplainsize");
    if (f == bddnull) return 0;
    std::unordered_set<bddp> visited;
    bddplainsize_traverse(f, is_zdd, visited);
    return visited.size();
}

uint64_t bddrawsize(const std::vector<bddp>& v) {
    std::unordered_set<bddp> visited;
    for (size_t i = 0; i < v.size(); i++) {
        bddsize_traverse(v[i], visited);
    }
    return visited.size();
}

uint64_t bddplainsize(const std::vector<bddp>& v, bool is_zdd) {
    std::unordered_set<bddp> visited;
    for (size_t i = 0; i < v.size(); i++) {
        bddplainsize_traverse(v[i], is_zdd, visited);
    }
    return visited.size();
}

uint64_t bddsize(bddp f) {
    bddp_validate(f, "bddsize");
    if (f == bddnull) return 0;
    std::unordered_set<bddp> visited;
    bddsize_traverse(f, visited);
    return visited.size();
}

uint64_t bddvsize(bddp* p, size_t lim) {
    if (lim == 0 || !p) return 0;
    std::unordered_set<bddp> visited;
    for (size_t i = 0; i < lim; i++) {
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

bddp allocate_node() {
    // 1. Free list
    if (bdd_free_list != 0) {
        bddp node_id = bdd_free_list;
        uint64_t idx = node_id / 2 - 1;
        bdd_free_list = bdd_nodes[idx].data[0];
        bdd_free_count--;
        return node_id;
    }
    // 2. Extend array
    if (bdd_node_used < bdd_node_count) {
        bdd_node_used++;
        return bdd_node_used * 2;
    }
    // 3. Grow array (realloc)
    node_array_grow();
    if (bdd_node_used < bdd_node_count) {
        bdd_node_used++;
        return bdd_node_used * 2;
    }
    // 4. GC as last resort (only at top level)
    if (bdd_gc_depth == 0) {
        bddgc();
        if (bdd_free_list != 0) {
            bddp node_id = bdd_free_list;
            uint64_t idx = node_id / 2 - 1;
            bdd_free_list = bdd_nodes[idx].data[0];
            bdd_free_count--;
            return node_id;
        }
    }
    throw std::overflow_error("node table exhausted");
}

// --- BDD node creation ---

bddp BDD::getnode_raw(bddvar var, bddp lo, bddp hi) {
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
    bddp node_id = allocate_node();
    node_write(node_id, var, lo, hi);
    BDD_DEBUG_ASSERT(bddp_is_reduced(lo) && bddp_is_reduced(hi));
    if (bddp_is_reduced(lo) && bddp_is_reduced(hi)) {
        node_set_reduced(node_id);
    }
    try {
        BDD_UniqueTableInsert(var, lo, hi, node_id);
    } catch (...) {
        if (node_id / 2 == bdd_node_used) {
            bdd_node_used--;
        } else {
            uint64_t idx = node_id / 2 - 1;
            bdd_nodes[idx].data[0] = bdd_free_list;
            bdd_free_list = node_id;
            bdd_free_count++;
        }
        throw;
    }
    return comp ? bddnot(node_id) : node_id;
}

bddp BDD::getnode(bddvar var, bddp lo, bddp hi) {
    // Validate inputs before applying reduction rule
    if (lo == bddnull || hi == bddnull)
        throw std::invalid_argument("BDD::getnode: bddnull child");
    if (var < 1 || var > bdd_varcount)
        throw std::invalid_argument("BDD::getnode: var out of range");
    bddvar var_level = var2level[var];
    if (!(lo & BDD_CONST_FLAG)) {
        bddp lo_phys = lo & ~BDD_COMP_FLAG;
        if (lo_phys < 2 || lo_phys / 2 - 1 >= bdd_node_used)
            throw std::invalid_argument("BDD::getnode: lo child is not a valid node");
        if (var2level[node_var(lo_phys)] >= var_level)
            throw std::invalid_argument("BDD::getnode: lo child level >= var level");
    }
    if (!(hi & BDD_CONST_FLAG)) {
        bddp hi_phys = hi & ~BDD_COMP_FLAG;
        if (hi_phys < 2 || hi_phys / 2 - 1 >= bdd_node_used)
            throw std::invalid_argument("BDD::getnode: hi child is not a valid node");
        if (var2level[node_var(hi_phys)] >= var_level)
            throw std::invalid_argument("BDD::getnode: hi child level >= var level");
    }
    // Apply reduction rule after validation (no node creation if lo == hi)
    if (lo == hi) return lo;
    return BDD::getnode_raw(var, lo, hi);
}

BDD BDD::getnode(bddvar var, const BDD& lo, const BDD& hi) {
    BDD b(0);
    b.root = BDD::getnode(var, lo.root, hi.root);
    return b;
}

// --- ZDD node creation ---

bddp ZDD::getnode_raw(bddvar var, bddp lo, bddp hi) {
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
    bddp node_id = allocate_node();
    node_write(node_id, var, lo, hi);
    BDD_DEBUG_ASSERT(bddp_is_reduced(lo) && bddp_is_reduced(hi));
    if (bddp_is_reduced(lo) && bddp_is_reduced(hi)) {
        node_set_reduced(node_id);
    }
    try {
        BDD_UniqueTableInsert(var, lo, hi, node_id);
    } catch (...) {
        if (node_id / 2 == bdd_node_used) {
            bdd_node_used--;
        } else {
            uint64_t idx = node_id / 2 - 1;
            bdd_nodes[idx].data[0] = bdd_free_list;
            bdd_free_list = node_id;
            bdd_free_count++;
        }
        throw;
    }
    return comp ? bddnot(node_id) : node_id;
}

bddp ZDD::getnode(bddvar var, bddp lo, bddp hi) {
    // Validate inputs before applying zero-suppression rule
    if (lo == bddnull || hi == bddnull)
        throw std::invalid_argument("ZDD::getnode: bddnull child");
    if (var < 1 || var > bdd_varcount)
        throw std::invalid_argument("ZDD::getnode: var out of range");
    bddvar var_level = var2level[var];
    if (!(lo & BDD_CONST_FLAG)) {
        bddp lo_phys = lo & ~BDD_COMP_FLAG;
        if (lo_phys < 2 || lo_phys / 2 - 1 >= bdd_node_used)
            throw std::invalid_argument("ZDD::getnode: lo child is not a valid node");
        if (var2level[node_var(lo_phys)] >= var_level)
            throw std::invalid_argument("ZDD::getnode: lo child level >= var level");
    }
    if (!(hi & BDD_CONST_FLAG)) {
        bddp hi_phys = hi & ~BDD_COMP_FLAG;
        if (hi_phys < 2 || hi_phys / 2 - 1 >= bdd_node_used)
            throw std::invalid_argument("ZDD::getnode: hi child is not a valid node");
        if (var2level[node_var(hi_phys)] >= var_level)
            throw std::invalid_argument("ZDD::getnode: hi child level >= var level");
    }
    // Apply zero-suppression rule after validation (no node creation if hi == bddempty)
    if (hi == bddempty) return lo;
    return ZDD::getnode_raw(var, lo, hi);
}

ZDD ZDD::getnode(bddvar var, const ZDD& lo, const ZDD& hi) {
    ZDD z(0);
    z.root = ZDD::getnode(var, lo.root, hi.root);
    return z;
}

// --- MTBDD node creation (no complement edges) ---

bddp mtbdd_getnode_raw(bddvar var, bddp lo, bddp hi) {
    // BDD reduction rule: lo == hi -> return lo
    if (lo == hi) return lo;

    // No complement edge normalization.
    // MTBDD never uses complement edges on non-terminal nodes.
    // (Terminal bddp values may have bit 0 set as part of the index.)
    BDD_DEBUG_ASSERT((lo & BDD_CONST_FLAG) || (lo & BDD_COMP_FLAG) == 0);
    BDD_DEBUG_ASSERT((hi & BDD_CONST_FLAG) || (hi & BDD_COMP_FLAG) == 0);

    bddp found = BDD_UniqueTableLookup(var, lo, hi);
    if (found != 0) {
        return found;
    }
    bddp node_id = allocate_node();
    node_write(node_id, var, lo, hi);
    if (bddp_is_reduced(lo) && bddp_is_reduced(hi)) {
        node_set_reduced(node_id);
    }
    try {
        BDD_UniqueTableInsert(var, lo, hi, node_id);
    } catch (...) {
        if (node_id / 2 == bdd_node_used) {
            bdd_node_used--;
        } else {
            uint64_t idx = node_id / 2 - 1;
            bdd_nodes[idx].data[0] = bdd_free_list;
            bdd_free_list = node_id;
            bdd_free_count++;
        }
        throw;
    }
    return node_id;
}

bddp mtbdd_getnode(bddvar var, bddp lo, bddp hi) {
    if (lo == bddnull || hi == bddnull)
        throw std::invalid_argument("mtbdd_getnode: bddnull child");
    if (var < 1 || var > bdd_varcount)
        throw std::invalid_argument("mtbdd_getnode: var out of range");
    bddvar var_level = var2level[var];
    if (!(lo & BDD_CONST_FLAG)) {
        if (lo & BDD_COMP_FLAG)
            throw std::invalid_argument("mtbdd_getnode: lo has complement flag");
        if (lo < 2 || lo / 2 - 1 >= bdd_node_used)
            throw std::invalid_argument("mtbdd_getnode: lo child is not a valid node");
        if (var2level[node_var(lo)] >= var_level)
            throw std::invalid_argument("mtbdd_getnode: lo child level >= var level");
    }
    if (!(hi & BDD_CONST_FLAG)) {
        if (hi & BDD_COMP_FLAG)
            throw std::invalid_argument("mtbdd_getnode: hi has complement flag");
        if (hi < 2 || hi / 2 - 1 >= bdd_node_used)
            throw std::invalid_argument("mtbdd_getnode: hi child is not a valid node");
        if (var2level[node_var(hi)] >= var_level)
            throw std::invalid_argument("mtbdd_getnode: hi child level >= var level");
    }
    return mtbdd_getnode_raw(var, lo, hi);
}

// --- MTZDD node creation (no complement edges) ---

bddp mtzdd_getnode_raw(bddvar var, bddp lo, bddp hi) {
    // ZDD zero-suppression rule: hi == zero_terminal -> return lo
    if (hi == (BDD_CONST_FLAG | 0)) return lo;

    // No complement edge normalization.
    BDD_DEBUG_ASSERT((lo & BDD_CONST_FLAG) || (lo & BDD_COMP_FLAG) == 0);
    BDD_DEBUG_ASSERT((hi & BDD_CONST_FLAG) || (hi & BDD_COMP_FLAG) == 0);

    bddp found = BDD_UniqueTableLookup(var, lo, hi);
    if (found != 0) {
        return found;
    }
    bddp node_id = allocate_node();
    node_write(node_id, var, lo, hi);
    if (bddp_is_reduced(lo) && bddp_is_reduced(hi)) {
        node_set_reduced(node_id);
    }
    try {
        BDD_UniqueTableInsert(var, lo, hi, node_id);
    } catch (...) {
        if (node_id / 2 == bdd_node_used) {
            bdd_node_used--;
        } else {
            uint64_t idx = node_id / 2 - 1;
            bdd_nodes[idx].data[0] = bdd_free_list;
            bdd_free_list = node_id;
            bdd_free_count++;
        }
        throw;
    }
    return node_id;
}

bddp mtzdd_getnode(bddvar var, bddp lo, bddp hi) {
    if (lo == bddnull || hi == bddnull)
        throw std::invalid_argument("mtzdd_getnode: bddnull child");
    if (var < 1 || var > bdd_varcount)
        throw std::invalid_argument("mtzdd_getnode: var out of range");
    bddvar var_level = var2level[var];
    if (!(lo & BDD_CONST_FLAG)) {
        if (lo & BDD_COMP_FLAG)
            throw std::invalid_argument("mtzdd_getnode: lo has complement flag");
        if (lo < 2 || lo / 2 - 1 >= bdd_node_used)
            throw std::invalid_argument("mtzdd_getnode: lo child is not a valid node");
        if (var2level[node_var(lo)] >= var_level)
            throw std::invalid_argument("mtzdd_getnode: lo child level >= var level");
    }
    if (!(hi & BDD_CONST_FLAG)) {
        if (hi & BDD_COMP_FLAG)
            throw std::invalid_argument("mtzdd_getnode: hi has complement flag");
        if (hi < 2 || hi / 2 - 1 >= bdd_node_used)
            throw std::invalid_argument("mtzdd_getnode: hi child is not a valid node");
        if (var2level[node_var(hi)] >= var_level)
            throw std::invalid_argument("mtzdd_getnode: hi child level >= var level");
    }
    return mtzdd_getnode_raw(var, lo, hi);
}

// --- QDD node creation ---

bddp QDD::getnode_raw(bddvar var, bddp lo, bddp hi) {
    // No jump rule: lo == hi does NOT return lo

    // Complement edge normalization: lo must not be complemented.
    // BDD semantics: ~(var, lo, hi) = (var, ~lo, ~hi)
    bool comp = (lo & BDD_COMP_FLAG) != 0;
    if (comp) {
        lo = bddnot(lo);
        hi = bddnot(hi);
    }

    bddp found = BDD_UniqueTableLookup(var, lo, hi);
    if (found != 0) {
        return comp ? bddnot(found) : found;
    }
    bddp node_id = allocate_node();
    node_write(node_id, var, lo, hi);
    BDD_DEBUG_ASSERT(bddp_is_reduced(lo) && bddp_is_reduced(hi));
    if (bddp_is_reduced(lo) && bddp_is_reduced(hi)) {
        node_set_reduced(node_id);
    }
    try {
        BDD_UniqueTableInsert(var, lo, hi, node_id);
    } catch (...) {
        if (node_id / 2 == bdd_node_used) {
            bdd_node_used--;
        } else {
            uint64_t idx = node_id / 2 - 1;
            bdd_nodes[idx].data[0] = bdd_free_list;
            bdd_free_list = node_id;
            bdd_free_count++;
        }
        throw;
    }
    return comp ? bddnot(node_id) : node_id;
}

bddp QDD::getnode(bddvar var, bddp lo, bddp hi) {
    if (lo == bddnull || hi == bddnull)
        throw std::invalid_argument("QDD::getnode: bddnull child");
    if (var < 1 || var > bdd_varcount)
        throw std::invalid_argument("QDD::getnode: var out of range");
    // Level validation: children must be at var's level - 1
    bddvar expected_child_level = var2level[var] - 1;

    auto child_level = [](bddp child) -> bddvar {
        if (child & BDD_CONST_FLAG) return 0;  // terminal is level 0
        bddp phys = child & ~BDD_COMP_FLAG;
        if (phys < 2 || phys / 2 - 1 >= bdd_node_used)
            throw std::invalid_argument("QDD::getnode: child is not a valid node");
        return var2level[node_var(phys)];
    };

    bddvar lo_level = child_level(lo);
    bddvar hi_level = child_level(hi);
    if (lo_level != expected_child_level || hi_level != expected_child_level) {
        throw std::invalid_argument("QDD::getnode: child level mismatch");
    }

    return QDD::getnode_raw(var, lo, hi);
}

QDD QDD::getnode(bddvar var, const QDD& lo, const QDD& hi) {
    QDD q(0);
    q.root = QDD::getnode(var, lo.root, hi.root);
    return q;
}

bddp bddconst(uint64_t val) {
    if (val > 1) {
        throw std::invalid_argument("bddconst: val must be 0 or 1");
    }
    return BDD_CONST_FLAG | val;
}

bddp bddprime(bddvar v) {
    if (v < 1) {
        throw std::invalid_argument("bddprime: var out of range");
    }
    if (v > 65536) {
        throw std::invalid_argument(
            "bddprime: auto-expansion beyond 2^16 variables is not supported");
    }
    // Auto-expand variables if needed
    while (bdd_varcount < v) {
        bddnewvar();
    }
    return BDD::getnode_raw(v, bddfalse, bddtrue);
}

BDD BDD_ID(bddp p) {
    if (p != bddnull && !(p & BDD_CONST_FLAG)) {
        if (!bddp_is_reduced(p)) {
            throw std::invalid_argument("BDD_ID: node is not reduced");
        }
    }
    BDD b(0);
    b.root = p;
    return b;
}

ZDD ZDD_ID(bddp p) {
    if (p != bddnull && !(p & BDD_CONST_FLAG)) {
        if (!bddp_is_reduced(p)) {
            throw std::invalid_argument("ZDD_ID: node is not reduced");
        }
    }
    ZDD z(0);
    z.root = p;
    return z;
}

uint64_t BDD_Used() {
    return bddused();
}

int BDD_GC() {
    return bddgc();
}

BDD BDDvar(bddvar v) {
    return BDD_ID(bddprime(v));
}

ZDD ZDD_Meet(const ZDD& f, const ZDD& g) {
    ZDD z(0);
    z.root = bddmeet(f.root, g.root);
    return z;
}

BDD BDD_CacheBDD(uint8_t op, bddp f, bddp g) {
    bddp r = bddrcache(op, f, g);
    return BDD_ID(r);
}

ZDD BDD_CacheZDD(uint8_t op, bddp f, bddp g) {
    bddp r = bddrcache(op, f, g);
    return ZDD_ID(r);
}

BDD BDD::cache_get(uint8_t op, const BDD& f, const BDD& g) {
    return BDD_ID(bddrcache(op, f.get_id(), g.get_id()));
}

void BDD::cache_put(uint8_t op, const BDD& f, const BDD& g, const BDD& result) {
    bddwcache(op, f.get_id(), g.get_id(), result.get_id());
}

ZDD ZDD::cache_get(uint8_t op, const ZDD& f, const ZDD& g) {
    return ZDD_ID(bddrcache(op, f.get_id(), g.get_id()));
}

void ZDD::cache_put(uint8_t op, const ZDD& f, const ZDD& g, const ZDD& result) {
    bddwcache(op, f.get_id(), g.get_id(), result.get_id());
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

bool bdd_check_reduced(bddp root) {
    if (root == bddnull) return true;
    if (root & BDD_CONST_FLAG) return true;
    std::unordered_set<bddp> visited;
    std::vector<bddp> stack;
    stack.push_back(root & ~BDD_COMP_FLAG);
    while (!stack.empty()) {
        bddp node = stack.back();
        stack.pop_back();
        if (node & BDD_CONST_FLAG) continue;
        bddp n = node & ~BDD_COMP_FLAG;
        if (!visited.insert(n).second) continue;
        if (!node_is_reduced(n)) return false;
        stack.push_back(node_lo(n));
        stack.push_back(node_hi(n));
    }
    return true;
}

