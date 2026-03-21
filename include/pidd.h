#ifndef KYOTODD_PIDD_H
#define KYOTODD_PIDD_H

#include "bdd.h"

/** @brief Maximum number of elements in PiDD permutations. */
static const int PiDD_MaxVar = 254;

/** @brief Cache operation code for PiDD Swap. */
static const uint8_t BDD_OP_PIDD_SWAP   = 49;
/** @brief Cache operation code for PiDD Cofact. */
static const uint8_t BDD_OP_PIDD_COFACT = 50;
/** @brief Cache operation code for PiDD multiplication (composition). */
static const uint8_t BDD_OP_PIDD_MULT   = 51;
/** @brief Cache operation code for PiDD division. */
static const uint8_t BDD_OP_PIDD_DIV    = 52;
/** @brief Cache operation code for PiDD Odd (odd permutation extraction). */
static const uint8_t BDD_OP_PIDD_ODD    = 53;

/** @brief The current highest element value in PiDD (permutation size - 1). */
extern int PiDD_TopVar;
/** @brief The number of BDD variables allocated for PiDD. */
extern int PiDD_VarTableSize;
/** @brief Maps element value x to its base BDD level. PiDD_LevOfX[x] is the level of the first transposition involving x. */
extern int PiDD_LevOfX[];
/** @brief Maps a BDD level to its x value. Inverse of PiDD_LevOfX. */
extern int* PiDD_XOfLev;

/**
 * @brief Create a new PiDD variable (extend permutation size by 1).
 *
 * Allocates the BDD variables needed for transpositions involving
 * the new element.
 *
 * @warning PiDD internal level tables (PiDD_XOfLev / PiDD_LevOfX) are
 *          only updated by this function. Calling bddnewvaroflev() after
 *          PiDD_NewVar() will desynchronize the tables and cause existing
 *          PiDD objects to produce incorrect results.
 *
 * @return The new permutation size.
 */
int PiDD_NewVar();
/**
 * @brief Get the current PiDD permutation size.
 * @return The number of elements (permutation size).
 */
int PiDD_VarUsed();

/**
 * @brief Get the x value for a given BDD level.
 * @param lev BDD level.
 */
#define PiDD_X_Lev(lev) (PiDD_XOfLev[(lev)])
/**
 * @brief Get the y value for a given BDD level.
 * @param lev BDD level.
 */
#define PiDD_Y_Lev(lev) (PiDD_LevOfX[PiDD_XOfLev[(lev)]] - (lev) + 1)
/**
 * @brief Convert (x, y) pair to BDD level.
 * @param x First element.
 * @param y Second element (y <= x).
 */
#define PiDD_Lev_XY(x, y) (PiDD_LevOfX[(x)] - (y) + 1)

/**
 * @brief Apply transposition (u, v) to value y.
 *
 * Returns v if y == u, u if y == v, y otherwise.
 */
#define PiDD_Y_YUV(y, u, v) ((y) == (u) ? (v) : (y) == (v) ? (u) : (y))
/**
 * @brief Compute the transformed u value under transposition (x, y).
 *
 * Returns y if x == u, u otherwise.
 */
#define PiDD_U_XYU(x, y, u) ((x) == (u) ? (y) : (u))

/**
 * @brief A Permutation Decision Diagram based on adjacent transpositions.
 *
 * PiDD represents a set of permutations using a ZDD, where each ZDD variable
 * corresponds to an adjacent transposition Swap(x, y). The internal
 * representation uses composition of transpositions to encode permutations.
 *
 * Does NOT inherit from DDBase; uses composition (wrapping a ZDD internally).
 */
class PiDD {
    ZDD zdd_;

    friend PiDD operator&(const PiDD& p, const PiDD& q);
    friend PiDD operator+(const PiDD& p, const PiDD& q);
    friend PiDD operator-(const PiDD& p, const PiDD& q);
    friend PiDD operator*(const PiDD& p, const PiDD& q);
    friend PiDD operator/(const PiDD& f, const PiDD& p);
    friend bool operator==(const PiDD& p, const PiDD& q);

public:
    /** @brief Default constructor. Constructs an empty PiDD (no permutations). */
    PiDD() : zdd_(0) {}
    /**
     * @brief Construct from an integer value.
     * @param a 0 for empty set, 1 for {identity}, negative for null.
     */
    explicit PiDD(int a) : zdd_(a) {}
    /** @brief Copy constructor. */
    PiDD(const PiDD& f) : zdd_(f.zdd_) {}
    /**
     * @brief Construct from an existing ZDD.
     * @param zbdd A ZDD to interpret as a permutation set.
     */
    explicit PiDD(const ZDD& zbdd) : zdd_(zbdd) {}
    /** @brief Destructor. */
    ~PiDD() {}

    /** @brief Copy assignment operator. */
    PiDD& operator=(const PiDD& f) { zdd_ = f.zdd_; return *this; }

    /** @brief In-place intersection. */
    PiDD& operator&=(const PiDD& f) { *this = *this & f; return *this; }
    /** @brief In-place union. */
    PiDD& operator+=(const PiDD& f) { *this = *this + f; return *this; }
    /** @brief In-place difference. */
    PiDD& operator-=(const PiDD& f) { *this = *this - f; return *this; }
    /** @brief In-place composition (multiplication). */
    PiDD& operator*=(const PiDD& f) { *this = *this * f; return *this; }
    /** @brief In-place division. */
    PiDD& operator/=(const PiDD& f) { *this = *this / f; return *this; }
    /** @brief In-place remainder. */
    PiDD& operator%=(const PiDD& f);

    /**
     * @brief Get the x coordinate of the top variable.
     * @return The x value of the highest-level transposition.
     */
    int TopX() const {
        if (zdd_.is_terminal()) return 0;
        return PiDD_X_Lev(TopLev());
    }
    /**
     * @brief Get the y coordinate of the top variable.
     * @return The y value of the highest-level transposition.
     */
    int TopY() const {
        if (zdd_.is_terminal()) return 0;
        return PiDD_Y_Lev(TopLev());
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
     * @brief Apply transposition Swap(u, v) to all permutations.
     *
     * Composes each permutation in the set with the transposition (u, v).
     *
     * @param u First position.
     * @param v Second position.
     * @return A new PiDD with the transposition applied.
     */
    PiDD Swap(int u, int v) const;
    /**
     * @brief Extract permutations where position u maps to value v.
     *
     * Returns the sub-permutations (with element v removed) of all
     * permutations pi such that pi(u) = v.
     *
     * @param u Position to check.
     * @param v Required value at position u.
     * @return A PiDD of sub-permutations satisfying the condition.
     */
    PiDD Cofact(int u, int v) const;
    /**
     * @brief Extract odd permutations from the set.
     * @return A PiDD containing only the odd permutations.
     */
    PiDD Odd() const;
    /**
     * @brief Extract even permutations from the set.
     * @return A PiDD containing only the even permutations.
     */
    PiDD Even() const;
    /**
     * @brief Apply symmetric constraint (PermitSym) to the internal ZDD.
     *
     * Limits the number of transpositions.
     *
     * @param n Constraint parameter.
     * @return A PiDD with the constraint applied.
     */
    PiDD SwapBound(int n) const;

    /** @brief Print all permutations to stdout. */
    void Print() const;
    /** @brief Enumerate all permutations in compact form. */
    void Enum() const;
    /** @brief Enumerate all permutations in expanded form. */
    void Enum2() const;
};

/** @brief Intersection of two permutation sets. */
inline PiDD operator&(const PiDD& p, const PiDD& q) { return PiDD(p.zdd_ & q.zdd_); }
/** @brief Union of two permutation sets. */
inline PiDD operator+(const PiDD& p, const PiDD& q) { return PiDD(p.zdd_ + q.zdd_); }
/** @brief Difference of two permutation sets. */
inline PiDD operator-(const PiDD& p, const PiDD& q) { return PiDD(p.zdd_ - q.zdd_); }
/** @brief Equality comparison. */
inline bool operator==(const PiDD& p, const PiDD& q) { return p.zdd_ == q.zdd_; }
/** @brief Inequality comparison. */
inline bool operator!=(const PiDD& p, const PiDD& q) { return !(p == q); }

/** @brief Remainder: f - (f / p) * p. */
inline PiDD operator%(const PiDD& f, const PiDD& p) { return f - (f / p) * p; }

inline PiDD& PiDD::operator%=(const PiDD& f) { *this = *this % f; return *this; }

#endif
