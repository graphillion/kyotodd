/***************************************************************
 *  Coloring reconfiguration — used-color-count families.       *
 *                                                              *
 *  Utilities to split the solution space (all proper           *
 *  k-colorings as an MVZDD) into subfamilies by the number of  *
 *  DISTINCT colors actually used.  Everything works directly   *
 *  on the one-hot encoded ZDD with offset operations, so no    *
 *  fresh DD manager is required (unlike build_solution_space). *
 *                                                              *
 *  Header-only, shared by the CLI driver and the tests.        *
 ***************************************************************/

#ifndef COLORING_RECONF_COLOR_COUNT_HPP
#define COLORING_RECONF_COLOR_COUNT_HPP

#include <vector>

#include "bdd.h"
#include "coloring_reconf.hpp"

namespace coloring_reconf {

/**
 * @brief Number of distinct colors used by one coloring.
 */
inline int count_used_colors(const std::vector<int>& coloring) {
    unsigned long long mask = 0;
    for (std::size_t i = 0; i < coloring.size(); ++i) {
        mask |= 1ull << coloring[i];
    }
    int cnt = 0;
    while (mask) { cnt += (int)(mask & 1); mask >>= 1; }
    return cnt;
}

/**
 * @brief Subfamily of f in which NO vertex has color c.
 *
 * In the one-hot encoding, color c > 0 of MVDD variable mv corresponds to
 * DD variable dd_vars_of(mv)[c-1] being present, and color 0 to all of
 * mv's DD variables being absent.  So for c > 0 this is a chain of
 * offset() calls (members not containing the bit, kept intact), and for
 * c == 0 it is f minus the union over vertices of "vertex v has color 0".
 *
 * @param f A family over the one-hot canonical encoding.
 * @param c A color in [0, k-1].
 * @return The members of f that use color c on no vertex.
 */
inline MVZDD avoid_color(const MVZDD& f, int c) {
    std::shared_ptr<MVDDVarTable> table = f.var_table();
    const int k = table->k();
    const bddvar n = table->mvdd_var_count();
    ZDD z = f.to_zdd();
    if (c > 0) {
        for (bddvar mv = 1; mv <= n; ++mv) {
            z = z.offset(table->dd_vars_of(mv)[c - 1]);
        }
        return MVZDD(table, z);
    }
    // c == 0: collect members where some vertex has all one-hot bits absent.
    ZDD some_zero = ZDD::Empty;
    for (bddvar mv = 1; mv <= n; ++mv) {
        const std::vector<bddvar>& dvars = table->dd_vars_of(mv);
        ZDD zv = z;
        for (int j = 0; j < k - 1; ++j) zv = zv.offset(dvars[j]);
        some_zero = some_zero + zv;
    }
    return MVZDD(table, z - some_zero);
}

/**
 * @brief Split a family by the number of distinct colors used.
 *
 * Dynamic programming over the colors: after processing color c,
 * d[j] holds the members using exactly j of the colors 0..c.  One color
 * step is d[j] = avoid_color(d[j], c) + (d[j-1] - avoid_color(d[j-1], c)).
 *
 * @param space Any family of colorings (typically all proper colorings).
 * @return Vector e of size k+1; e[m] is the subfamily of space whose
 *         members use exactly m distinct colors.  The e[m] partition
 *         space (members using no color at all, m = 0, can only occur
 *         for the empty assignment).
 */
inline std::vector<MVZDD> exact_color_count_families(const MVZDD& space) {
    std::shared_ptr<MVDDVarTable> table = space.var_table();
    const int k = table->k();
    const MVZDD empty = MVZDD::zero(table);
    std::vector<MVZDD> d(k + 1, empty);
    d[0] = space;
    for (int c = 0; c < k; ++c) {
        // avoid_color is a selection, so applying it once to the whole
        // space and intersecting is equivalent to (and much cheaper than)
        // running the offset chain on every DP family.
        MVZDD avoid = avoid_color(space, c);
        for (int j = k; j >= 0; --j) {
            MVZDD keep = d[j] & avoid;
            MVZDD gain = (j > 0) ? (d[j - 1] - avoid) : empty;
            d[j] = keep + gain;
        }
    }
    return d;
}

/**
 * @brief Union of the families using at most m distinct colors.
 *
 * @param exact The output of exact_color_count_families().
 * @param m Maximum number of distinct colors (clamped to the valid range).
 */
inline MVZDD at_most_color_count(const std::vector<MVZDD>& exact, int m) {
    MVZDD result = MVZDD::zero(exact[0].var_table());
    for (int j = 0; j < (int)exact.size() && j <= m; ++j) {
        result = result + exact[j];
    }
    return result;
}

}  // namespace coloring_reconf

#endif  // COLORING_RECONF_COLOR_COUNT_HPP
