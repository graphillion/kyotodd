#ifndef KYOTODD_ROTPIDD_H
#define KYOTODD_ROTPIDD_H

#include "bdd.h"
#include <vector>
#include <unordered_map>

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
    int TopX() const {
        if (zdd_.is_terminal()) return 0;
        return RotPiDD_X_Lev(TopLev());
    }
    /**
     * @brief Get the y coordinate of the top variable.
     * @return The y value of the highest-level rotation.
     */
    int TopY() const {
        if (zdd_.is_terminal()) return 0;
        return RotPiDD_Y_Lev(TopLev());
    }
    /**
     * @brief Get the BDD level of the top variable.
     * @return The BDD level of the root node.
     */
    int TopLev() const { return static_cast<int>(bddlevofvar(zdd_.Top())); }
    /**
     * @brief Get the number of nodes in the internal ZDD.
     * @return The node count.
     */
    uint64_t Size() const { return zdd_.Size(); }
    /**
     * @brief Get the number of permutations in the set.
     * @return The cardinality (number of permutations).
     */
    uint64_t Card() const { return zdd_.Card(); }
    /**
     * @brief Get the internal ZDD representation.
     * @return A copy of the internal ZDD.
     */
    ZDD GetZDD() const { return zdd_; }

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
    RotPiDD LeftRot(int u, int v) const;
    /**
     * @brief Swap positions a and b in all permutations.
     * @param a First position.
     * @param b Second position.
     * @return A new RotPiDD with the swap applied.
     */
    RotPiDD Swap(int a, int b) const;
    /**
     * @brief Reverse positions l..r in all permutations.
     * @param l Left position.
     * @param r Right position.
     * @return A new RotPiDD with the reversal applied.
     */
    RotPiDD Reverse(int l, int r) const;
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
    RotPiDD Cofact(int u, int v) const;
    /**
     * @brief Extract odd permutations from the set.
     * @return A RotPiDD containing only the odd permutations.
     */
    RotPiDD Odd() const;
    /**
     * @brief Extract even permutations from the set.
     * @return A RotPiDD containing only the even permutations.
     */
    RotPiDD Even() const;
    /**
     * @brief Apply symmetric constraint (PermitSym) to the internal ZDD.
     * @param n Constraint parameter.
     * @return A RotPiDD with the constraint applied.
     */
    RotPiDD RotBound(int n) const;
    /**
     * @brief Extract permutations where pi(a) < pi(b).
     * @param a First position.
     * @param b Second position.
     * @return A RotPiDD containing only permutations satisfying the order.
     */
    RotPiDD Order(int a, int b) const;
    /**
     * @brief Compute the inverse of each permutation in the set.
     * @return A RotPiDD of inverse permutations.
     */
    RotPiDD Inverse() const;
    /**
     * @brief Insert value v at position p in each permutation.
     *
     * Shifts existing elements to make room for the new value.
     *
     * @param p Insertion position.
     * @param v Value to insert.
     * @return A RotPiDD with the element inserted.
     */
    RotPiDD Insert(int p, int v) const;
    /**
     * @brief Remove variables with size >= k from each permutation.
     * @param k Threshold value.
     * @return A RotPiDD with large variables removed.
     */
    RotPiDD RemoveMax(int k) const;
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
    static RotPiDD VECtoRotPiDD(std::vector<int> v);
    /**
     * @brief Convert to a list of permutation vectors.
     * @return A vector of vectors, each representing a permutation as [1..n].
     */
    std::vector< std::vector<int> > RotPiDDToVectorOfPerms() const;

    /**
     * @brief Extract a single permutation from the set.
     *
     * Returns an arbitrary permutation (the lexicographically first one
     * by ZDD traversal).
     *
     * @return A RotPiDD containing exactly one permutation.
     */
    RotPiDD Extract_One();
    /** @brief Print all permutations to stdout. */
    void Print() const;
    /** @brief Enumerate all permutations in compact form. */
    void Enum() const;
    /** @brief Enumerate all permutations in expanded form. */
    void Enum2() const;

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
#include "svg_export.h"

inline void RotPiDD::save_svg(const char* filename, const SvgParams& params) const {
    zdd_save_svg(filename, zdd_.get_id(), params);
}
inline void RotPiDD::save_svg(const char* filename) const {
    save_svg(filename, SvgParams());
}
inline void RotPiDD::save_svg(std::ostream& strm, const SvgParams& params) const {
    zdd_save_svg(strm, zdd_.get_id(), params);
}
inline void RotPiDD::save_svg(std::ostream& strm) const {
    save_svg(strm, SvgParams());
}
inline std::string RotPiDD::save_svg(const SvgParams& params) const {
    return zdd_save_svg(zdd_.get_id(), params);
}
inline std::string RotPiDD::save_svg() const {
    return save_svg(SvgParams());
}

#endif
