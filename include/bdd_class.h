#ifndef KYOTODD_BDD_CLASS_H
#define KYOTODD_BDD_CLASS_H

// Included by bdd_types.h. Declares the BDD class. Pull in the umbrella
// header first so the types, constants, DDBase, and Memo classes that
// BDD depends on are already visible.

#include "bdd_types.h"

namespace kyotodd {


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
    /** @brief Get the raw node ID.
     *  @deprecated Use id() instead. */
    bddp GetID() const { return id(); }

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
    BDD at0(bddvar v) const;
    /** @deprecated Use at0() instead. */
    BDD At0(bddvar v) const { return at0(v); }
    /**
     * @brief Cofactor: restrict variable @p v to 1.
     * @param v Variable number.
     * @return The resulting BDD.
     */
    BDD at1(bddvar v) const;
    /** @deprecated Use at1() instead. */
    BDD At1(bddvar v) const { return at1(v); }
    /**
     * @brief Existential quantification by a variable-set BDD.
     * @param cube A BDD encoding the set of variables to quantify out (as returned by support()).
     * @return The resulting BDD.
     */
    BDD exist(const BDD& cube) const;
    /** @deprecated Use exist() instead. */
    BDD Exist(const BDD& cube) const { return exist(cube); }
    /**
     * @brief Existential quantification by a list of variables.
     * @param vars Variable numbers to quantify out.
     * @return The resulting BDD.
     */
    BDD exist(const std::vector<bddvar>& vars) const;
    /** @deprecated Use exist() instead. */
    BDD Exist(const std::vector<bddvar>& vars) const { return exist(vars); }
    /**
     * @brief Existential quantification of a single variable.
     * @param v Variable number to quantify out.
     * @return The resulting BDD.
     */
    BDD exist(bddvar v) const;
    /** @deprecated Use exist() instead. */
    BDD Exist(bddvar v) const { return exist(v); }
    /**
     * @brief Universal quantification by a variable-set BDD.
     * @param cube A BDD encoding the set of variables to quantify (as returned by support()).
     * @return The resulting BDD.
     */
    BDD univ(const BDD& cube) const;
    /** @deprecated Use univ() instead. */
    BDD Univ(const BDD& cube) const { return univ(cube); }
    /**
     * @brief Universal quantification by a list of variables.
     * @param vars Variable numbers to quantify.
     * @return The resulting BDD.
     */
    BDD univ(const std::vector<bddvar>& vars) const;
    /** @deprecated Use univ() instead. */
    BDD Univ(const std::vector<bddvar>& vars) const { return univ(vars); }
    /**
     * @brief Universal quantification of a single variable.
     * @param v Variable number to quantify.
     * @return The resulting BDD.
     */
    BDD univ(bddvar v) const;
    /** @deprecated Use univ() instead. */
    BDD Univ(bddvar v) const { return univ(v); }
    /**
     * @brief Generalized cofactor by BDD @p g.
     * @param g The constraining BDD (must not be false).
     * @return The generalized cofactor of this BDD with respect to @p g.
     */
    BDD cofactor(const BDD& g) const;
    /** @deprecated Use cofactor() instead. */
    BDD Cofactor(const BDD& g) const { return cofactor(g); }
    /**
     * @brief Compute the support set as a BDD (disjunction of variables).
     * @return A BDD representing the disjunction of all variables in the support.
     */
    BDD support() const;
    /** @deprecated Use support() instead. */
    BDD Support() const { return support(); }
    /**
     * @brief Compute the support set as a vector of variable numbers.
     * @return A vector of variable numbers that this BDD depends on.
     */
    std::vector<bddvar> support_vec() const;
    /** @deprecated Use support_vec() instead. */
    std::vector<bddvar> SupportVec() const { return support_vec(); }
    /**
     * @brief Check implication: whether this BDD implies @p g.
     * @param g The BDD to check implication against.
     * @return 1 if this BDD implies @p g, 0 otherwise.
     */
    int imply(const BDD& g) const;
    /** @deprecated Use imply() instead. */
    int Imply(const BDD& g) const { return imply(g); }
    /** @deprecated Use top() instead. */
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
    /** @brief Export to a FILE stream.
     *  @deprecated Use export_sapporo() instead. */
    void Export(FILE* strm) const;
    /** @brief Export to an output stream.
     *  @deprecated Use export_sapporo() instead. */
    void Export(std::ostream& strm) const;
    /** @brief Export this BDD in BDD binary format to a FILE stream. */
    void export_binary(FILE* strm) const;
    /** @brief Export this BDD in BDD binary format to an output stream. */
    void export_binary(std::ostream& strm) const;
    /** @brief Import a BDD from BDD binary format from a FILE stream.
     *  @param strm Input FILE stream.
     *  @param ignore_type If true, skip dd_type validation (default: false). */
    static BDD import_binary(FILE* strm, bool ignore_type = false);
    /** @brief Import a BDD from BDD binary format from an input stream. */
    static BDD import_binary(std::istream& strm, bool ignore_type = false);
    /** @brief Export multiple BDDs in binary format to a FILE stream. */
    static void export_binary_multi(FILE* strm, const std::vector<BDD>& bdds);
    /** @brief Export multiple BDDs in binary format to an output stream. */
    static void export_binary_multi(std::ostream& strm, const std::vector<BDD>& bdds);
    /** @brief Import multiple BDDs from binary format from a FILE stream. */
    static std::vector<BDD> import_binary_multi(FILE* strm, bool ignore_type = false);
    /** @brief Import multiple BDDs from binary format from an input stream. */
    static std::vector<BDD> import_binary_multi(std::istream& strm, bool ignore_type = false);
    /** @deprecated Use export_sapporo() or export_binary() instead. */
    void export_knuth(FILE* strm, bool is_hex = false, int offset = 0) const;
    /** @deprecated Use export_sapporo() or export_binary() instead. */
    void export_knuth(std::ostream& strm, bool is_hex = false, int offset = 0) const;
    /** @deprecated Use import_sapporo() or import_binary() instead. */
    static BDD import_knuth(FILE* strm, bool is_hex = false, int offset = 0);
    /** @deprecated Use import_sapporo() or import_binary() instead. */
    static BDD import_knuth(std::istream& strm, bool is_hex = false, int offset = 0);
    /** @brief Export this BDD in Sapporo format to a FILE stream. */
    void export_sapporo(FILE* strm) const;
    /** @brief Export this BDD in Sapporo format to an output stream. */
    void export_sapporo(std::ostream& strm) const;
    /** @brief Import a BDD from Sapporo format from a FILE stream. */
    static BDD import_sapporo(FILE* strm);
    /** @brief Import a BDD from Sapporo format from an input stream. */
    static BDD import_sapporo(std::istream& strm);
    /** @brief Save Graphviz DOT representation to a FILE stream. */
    void save_graphviz(FILE* strm, DrawMode mode = DrawMode::Expanded) const;
    /** @brief Save Graphviz DOT representation to an output stream. */
    void save_graphviz(std::ostream& strm, DrawMode mode = DrawMode::Expanded) const;
    /** @brief Save BDD as SVG to a file. */
    void save_svg(const char* filename, const SvgParams& params) const;
    void save_svg(const char* filename) const;
    /** @brief Save BDD as SVG to an output stream. */
    void save_svg(std::ostream& strm, const SvgParams& params) const;
    void save_svg(std::ostream& strm) const;
    /** @brief Export BDD as SVG and return the SVG string. */
    std::string save_svg(const SvgParams& params) const;
    std::string save_svg() const;
    /** @brief Print BDD summary (ID, Var, Level, Size) to stdout.
     *  @throws std::invalid_argument if this is BDD::Null. */
    void print() const;
    /** @deprecated Use print() instead. */
    void Print() const { print(); }
    /** @brief @deprecated Always throws std::logic_error. Use save_graphviz() instead.
     *  @note C++ only. Not available in the Python binding. */
    void XPrint0() const;
    /** @brief @deprecated Always throws std::logic_error. Use save_graphviz() instead.
     *  @note C++ only. Not available in the Python binding. */
    void XPrint() const;
    /**
     * @brief Swap variables v1 and v2 in the BDD.
     * @param v1 First variable number.
     * @param v2 Second variable number.
     * @return The BDD with v1 and v2 swapped.
     */
    BDD swap(bddvar v1, bddvar v2) const;
    /** @deprecated Use swap() instead. */
    BDD Swap(bddvar v1, bddvar v2) const { return swap(v1, v2); }
    /**
     * @brief Smooth (existential quantification) of variable v.
     * @param v Variable number to quantify out.
     * @return The BDD with variable v existentially quantified.
     */
    BDD smooth(bddvar v) const;
    /** @deprecated Use smooth() instead. */
    BDD Smooth(bddvar v) const { return smooth(v); }
    /**
     * @brief Spread variable values to neighboring k levels.
     * @param k Number of levels to spread (must be >= 0).
     * @return The BDD with values spread by k levels.
     */
    BDD spread(int k) const;
    /** @deprecated Use spread() instead. */
    BDD Spread(int k) const { return spread(k); }
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
    static BDD ite(const BDD& f, const BDD& g, const BDD& h);
    /** @deprecated Use ite() instead. */
    static BDD Ite(const BDD& f, const BDD& g, const BDD& h) { return ite(f, g, h); }

    using DDBase::raw_child0;
    using DDBase::raw_child1;
    using DDBase::raw_child;
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

    /** @brief Convert to a QDD (quasi-reduced BDD) by inserting lo==hi identity nodes at skipped levels. */
    QDD to_qdd() const;
    /** @brief Convert BDD (characteristic function) to ZDD (family) over n variables. If n=0, uses var_used(). */
    ZDD to_zdd(int n = 0) const;

    /** @brief Return the BDD representing variable v (positive literal). */
    static BDD prime(bddvar v);
    /** @brief Return the BDD representing ¬v (negative literal). */
    static BDD prime_not(bddvar v);

    /** @brief Return the conjunction of literals (DIMACS sign convention: positive = var, negative = ¬var). */
    static BDD cube(const std::vector<int>& lits);
    /** @brief Return the disjunction of literals (DIMACS sign convention: positive = var, negative = ¬var). */
    static BDD clause(const std::vector<int>& lits);

    /** @brief Read 2-operand cache and return as BDD. Returns BDD::Null on miss.
     *
     * @param op Operation code in [BDD_OP_USER_MIN, 255]. Lower codes are reserved
     *           for built-in operations and would let cached entries collide with
     *           internal algorithms; passing one throws std::invalid_argument.
     */
    static BDD cache_get(uint8_t op, const BDD& f, const BDD& g);
    /** @brief Write 2-operand cache entry.
     *
     * @param op Operation code in [BDD_OP_USER_MIN, 255]. See cache_get for the
     *           rationale; passing a reserved code throws std::invalid_argument.
     */
    static void cache_put(uint8_t op, const BDD& f, const BDD& g, const BDD& result);

    // --- Node creation ---

    /** @brief Create a BDD node with level validation.
     *
     * Validates that var is in range and that children's levels are
     * strictly less than var's level. Applies BDD reduction rules
     * and complement edge normalization.
     *
     * @param var Variable number.
     * @param lo  The low (0-edge) child node ID.
     * @param hi  The high (1-edge) child node ID.
     * @return The node ID for the (var, lo, hi) triple.
     * @throws std::invalid_argument if var is out of range or child levels are invalid.
     */
    static bddp getnode(bddvar var, bddp lo, bddp hi);

    /** @brief Create a BDD node with level validation (class type version).
     *
     * Same as the bddp version but takes and returns BDD objects.
     *
     * @param var Variable number.
     * @param lo  The low (0-edge) child.
     * @param hi  The high (1-edge) child.
     * @return The created BDD node.
     * @throws std::invalid_argument if var is out of range or child levels are invalid.
     */
    static BDD getnode(bddvar var, const BDD& lo, const BDD& hi);

    // Advanced: no error checking — no variable range or level validation.
    // Use with extreme caution. Incorrect usage may create invalid BDDs.
    // C++ only. Not available in the Python binding.
    static bddp getnode_raw(bddvar var, bddp lo, bddp hi);

    static const BDD False;  /**< @brief Constant false BDD. */
    static const BDD True;   /**< @brief Constant true BDD. */
    static const BDD Null;   /**< @brief Null (error) BDD. */
};


} // namespace kyotodd

#endif
