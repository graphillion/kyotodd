#ifndef KYOTODD_ROTPIDD_H
#define KYOTODD_ROTPIDD_H

#include "bdd.h"
#include "svg_export.h"
#include <vector>
#include <unordered_map>

namespace kyotodd {

struct SvgParams;

/** @brief Maximum number of elements in RotPiDD permutations. */
static const int RotPiDD_MaxVar = 254;

/** @brief Cache operation code for RotPiDD left rotation. */
static const uint8_t BDD_OP_ROTPIDD_LEFTROT   = 54;
/** @brief Cache operation code for RotPiDD Cofact. */
static const uint8_t BDD_OP_ROTPIDD_COFACT    = 55;
/** @brief Cache operation code for RotPiDD multiplication (composition). */
static const uint8_t BDD_OP_ROTPIDD_MULT      = 56;
/** @brief Cache operation code for RotPiDD Odd (odd permutation extraction). */
static const uint8_t BDD_OP_ROTPIDD_ODD       = 57;
/** @brief Cache operation code for RotPiDD Swap. */
static const uint8_t BDD_OP_ROTPIDD_SWAP      = 58;
/** @brief Cache operation code for RotPiDD Reverse. */
static const uint8_t BDD_OP_ROTPIDD_REVERSE   = 59;
/** @brief Cache operation code for RotPiDD Order. */
static const uint8_t BDD_OP_ROTPIDD_ORDER     = 60;
/** @brief Cache operation code for RotPiDD Inverse. */
static const uint8_t BDD_OP_ROTPIDD_INVERSE   = 61;
/** @brief Cache operation code for RotPiDD Insert. */
static const uint8_t BDD_OP_ROTPIDD_INSERT    = 62;
/** @brief Cache operation code for RotPiDD RemoveMax. */
static const uint8_t BDD_OP_ROTPIDD_REMOVEMAX = 63;
/** @brief Cache operation code for RotPiDD normalize. */
static const uint8_t BDD_OP_ROTPIDD_NORMALIZE = 64;
/** @brief Cache operation code for RotPiDD contradiction maximization. */
static const uint8_t BDD_OP_ROTPIDD_CONTMAX   = 65;

/** @brief The current highest element value in RotPiDD (permutation size - 1). */
extern int RotPiDD_TopVar;
/** @brief The number of BDD variables allocated for RotPiDD. */
extern int RotPiDD_VarTableSize;
/** @brief Maps element value x to its base BDD level. */
extern int RotPiDD_LevOfX[];
/** @brief Maps a BDD level to its x value. Inverse of RotPiDD_LevOfX. */
extern int* RotPiDD_XOfLev;

/**
 * @brief Create a new RotPiDD variable (extend permutation size by 1).
 *
 * Allocates the BDD variables needed for rotations and transpositions
 * involving the new element.
 *
 * @warning RotPiDD internal level tables (RotPiDD_XOfLev / RotPiDD_LevOfX)
 *          are only updated by this function. Calling bddnewvaroflev() after
 *          RotPiDD_NewVar() will desynchronize the tables and cause existing
 *          RotPiDD objects to produce incorrect results.
 *
 * @return The new permutation size.
 */
int RotPiDD_NewVar();
/**
 * @brief Get the current RotPiDD permutation size.
 * @return The number of elements (permutation size).
 */
int RotPiDD_VarUsed();

/**
 * @brief Get the x value for a given BDD level.
 * @param lev BDD level.
 */
#define RotPiDD_X_Lev(lev) (RotPiDD_XOfLev[(lev)])
/**
 * @brief Get the y value for a given BDD level.
 * @param lev BDD level.
 */
#define RotPiDD_Y_Lev(lev) (RotPiDD_LevOfX[RotPiDD_XOfLev[(lev)]] - (lev) + 1)
/**
 * @brief Convert (x, y) pair to BDD level.
 * @param x First element.
 * @param y Second element (y <= x).
 */
#define RotPiDD_Lev_XY(x, y) (RotPiDD_LevOfX[(x)] - (y) + 1)

/**
 * @brief Apply transposition (u, v) to value y.
 *
 * Returns v if y == u, u if y == v, y otherwise.
 */
#define RotPiDD_Y_YUV(y, u, v) ((y) == (u) ? (v) : (y) == (v) ? (u) : (y))
/**
 * @brief Compute the transformed u value under transposition (x, y).
 *
 * Returns y if x == u, u otherwise.
 */
#define RotPiDD_U_XYU(x, y, u) ((x) == (u) ? (y) : (u))

/**
 * @brief A Rotational Permutation Decision Diagram.
 *
 * RotPiDD extends PiDD to handle cyclic rotations in addition to
 * adjacent transpositions. Each ZDD variable corresponds to a left
 * rotation LeftRot(x, y), which cyclically shifts positions y, y+1, ..., x.
 *
 * Does NOT inherit from DDBase; uses composition (wrapping a ZDD internally).
 */
class RotPiDD {
    ZDD zdd_;

    friend RotPiDD operator&(const RotPiDD& p, const RotPiDD& q);
    friend RotPiDD operator+(const RotPiDD& p, const RotPiDD& q);
    friend RotPiDD operator-(const RotPiDD& p, const RotPiDD& q);
    friend RotPiDD operator*(const RotPiDD& p, const RotPiDD& q);
    friend bool operator==(const RotPiDD& p, const RotPiDD& q);

public:
    /** @brief Default constructor. Constructs an empty RotPiDD (no permutations). */
    RotPiDD() : zdd_(0) {}
    /**
     * @brief Construct from an integer value.
     * @param a 0 for empty set, 1 for {identity}, negative for null.
     */
    explicit RotPiDD(int a) : zdd_(a) {}
    /** @brief Copy constructor. */
    RotPiDD(const RotPiDD& f) : zdd_(f.zdd_) {}
    /** @brief Move constructor. */
    RotPiDD(RotPiDD&& f) : zdd_(std::move(f.zdd_)) {}
    /**
     * @brief Construct from an existing ZDD.
     * @param zbdd A ZDD to interpret as a permutation set.
     */
    explicit RotPiDD(const ZDD& zbdd) : zdd_(zbdd) {}
    /** @brief Destructor. */
    ~RotPiDD() {}

    /** @brief Copy assignment operator. */
    RotPiDD& operator=(const RotPiDD& f) { zdd_ = f.zdd_; return *this; }
    /** @brief Move assignment operator. */
    RotPiDD& operator=(RotPiDD&& f) { zdd_ = std::move(f.zdd_); return *this; }

    /** @brief In-place intersection. */
    RotPiDD& operator&=(const RotPiDD& f) { *this = *this & f; return *this; }
    /** @brief In-place union. */
    RotPiDD& operator+=(const RotPiDD& f) { *this = *this + f; return *this; }
    /** @brief In-place difference. */
    RotPiDD& operator-=(const RotPiDD& f) { *this = *this - f; return *this; }
    /** @brief In-place composition (multiplication). */
    RotPiDD& operator*=(const RotPiDD& f) { *this = *this * f; return *this; }

    /**
     * @brief Get the x coordinate of the top variable.
     * @return The x value of the highest-level rotation.
     */
    int top_x() const {
        if (zdd_.is_terminal()) return 0;
        return RotPiDD_X_Lev(top_lev());
    }
    /** @deprecated Use top_x() instead. */
    int TopX() const { return top_x(); }
    /**
     * @brief Get the y coordinate of the top variable.
     * @return The y value of the highest-level rotation.
     */
    int top_y() const {
        if (zdd_.is_terminal()) return 0;
        return RotPiDD_Y_Lev(top_lev());
    }
    /** @deprecated Use top_y() instead. */
    int TopY() const { return top_y(); }
    /**
     * @brief Get the BDD level of the top variable.
     * @return The BDD level of the root node.
     */
    int top_lev() const { return static_cast<int>(bddlevofvar(zdd_.Top())); }
    /** @deprecated Use top_lev() instead. */
    int TopLev() const { return top_lev(); }
    /**
     * @brief Get the number of nodes in the internal ZDD.
     * @return The node count.
     */
    uint64_t size() const { return zdd_.Size(); }
    /** @deprecated Use size() instead. */
    uint64_t Size() const { return size(); }
    /**
     * @brief Get the number of permutations in the set.
     * @deprecated Use count() or exact_count() instead. This wraps the
     *             obsolete recursive bddcard(): the result saturates at
     *             2^39 - 1 and sets deeper than BDD_RecurLimit throw
     *             std::overflow_error because no iterative version exists.
     * @return The cardinality (number of permutations).
     */
    uint64_t card() const { return zdd_.Card(); }
    /** @deprecated Use card() instead. */
    uint64_t Card() const { return card(); }
    /**
     * @brief Count the permutations in the set (double approximation).
     * @return The cardinality as a double.
     */
    double count() const { return zdd_.count(); }
    /**
     * @brief Count the permutations in the set (arbitrary precision).
     * @return The cardinality as a BigInt.
     */
    bigint::BigInt exact_count() const { return zdd_.exact_count(); }
    /**
     * @brief Get the internal ZDD representation.
     * @return A copy of the internal ZDD.
     */
    ZDD get_zdd() const { return zdd_; }
    /** @deprecated Use get_zdd() instead. */
    ZDD GetZDD() const { return get_zdd(); }

    /**
     * @brief Build a variable name map labeling each RotPiDD variable
     *        with its rotation "LeftRot(x,y)".
     *
     * Useful for passing to SvgParams::var_name_map so that SVG node
     * labels show rotation names instead of raw variable numbers.
     *
     * @return A map from BDD variable number to rotation label string.
     */
    static std::map<bddvar, std::string> svg_var_name_map();

    /**
     * @brief Apply left rotation LeftRot(u, v) to all permutations.
     *
     * Left-rotates positions v, v+1, ..., u cyclically:
     * the value at position u moves to v, and others shift up by one.
     *
     * @param u Upper position.
     * @param v Lower position (v <= u).
     * @return A new RotPiDD with the rotation applied.
     */
    RotPiDD left_rot(int u, int v) const;
    /** @deprecated Use left_rot() instead. */
    RotPiDD LeftRot(int u, int v) const { return left_rot(u, v); }
    /**
     * @brief Apply left rotation LeftRot(u, v) with execution mode selection.
     * @param u Upper position.
     * @param v Lower position (v <= u).
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     *             Auto selects recursive if the internal ZDD's level
     *             <= BDD_RecurLimit, else iterative.
     * @return A new RotPiDD with the rotation applied.
     */
    RotPiDD left_rot(int u, int v, BddExecMode mode) const;
    /**
     * @brief Swap positions a and b in all permutations.
     * @param a First position.
     * @param b Second position.
     * @return A new RotPiDD with the swap applied.
     */
    RotPiDD swap(int a, int b) const;
    /** @deprecated Use swap() instead. */
    RotPiDD Swap(int a, int b) const { return swap(a, b); }
    /**
     * @brief Swap positions a and b with execution mode selection.
     *
     * The mode is forwarded to the underlying left_rot / multiply calls.
     *
     * @param a First position.
     * @param b Second position.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A new RotPiDD with the swap applied.
     */
    RotPiDD swap(int a, int b, BddExecMode mode) const;
    /**
     * @brief Reverse positions l..r in all permutations.
     * @param l Left position.
     * @param r Right position.
     * @return A new RotPiDD with the reversal applied.
     */
    RotPiDD reverse(int l, int r) const;
    /** @deprecated Use reverse() instead. */
    RotPiDD Reverse(int l, int r) const { return reverse(l, r); }
    /**
     * @brief Reverse positions l..r with execution mode selection.
     *
     * The mode is forwarded to the underlying left_rot / multiply calls.
     *
     * @param l Left position.
     * @param r Right position.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A new RotPiDD with the reversal applied.
     */
    RotPiDD reverse(int l, int r, BddExecMode mode) const;
    /**
     * @brief Extract permutations where position u maps to value v.
     *
     * Returns sub-permutations (with element v removed) of all
     * permutations pi such that pi(u) = v.
     *
     * @param u Position to check.
     * @param v Required value at position u.
     * @return A RotPiDD of sub-permutations satisfying the condition.
     */
    RotPiDD cofact(int u, int v) const;
    /** @deprecated Use cofact() instead. */
    RotPiDD Cofact(int u, int v) const { return cofact(u, v); }
    /**
     * @brief Extract permutations with pi(u) = v, with execution mode selection.
     * @param u Position to check.
     * @param v Required value at position u.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A RotPiDD of sub-permutations satisfying the condition.
     */
    RotPiDD cofact(int u, int v, BddExecMode mode) const;
    /**
     * @brief Extract odd permutations from the set.
     * @return A RotPiDD containing only the odd permutations.
     */
    RotPiDD odd() const;
    /** @deprecated Use odd() instead. */
    RotPiDD Odd() const { return odd(); }
    /**
     * @brief Extract odd permutations with execution mode selection.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A RotPiDD containing only the odd permutations.
     */
    RotPiDD odd(BddExecMode mode) const;
    /**
     * @brief Extract even permutations from the set.
     * @return A RotPiDD containing only the even permutations.
     */
    RotPiDD even() const;
    /** @deprecated Use even() instead. */
    RotPiDD Even() const { return even(); }
    /**
     * @brief Extract even permutations with execution mode selection.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A RotPiDD containing only the even permutations.
     */
    RotPiDD even(BddExecMode mode) const;
    /**
     * @brief Composition (multiplication) of permutation sets.
     *
     * Same as operator*: computes { pi . sigma | sigma in *this, pi in q }.
     *
     * @param q The right operand.
     * @return The composed permutation set.
     */
    RotPiDD multiply(const RotPiDD& q) const;
    /**
     * @brief Composition (multiplication) with execution mode selection.
     * @param q The right operand.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return The composed permutation set.
     */
    RotPiDD multiply(const RotPiDD& q, BddExecMode mode) const;
    /**
     * @brief Apply symmetric constraint (PermitSym) to the internal ZDD.
     * @param n Constraint parameter.
     * @return A RotPiDD with the constraint applied.
     */
    RotPiDD rot_bound(int n) const;
    /** @deprecated Use rot_bound() instead. */
    RotPiDD RotBound(int n) const { return rot_bound(n); }
    /**
     * @brief Extract permutations where pi(a) < pi(b).
     * @param a First position.
     * @param b Second position.
     * @return A RotPiDD containing only permutations satisfying the order.
     */
    RotPiDD order(int a, int b) const;
    /** @deprecated Use order() instead. */
    RotPiDD Order(int a, int b) const { return order(a, b); }
    /**
     * @brief Extract permutations where pi(a) < pi(b), with execution
     *        mode selection.
     * @param a First position.
     * @param b Second position.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A RotPiDD containing only permutations satisfying the order.
     */
    RotPiDD order(int a, int b, BddExecMode mode) const;
    /**
     * @brief Compute the inverse of each permutation in the set.
     * @return A RotPiDD of inverse permutations.
     */
    RotPiDD inverse() const;
    /** @deprecated Use inverse() instead. */
    RotPiDD Inverse() const { return inverse(); }
    /**
     * @brief Compute the inverse permutations with execution mode selection.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A RotPiDD of inverse permutations.
     */
    RotPiDD inverse(BddExecMode mode) const;
    /**
     * @brief Insert value v at position p in each permutation.
     *
     * Shifts existing elements to make room for the new value.
     *
     * @param p Insertion position.
     * @param v Value to insert.
     * @return A RotPiDD with the element inserted.
     */
    RotPiDD insert(int p, int v) const;
    /** @deprecated Use insert() instead. */
    RotPiDD Insert(int p, int v) const { return insert(p, v); }
    /**
     * @brief Insert value v at position p with execution mode selection.
     * @param p Insertion position.
     * @param v Value to insert.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A RotPiDD with the element inserted.
     */
    RotPiDD insert(int p, int v, BddExecMode mode) const;
    /**
     * @brief Remove variables with size >= k from each permutation.
     * @param k Threshold value.
     * @return A RotPiDD with large variables removed.
     */
    RotPiDD remove_max(int k) const;
    /** @deprecated Use remove_max() instead. */
    RotPiDD RemoveMax(int k) const { return remove_max(k); }
    /**
     * @brief Remove variables with size >= k, with execution mode selection.
     * @param k Threshold value.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A RotPiDD with large variables removed.
     */
    RotPiDD remove_max(int k, BddExecMode mode) const;
    /**
     * @brief Normalize by removing variables with x > k.
     *
     * Projects out variables beyond the threshold, reducing the
     * permutation size.
     *
     * @param k Upper bound for retained variables.
     * @return A normalized RotPiDD.
     */
    RotPiDD normalizeRotPiDD(int k) const;
    /**
     * @brief Normalize by removing variables with x > k, with execution
     *        mode selection.
     * @param k Upper bound for retained variables.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A normalized RotPiDD.
     */
    RotPiDD normalizeRotPiDD(int k, BddExecMode mode) const;

    /**
     * @brief Normalize a permutation vector to use values [1..n].
     *
     * Replaces the values in-place with their rank ordering.
     *
     * @note C++ only. Not available in the Python binding.
     *       In Python, rotpidd_from_perm() calls this automatically.
     *
     * @param v Permutation vector to normalize (modified in place).
     */
    static void normalizePerm(std::vector<int>& v);
    /**
     * @brief Convert a permutation vector to a RotPiDD.
     *
     * The vector is automatically normalized to [1..n] before encoding.
     *
     * @param v A vector of integers representing a permutation.
     * @return A RotPiDD containing the single permutation.
     */
    static RotPiDD from_vector(std::vector<int> v);
    /** @deprecated Use from_vector() instead. */
    static RotPiDD VECtoRotPiDD(std::vector<int> v) { return from_vector(v); }
    /**
     * @brief Convert a permutation vector to a RotPiDD with execution
     *        mode selection (forwarded to the left_rot calls).
     * @param v A vector of integers representing a permutation.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A RotPiDD containing the single permutation.
     */
    static RotPiDD from_vector(std::vector<int> v, BddExecMode mode);
    /**
     * @brief Convert to a list of permutation vectors.
     * @return A vector of vectors, each representing a permutation as [1..n].
     */
    std::vector< std::vector<int> > to_vector_of_perms() const;
    /** @deprecated Use to_vector_of_perms() instead. */
    std::vector< std::vector<int> > RotPiDDToVectorOfPerms() const { return to_vector_of_perms(); }
    /**
     * @brief Convert to a list of permutation vectors with execution
     *        mode selection.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A vector of vectors, each representing a permutation as [1..n].
     */
    std::vector< std::vector<int> > to_vector_of_perms(BddExecMode mode) const;

    /**
     * @brief Extract a single permutation from the set.
     *
     * Returns an arbitrary permutation (the lexicographically first one
     * by ZDD traversal).
     *
     * @return A RotPiDD containing exactly one permutation.
     */
    RotPiDD extract_one();
    /** @deprecated Use extract_one() instead. */
    RotPiDD Extract_One() { return extract_one(); }
    /**
     * @brief Extract a single permutation with execution mode selection.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     * @return A RotPiDD containing exactly one permutation.
     */
    RotPiDD extract_one(BddExecMode mode) const;
    /** @brief Print all permutations to stdout. */
    void print() const;
    /** @deprecated Use print() instead. */
    void Print() const { print(); }
    /** @brief Enumerate all permutations in compact form. */
    void enumerate() const;
    /** @deprecated Use enumerate() instead. */
    void Enum() const { enumerate(); }
    /**
     * @brief Enumerate all permutations in compact form with execution
     *        mode selection.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     */
    void enumerate(BddExecMode mode) const;
    /** @brief Enumerate all permutations in expanded form. */
    void enumerate2() const;
    /** @deprecated Use enumerate2() instead. */
    void Enum2() const { enumerate2(); }
    /**
     * @brief Enumerate all permutations in expanded form with execution
     *        mode selection.
     * @param mode Execution mode (Recursive, Iterative, or Auto).
     */
    void enumerate2(BddExecMode mode) const;

    /**
     * @brief Export the internal ZDD as SVG to a file.
     * @param filename Output file path.
     * @param params SVG rendering parameters.
     */
    void save_svg(const char* filename, const SvgParams& params) const;
    /** @overload */
    void save_svg(const char* filename) const;
    /**
     * @brief Export the internal ZDD as SVG to a stream.
     * @param strm Output stream.
     * @param params SVG rendering parameters.
     */
    void save_svg(std::ostream& strm, const SvgParams& params) const;
    /** @overload */
    void save_svg(std::ostream& strm) const;
    /**
     * @brief Export the internal ZDD as an SVG string.
     * @param params SVG rendering parameters.
     * @return An SVG format string.
     */
    std::string save_svg(const SvgParams& params) const;
    /** @overload */
    std::string save_svg() const;

    /**
     * @brief Custom hash function for (bddp, bitmask) pairs.
     *
     * Used as the hash function for the memoization table in
     * contradictionMaximization().
     */
    struct hash_func {
        size_t operator()(const std::pair<bddp, unsigned long long int>& a) const {
            size_t h1 = std::hash<bddp>()(a.first);
            size_t h2 = std::hash<unsigned long long int>()(a.second);
            return h1 ^ (h2 * 0x9e3779b9U + (h1 << 6) + (h1 >> 2));
        }
    };

    /**
     * @brief Compute the maximum weighted contradiction value.
     *
     * Traverses the RotPiDD to find the permutation assignment
     * that maximizes the sum of weights in the conflict matrix.
     *
     * @param used_set  Bitmask of already-assigned positions.
     * @param unused_list List of unassigned positions.
     * @param n         Total number of positions (must be <= 63).
     * @param hash      Memoization table (modified in place).
     * @param w         Weight matrix: w[i][j] is the weight for assigning
     *                  position i to value j.
     * @return The maximum contradiction value.
     * @throws std::invalid_argument if n > 63.
     */
    long long int contradictionMaximization(
        unsigned long long int used_set,
        std::vector<int>& unused_list,
        int n,
        std::unordered_map< std::pair<bddp, unsigned long long int>, long long int, hash_func >& hash,
        const std::vector< std::vector<int> >& w) const;

    /** @brief Memoization table type used by contradictionMaximization(). */
    typedef std::unordered_map< std::pair<bddp, unsigned long long int>,
                                long long int, hash_func > contmax_hash;

    /**
     * @brief Compute the maximum weighted contradiction value with
     *        execution mode selection.
     *
     * Same as the overload above; @p mode selects the recursive or the
     * explicit-stack implementation (Auto decides by the internal ZDD's
     * level against BDD_RecurLimit).
     */
    long long int contradictionMaximization(
        unsigned long long int used_set,
        std::vector<int>& unused_list,
        int n,
        contmax_hash& hash,
        const std::vector< std::vector<int> >& w,
        BddExecMode mode) const;

private:
    // Recursive (_rec) and explicit-stack (_iter) cores of the recursive
    // operations. Both share the same cache op codes, so their results are
    // interchangeable; the public wrappers above dispatch between them via
    // select_iter_1op / select_iter_2op. _rec bodies live in
    // src/rotpidd.cpp, _iter bodies in src/rotpidd_iter.cpp. The frames
    // hold RotPiDD objects (GC roots), so no bdd_gc_guard is needed around
    // the _iter calls.
    static RotPiDD left_rot_rec(const RotPiDD& p, int u, int v);
    static RotPiDD left_rot_iter(const RotPiDD& p, int u, int v);
    static RotPiDD cofact_rec(const RotPiDD& p, int u, int v);
    static RotPiDD cofact_iter(const RotPiDD& p, int u, int v);
    static RotPiDD odd_rec(const RotPiDD& p);
    static RotPiDD odd_iter(const RotPiDD& p);
    static RotPiDD multiply_rec(const RotPiDD& p, const RotPiDD& q);
    static RotPiDD multiply_iter(const RotPiDD& p, const RotPiDD& q);
    static RotPiDD order_rec(const RotPiDD& p, int a, int b);
    static RotPiDD order_iter(const RotPiDD& p, int a, int b);
    static RotPiDD inverse_rec(const RotPiDD& p);
    static RotPiDD inverse_iter(const RotPiDD& p);
    static RotPiDD insert_rec(const RotPiDD& f, int p, int v);
    static RotPiDD insert_iter(const RotPiDD& f, int p, int v);
    static RotPiDD remove_max_rec(const RotPiDD& p, int k);
    static RotPiDD remove_max_iter(const RotPiDD& p, int k);
    static RotPiDD normalize_rec(const RotPiDD& p, int k);
    static RotPiDD normalize_iter(const RotPiDD& p, int k);
    static RotPiDD extract_one_rec(const RotPiDD& p);
    static RotPiDD extract_one_iter(const RotPiDD& p);
    static void to_perms_rec(const RotPiDD& p, int dim, int* varmap,
                             std::vector< std::vector<int> >& result);
    static void to_perms_iter(const RotPiDD& p, int dim, int* varmap,
                              std::vector< std::vector<int> >& result);
    static void enumerate_rec(const RotPiDD& p, int dim, int* varmap, bool& flag);
    static void enumerate_iter(const RotPiDD& p, int dim, int* varmap, bool& flag);
    static void enumerate2_rec(const RotPiDD& p, std::vector<int>& levs, bool& flag);
    static void enumerate2_iter(const RotPiDD& p, std::vector<int>& levs, bool& flag);
    static long long int contmax_rec(
        const RotPiDD& p, unsigned long long int used_set,
        std::vector<int>& unused_list, int n, contmax_hash& hash,
        const std::vector< std::vector<int> >& w);
    static long long int contmax_iter(
        const RotPiDD& p, unsigned long long int used_set,
        std::vector<int>& unused_list, int n, contmax_hash& hash,
        const std::vector< std::vector<int> >& w);
    // Leaf printers shared by the _rec / _iter enumerators.
    static void enumerate_print(int dim, const int* varmap, bool& flag);
    static void enumerate2_print(const std::vector<int>& levs, bool& flag);
};

/** @brief Intersection of two permutation sets. */
inline RotPiDD operator&(const RotPiDD& p, const RotPiDD& q) { return RotPiDD(p.zdd_ & q.zdd_); }
/** @brief Union of two permutation sets. */
inline RotPiDD operator+(const RotPiDD& p, const RotPiDD& q) { return RotPiDD(p.zdd_ + q.zdd_); }
/** @brief Difference of two permutation sets. */
inline RotPiDD operator-(const RotPiDD& p, const RotPiDD& q) { return RotPiDD(p.zdd_ - q.zdd_); }
/** @brief Equality comparison. */
inline bool operator==(const RotPiDD& p, const RotPiDD& q) { return p.zdd_ == q.zdd_; }
/** @brief Inequality comparison. */
inline bool operator!=(const RotPiDD& p, const RotPiDD& q) { return !(p == q); }

// --- RotPiDD save_svg inline implementations ---

inline void RotPiDD::save_svg(const char* filename, const SvgParams& params) const {
    zdd_save_svg(filename, zdd_.id(), params);
}
inline void RotPiDD::save_svg(const char* filename) const {
    save_svg(filename, SvgParams());
}
inline void RotPiDD::save_svg(std::ostream& strm, const SvgParams& params) const {
    zdd_save_svg(strm, zdd_.id(), params);
}
inline void RotPiDD::save_svg(std::ostream& strm) const {
    save_svg(strm, SvgParams());
}
inline std::string RotPiDD::save_svg(const SvgParams& params) const {
    return zdd_save_svg(zdd_.id(), params);
}
inline std::string RotPiDD::save_svg() const {
    return save_svg(SvgParams());
}

} // namespace kyotodd

#endif
