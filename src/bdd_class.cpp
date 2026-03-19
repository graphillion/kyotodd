#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <stdexcept>

const BDD BDD::False(0);
const BDD BDD::True(1);
const BDD BDD::Null(-1);

BDD BDD::prime(bddvar v) {
    return BDD_ID(bddprime(v));
}

BDD BDD::prime_not(bddvar v) {
    return BDD_ID(bddnot(bddprime(v)));
}

BDD BDD::cube(const std::vector<int>& lits) {
    bddp f = bddtrue;
    for (int lit : lits) {
        if (lit == 0) {
            throw std::invalid_argument("BDD::cube: literal 0 is not allowed");
        }
        bddp l = bddprime(static_cast<bddvar>(lit < 0 ? -lit : lit));
        if (lit < 0) l = bddnot(l);
        f = bddand(f, l);
    }
    return BDD_ID(f);
}

BDD BDD::clause(const std::vector<int>& lits) {
    bddp f = bddfalse;
    for (int lit : lits) {
        if (lit == 0) {
            throw std::invalid_argument("BDD::clause: literal 0 is not allowed");
        }
        bddp l = bddprime(static_cast<bddvar>(lit < 0 ? -lit : lit));
        if (lit < 0) l = bddnot(l);
        f = bddor(f, l);
    }
    return BDD_ID(f);
}

const ZDD ZDD::Empty(0);
const ZDD ZDD::Single(1);
const ZDD ZDD::Null(-1);

// --- ZddCountMemo constructors ---

ZddCountMemo::ZddCountMemo(bddp f) : f_(f), stored_(false), map_() {}
ZddCountMemo::ZddCountMemo(const ZDD& f) : f_(f.get_id()), stored_(false), map_() {}

// --- BddCountMemo constructors ---

BddCountMemo::BddCountMemo(bddp f, bddvar n) : f_(f), n_(n), stored_(false), map_() {}
BddCountMemo::BddCountMemo(const BDD& f, bddvar n) : f_(f.get_id()), n_(n), stored_(false), map_() {}

bigint::BigInt ZDD::exact_count() const {
    return bddexactcount(root);
}

bigint::BigInt ZDD::exact_count(ZddCountMemo& memo) const {
    if (memo.f() != root) {
        throw std::invalid_argument(
            "exact_count: memo was created for a different ZDD");
    }
    if (memo.stored()) {
        return bddexactcount(root, memo.map());
    }
    bigint::BigInt result = bddexactcount(root, memo.map());
    memo.mark_stored();
    return result;
}

std::vector<bddvar> ZDD::uniform_sample_impl(
        std::function<bigint::BigInt(const bigint::BigInt&)> rand_func,
        ZddCountMemo& memo) {
    if (root == bddempty || root == bddnull) {
        throw std::invalid_argument(
            "uniform_sample: cannot sample from empty family");
    }
    if (memo.f() != root) {
        throw std::invalid_argument(
            "uniform_sample: memo was created for a different ZDD");
    }

    // Ensure memo is populated
    if (!memo.stored()) {
        exact_count(memo);
    }

    std::vector<bddvar> result;
    bddp f = root;

    while (f != bddsingle && f != bddempty) {
        // ZDD complement semantics: complement toggles lo only
        bool comp = (f & BDD_COMP_FLAG) != 0;
        bddp f_raw = f & ~BDD_COMP_FLAG;

        bddvar var = node_var(f_raw);
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);

        if (comp) {
            lo = bddnot(lo);
        }

        bigint::BigInt count_lo = bddexactcount(lo, memo.map());
        bigint::BigInt count_hi = bddexactcount(hi, memo.map());
        bigint::BigInt total = count_lo + count_hi;

        bigint::BigInt r = rand_func(total);

        if (r < count_lo) {
            f = lo;
        } else {
            result.push_back(var);
            f = hi;
        }
    }

    return result;
}

std::vector<bddvar> BDD::uniform_sample_impl(
        std::function<bigint::BigInt(const bigint::BigInt&)> rand_func,
        bddvar n, BddCountMemo& memo) {
    if (root == bddfalse || root == bddnull) {
        throw std::invalid_argument(
            "uniform_sample: cannot sample from unsatisfiable function");
    }
    if (memo.f() != root) {
        throw std::invalid_argument(
            "uniform_sample: memo was created for a different BDD");
    }
    if (memo.n() != n) {
        throw std::invalid_argument(
            "uniform_sample: memo was created with a different n");
    }

    // Ensure memo is populated
    if (!memo.stored()) {
        exact_count(n, memo);
    }

    std::vector<bddvar> result;
    bddp f = root;
    bddvar current_level = n;
    const bigint::BigInt two(2);
    const bigint::BigInt zero(0);

    while (!(f & BDD_CONST_FLAG)) {
        // BDD complement semantics: complement toggles both lo and hi
        bool comp = (f & BDD_COMP_FLAG) != 0;
        bddp f_raw = f & ~BDD_COMP_FLAG;

        bddvar var = node_var(f_raw);
        bddvar node_level = var2level[var];

        // Randomly assign skipped variables (current_level down to node_level+1)
        for (bddvar lev = current_level; lev > node_level; --lev) {
            if (rand_func(two) != zero) {
                result.push_back(bddvaroflev(lev));
            }
        }

        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);

        if (comp) {
            lo = bddnot(lo);
            hi = bddnot(hi);
        }

        bigint::BigInt count_lo = bddexactcount(lo, n, memo.map());
        bigint::BigInt count_hi = bddexactcount(hi, n, memo.map());
        bigint::BigInt total = count_lo + count_hi;

        bigint::BigInt r = rand_func(total);

        if (r < count_lo) {
            f = lo;
        } else {
            result.push_back(var);
            f = hi;
        }

        current_level = node_level - 1;
    }

    // Terminal reached: randomly assign remaining levels
    if (f == bddtrue) {
        for (bddvar lev = current_level; lev > 0; --lev) {
            if (rand_func(two) != zero) {
                result.push_back(bddvaroflev(lev));
            }
        }
    }

    return result;
}

static void enumerate_rec(bddp f, std::vector<bddvar>& current,
                          std::vector<std::vector<bddvar>>& result) {
    if (f == bddempty) return;
    if (f == bddsingle) {
        result.push_back(current);
        return;
    }

    // ZDD complement semantics: complement toggles lo only
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddvar var = node_var(f_raw);
    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);

    if (comp) {
        lo = bddnot(lo);
    }

    // 0-edge: sets not containing var
    enumerate_rec(lo, current, result);
    // 1-edge: sets containing var
    current.push_back(var);
    enumerate_rec(hi, current, result);
    current.pop_back();
}

bool ZDD::has_empty() const {
    return bddhasempty(root);
}

ZDD ZDD::singleton(bddvar v) {
    return ZDD_ID(bddchange(bddsingle, v));
}

ZDD ZDD::power_set(bddvar n) {
    bddp f = bddsingle;
    for (bddvar v = 1; v <= n; ++v) {
        f = bddunion(f, bddchange(f, v));
    }
    return ZDD_ID(f);
}

ZDD ZDD::power_set(const std::vector<bddvar>& vars) {
    bddp f = bddsingle;
    for (bddvar v : vars) {
        f = bddunion(f, bddchange(f, v));
    }
    return ZDD_ID(f);
}

ZDD ZDD::single_set(const std::vector<bddvar>& vars) {
    bddp f = bddsingle;
    for (bddvar v : vars) {
        f = bddchange(f, v);
    }
    return ZDD_ID(f);
}

ZDD ZDD::from_sets(const std::vector<std::vector<bddvar>>& sets) {
    bddp f = bddempty;
    for (const auto& s : sets) {
        bddp t = bddsingle;
        for (bddvar v : s) {
            t = bddchange(t, v);
        }
        f = bddunion(f, t);
    }
    return ZDD_ID(f);
}

// Recursive helper: all k-element subsets of {v, v+1, ..., n}
static bddp combination_rec(bddvar v, bddvar n, bddvar k) {
    if (k == 0) return bddsingle;
    if (n - v + 1 < k) return bddempty;  // not enough variables left
    if (n - v + 1 == k) {
        // must take all remaining variables
        bddp f = bddsingle;
        for (bddvar i = v; i <= n; ++i) {
            f = bddchange(f, i);
        }
        return f;
    }
    // getznode(v, lo, hi): lo = subsets not containing v, hi = subsets containing v
    bddp lo = combination_rec(v + 1, n, k);
    bddp hi = combination_rec(v + 1, n, k - 1);
    return getznode(v, lo, hi);
}

ZDD ZDD::combination(bddvar n, bddvar k) {
    if (k > n) return ZDD::Empty;
    // Ensure variables exist
    while (bdd_varcount < n) {
        bddnewvar();
    }
    return ZDD_ID(combination_rec(1, n, k));
}

std::vector<std::vector<bddvar>> ZDD::enumerate() const {
    std::vector<std::vector<bddvar>> result;
    std::vector<bddvar> current;
    enumerate_rec(root, current, result);
    return result;
}

static void print_sets_rec(std::ostream& os, bddp f,
                           std::vector<bddvar>& current,
                           bool& first_set,
                           const std::string& delim1,
                           const std::string& delim2,
                           const std::vector<std::string>* var_name_map) {
    if (f == bddempty) return;
    if (f == bddsingle) {
        if (!first_set) os << delim1;
        first_set = false;
        for (size_t i = 0; i < current.size(); ++i) {
            if (i > 0) os << delim2;
            bddvar v = current[i];
            if (var_name_map && v < var_name_map->size()
                && !(*var_name_map)[v].empty()) {
                os << (*var_name_map)[v];
            } else {
                os << v;
            }
        }
        return;
    }

    // ZDD complement semantics: complement toggles lo only
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddvar var = node_var(f_raw);
    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);

    if (comp) {
        lo = bddnot(lo);
    }

    // 0-edge: sets not containing var
    print_sets_rec(os, lo, current, first_set, delim1, delim2, var_name_map);
    // 1-edge: sets containing var
    current.push_back(var);
    print_sets_rec(os, hi, current, first_set, delim1, delim2, var_name_map);
    current.pop_back();
}

void ZDD::print_sets(std::ostream& os) const {
    if (root == bddnull) { os << "N"; return; }
    if (root == bddempty) { os << "E"; return; }
    std::vector<bddvar> current;
    bool first_set = true;
    os << "{";
    print_sets_rec(os, root, current, first_set, "},{", ",", nullptr);
    os << "}";
}

void ZDD::print_sets(std::ostream& os, const std::string& delim1,
                     const std::string& delim2) const {
    if (root == bddnull) { os << "N"; return; }
    if (root == bddempty) { os << "E"; return; }
    std::vector<bddvar> current;
    bool first_set = true;
    print_sets_rec(os, root, current, first_set, delim1, delim2, nullptr);
}

void ZDD::print_sets(std::ostream& os, const std::string& delim1,
                     const std::string& delim2,
                     const std::vector<std::string>& var_name_map) const {
    if (root == bddnull) { os << "N"; return; }
    if (root == bddempty) { os << "E"; return; }
    std::vector<bddvar> current;
    bool first_set = true;
    print_sets_rec(os, root, current, first_set, delim1, delim2, &var_name_map);
}

void ZDD::Print() const {
    bddvar v = Top();
    std::cout << "[ " << GetID()
              << " Var:" << v << "(" << bddlevofvar(v) << ")"
              << " Size:" << Size()
              << " Card:" << Card()
              << " Lit:" << Lit()
              << " Len:" << Len()
              << " ]" << std::endl;
    std::cout.flush();
}

void ZDD::PrintPla() const {
    throw std::logic_error("ZDD::PrintPla: not implemented");
}

ZDD ZDD::ZLev(int /*lev*/, int /*last*/) const {
    throw std::logic_error("ZDD::ZLev: not implemented");
}

void ZDD::SetZSkip() const {
    throw std::logic_error("ZDD::SetZSkip: not implemented");
}
