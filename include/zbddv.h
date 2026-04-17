#ifndef KYOTODD_ZBDDV_H
#define KYOTODD_ZBDDV_H

/**
 * @file zbddv.h
 * @brief ZBDDV class — a sparse array of ZBDDs encoded as a single meta-ZBDD.
 *
 * @deprecated This class is obsolete and retained only for backward
 * compatibility. For new code, use @c std::vector<ZDD> to manage multiple
 * ZDDs, and @c ZDD::raw_size(const std::vector<ZDD>&) to count shared
 * nodes across multiple ZDD roots.
 *
 * This header is NOT included by the umbrella header @c bdd.h.
 * You must explicitly @c \#include @c "zbddv.h" to use this class.
 *
 * Requires BDDV_Init() to have been called (from bddv.h).
 *
 * No Python binding is provided for this class.
 */

#include "bddv.h"

namespace kyotodd {

/**
 * @brief A sparse array of ZBDDs encoded into a single meta-ZBDD.
 *
 * @deprecated Use @c std::vector<ZDD> instead for new code.
 *
 * ZBDDV uses ZBDD Change operations on system variables (1..BDDV_SysVarTop)
 * to encode array indices. Elements that are empty (ZDD 0) contribute nothing
 * to the meta-ZBDD, so sparse arrays are efficient. Unlike BDDV, array length
 * is implicit — determined by the highest non-empty index.
 */
class ZBDDV {
    ZDD _zbdd;

    friend ZBDDV operator&(const ZBDDV& fv, const ZBDDV& gv);
    friend ZBDDV operator+(const ZBDDV& fv, const ZBDDV& gv);
    friend ZBDDV operator-(const ZBDDV& fv, const ZBDDV& gv);
    friend int operator==(const ZBDDV& fv, const ZBDDV& gv);
    friend int operator!=(const ZBDDV& fv, const ZBDDV& gv);

public:
    /** @brief Default constructor. All elements are empty (ZDD 0). */
    ZBDDV();

    /** @brief Copy constructor. */
    ZBDDV(const ZBDDV& fv);

    /**
     * @brief Construct a ZBDDV with element @p f at index @p location.
     *
     * All other elements are empty (ZDD 0).
     * @param f A ZBDD (must not contain system variables).
     * @param location Array index (0-based, must be < BDDV_MaxLen).
     */
    ZBDDV(const ZDD& f, int location = 0);

    /** @brief Destructor. */
    ~ZBDDV();

    /** @brief Copy assignment. */
    ZBDDV& operator=(const ZBDDV& fv);

    // --- Set operators ---

    /** @brief In-place element-wise intersection. */
    ZBDDV& operator&=(const ZBDDV& fv);
    /** @brief In-place element-wise union. */
    ZBDDV& operator+=(const ZBDDV& fv);
    /** @brief In-place element-wise subtract. */
    ZBDDV& operator-=(const ZBDDV& fv);

    // --- Shift operators ---

    /** @brief Shift all item variable levels up by @p s. */
    ZBDDV operator<<(int s) const;
    /** @brief Shift all item variable levels down by @p s. */
    ZBDDV operator>>(int s) const;
    /** @brief In-place left shift. */
    ZBDDV& operator<<=(int s);
    /** @brief In-place right shift. */
    ZBDDV& operator>>=(int s);

    // --- Member functions ---

    /**
     * @brief Offset: keep only sets NOT containing item @p v in each element.
     * @param v Variable number (must be a user variable).
     */
    ZBDDV OffSet(int v) const;

    /**
     * @brief OnSet: keep only sets containing item @p v in each element.
     * @param v Variable number (must be a user variable).
     */
    ZBDDV OnSet(int v) const;

    /**
     * @brief OnSet0: like OnSet(v) then remove item @p v.
     * @param v Variable number (must be a user variable).
     */
    ZBDDV OnSet0(int v) const;

    /**
     * @brief Toggle item @p v in all sets of each element.
     * @param v Variable number (must be a user variable).
     */
    ZBDDV Change(int v) const;

    /**
     * @brief Swap items @p v1 and @p v2 in all elements.
     * @param v1 First variable number.
     * @param v2 Second variable number.
     */
    ZBDDV Swap(int v1, int v2) const;

    /**
     * @brief Return the top (highest-level) user variable across all elements.
     * @return Variable number, or 0 if all elements are empty.
     */
    int Top() const;

    /**
     * @brief Return the highest index with a non-empty element.
     * @return Maximum non-empty index, or 0 if all elements are empty.
     */
    int Last() const;

    /**
     * @brief Extract a sub-array from @p start to @p start+length-1.
     * @param start Starting index.
     * @param length Number of elements (default: 1).
     */
    ZBDDV Mask(int start, int length = 1) const;

    /**
     * @brief Return the ZBDD at the given index.
     * @param index Element index (0-based).
     */
    ZDD GetZBDD(int index) const;

    /** @brief Return the internal meta-ZBDD (includes system variables). */
    ZDD GetMetaZBDD() const;

    /**
     * @brief Return the total shared node count across all elements.
     */
    uint64_t Size() const;

    /** @brief Print internal info for each element to stdout. */
    void Print() const;

    /**
     * @brief Export the ZBDDV structure to a file.
     * @param strm Output file (default: stdout).
     */
    void Export(FILE* strm = stdout) const;

    /**
     * @brief Print the ZBDDV as PLA format to stdout.
     * @return 0 on success, 1 on error.
     */
    int PrintPla() const;

    /** @brief Draw graph in X-Window. @deprecated Always throws. */
    void XPrint() const;
};

// --- Friend operators (declared outside class) ---

/** @brief Element-wise intersection. */
ZBDDV operator&(const ZBDDV& fv, const ZBDDV& gv);
/** @brief Element-wise union. */
ZBDDV operator+(const ZBDDV& fv, const ZBDDV& gv);
/** @brief Element-wise subtract. */
ZBDDV operator-(const ZBDDV& fv, const ZBDDV& gv);
/** @brief Equality. */
int operator==(const ZBDDV& fv, const ZBDDV& gv);
/** @brief Inequality. */
int operator!=(const ZBDDV& fv, const ZBDDV& gv);

// --- Free functions ---

/**
 * @brief Import a ZBDDV from a file.
 * @param strm Input file (default: stdin).
 * @return The imported ZBDDV, or a null ZBDDV on error.
 */
ZBDDV ZBDDV_Import(FILE* strm = stdin);

} // namespace kyotodd

#endif
