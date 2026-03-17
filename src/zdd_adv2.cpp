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

// --- bddimplychk ---

int bddimplychk(bddp f, bddvar v1, bddvar v2) {
    if (f == bddnull) return -1;
    if (v1 < 1 || v1 > bdd_varcount || v2 < 1 || v2 > bdd_varcount) {
        throw std::invalid_argument("bddimplychk: variable out of range");
    }
    if (v1 == v2) return 1;
    if (f & BDD_CONST_FLAG) return 1;

    bddp r = bdd_gc_guard([&]() -> bddp {
        // f10 = sets containing v1 but not v2
        bddp f1 = bddonset0(f, v1);
        bddp f10 = bddoffset(f1, v2);
        return (f10 == bddempty) ? (bddp)1 : (bddp)0;
    });
    return (int)r;
}

// --- bddcoimplychk ---

int bddcoimplychk(bddp f, bddvar v1, bddvar v2) {
    if (f == bddnull) return -1;
    if (v1 < 1 || v1 > bdd_varcount || v2 < 1 || v2 > bdd_varcount) {
        throw std::invalid_argument("bddcoimplychk: variable out of range");
    }
    if (v1 == v2) return 1;
    if (f & BDD_CONST_FLAG) return 1;

    bddp r = bdd_gc_guard([&]() -> bddp {
        // f10 = sets with v1 but not v2
        bddp f10 = bddoffset(bddonset0(f, v1), v2);
        if (f10 == bddempty) return (bddp)1;
        // f01 = sets with v2 but not v1
        bddp f01 = bddonset0(bddoffset(f, v1), v2);
        // co-implication holds iff f10 ⊆ f01
        bddp chk = bddsubtract(f10, f01);
        return (chk == bddempty) ? (bddp)1 : (bddp)0;
    });
    return (int)r;
}

// --- bddpermitsym ---

static bddp bddpermitsym_rec(bddp f, int n);

bddp bddpermitsym(bddp f, int n) {
    if (f == bddnull) return bddnull;
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;
    if (n < 1) return bddintersec(f, bddsingle);
    if (f & BDD_CONST_FLAG) return f;

    return bdd_gc_guard([&]() -> bddp { return bddpermitsym_rec(f, n); });
}

static bddp bddpermitsym_rec(bddp f, int n) {
    BDD_RecurGuard guard;

    if (f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;
    if (n < 1) return bddintersec(f, bddsingle);
    if (f & BDD_CONST_FLAG) return f;

    bddp cached = bddrcache(BDD_OP_PERMITSYM, f, static_cast<bddp>(n));
    if (cached != bddnull) return cached;

    bddvar top = bddtop(f);
    bddp f1 = bddonset0(f, top);
    bddp f0 = bddoffset(f, top);

    bddp r1 = bddchange(bddpermitsym_rec(f1, n - 1), top);
    bddp r0 = bddpermitsym_rec(f0, n);
    bddp result = bddunion(r1, r0);

    bddwcache(BDD_OP_PERMITSYM, f, static_cast<bddp>(n), result);
    return result;
}
