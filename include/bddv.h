#ifndef KYOTODD_BDDV_H
#define KYOTODD_BDDV_H

/**
 * @file bddv.h
 * @brief BDDV class — an array of BDDs encoded as a single meta-BDD.
 *
 * @deprecated This class is obsolete and retained only for backward
 * compatibility. For new code, use @c std::vector<BDD> to manage multiple
 * BDDs, and @c BDD::raw_size(const std::vector<BDD>&) to count shared
 * nodes across multiple BDD roots.
 *
 * This header is NOT included by the umbrella header @c bdd.h.
 * You must explicitly @c \#include @c "bddv.h" to use this class.
 */

#include "bdd.h"

/** @brief Number of output-selection (system) variables. */
static const int BDDV_SysVarTop = 20;

/** @brief Maximum array length (2^BDDV_SysVarTop). */
static const int BDDV_MaxLen = (1 << BDDV_SysVarTop);

/** @brief Maximum number of outputs when importing. */
static const int BDDV_MaxLenImport = 1000;

/** @brief Nonzero if BDDV has been initialized via BDDV_Init(). */
extern int BDDV_Active;

/**
 * @brief An array of BDDs encoded into a single meta-BDD.
 *
 * @deprecated Use @c std::vector<BDD> instead for new code.
 *
 * BDDV uses "output selection variables" (system variables 1..BDDV_SysVarTop)
 * to multiplex multiple BDD functions into a single BDD. When all array
 * elements are identical, the internal representation is compact regardless
 * of array length.
 */
class BDDV {
    BDD _bdd;
    int _len;
    int _lev;

    int GetLev(int len) const;

    friend BDDV operator&(const BDDV& fv, const BDDV& gv);
    friend BDDV operator|(const BDDV& fv, const BDDV& gv);
    friend BDDV operator^(const BDDV& fv, const BDDV& gv);
    friend BDDV operator||(const BDDV& fv, const BDDV& gv);
    friend int operator==(const BDDV& fv, const BDDV& gv);
    friend int operator!=(const BDDV& fv, const BDDV& gv);

public:
    /** @brief Default constructor. Creates an empty (length 0) BDDV. */
    BDDV();

    /** @brief Copy constructor. */
    BDDV(const BDDV& fv);

    /**
     * @brief Construct a length-1 BDDV from a single BDD.
     * @param f A BDD (must not contain system variables).
     */
    BDDV(const BDD& f);

    /**
     * @brief Construct a BDDV where all @p len elements are the same BDD @p f.
     * @param f A BDD (must not contain system variables).
     * @param len Array length (0..BDDV_MaxLen).
     */
    BDDV(const BDD& f, int len);

    /** @brief Destructor. */
    ~BDDV();

    /** @brief Copy assignment. */
    BDDV& operator=(const BDDV& fv);

    // --- Logic operators ---

    /** @brief Element-wise NOT. */
    BDDV operator~() const;

    /** @brief In-place element-wise AND. */
    BDDV& operator&=(const BDDV& fv);
    /** @brief In-place element-wise OR. */
    BDDV& operator|=(const BDDV& fv);
    /** @brief In-place element-wise XOR. */
    BDDV& operator^=(const BDDV& fv);

    // --- Shift operators ---

    /** @brief Shift all input variable levels up by @p s. */
    BDDV operator<<(int s) const;
    /** @brief Shift all input variable levels down by @p s. */
    BDDV operator>>(int s) const;
    /** @brief In-place left shift. */
    BDDV& operator<<=(int s);
    /** @brief In-place right shift. */
    BDDV& operator>>=(int s);

    // --- Member functions ---

    /**
     * @brief Cofactor: restrict variable @p v to 0 in all elements.
     * @param v Variable number (must be a user variable).
     */
    BDDV At0(int v) const;

    /**
     * @brief Cofactor: restrict variable @p v to 1 in all elements.
     * @param v Variable number (must be a user variable).
     */
    BDDV At1(int v) const;

    /**
     * @brief Generalized cofactor of each element with respect to @p fv.
     * @param fv A BDDV of the same length.
     */
    BDDV Cofact(const BDDV& fv) const;

    /**
     * @brief Swap variables @p v1 and @p v2 in all elements.
     * @param v1 First variable number.
     * @param v2 Second variable number.
     */
    BDDV Swap(int v1, int v2) const;

    /**
     * @brief Apply Spread operation to all elements.
     * @param k Number of levels to spread.
     */
    BDDV Spread(int k) const;

    /**
     * @brief Return the top (highest-level) user variable across all elements.
     * @return Variable number, or 0 if all elements are constant.
     */
    int Top() const;

    /**
     * @brief Return the total shared node count across all elements.
     */
    uint64_t Size() const;

    /**
     * @brief Export the BDDV structure to a file.
     * @param strm Output file (default: stdout).
     */
    void Export(FILE* strm = stdout) const;

    /** @brief Print internal info for each element to stdout. */
    void Print() const;

    /** @brief Draw graph in X-Window (no complement edges). @deprecated Always throws. */
    void XPrint0() const;

    /** @brief Draw graph in X-Window (with complement edges). @deprecated Always throws. */
    void XPrint() const;

    /** @brief Return the first half of the array. */
    BDDV Former() const;

    /** @brief Return the second half of the array. */
    BDDV Latter() const;

    /**
     * @brief Return a sub-array starting at @p start with length @p len.
     * @param start Starting index.
     * @param len Number of elements.
     */
    BDDV Part(int start, int len) const;

    /**
     * @brief Return the BDD at the given index.
     * @param index Element index (0-based).
     */
    BDD GetBDD(int index) const;

    /** @brief Return the internal meta-BDD (includes system variables). */
    BDD GetMetaBDD() const;

    /** @brief Return 1 if all elements are identical, 0 otherwise. */
    int Uniform() const;

    /** @brief Return the array length. */
    int Len() const;
};

// --- Friend operators (declared outside class) ---

/** @brief Element-wise AND. Requires same length. */
BDDV operator&(const BDDV& fv, const BDDV& gv);
/** @brief Element-wise OR. Requires same length. */
BDDV operator|(const BDDV& fv, const BDDV& gv);
/** @brief Element-wise XOR. Requires same length. */
BDDV operator^(const BDDV& fv, const BDDV& gv);
/** @brief Concatenation. */
BDDV operator||(const BDDV& fv, const BDDV& gv);
/** @brief Equality (same elements and same length). */
int operator==(const BDDV& fv, const BDDV& gv);
/** @brief Inequality. */
int operator!=(const BDDV& fv, const BDDV& gv);

// --- Free functions ---

/**
 * @brief Initialize the BDDV subsystem.
 *
 * Calls bddinit() and allocates BDDV_SysVarTop output-selection variables.
 *
 * @param init Initial node table capacity.
 * @param limit Maximum node table size.
 * @return 0 on success, 1 on failure.
 */
int BDDV_Init(uint64_t init = 256, uint64_t limit = BDD_MaxNode);

/**
 * @brief Return the number of user variables (excluding system variables).
 */
int BDDV_UserTopLev();

/**
 * @brief Create a new user input variable.
 * @return The variable number (VarID) of the new variable.
 */
int BDDV_NewVar();

/**
 * @brief Create a new user input variable at a specific level.
 * @param lev The level at which to insert the variable.
 * @return The variable number (VarID) of the new variable.
 */
int BDDV_NewVarOfLev(int lev);

/**
 * @brief Check implication: all elements of ~fv | gv are tautologies.
 * @return 1 if fv implies gv element-wise, 0 otherwise.
 */
int BDDV_Imply(const BDDV& fv, const BDDV& gv);

/**
 * @brief Create a BDDV where only element @p index is true, others false.
 * @param index The index of the true element.
 * @param len Array length.
 */
BDDV BDDV_Mask1(int index, int len);

/**
 * @brief Create a BDDV where elements 0..index-1 are false, index..len-1 are true.
 * @param index The boundary index.
 * @param len Array length.
 */
BDDV BDDV_Mask2(int index, int len);

/**
 * @brief Import a BDDV from a file.
 * @param strm Input file (default: stdin).
 * @return The imported BDDV, or a null BDDV on error.
 */
BDDV BDDV_Import(FILE* strm = stdin);

/**
 * @brief Import a BDDV from an ESPRESSO PLA file.
 * @param strm Input file (default: stdin).
 * @param sopf If nonzero, use even-numbered levels for SOP compatibility.
 * @return The imported BDDV (onset || dcset), or a null BDDV on error.
 */
BDDV BDDV_ImportPla(FILE* strm = stdin, int sopf = 0);

#endif
