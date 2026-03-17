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
