#ifndef KYOTODD_BDD_TYPES_H
#define KYOTODD_BDD_TYPES_H

#include <cstdint>
#include <cstdio>
#include <functional>
#include <iosfwd>
#include <unordered_map>
#include <vector>
#include "bdd_node.h"

#include "bigint.hpp"

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
static const uint8_t BDD_OP_NONSUP = 27;
static const uint8_t BDD_OP_NONSUB = 28;
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

/// @cond INTERNAL
// Forward declarations for GC root registration (defined in bdd_base.h)
void bddgc_protect(bddp* p);
void bddgc_unprotect(bddp* p);
/// @endcond

#include "dd_base.h"

class BDD;
class ZDD;
class QDD;

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
 * @brief A Binary Decision Diagram (BDD) node.
 *
 * Represents a Boolean function as a directed acyclic graph with complement
 * edges. Each BDD object holds a single root node ID and is automatically
 * protected from garbage collection during its lifetime.
 */
class BDD : public DDBase {
    /// @cond INTERNAL
    friend BDD BDD_ID(bddp p);
    /// @endcond

public:
    /** @brief Get the raw node ID. */
    bddp GetID() const { return get_id(); }

    /** @brief Default constructor. Constructs a false BDD. */
    BDD() : DDBase() {}
    /**
     * @brief Construct a BDD from an integer value.
     * @param val 0 for false, 1 for true, negative for null.
     */
    explicit BDD(int val) : DDBase(val) {}
    /** @brief Copy constructor. */
    BDD(const BDD& other) : DDBase(other) {}
    /** @brief Move constructor. */
    BDD(BDD&& other) : DDBase(std::move(other)) {}
    /** @brief Copy assignment operator. */
    BDD& operator=(const BDD& other) {
        DDBase::operator=(other);
        return *this;
    }
    /** @brief Move assignment operator. */
    BDD& operator=(BDD&& other) {
        DDBase::operator=(std::move(other));
        return *this;
    }

    /** @brief Logical AND. */
    BDD operator&(const BDD& other) const;
    /** @brief In-place logical AND. */
    BDD& operator&=(const BDD& other);
    /** @brief Logical OR. */
    BDD operator|(const BDD& other) const;
    /** @brief In-place logical OR. */
    BDD& operator|=(const BDD& other);
    /** @brief Logical XOR. */
    BDD operator^(const BDD& other) const;
    /** @brief In-place logical XOR. */
    BDD& operator^=(const BDD& other);
    /** @brief Logical NOT (complement). */
    BDD operator~() const;
    /** @brief Equality comparison by node ID. */
    bool operator==(const BDD& other) const { return root == other.root; }
    /** @brief Inequality comparison by node ID. */
    bool operator!=(const BDD& other) const { return root != other.root; }
    /** @brief Left shift: rename variable i to variable i+shift for all variables. */
    BDD operator<<(bddvar shift) const;
    /** @brief In-place left shift. */
    BDD& operator<<=(bddvar shift);
    /** @brief Right shift: rename variable i to variable i-shift for all variables. */
    BDD operator>>(bddvar shift) const;
    /** @brief In-place right shift. */
    BDD& operator>>=(bddvar shift);

    /**
     * @brief Cofactor: restrict variable @p v to 0.
     * @param v Variable number.
     * @return The resulting BDD.
     */
    BDD At0(bddvar v) const;
    /**
     * @brief Cofactor: restrict variable @p v to 1.
     * @param v Variable number.
     * @return The resulting BDD.
     */
    BDD At1(bddvar v) const;
    /**
     * @brief Existential quantification by a cube BDD.
     * @param cube A conjunction of variables to quantify out.
     * @return The resulting BDD.
     */
    BDD Exist(const BDD& cube) const;
    /**
     * @brief Existential quantification by a list of variables.
     * @param vars Variable numbers to quantify out.
     * @return The resulting BDD.
     */
    BDD Exist(const std::vector<bddvar>& vars) const;
    /**
     * @brief Existential quantification of a single variable.
     * @param v Variable number to quantify out.
     * @return The resulting BDD.
     */
    BDD Exist(bddvar v) const;
    /**
     * @brief Universal quantification by a cube BDD.
     * @param cube A conjunction of variables to quantify.
     * @return The resulting BDD.
     */
    BDD Univ(const BDD& cube) const;
    /**
     * @brief Universal quantification by a list of variables.
     * @param vars Variable numbers to quantify.
     * @return The resulting BDD.
     */
    BDD Univ(const std::vector<bddvar>& vars) const;
    /**
     * @brief Universal quantification of a single variable.
     * @param v Variable number to quantify.
     * @return The resulting BDD.
     */
    BDD Univ(bddvar v) const;
    /**
     * @brief Generalized cofactor by BDD @p g.
     * @param g The constraining BDD (must not be false).
     * @return The generalized cofactor of this BDD with respect to @p g.
     */
    BDD Cofactor(const BDD& g) const;
    /**
     * @brief Compute the support set as a BDD (disjunction of variables).
     * @return A BDD representing the disjunction of all variables in the support.
     */
    BDD Support() const;
    /**
     * @brief Compute the support set as a vector of variable numbers.
     * @return A vector of variable numbers that this BDD depends on.
     */
    std::vector<bddvar> SupportVec() const;
    /**
     * @brief Check implication: whether this BDD implies @p g.
     * @param g The BDD to check implication against.
     * @return 1 if this BDD implies @p g, 0 otherwise.
     */
    int Imply(const BDD& g) const;
    /** @brief Get the top variable number. */
    bddvar Top() const { return top(); }
    using DDBase::raw_size;
    /**
     * @brief Return the number of nodes in this BDD (with complement edge sharing).
     * @deprecated Use raw_size() or plain_size() instead.
     */
    uint64_t Size() const { return raw_size(); }
    /** @brief Return the number of nodes without complement edge sharing. */
    uint64_t plain_size() const;
    /** @brief Return the shared node count across multiple BDDs (with complement edge sharing). */
    static uint64_t raw_size(const std::vector<BDD>& v);
    /** @brief Return the shared node count across multiple BDDs (without complement edge sharing). */
    static uint64_t plain_size(const std::vector<BDD>& v);
    /** @brief Export to a FILE stream. */
    void Export(FILE* strm) const;
    /** @brief Export to an output stream. */
    void Export(std::ostream& strm) const;
    /** @brief Print BDD summary (ID, Var, Level, Size) to stdout. */
    void Print() const;
    /** @brief Print BDD graph (bddgraph0 wrapper). */
    void XPrint0() const;
    /** @brief Print BDD graph (bddgraph wrapper). */
    void XPrint() const;
    /**
     * @brief Swap variables v1 and v2 in the BDD.
     * @param v1 First variable number.
     * @param v2 Second variable number.
     * @return The BDD with v1 and v2 swapped.
     */
    BDD Swap(bddvar v1, bddvar v2) const;
    /**
     * @brief Smooth (existential quantification) of variable v.
     * @param v Variable number to quantify out.
     * @return The BDD with variable v existentially quantified.
     */
    BDD Smooth(bddvar v) const;
    /**
     * @brief Spread variable values to neighboring k levels.
     * @param k Number of levels to spread (must be >= 0).
     * @return The BDD with values spread by k levels.
     */
    BDD Spread(int k) const;
    /**
     * @brief Count the number of satisfying assignments (double approximation).
     *
     * Returns the number of assignments in {0,1}^n that satisfy this
     * Boolean function. Variables 1..n not appearing in the BDD are
     * treated as don't-cares.
     *
     * @param n The number of Boolean variables.
     * @return The satisfying assignment count as a double.
     */
    double count(bddvar n) const;
    /**
     * @brief Count the number of satisfying assignments (arbitrary precision).
     *
     * Same as count(n) but returns an exact BigInt result.
     *
     * @param n The number of Boolean variables.
     * @return The satisfying assignment count as a BigInt.
     */
    bigint::BigInt exact_count(bddvar n) const;
    /**
     * @brief Count the number of satisfying assignments (arbitrary precision),
     *        using an external memo.
     *
     * @param n The number of Boolean variables.
     * @param memo A BddCountMemo created for this BDD and n.
     * @return The satisfying assignment count as a BigInt.
     */
    bigint::BigInt exact_count(bddvar n, BddCountMemo& memo) const;
    /**
     * @brief Uniformly sample one satisfying assignment at random.
     *
     * Each satisfying assignment in {0,1}^n is selected with equal
     * probability. Variables assigned to 1 are returned in decreasing
     * level order. Populates the memo if not already stored.
     *
     * @tparam RNG A uniform random bit generator (e.g. std::mt19937_64).
     * @param rng The random number generator.
     * @param n The number of Boolean variables.
     * @param memo A BddCountMemo created for this BDD and n.
     * @return The sampled assignment as a vector of variables set to 1.
     */
    template<typename RNG>
    std::vector<bddvar> uniform_sample(RNG& rng, bddvar n, BddCountMemo& memo);

    /// @cond INTERNAL
    /// Internal: traverse BDD and collect sampled variables using
    /// a function that generates uniform random BigInts.
    /// @param rand_func A callable taking BigInt upper → BigInt in [0, upper).
    /// @param n The number of Boolean variables.
    /// @param memo A BddCountMemo created for this BDD and n.
    std::vector<bddvar> uniform_sample_impl(
        std::function<bigint::BigInt(const bigint::BigInt&)> rand_func,
        bddvar n, BddCountMemo& memo);
    /// @endcond
    /**
     * @brief If-then-else operation: (f AND g) OR (NOT f AND h).
     * @param f Condition BDD.
     * @param g Then-branch BDD.
     * @param h Else-branch BDD.
     * @return The resulting BDD.
     */
    static BDD Ite(const BDD& f, const BDD& g, const BDD& h);

    /** @brief Get the raw 0-child (lo) node ID without complement resolution. */
    static bddp raw_child0(bddp f);
    /** @brief Get the raw 1-child (hi) node ID without complement resolution. */
    static bddp raw_child1(bddp f);
    /** @brief Get the raw child node ID by index (0 or 1) without complement resolution. */
    static bddp raw_child(bddp f, int child);
    /** @brief Get the 0-child (lo) node ID with complement edge resolution. */
    static bddp child0(bddp f);
    /** @brief Get the 1-child (hi) node ID with complement edge resolution. */
    static bddp child1(bddp f);
    /** @brief Get the child node ID by index (0 or 1) with complement edge resolution. */
    static bddp child(bddp f, int child);

    /** @brief Get the raw 0-child (lo) as a BDD without complement resolution. */
    BDD raw_child0() const;
    /** @brief Get the raw 1-child (hi) as a BDD without complement resolution. */
    BDD raw_child1() const;
    /** @brief Get the raw child by index (0 or 1) as a BDD without complement resolution. */
    BDD raw_child(int child) const;
    /** @brief Get the 0-child (lo) as a BDD with complement edge resolution. */
    BDD child0() const;
    /** @brief Get the 1-child (hi) as a BDD with complement edge resolution. */
    BDD child1() const;
    /** @brief Get the child by index (0 or 1) as a BDD with complement edge resolution. */
    BDD child(int child) const;

    static const BDD False;  /**< @brief Constant false BDD. */
    static const BDD True;   /**< @brief Constant true BDD. */
    static const BDD Null;   /**< @brief Null (error) BDD. */
};

/**
 * @brief A Zero-suppressed Decision Diagram (ZDD) node.
 *
 * Represents a family of sets as a DAG with complement edges.
 * Each ZDD object holds a single root node ID and is automatically
 * protected from garbage collection during its lifetime.
 */
class ZDD : public DDBase {
    /// @cond INTERNAL
    friend ZDD ZDD_ID(bddp p);
    friend ZDD ZDD_Meet(const ZDD& f, const ZDD& g);
    /// @endcond

public:
    /** @brief Get the raw node ID. */
    bddp GetID() const { return get_id(); }

    /** @brief Default constructor: empty family. */
    ZDD() : DDBase() {}
    /**
     * @brief Construct a ZDD from an integer value.
     * @param val 0 for empty family, 1 for unit family {∅}, negative for null.
     */
    explicit ZDD(int val) : DDBase(val) {}

    /**
     * @brief Toggle membership of variable @p var in all sets.
     *
     * For each set S in the family, if var is in S then remove it,
     * otherwise add it.
     * @param var Variable number.
     * @return The resulting ZDD.
     */
    ZDD Change(bddvar var) const;
    /**
     * @brief Remove variable @p var from all sets (0-edge selection).
     *
     * Select sets that do NOT contain @p var. Equivalent to
     * restricting @p var to 0.
     * @param var Variable number.
     * @return The resulting ZDD.
     */
    ZDD Offset(bddvar var) const;
    /**
     * @brief Select sets containing variable @p var, then remove @p var (1-edge selection).
     *
     * Select sets that contain @p var and remove @p var from each.
     * @param var Variable number.
     * @return The resulting ZDD.
     */
    ZDD OnSet(bddvar var) const;
    /**
     * @brief Select sets containing variable @p var and remove @p var from them (same as OnSet with var removed).
     * @param var Variable number.
     * @return The resulting ZDD.
     */
    ZDD OnSet0(bddvar var) const;

    /** @brief Union of two families (set-theoretic union). */
    ZDD operator+(const ZDD& other) const;
    /** @brief In-place union. */
    ZDD& operator+=(const ZDD& other);
    /** @brief Subtraction (set difference). */
    ZDD operator-(const ZDD& other) const;
    /** @brief In-place subtraction. */
    ZDD& operator-=(const ZDD& other);
    /** @brief Intersection of two families. */
    ZDD operator&(const ZDD& other) const;
    /** @brief In-place intersection. */
    ZDD& operator&=(const ZDD& other);
    /** @brief Intersection with another family. */
    ZDD Intersec(const ZDD& other) const;
    /** @brief Division (quotient). */
    ZDD operator/(const ZDD& other) const;
    /** @brief In-place division. */
    ZDD& operator/=(const ZDD& other);
    /** @brief Symmetric difference. */
    ZDD operator^(const ZDD& other) const;
    /** @brief In-place symmetric difference. */
    ZDD& operator^=(const ZDD& other);
    /** @brief Join (cross product with union of elements). */
    ZDD operator*(const ZDD& other) const;
    /** @brief In-place join. */
    ZDD& operator*=(const ZDD& other);
    /** @brief Remainder (modulo). */
    ZDD operator%(const ZDD& other) const;
    /** @brief In-place remainder. */
    ZDD& operator%=(const ZDD& other);
    /** @brief Left shift (increase variable numbers by @p s). */
    ZDD operator<<(bddvar s) const;
    /** @brief In-place left shift. */
    ZDD& operator<<=(bddvar s);
    /** @brief Right shift (decrease variable numbers by @p s). */
    ZDD operator>>(bddvar s) const;
    /** @brief In-place right shift. */
    ZDD& operator>>=(bddvar s);
    /** @brief Complement: toggle empty set membership in the family. */
    ZDD operator~() const;
    /** @brief Equality comparison by node ID. */
    bool operator==(const ZDD& other) const { return root == other.root; }
    /** @brief Inequality comparison by node ID. */
    bool operator!=(const ZDD& other) const { return root != other.root; }

    /**
     * @brief Meet operation (intersection of all element pairs).
     *
     * For each pair of sets (one from this family, one from @p other),
     * compute their intersection and collect all results.
     * @param other Another ZDD family.
     * @return The resulting ZDD.
     */
    ZDD Meet(const ZDD& other) const;

    /**
     * @brief Extract maximal sets (no proper superset in the family).
     * @return A ZDD containing only the maximal sets.
     */
    ZDD Maximal() const;
    /**
     * @brief Extract minimal sets (no proper subset in the family).
     * @return A ZDD containing only the minimal sets.
     */
    ZDD Minimal() const;
    /**
     * @brief Compute minimum hitting sets.
     *
     * A hitting set intersects every set in the family. Returns the
     * family of all inclusion-minimal hitting sets.
     * @return A ZDD of minimal hitting sets.
     */
    ZDD Minhit() const;
    /**
     * @brief Compute the downward closure.
     *
     * Returns all subsets of sets in the family.
     * @return A ZDD representing the downward closure.
     */
    ZDD Closure() const;
    /**
     * @brief Count the number of sets in the family.
     * @deprecated Use count() or exact_count() instead.
     * @return The cardinality of the family.
     */
    uint64_t Card() const;
    /**
     * @brief Count the number of sets in the family (double approximation).
     * @return The cardinality of the family as a double.
     */
    double count() const;
    /**
     * @brief Count the number of sets in the family (arbitrary precision).
     * @return The cardinality of the family as a BigInt.
     */
    bigint::BigInt exact_count() const;
    /**
     * @brief Count the number of sets in the family (arbitrary precision),
     *        using an external memo.
     *
     * @param memo A ZddCountMemo created for this ZDD.
     * @return The cardinality of the family as a BigInt.
     */
    bigint::BigInt exact_count(ZddCountMemo& memo) const;
    /**
     * @brief Uniformly sample one set from the family at random.
     *
     * Each set in the family is selected with equal probability.
     * Populates the memo if not already stored.
     *
     * @tparam RNG A uniform random bit generator (e.g. std::mt19937_64).
     * @param rng The random number generator.
     * @param memo A ZddCountMemo created for this ZDD.
     * @return The sampled set as a sorted vector of variable numbers.
     */
    template<typename RNG>
    std::vector<bddvar> uniform_sample(RNG& rng, ZddCountMemo& memo);

    /// @cond INTERNAL
    /// Internal: traverse ZDD and collect sampled variables using
    /// a function that generates uniform random BigInts.
    /// @param rand_func A callable taking BigInt upper → BigInt in [0, upper).
    /// @param memo A ZddCountMemo created for this ZDD.
    std::vector<bddvar> uniform_sample_impl(
        std::function<bigint::BigInt(const bigint::BigInt&)> rand_func,
        ZddCountMemo& memo);
    /// @endcond
    /**
     * @brief Restrict to sets that are subsets of some set in @p g.
     * @param g The constraining family.
     * @return The restricted ZDD.
     */
    ZDD Restrict(const ZDD& g) const;
    /**
     * @brief Keep sets whose elements are all permitted by @p g.
     * @param g The permitting family.
     * @return The resulting ZDD.
     */
    ZDD Permit(const ZDD& g) const;
    /**
     * @brief Remove sets that are supersets of some set in @p g.
     * @param g The constraining family.
     * @return The resulting ZDD.
     */
    ZDD Nonsup(const ZDD& g) const;
    /**
     * @brief Remove sets that are subsets of some set in @p g.
     * @param g The constraining family.
     * @return The resulting ZDD.
     */
    ZDD Nonsub(const ZDD& g) const;
    /**
     * @brief Disjoint product of two families.
     *
     * For each pair (A, B) where A is in this family and B is in @p g,
     * if A and B are disjoint, include A ∪ B in the result.
     * @param g The other family.
     * @return The resulting ZDD.
     */
    ZDD Disjoin(const ZDD& g) const;
    /**
     * @brief Joint join of two families.
     *
     * For each pair (A, B) where A is in this family and B is in @p g
     * with A ∩ B ≠ ∅, include A ∪ B in the result.
     * Pairs with no overlap are excluded.
     * @param g The other family.
     * @return The resulting ZDD.
     */
    ZDD Jointjoin(const ZDD& g) const;
    /**
     * @brief Delta operation (symmetric difference of elements within pairs).
     * @param g The other family.
     * @return The resulting ZDD.
     */
    ZDD Delta(const ZDD& g) const;

    /** @brief Compute the support set (bddsupport wrapper). */
    ZDD Support() const;
    /** @brief Get the top variable number. */
    bddvar Top() const { return top(); }
    /**
     * @brief Count the number of nodes (with complement edge sharing).
     * @deprecated Use raw_size() or plain_size() instead.
     */
    uint64_t Size() const { return raw_size(); }
    using DDBase::raw_size;
    /** @brief Return the number of nodes without complement edge sharing. */
    uint64_t plain_size() const;
    /** @brief Return the shared node count across multiple ZDDs (with complement edge sharing). */
    static uint64_t raw_size(const std::vector<ZDD>& v);
    /** @brief Return the shared node count across multiple ZDDs (without complement edge sharing). */
    static uint64_t plain_size(const std::vector<ZDD>& v);
    /** @brief Count the total number of literals across all sets. */
    uint64_t Lit() const;
    /** @brief Return the maximum set size in the family. */
    uint64_t Len() const;
    /**
     * @brief Return the cardinality as a hexadecimal string.
     * @deprecated Use count() or exact_count() instead.
     */
    char* CardMP16(char* s) const;
    /** @brief Export to a FILE stream. */
    void Export(FILE* strm) const;
    /** @brief Export to an output stream. */
    void Export(std::ostream& strm) const;
    /** @brief Print ZDD statistics (ID, Var, Size, Card, Lit, Len). */
    void Print() const;
    /** @brief Print ZDD graph (bddgraph wrapper). */
    void XPrint() const;
    /** @brief Print in PLA format. */
    void PrintPla() const;
    /** @brief Adjust ZDD to a given number of levels. */
    ZDD ZLev(int lev, int last) const;
    /** @brief Set ZDD skip flags. */
    void SetZSkip() const;

    /** @brief Check if the family is a polynomial (has ≥ 2 sets). */
    int IsPoly() const;
    /** @brief Swap two variables in the family. */
    ZDD Swap(bddvar v1, bddvar v2) const;
    /** @brief Check if v1 implies v2 (all sets with v1 also have v2). */
    int ImplyChk(bddvar v1, bddvar v2) const;
    /** @brief Check co-implication between v1 and v2. */
    int CoImplyChk(bddvar v1, bddvar v2) const;
    /** @brief Keep sets with at most n elements. */
    ZDD PermitSym(int n) const;
    /** @brief Find elements common to all sets in the family. */
    ZDD Always() const;
    /** @brief Check if v1 and v2 are symmetric in the family. */
    int SymChk(bddvar v1, bddvar v2) const;
    /** @brief Find all variables implied by v. */
    ZDD ImplySet(bddvar v) const;
    /** @brief Find symmetry groups (size ≥ 2). */
    ZDD SymGrp() const;
    /** @brief Find symmetry groups (naive, includes size 1). */
    ZDD SymGrpNaive() const;
    /** @brief Find all variables symmetric with v. */
    ZDD SymSet(bddvar v) const;
    /** @brief Find all variables in co-implication with v. */
    ZDD CoImplySet(bddvar v) const;
    /** @brief Find a non-trivial divisor of the family. */
    ZDD Divisor() const;

    /** @brief Get the raw 0-child (lo) node ID without complement resolution. */
    static bddp raw_child0(bddp f);
    /** @brief Get the raw 1-child (hi) node ID without complement resolution. */
    static bddp raw_child1(bddp f);
    /** @brief Get the raw child node ID by index (0 or 1) without complement resolution. */
    static bddp raw_child(bddp f, int child);
    /** @brief Get the 0-child (lo) node ID with complement edge resolution (ZDD semantics). */
    static bddp child0(bddp f);
    /** @brief Get the 1-child (hi) node ID with complement edge resolution (ZDD semantics). */
    static bddp child1(bddp f);
    /** @brief Get the child node ID by index (0 or 1) with complement edge resolution (ZDD semantics). */
    static bddp child(bddp f, int child);

    /** @brief Get the raw 0-child (lo) as a ZDD without complement resolution. */
    ZDD raw_child0() const;
    /** @brief Get the raw 1-child (hi) as a ZDD without complement resolution. */
    ZDD raw_child1() const;
    /** @brief Get the raw child by index (0 or 1) as a ZDD without complement resolution. */
    ZDD raw_child(int child) const;
    /** @brief Get the 0-child (lo) as a ZDD with complement edge resolution (ZDD semantics). */
    ZDD child0() const;
    /** @brief Get the 1-child (hi) as a ZDD with complement edge resolution (ZDD semantics). */
    ZDD child1() const;
    /** @brief Get the child by index (0 or 1) as a ZDD with complement edge resolution (ZDD semantics). */
    ZDD child(int child) const;

    static const ZDD Empty;   /**< @brief Empty family (no sets). */
    static const ZDD Single;  /**< @brief Unit family containing only the empty set {∅}. */
    static const ZDD Null;    /**< @brief Null (error) ZDD. */
};

#endif
