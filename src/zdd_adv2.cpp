#include "bdd.h"
#include "bdd_internal.h"
#include <stdexcept>
#include <vector>
#include <unordered_set>

// --- bddispoly ---

int bddispoly(bddp f) {
    if (f == bddnull) return -1;

    while (!(f & BDD_CONST_FLAG)) {
        bool f_comp = (f & BDD_COMP_FLAG) != 0;
        bddp f_lo = node_lo(f);
        bddp f_hi = node_hi(f);
        if (f_comp) { f_lo = bddnot(f_lo); }

        if (f_lo != bddempty) return 1;
        f = f_hi;
    }
    return 0;
}

// --- bddswap ---

bddp bddswap(bddp f, bddvar v1, bddvar v2) {
    if (f == bddnull) return bddnull;
    if (v1 < 1 || v1 > bdd_varcount || v2 < 1 || v2 > bdd_varcount) {
        throw std::invalid_argument("bddswap: variable out of range");
    }
    if (v1 == v2) return f;
    if (f & BDD_CONST_FLAG) return f;

    return bdd_gc_guard([&]() -> bddp {
        // f00: sets containing neither v1 nor v2
        bddp f00 = bddoffset(bddoffset(f, v1), v2);
        // f11_raw: sets containing both v1 and v2 (with v1 and v2 removed)
        bddp f11_raw = bddonset0(bddonset0(f, v1), v2);
        // f11: restore v1 and v2
        bddp f11 = bddchange(bddchange(f11_raw, v1), v2);
        // h: sets containing exactly one of v1, v2
        bddp h = bddsubtract(bddsubtract(f, f00), f11);
        // Swap v1 and v2 in h by toggling both
        bddp h_swapped = bddchange(bddchange(h, v1), v2);
        // Result: f00 + f11 + h_swapped
        return bddunion(bddunion(f00, f11), h_swapped);
    });
}
