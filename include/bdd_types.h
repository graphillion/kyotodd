#ifndef KYOTODD_BDD_TYPES_H
#define KYOTODD_BDD_TYPES_H

#include <cstdint>
#include <cstdio>
#include <iosfwd>
#include <vector>
#include "bdd_node.h"

namespace bigint { class BigInt; }

/** @brief DD node ID type (48-bit value stored in uint64_t). */
typedef uint64_t bddp;

/** @brief Variable number type (31-bit value stored in uint32_t).
 *
 * Variable numbers start from 1 (1, 2, 3, ...). Stored in 31 bits
 * of a BddNode.
 */
typedef uint32_t bddvar;

/** @brief Constant flag (bit 47). When set, the node ID represents a terminal node. */
static const bddp BDD_CONST_FLAG = UINT64_C(0x800000000000);  // bit 47: constant flag
/** @brief Complement flag (bit 0). When set, the edge is a complement (negated) edge. */
static const bddp BDD_COMP_FLAG  = UINT64_C(0x000000000001);  // bit 0: complement flag

/** @brief Maximum variable number (31-bit). */
static const bddvar bddvarmax = (UINT32_C(1) << (sizeof(uint64_t) * 8 - BDD_NODE_VAR_SHIFT)) - 1;
/** @brief Maximum constant value for terminal nodes (47-bit). */
static const bddp bddvalmax = BDD_CONST_FLAG - 1;

/** @brief Constant false (0-terminal) for BDD. */
static const bddp bddfalse  = BDD_CONST_FLAG | 0;  // 0-terminal
/** @brief Empty family (0-terminal) for ZDD. Alias of bddfalse. */
static const bddp bddempty  = BDD_CONST_FLAG | 0;  // 0-terminal (ZDD alias)
/** @brief Constant true (1-terminal) for BDD. */
static const bddp bddtrue   = BDD_CONST_FLAG | 1;  // 1-terminal
/** @brief Unit family {∅} (1-terminal) for ZDD. Alias of bddtrue. */
static const bddp bddsingle = BDD_CONST_FLAG | 1;  // 1-terminal (ZDD alias)
/** @brief Null (error) node ID. Returned on invalid operations. */
static const bddp bddnull   = UINT64_C(0x7FFFFFFFFFFF);  // error

/** @brief Maximum recursion depth for recursive operations. */
#define BDD_RecurLimit 8192
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

/// @cond INTERNAL
// Forward declarations for GC root registration (defined in bdd_base.h)
void bddgc_protect(bddp* p);
void bddgc_unprotect(bddp* p);
/// @endcond

/**
 * @brief A Binary Decision Diagram (BDD) node.
 *
 * Represents a Boolean function as a directed acyclic graph with complement
 * edges. Each BDD object holds a single root node ID and is automatically
 * protected from garbage collection during its lifetime.
 */
class BDD {
public:
    bddp root;  /**< @brief The root node ID of this BDD. */

    /**
     * @brief Construct a BDD from an integer value.
     * @param val 0 for false, 1 for true, negative for null.
     */
    explicit BDD(int val) : root(val < 0 ? bddnull : val == 0 ? bddfalse : bddtrue) {
        bddgc_protect(&root);
    }
    /** @brief Copy constructor. */
    BDD(const BDD& other) : root(other.root) {
        bddgc_protect(&root);
    }
    /** @brief Move constructor. */
    BDD(BDD&& other) : root(other.root) {
        bddgc_protect(&root);
        other.root = bddnull;
    }
    ~BDD() {
        bddgc_unprotect(&root);
    }
    /** @brief Copy assignment operator. */
    BDD& operator=(const BDD& other) {
        root = other.root;
        return *this;
    }
    /** @brief Move assignment operator. */
    BDD& operator=(BDD&& other) {
        root = other.root;
        other.root = bddnull;
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
     * @brief Compute the support set as a BDD (conjunction of variables).
     * @return A BDD representing the conjunction of all variables in the support.
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
    /**
     * @brief Return the number of nodes in this BDD.
     * @return The DAG node count.
     */
    uint64_t Size() const;
    /**
     * @brief If-then-else operation: (f AND g) OR (NOT f AND h).
     * @param f Condition BDD.
     * @param g Then-branch BDD.
     * @param h Else-branch BDD.
     * @return The resulting BDD.
     */
    static BDD Ite(const BDD& f, const BDD& g, const BDD& h);

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
class ZDD {
    friend ZDD ZDD_ID(bddp p);
    friend ZDD ZDD_Meet(const ZDD& f, const ZDD& g);

    bddp root;  /**< @brief The root node ID of this ZDD. */

public:
    /** @brief Get the raw node ID. */
    bddp GetID() const { return root; }

    /**
     * @brief Construct a ZDD from an integer value.
     * @param val 0 for empty family, 1 for unit family {∅}, negative for null.
     */
    explicit ZDD(int val) : root(val < 0 ? bddnull : val == 0 ? bddempty : bddsingle) {
        bddgc_protect(&root);
    }
    /** @brief Copy constructor. */
    ZDD(const ZDD& other) : root(other.root) {
        bddgc_protect(&root);
    }
    /** @brief Move constructor. */
    ZDD(ZDD&& other) : root(other.root) {
        bddgc_protect(&root);
        other.root = bddnull;
    }
    ~ZDD() {
        bddgc_unprotect(&root);
    }
    /** @brief Copy assignment operator. */
    ZDD& operator=(const ZDD& other) {
        root = other.root;
        return *this;
    }
    /** @brief Move assignment operator. */
    ZDD& operator=(ZDD&& other) {
        root = other.root;
        other.root = bddnull;
        return *this;
    }

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
     * @brief Select sets NOT containing variable @p var (same as Offset).
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
    ZDD operator<<(int s) const;
    /** @brief In-place left shift. */
    ZDD& operator<<=(int s);
    /** @brief Right shift (decrease variable numbers by @p s). */
    ZDD operator>>(int s) const;
    /** @brief In-place right shift. */
    ZDD& operator>>=(int s);
    /** @brief Equality comparison by node ID. */
    bool operator==(const ZDD& other) const { return root == other.root; }
    /** @brief Inequality comparison by node ID. */
    bool operator!=(const ZDD& other) const { return root != other.root; }

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
     * @return The cardinality of the family.
     */
    uint64_t Card() const;
    /**
     * @brief Count the number of sets in the family (arbitrary precision).
     * @return The cardinality of the family as a BigInt.
     */
    bigint::BigInt ExactCount() const;
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
     * For each pair (A, B) where A is in this family and B is in @p g,
     * include A ∪ B in the result (regardless of overlap).
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

    /** @brief Get the top variable number. */
    bddvar Top() const;
    /** @brief Count the number of nodes. */
    uint64_t Size() const;
    /** @brief Count the total number of literals across all sets. */
    uint64_t Lit() const;
    /** @brief Count the total number of sets weighted by set size. */
    uint64_t Len() const;
    /** @brief Return the cardinality as a hexadecimal string. */
    char* CardMP16(char* s) const;
    /** @brief Export to a FILE stream. */
    void Export(FILE* strm) const;
    /** @brief Export to an output stream. */
    void Export(std::ostream& strm) const;
    /** @brief Print ZDD statistics (ID, Var, Size, Card, Lit, Len). */
    void Print() const;
    /** @brief Print in PLA format. */
    void PrintPla() const;
    /** @brief Adjust ZDD to a given number of levels. */
    ZDD ZLev(int lev, int last) const;
    /** @brief Set ZDD skip flags. */
    void SetZSkip() const;

    /** @brief Check if the family is a polynomial (has ≥ 2 sets). */
    int IsPoly() const;
    /** @brief Swap two variables in the family. */
    ZDD Swap(int v1, int v2) const;

    static const ZDD Empty;   /**< @brief Empty family (no sets). */
    static const ZDD Single;  /**< @brief Unit family containing only the empty set {∅}. */
    static const ZDD Null;    /**< @brief Null (error) ZDD. */
};

#endif
