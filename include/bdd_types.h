#ifndef KYOTODD_BDD_TYPES_H
#define KYOTODD_BDD_TYPES_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <iosfwd>
#include <map>
#include <unordered_map>
#include <vector>
#include "bdd_node.h"
#include <stdexcept>

#include "bigint.hpp"

namespace kyotodd {

/** @brief DD node ID type (48-bit value stored in uint64_t). */
typedef uint64_t bddp;

/** @brief Memo table for exact counting (maps node ID to BigInt cardinality). */
typedef std::unordered_map<bddp, bigint::BigInt> CountMemoMap;

/** @brief Variable number type (31-bit value stored in uint32_t).
 *
 * Variable numbers start from 1 (1, 2, 3, ...). Stored in 31 bits
 * of a BddNode.
 */
typedef uint32_t bddvar;

/** @brief Constant flag (bit 47). When set, the node ID represents a terminal node. */
static constexpr bddp BDD_CONST_FLAG = UINT64_C(0x800000000000);  // bit 47: constant flag
/** @brief Complement flag (bit 0). When set, the edge is a complement (negated) edge. */
static constexpr bddp BDD_COMP_FLAG  = UINT64_C(0x000000000001);  // bit 0: complement flag

/** @brief Maximum variable number (31-bit). */
static constexpr bddvar bddvarmax = (UINT32_C(1) << (sizeof(uint64_t) * 8 - BDD_NODE_VAR_SHIFT)) - 1;
static constexpr bddvar BDD_MaxVar = bddvarmax;
static constexpr bddp BDD_MaxNode = BDD_CONST_FLAG / 2 - 2;
/** @brief Maximum constant value for terminal nodes (47-bit). */
static constexpr bddp bddvalmax = BDD_CONST_FLAG - 1;

/** @brief Constant false (0-terminal) for BDD. */
static constexpr bddp bddfalse  = BDD_CONST_FLAG | 0;  // 0-terminal
/** @brief Empty family (0-terminal) for ZDD. Alias of bddfalse. */
static constexpr bddp bddempty  = BDD_CONST_FLAG | 0;  // 0-terminal (ZDD alias)
/** @brief Constant true (1-terminal) for BDD. */
static constexpr bddp bddtrue   = BDD_CONST_FLAG | 1;  // 1-terminal
/** @brief Unit family {∅} (1-terminal) for ZDD. Alias of bddtrue. */
static constexpr bddp bddsingle = BDD_CONST_FLAG | 1;  // 1-terminal (ZDD alias)
/** @brief Null (error) node ID. Returned on invalid operations. */
static constexpr bddp bddnull   = UINT64_C(0x7FFFFFFFFFFF);  // error

/** @brief Maximum recursion depth for recursive operations.
 *  Chosen conservatively for typical thread stack sizes (1-8 MB).
 *  Each recursion frame is ~100-200 bytes, so 8192 levels ≈ 0.8-1.6 MB. */
static constexpr int BDD_RecurLimit = 8192;
/** @brief Current recursion depth counter. */
extern int BDD_RecurCount;

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
static const uint8_t BDD_OP_DIV = 18;
static const uint8_t BDD_OP_SYMDIFF = 19;
static const uint8_t BDD_OP_JOIN = 20;
static const uint8_t BDD_OP_MEET = 21;
static const uint8_t BDD_OP_DELTA = 22;
static const uint8_t BDD_OP_DISJOIN = 23;
static const uint8_t BDD_OP_JOINTJOIN = 24;
static const uint8_t BDD_OP_RESTRICT = 25;
static const uint8_t BDD_OP_PERMIT = 26;
static const uint8_t BDD_OP_REMOVE_SUPERSETS = 27;
static const uint8_t BDD_OP_REMOVE_SUBSETS = 28;
static const uint8_t BDD_OP_MAXIMAL = 29;
static const uint8_t BDD_OP_MINIMAL = 30;
static const uint8_t BDD_OP_MINHIT = 31;
static const uint8_t BDD_OP_CLOSURE = 32;
static const uint8_t BDD_OP_CARD = 33;
static const uint8_t BDD_OP_LIT = 34;
static const uint8_t BDD_OP_LEN = 35;
static const uint8_t BDD_OP_PERMITSYM  = 36;
static const uint8_t BDD_OP_ALWAYS     = 37;
static const uint8_t BDD_OP_SYMCHK     = 38;
static const uint8_t BDD_OP_SYMSET     = 39;
static const uint8_t BDD_OP_COIMPLYSET = 40;
static const uint8_t BDD_OP_SMOOTH = 41;
static const uint8_t BDD_OP_SPREAD = 42;
static const uint8_t BDD_OP_SWAP = 43;
static const uint8_t BDD_OP_REMAINDER = 44;
static const uint8_t BDD_OP_LSHIFTB = 45;
static const uint8_t BDD_OP_RSHIFTB = 46;
static const uint8_t BDD_OP_LSHIFTZ = 47;
static const uint8_t BDD_OP_RSHIFTZ = 48;
static const uint8_t BDD_OP_PRODUCT = 49;
static const uint8_t BDD_OP_CHOOSE  = 68;
static const uint8_t BDD_OP_MINSIZE = 69;

// Lowest op code reserved for user-supplied BDD/ZDD/QDD::cache_get/cache_put
// entries. Op codes [0, BDD_OP_USER_MIN) are reserved for built-in operations
// and must never collide with cache entries written by external callers; if
// they did, a poisoned entry could be read back by a subsequent core
// algorithm and falsify its result. See BDD::cache_get/put validation.
static const uint8_t BDD_OP_USER_MIN = 128;

/// @cond INTERNAL
// Forward declarations for GC root registration (defined in bdd_base.h)
void bddgc_protect(bddp* p);
void bddgc_unprotect(bddp* p);
/// @endcond

} // namespace kyotodd

#include "dd_base.h"

namespace kyotodd {

class BDD;
class ZDD;
class QDD;
struct SvgParams;

/** @brief Drawing mode for save_graphviz() and save_svg(). */
enum class DrawMode {
    Expanded, ///< Complement edges expanded into full nodes (default).
    Raw       ///< Physical DAG with complement edge markers (open circle).
};

/** @brief Weight aggregation mode for weighted_sample(). */
enum class WeightMode {
    Sum,     ///< w(S) = sum of weights[v] for v in S. Empty set weight = 0.
    Product  ///< w(S) = product of weights[v] for v in S. Empty set weight = 1.
};

/** @brief Execution mode for BDD operations. */
enum class BddExecMode {
    Recursive, ///< Use recursive implementation (uses call stack).
    Iterative, ///< Use iterative implementation (explicit heap stack, no depth limit).
    Auto       ///< Auto-select: recursive if max operand level <= BDD_RecurLimit, else iterative.
};
static constexpr BddExecMode BDD_EXEC_RECURSIVE = BddExecMode::Recursive;
static constexpr BddExecMode BDD_EXEC_ITERATIVE = BddExecMode::Iterative;
static constexpr BddExecMode BDD_EXEC_AUTO = BddExecMode::Auto;

/**
 * @brief Memo for ZDD exact counting, associated with a specific ZDD root.
 *
 * Stores the memo table for bddexactcount results. Once populated via
 * exact_count(), the memo is marked as stored and never changed.
 */
class ZddCountMemo {
public:
    explicit ZddCountMemo(bddp f);
    ZddCountMemo(const ZDD& f);

    bddp f() const { return f_; }
    bool stored() const { return stored_; }
    CountMemoMap& map() { return map_; }
    const CountMemoMap& map() const { return map_; }
    void mark_stored() { stored_ = true; }

private:
    bddp f_;
    bool stored_;
    CountMemoMap map_;
};

/**
 * @brief Memo for BDD exact counting, associated with a specific BDD root and variable count.
 *
 * Stores the memo table for bddexactcount results. Once populated via
 * exact_count(), the memo is marked as stored and never changed.
 *
 * @warning The memo caches results based on the variable-level mapping at
 * population time. If the mapping changes (e.g. via bddnewvaroflev()),
 * discard the memo and create a new one — cached values will be incorrect.
 */
class BddCountMemo {
public:
    BddCountMemo(bddp f, bddvar n);
    BddCountMemo(const BDD& f, bddvar n);

    bddp f() const { return f_; }
    bddvar n() const { return n_; }
    bool stored() const { return stored_; }
    CountMemoMap& map() { return map_; }
    const CountMemoMap& map() const { return map_; }
    void mark_stored() { stored_ = true; }

private:
    bddp f_;
    bddvar n_;
    bool stored_;
    CountMemoMap map_;
};

/**
 * @brief Interval-memoization table for BkTrk-IntervalMemo cost-bound algorithm.
 *
 * Stores (ZDD node, interval [aw, rb)) → result ZDD mappings. When the same
 * ZDD node is queried with a different cost bound that falls within a cached
 * interval, the cached result is reused without recomputation.
 *
 * Can be reused across multiple cost_bound_le() / cost_bound_eq() calls with
 * different bounds on the same ZDD to benefit from accumulated memoization.
 * Note: cost_bound_ge() does not use the caller-provided memo internally
 * (it uses negated weights, which would conflict with the memo's binding).
 *
 * A single CostBoundMemo must only be used with one weights vector. On first
 * use, the weights are bound via bind_weights(); subsequent calls with a
 * different weights vector throw std::invalid_argument.
 */
class CostBoundMemo {
public:
    CostBoundMemo();

    /**
     * @brief Look up a cached result for (f, b).
     * @return true if a cached interval [aw, rb) containing b was found.
     */
    bool lookup(bddp f, long long b,
                bddp& h, long long& aw, long long& rb) const;

    /**
     * @brief Insert a result for node f with interval [aw, rb).
     */
    void insert(bddp f, long long aw, long long rb, bddp h);

    /** @brief Clear all cached entries. The weights binding is preserved. */
    void clear();

    /**
     * @brief Invalidate memo if GC has run since last use.
     *
     * Called at the start of bddcostbound_le (inside bdd_gc_guard,
     * after potential GC at entry) to ensure stale bddp values
     * are not returned.
     */
    void invalidate_if_stale();

    /**
     * @brief Bind this memo to a specific weights vector.
     *
     * On first call, stores the weights. On subsequent calls,
     * throws std::invalid_argument if a different weights vector is passed.
     */
    void bind_weights(const std::vector<int>& weights);

private:
    // map_[f][aw] = (rb, h)
    std::unordered_map<bddp,
        std::map<long long, std::pair<long long, bddp>>> map_;
    uint64_t gc_generation_;
    std::vector<int> weights_;
    bool weights_bound_;
};

/** @brief Memo map type for weighted sampling (maps node ID to double). */
typedef std::unordered_map<bddp, double> WeightMemoMap;

/**
 * @brief Memo for weighted sampling, associated with a specific ZDD root.
 *
 * Caches precomputed weight aggregation values (total_sum or total_prod)
 * and set counts for the ZDD's DAG. Can be reused across multiple
 * weighted_sample calls with the same ZDD, weights, and mode.
 */
class WeightedSampleMemo {
public:
    WeightedSampleMemo(const ZDD& f, const std::vector<double>& weights,
                       WeightMode mode);

    bddp f() const { return f_; }
    bool stored() const { return stored_; }
    void mark_stored() { stored_ = true; }
    WeightMode mode() const { return mode_; }
    const std::vector<double>& weights() const { return weights_; }

    WeightMemoMap& weight_map() { return weight_map_; }
    const WeightMemoMap& weight_map() const { return weight_map_; }

    std::unordered_map<bddp, double>& count_map() { return count_map_; }
    const std::unordered_map<bddp, double>& count_map() const { return count_map_; }

private:
    bddp f_;
    bool stored_;
    WeightMode mode_;
    std::vector<double> weights_;
    WeightMemoMap weight_map_;
    std::unordered_map<bddp, double> count_map_;
};

} // namespace kyotodd

// BDD and ZDD class definitions live in dedicated headers so this file
// stays under the 2000-line limit from CLAUDE.md. They are included here
// so existing code that relies on "bdd_types.h" keeps compiling unchanged.
#include "bdd_class.h"
#include "zdd_class.h"

#endif
