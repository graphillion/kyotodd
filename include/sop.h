#ifndef KYOTODD_SOP_H
#define KYOTODD_SOP_H

/**
 * @file sop.h
 * @brief SOP and SOPV classes — Sum-of-Products representation on ZBDDs.
 *
 * SOP represents a Boolean function in Sum-of-Products (SOP) form using
 * ZBDDs. Each logic variable uses a pair of ZBDD variables: an even VarID
 * for the positive literal and the preceding odd VarID for the negative
 * literal.
 *
 * SOPV is a vector of SOPs stored in a single ZBDDV, useful for
 * multi-output logic functions.
 *
 * This header is NOT included by the umbrella header @c bdd.h.
 * You must explicitly @c \#include @c "sop.h" to use these classes.
 *
 * Requires BDDV_Init() to have been called (from bddv.h) before using SOPV.
 */

#include "zbddv.h"

// --- Operation cache codes for SOP ---
static const uint8_t BDD_OP_SOP_MULT = 60;
static const uint8_t BDD_OP_SOP_DIV  = 61;
static const uint8_t BDD_OP_SOP_BDD  = 62;
static const uint8_t BDD_OP_ISOP1    = 63;
static const uint8_t BDD_OP_ISOP2    = 64;
static const uint8_t BDD_OP_SOP_IMPL = 65;

class SOPV;

/**
 * @brief Sum-of-Products representation using a ZBDD.
 *
 * Each cube (product term) is a ZBDD set. The family of all cubes is the SOP.
 * A logic variable with even VarID @c v uses ZBDD variable @c v for the
 * positive literal and ZBDD variable @c v-1 for the negative literal.
 */
class SOP {
    ZDD _zbdd;

    friend SOP operator&(const SOP& f, const SOP& g);
    friend SOP operator+(const SOP& f, const SOP& g);
    friend SOP operator-(const SOP& f, const SOP& g);
    friend SOP operator*(const SOP& f, const SOP& g);
    friend SOP operator/(const SOP& f, const SOP& g);
    friend SOP operator%(const SOP& f, const SOP& g);
    friend int operator==(const SOP& f, const SOP& g);
    friend int operator!=(const SOP& f, const SOP& g);

public:
    /** @brief Default constructor. Empty SOP (constant 0). */
    SOP();

    /**
     * @brief Construct from integer.
     * @param val 0 = constant false (no cubes), 1 = tautology (one empty cube).
     */
    SOP(int val);

    /** @brief Copy constructor. */
    SOP(const SOP& f);

    /**
     * @brief Construct from a ZBDD directly.
     *
     * The caller must ensure that the ZBDD follows SOP variable encoding.
     */
    SOP(const ZDD& zbdd);

    /** @brief Destructor. */
    ~SOP();

    /** @brief Copy assignment. */
    SOP& operator=(const SOP& f);

    // --- Factor operations ---

    /**
     * @brief Extract cubes containing positive literal x_v (with x_v removed).
     * @param v Even VarID of the logic variable.
     */
    SOP Factor1(int v) const;

    /**
     * @brief Extract cubes containing negative literal ~x_v (with ~x_v removed).
     * @param v Even VarID of the logic variable.
     */
    SOP Factor0(int v) const;

    /**
     * @brief Extract cubes not depending on variable v.
     * @param v Even VarID of the logic variable.
     */
    SOP FactorD(int v) const;

    // --- And operations ---

    /**
     * @brief AND positive literal x_v to all cubes.
     * @param v Even VarID of the logic variable.
     */
    SOP And1(int v) const;

    /**
     * @brief AND negative literal ~x_v to all cubes.
     * @param v Even VarID of the logic variable.
     */
    SOP And0(int v) const;

    // --- Shift operators ---

    /**
     * @brief Shift all variables up by @p n levels.
     * @param n Must be even.
     */
    SOP operator<<(int n) const;

    /**
     * @brief Shift all variables down by @p n levels.
     * @param n Must be even.
     */
    SOP operator>>(int n) const;

    /** @brief In-place left shift. */
    SOP& operator<<=(int n);

    /** @brief In-place right shift. */
    SOP& operator>>=(int n);

    // --- Compound assignment ---

    SOP& operator&=(const SOP& f);
    SOP& operator+=(const SOP& f);
    SOP& operator-=(const SOP& f);
    SOP& operator*=(const SOP& f);
    SOP& operator/=(const SOP& f);
    SOP& operator%=(const SOP& f);

    // --- Query ---

    /**
     * @brief Top logic variable VarID (even).
     *
     * Returns (_zbdd.Top() + 1) & ~1 to round up to even.
     */
    int Top() const;

    /** @brief Node count of internal ZBDD. */
    uint64_t Size() const;

    /** @brief Number of cubes (product terms). */
    uint64_t Cube() const;

    /** @brief Total number of literals across all cubes. */
    uint64_t Lit() const;

    /** @brief Return the internal ZBDD. */
    ZDD GetZBDD() const;

    /**
     * @brief Convert SOP to BDD.
     *
     * Builds a BDD via Shannon expansion on SOP factor decomposition.
     */
    BDD GetBDD() const;

    // --- Predicates ---

    /**
     * @brief Check if SOP contains multiple cubes.
     * @return 1 if multiple cubes, 0 otherwise.
     */
    int IsPolyCube() const;

    /**
     * @brief Check if SOP contains multiple literals.
     * @return 0 if SOP is a single literal, 1 otherwise.
     */
    int IsPolyLit() const;

    // --- Advanced operations ---

    /** @brief Set of all logic variables appearing in this SOP. */
    SOP Support() const;

    /**
     * @brief Find a non-trivial algebraic divisor.
     * @return A divisor SOP, or 1 if single-cube.
     */
    SOP Divisor() const;

    /**
     * @brief Select cubes that are implicants of BDD @p f.
     * @param f A BDD representing the target function.
     */
    SOP Implicants(BDD f) const;

    /**
     * @brief Swap two logic variables.
     * @param v1 Even VarID of first variable.
     * @param v2 Even VarID of second variable.
     */
    SOP Swap(int v1, int v2) const;

    /**
     * @brief Compute ISOP of the negation of this SOP's function.
     *
     * Equivalent to SOP_ISOP(~GetBDD()).
     */
    SOP InvISOP() const;

    // --- Output ---

    /** @brief Print debug info to stdout. */
    void Print() const;

    /**
     * @brief Print in PLA format.
     *
     * Internally constructs SOPV(*this) and delegates to SOPV::PrintPla().
     */
    void PrintPla() const;
};

// --- Binary operators ---

SOP operator&(const SOP& f, const SOP& g);
SOP operator+(const SOP& f, const SOP& g);
SOP operator-(const SOP& f, const SOP& g);
SOP operator*(const SOP& f, const SOP& g);
SOP operator/(const SOP& f, const SOP& g);
SOP operator%(const SOP& f, const SOP& g);
int operator==(const SOP& f, const SOP& g);
int operator!=(const SOP& f, const SOP& g);

// --- Free functions ---

/**
 * @brief Create a new SOP logic variable.
 *
 * Internally calls BDD_NewVar() twice to create a pair of ZBDD variables
 * (odd VarID for negative literal, even VarID for positive literal).
 * @return The even VarID.
 */
int SOP_NewVar();

/**
 * @brief Create a new SOP variable at the given level.
 * @param lev Must be even. Creates ZBDD variables at levels lev-1 and lev.
 * @return The even VarID.
 */
int SOP_NewVarOfLev(int lev);

/**
 * @brief Generate ISOP (Irredundant Sum-of-Products) from a BDD.
 * @param f The BDD to cover.
 * @return An irredundant SOP covering f.
 */
SOP SOP_ISOP(BDD f);

/**
 * @brief Generate ISOP with don't-care.
 * @param on The onset BDD (must be covered).
 * @param dc The don't-care BDD (may or may not be covered).
 * @return An irredundant SOP.
 */
SOP SOP_ISOP(BDD on, BDD dc);

// =====================================================================
//  SOPV class
// =====================================================================

/**
 * @brief Vector of SOPs stored in a ZBDDV.
 *
 * Used for multi-output logic functions. Each element is an SOP stored
 * at a ZBDDV index position.
 */
class SOPV {
    ZBDDV _v;

    friend SOPV operator&(const SOPV& f, const SOPV& g);
    friend SOPV operator+(const SOPV& f, const SOPV& g);
    friend SOPV operator-(const SOPV& f, const SOPV& g);
    friend int operator==(const SOPV& v1, const SOPV& v2);
    friend int operator!=(const SOPV& v1, const SOPV& v2);

public:
    /** @brief Default constructor. */
    SOPV();

    /** @brief Copy constructor. */
    SOPV(const SOPV& v);

    /** @brief Construct from a ZBDDV. */
    SOPV(const ZBDDV& zbddv);

    /**
     * @brief Construct from a single SOP at given index.
     * @param f The SOP element.
     * @param loc Index position (default 0).
     */
    SOPV(const SOP& f, int loc = 0);

    /** @brief Destructor. */
    ~SOPV();

    /** @brief Copy assignment. */
    SOPV& operator=(const SOPV& v);

    // --- Compound assignment ---

    SOPV& operator&=(const SOPV& v);
    SOPV& operator+=(const SOPV& v);
    SOPV& operator-=(const SOPV& v);

    // --- Shift ---

    /**
     * @brief Shift all variables up by @p n logic variables.
     *
     * Internally shifts ZBDDV by n*2 levels.
     */
    SOPV operator<<(int n) const;

    /** @brief Shift all variables down by @p n logic variables. */
    SOPV operator>>(int n) const;

    SOPV& operator<<=(int n);
    SOPV& operator>>=(int n);

    // --- Factor / And operations ---

    SOPV Factor1(int v) const;
    SOPV Factor0(int v) const;
    SOPV FactorD(int v) const;
    SOPV And1(int v) const;
    SOPV And0(int v) const;

    // --- Query ---

    /** @brief Top logic variable VarID (even). */
    int Top() const;

    /** @brief Node count of internal ZBDDV. */
    uint64_t Size() const;

    /** @brief Total cube count across all elements. */
    uint64_t Cube() const;

    /** @brief Total literal count across all elements. */
    uint64_t Lit() const;

    /** @brief Highest index with a non-empty element. */
    int Last() const;

    /**
     * @brief Get the SOP at the given index.
     * @param index Element index (0-based).
     */
    SOP GetSOP(int index) const;

    /** @brief Return the internal ZBDDV. */
    ZBDDV GetZBDDV() const;

    /**
     * @brief Extract a sub-range.
     * @param start Starting index.
     * @param length Number of elements (default 1).
     */
    SOPV Mask(int start, int length = 1) const;

    // --- Swap ---

    /**
     * @brief Swap two logic variables in all elements.
     * @param v1 Even VarID of first variable.
     * @param v2 Even VarID of second variable.
     */
    SOPV Swap(int v1, int v2) const;

    // --- Output ---

    /** @brief Print debug info for each element. */
    void Print() const;

    /**
     * @brief Print in PLA format.
     * @return 0 on success, 1 on error.
     */
    int PrintPla() const;
};

// --- SOPV binary operators ---

SOPV operator&(const SOPV& f, const SOPV& g);
SOPV operator+(const SOPV& f, const SOPV& g);
SOPV operator-(const SOPV& f, const SOPV& g);
int operator==(const SOPV& v1, const SOPV& v2);
int operator!=(const SOPV& v1, const SOPV& v2);

// --- SOPV free functions ---

/** @brief Alias for SOP_NewVar(). */
inline int SOPV_NewVar() { return SOP_NewVar(); }

/** @brief Alias for SOP_NewVarOfLev(). */
inline int SOPV_NewVarOfLev(int lev) { return SOP_NewVarOfLev(lev); }

/**
 * @brief Generate ISOP for each element of a BDDV.
 * @param v The BDDV (multi-output BDD).
 */
SOPV SOPV_ISOP(BDDV v);

/**
 * @brief Generate ISOP for each element with don't-care.
 * @param on Onset BDDV.
 * @param dc Don't-care BDDV.
 */
SOPV SOPV_ISOP(BDDV on, BDDV dc);

/**
 * @brief Generate ISOP with output polarity optimization.
 * @param v The BDDV (multi-output BDD).
 */
SOPV SOPV_ISOP2(BDDV v);

/**
 * @brief Generate ISOP with output polarity optimization and don't-care.
 * @param on Onset BDDV.
 * @param dc Don't-care BDDV.
 */
SOPV SOPV_ISOP2(BDDV on, BDDV dc);

#endif
