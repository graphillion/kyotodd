#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <climits>

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
        if (lit == INT_MIN) {
            throw std::invalid_argument("BDD::cube: INT_MIN is not a valid literal");
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
        if (lit == INT_MIN) {
            throw std::invalid_argument("BDD::clause: INT_MIN is not a valid literal");
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

    std::sort(result.begin(), result.end());
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
    if (n > bddvarused()) {
        throw std::invalid_argument(
            "uniform_sample: n exceeds the number of created variables");
    }

    // Ensure memo is populated
    if (!memo.stored()) {
        exact_count(n, memo);
    }

    std::vector<bddvar> result;
    bddp f = root;
    // Compute the max level among domain variables {1..n} so that
    // all don't-care variables above the root are handled correctly
    // even when variable ordering is non-default.
    bddvar current_level = 0;
    for (bddvar v = 1; v <= n; ++v) {
        bddvar lev = var2level[v];
        if (lev > current_level) current_level = lev;
    }
    const bigint::BigInt two(2);
    const bigint::BigInt zero(0);

    while (!(f & BDD_CONST_FLAG)) {
        // BDD complement semantics: complement toggles both lo and hi
        bool comp = (f & BDD_COMP_FLAG) != 0;
        bddp f_raw = f & ~BDD_COMP_FLAG;

        bddvar var = node_var(f_raw);
        bddvar node_level = var2level[var];

        // Randomly assign skipped variables (current_level down to node_level+1)
        // Filter out variables outside domain {1..n}
        for (bddvar lev = current_level; lev > node_level; --lev) {
            bddvar v = bddvaroflev(lev);
            if (v <= n) {
                if (rand_func(two) != zero) {
                    result.push_back(v);
                }
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
    // Filter out variables outside domain {1..n}
    if (f == bddtrue) {
        for (bddvar lev = current_level; lev > 0; --lev) {
            bddvar v = bddvaroflev(lev);
            if (v <= n) {
                if (rand_func(two) != zero) {
                    result.push_back(v);
                }
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
    // Deduplicate: bddchange toggles, so applying it twice cancels out.
    // Use sorted unique to ensure each variable is toggled exactly once.
    std::vector<bddvar> unique_vars(vars);
    std::sort(unique_vars.begin(), unique_vars.end());
    unique_vars.erase(std::unique(unique_vars.begin(), unique_vars.end()),
                      unique_vars.end());
    for (bddvar v : unique_vars) {
        f = bddchange(f, v);
    }
    return ZDD_ID(f);
}

ZDD ZDD::from_sets(const std::vector<std::vector<bddvar>>& sets) {
    bddp f = bddempty;
    for (const auto& s : sets) {
        bddp t = bddsingle;
        // Deduplicate each set
        std::vector<bddvar> unique_s(s);
        std::sort(unique_s.begin(), unique_s.end());
        unique_s.erase(std::unique(unique_s.begin(), unique_s.end()),
                       unique_s.end());
        for (bddvar v : unique_s) {
            t = bddchange(t, v);
        }
        f = bddunion(f, t);
    }
    return ZDD_ID(f);
}

// Recursive helper: all k-element subsets from sorted_vars[idx..end).
// sorted_vars is sorted by level descending so the root has the highest level.
static bddp combination_rec(const std::vector<bddvar>& sorted_vars,
                             size_t idx, bddvar k) {
    bddvar remaining = static_cast<bddvar>(sorted_vars.size() - idx);
    if (k == 0) return bddsingle;
    if (remaining < k) return bddempty;  // not enough variables left
    if (remaining == k) {
        // must take all remaining variables
        bddp f = bddsingle;
        for (size_t i = idx; i < sorted_vars.size(); ++i) {
            f = bddchange(f, sorted_vars[i]);
        }
        return f;
    }
    bddvar v = sorted_vars[idx];
    bddp lo = combination_rec(sorted_vars, idx + 1, k);
    bddp hi = combination_rec(sorted_vars, idx + 1, k - 1);
    return ZDD::getnode_raw(v, lo, hi);
}

ZDD ZDD::combination(bddvar n, bddvar k) {
    if (k > n) return ZDD::Empty;
    // Ensure variables exist
    while (bdd_varcount < n) {
        bddnewvar();
    }
    // Sort variables 1..n by level descending for correct ZDD node ordering
    std::vector<bddvar> sorted_vars(n);
    for (bddvar i = 0; i < n; ++i) sorted_vars[i] = i + 1;
    std::sort(sorted_vars.begin(), sorted_vars.end(),
              [](bddvar a, bddvar b) { return var2level[a] > var2level[b]; });
    return ZDD_ID(combination_rec(sorted_vars, 0, k));
}

std::vector<std::vector<bddvar>> ZDD::enumerate() const {
    if (root == bddnull) return {};
    std::vector<std::vector<bddvar>> result;
    std::vector<bddvar> current;
    enumerate_rec(root, current, result);
    for (auto& s : result) {
        std::sort(s.begin(), s.end());
    }
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

std::string ZDD::to_str() const {
    std::ostringstream oss;
    print_sets(oss);
    return oss.str();
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

long long ZDD::min_weight(const std::vector<int>& weights) const {
    return bddminweight(root, weights);
}

long long ZDD::max_weight(const std::vector<int>& weights) const {
    return bddmaxweight(root, weights);
}

std::vector<bddvar> ZDD::min_weight_set(const std::vector<int>& weights) const {
    return bddminweightset(root, weights);
}

std::vector<bddvar> ZDD::max_weight_set(const std::vector<int>& weights) const {
    return bddmaxweightset(root, weights);
}

// --- CostBoundMemo ---

CostBoundMemo::CostBoundMemo()
    : map_(), gc_generation_(0), weights_(), weights_bound_(false) {}

bool CostBoundMemo::lookup(bddp f, long long b,
                            bddp& h, long long& aw, long long& rb) const {
    auto it = map_.find(f);
    if (it == map_.end()) return false;
    const auto& intervals = it->second;
    // Find the last entry with aw <= b
    auto ub = intervals.upper_bound(b);
    if (ub == intervals.begin()) return false;
    --ub;
    // ub->first = aw, ub->second = (rb, h)
    if (b < ub->second.first) {
        aw = ub->first;
        rb = ub->second.first;
        h = ub->second.second;
        return true;
    }
    return false;
}

void CostBoundMemo::insert(bddp f, long long aw, long long rb, bddp h) {
    map_[f][aw] = {rb, h};
}

void CostBoundMemo::clear() {
    map_.clear();
}

void CostBoundMemo::invalidate_if_stale() {
    extern uint64_t bdd_gc_generation;
    if (gc_generation_ != bdd_gc_generation) {
        map_.clear();
        gc_generation_ = bdd_gc_generation;
    }
}

void CostBoundMemo::bind_weights(const std::vector<int>& weights) {
    if (!weights_bound_) {
        weights_ = weights;
        weights_bound_ = true;
    } else if (weights_ != weights) {
        throw std::invalid_argument(
            "CostBoundMemo: called with different weights vector");
    }
}

// --- ZDD::cost_bound ---

ZDD ZDD::cost_bound(const std::vector<int>& weights, long long b,
                     CostBoundMemo& memo) const {
    return ZDD_ID(bddcostbound(root, weights, b, memo));
}

ZDD ZDD::cost_bound(const std::vector<int>& weights, long long b) const {
    CostBoundMemo memo;
    return ZDD_ID(bddcostbound(root, weights, b, memo));
}
