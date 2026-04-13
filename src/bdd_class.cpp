#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <climits>
#include <cmath>

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
ZddCountMemo::ZddCountMemo(const ZDD& f) : f_(f.id()), stored_(false), map_() {}

// --- BddCountMemo constructors ---

BddCountMemo::BddCountMemo(bddp f, bddvar n) : f_(f), n_(n), stored_(false), map_() {}
BddCountMemo::BddCountMemo(const BDD& f, bddvar n) : f_(f.id()), n_(n), stored_(false), map_() {}

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

bool ZDD::contains(const std::vector<bddvar>& s) const {
    return bddcontains(root, s);
}

bool ZDD::is_subset_family(const ZDD& g) const {
    return bddissubset(root, g.root);
}

ZDD ZDD::flatten() const {
    return ZDD_ID(bddflatten(root));
}

ZDD ZDD::coalesce(bddvar v1, bddvar v2) const {
    return ZDD_ID(bddcoalesce(root, v1, v2));
}

ZDD ZDD::choose(int k) const {
    return ZDD_ID(bddchoose(root, k));
}

std::vector<bigint::BigInt> ZDD::profile() const {
    return bddprofile(root);
}

std::vector<bigint::BigInt> ZDD::element_frequency() const {
    return bddelmfreq(root);
}

std::vector<double> ZDD::profile_double() const {
    return bddprofile_double(root);
}

double ZDD::average_size() const {
    auto prof = profile_double();
    double total_sets = 0.0;
    double total_elements = 0.0;
    for (size_t i = 0; i < prof.size(); i++) {
        total_sets += prof[i];
        total_elements += static_cast<double>(i) * prof[i];
    }
    if (total_sets == 0.0) return 0.0;
    return total_elements / total_sets;
}

double ZDD::variance_size() const {
    auto prof = profile();
    bigint::BigInt total_sets(0);
    bigint::BigInt sum(0);
    bigint::BigInt sum_sq(0);
    for (size_t i = 0; i < prof.size(); i++) {
        bigint::BigInt bi(static_cast<long long>(i));
        total_sets += prof[i];
        sum += bi * prof[i];
        sum_sq += bi * bi * prof[i];
    }
    if (total_sets == bigint::BigInt(0)) return 0.0;
    // variance = E[X^2] - (E[X])^2
    //          = (sum_sq * total_sets - sum * sum) / (total_sets * total_sets)
    bigint::BigInt numer = sum_sq * total_sets - sum * sum;
    bigint::BigInt denom = total_sets * total_sets;
    return bigint_ratio_to_double(numer, denom);
}

double ZDD::median_size() const {
    auto prof = profile();
    bigint::BigInt total(0);
    for (auto& v : prof) total += v;
    if (total == bigint::BigInt(0)) return 0.0;

    // Find median via cumulative sum
    bigint::BigInt two(2);
    bigint::BigInt total_x2 = total * two;
    bigint::BigInt cumul(0);
    for (size_t i = 0; i < prof.size(); i++) {
        bigint::BigInt prev_cumul = cumul;
        cumul += prof[i];
        bigint::BigInt cumul_x2 = cumul * two;
        if (cumul_x2 > total) {
            // All of total/2 is within bin i
            return static_cast<double>(i);
        }
        if (cumul_x2 == total) {
            // Median is between bin i and the next non-empty bin
            for (size_t j = i + 1; j < prof.size(); j++) {
                if (prof[j] > bigint::BigInt(0)) {
                    return (static_cast<double>(i) + static_cast<double>(j)) / 2.0;
                }
            }
            // All sets have size i
            return static_cast<double>(i);
        }
    }
    return 0.0;
}

double ZDD::entropy() const {
    auto freq = element_frequency();
    bigint::BigInt total_lit(0);
    for (auto& v : freq) {
        total_lit += v;
    }
    if (total_lit == bigint::BigInt(0)) return 0.0;

    double h = 0.0;
    for (auto& v : freq) {
        if (v > bigint::BigInt(0)) {
            double p = bigint_ratio_to_double(v, total_lit);
            if (p > 0.0) {
                h -= p * std::log2(p);
            }
        }
    }
    return h;
}

bigint::BigInt ZDD::hamming_distance(const ZDD& g) const {
    return bddhammingdist(root, g.root);
}

double ZDD::overlap_coefficient(const ZDD& g) const {
    return bddoverlapcoeff(root, g.root);
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

std::string ZDD::to_cnf() const {
    if (root == bddnull) return "N";
    if (root == bddempty) return "T";
    std::ostringstream oss;
    std::vector<bddvar> current;
    bool first_set = true;
    oss << "(";
    print_sets_rec(oss, root, current, first_set, ") & (", " | ", nullptr);
    oss << ")";
    return oss.str();
}

std::string ZDD::to_cnf(
    const std::vector<std::string>& var_name_map) const {
    if (root == bddnull) return "N";
    if (root == bddempty) return "T";
    std::ostringstream oss;
    std::vector<bddvar> current;
    bool first_set = true;
    oss << "(";
    print_sets_rec(oss, root, current, first_set,
                   ") & (", " | ", &var_name_map);
    oss << ")";
    return oss.str();
}

std::string ZDD::to_dnf() const {
    if (root == bddnull) return "N";
    if (root == bddempty) return "F";
    std::ostringstream oss;
    std::vector<bddvar> current;
    bool first_set = true;
    oss << "(";
    print_sets_rec(oss, root, current, first_set, ") | (", " & ", nullptr);
    oss << ")";
    return oss.str();
}

std::string ZDD::to_dnf(
    const std::vector<std::string>& var_name_map) const {
    if (root == bddnull) return "N";
    if (root == bddempty) return "F";
    std::ostringstream oss;
    std::vector<bddvar> current;
    bool first_set = true;
    oss << "(";
    print_sets_rec(oss, root, current, first_set,
                   ") | (", " & ", &var_name_map);
    oss << ")";
    return oss.str();
}

void ZDD::print() const {
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

void ZDD::print_pla() const {
    if (root == bddnull) return;

    // Use the total number of declared variables for .i, not just the
    // ZDD's top level. This ensures all variables appear in the PLA
    // output even if some are not in the ZDD's support.
    bddvar tlev = bddvarused();

    std::cout << ".i " << tlev << "\n";
    std::cout << ".o 1" << "\n";

    if (tlev == 0) {
        // No input variables
        if (root != bddempty) {
            std::cout << "1" << "\n";
        } else {
            std::cout << "0" << "\n";
        }
    } else {
        // Recursive enumeration of product terms
        std::string cube(tlev, '0');
        // Lambda for recursive traversal
        std::function<bool(ZDD, bddvar)> rec = [&](ZDD f, bddvar lev) -> bool {
            BDD_RecurGuard guard;
            if (f.root == bddnull) return false; // error: abort
            if (f.root == bddempty) return true;  // empty: prune
            if (lev == 0) {
                char out_ch = (f.root != bddempty) ? '1' : '~';
                std::cout << cube << " " << out_ch << "\n";
                std::cout.flush();
                return true;
            }
            bddvar var = bddvaroflev(lev);
            // OnSet0 side (variable included, cube char = '1')
            cube[lev - 1] = '1';
            if (!rec(f.onset0(var), lev - 1)) return false;
            // OffSet side (variable not included, cube char = '0')
            cube[lev - 1] = '0';
            if (!rec(f.offset(var), lev - 1)) return false;
            return true;
        };

        bool ok = rec(*this, tlev);
        if (!ok) {
            // Error during traversal: do not output footer
            return;
        }
    }

    std::cout << ".e" << "\n";
    std::cout.flush();
}

// Operation code for ZSkip cache (BC_ZBDD_ZSkip = 65)
static const uint8_t BDD_OP_ZSKIP = 65;

// ZLevNum: compute the skip-target level for a given level n.
// Returns n - (skip width), i.e. the level to skip to.
static bddvar zlevnum(bddvar n) {
    bddvar skip;
    switch (n & 3) {
    case 3: // bit1=1, bit0=1: largest skip
        if      (n < 16)    skip = 4;
        else if (n < 64)    skip = 8;
        else if (n < 128)   skip = 32;
        else if (n < 256)   skip = 64;
        else if (n < 512)   skip = 128;
        else if (n < 1024)  skip = 256;
        else if (n < 2048)  skip = 512;
        else if (n < 4096)  skip = 1024;
        else if (n < 8192)  skip = 2048;
        else if (n < 32768) skip = 4096;
        else                skip = 8192;
        break;
    case 2: // bit1=1, bit0=0
        if      (n < 64)    skip = 4;
        else if (n < 256)   skip = 16;
        else if (n < 512)   skip = 32;
        else if (n < 1024)  skip = 64;
        else if (n < 4096)  skip = 128;
        else if (n < 32768) skip = 512;
        else                skip = 1024;
        break;
    case 1: // bit1=0, bit0=1
        if      (n < 16)    skip = 4;
        else if (n < 512)   skip = 8;
        else if (n < 1024)  skip = 16;
        else if (n < 2048)  skip = 32;
        else if (n < 32768) skip = 64;
        else                skip = 128;
        break;
    default: // case 0: bit1=0, bit0=0: smallest skip
        if      (n < 1024)  skip = 4;
        else if (n < 32768) skip = 8;
        else                skip = 16;
        break;
    }
    return n - skip;
}

ZDD ZDD::zlev(bddvar lev, int last) const {
    // Propagate bddnull
    if (root == bddnull) return ZDD(-1);

    // Handle complement: strip it, work on raw node, re-apply at end.
    // In ZDD, complement toggles ∅ membership. OffSet commutes with
    // complement: (~f).offset(v) == ~(f.offset(v)), so the skip
    // structure is identical and only ∅ membership differs.
    bool comp = (root & BDD_COMP_FLAG) != 0;

    // Base case: lev == 0
    if (lev == 0) {
        // Return intersection with constant 1 (single).
        // This yields {∅} if ∅ ∈ F, else ∅.
        if (bddhasempty(root))
            return ZDD(1);
        else
            return ZDD(0);
    }

    // Work on raw (non-complemented) node
    ZDD f = comp ? ZDD_ID(root & ~static_cast<bddp>(BDD_COMP_FLAG)) : *this;
    ZDD u = f;
    bddvar ftop = f.top();
    bddvar flev = bddlevofvar(ftop);

    while (flev > lev) {
        // 3a. Skip acceleration (when level gap >= 5)
        if (flev - lev >= 5) {
            bddvar n = zlevnum(flev);

            // Safety check (overshoot prevention)
            if (flev >= 66) {
                if (n < lev || ((flev & 3) < 3 && zlevnum(flev - 3) >= lev)) {
                    n = flev - 1;
                }
            } else if (flev >= 18) {
                if (n < lev || ((flev & 1) < 1 && zlevnum(flev - 1) >= lev)) {
                    n = flev - 1;
                }
            } else {
                if (n < lev) {
                    n = flev - 1;
                }
            }

            // If skip covers 2+ levels, try cache
            if (n < flev - 1) {
                bddp fn = f.root;  // already raw (no complement)
                bddp cached = bddrcache(BDD_OP_ZSKIP, fn, fn);
                if (cached != bddnull) {
                    ZDD g = ZDD_ID(cached);
                    bddvar gtop = g.top();
                    bddvar glev = bddlevofvar(gtop);
                    if (glev >= lev) {
                        f = g;
                        ftop = f.top();
                        flev = bddlevofvar(ftop);
                        continue;
                    }
                }
            }
        }

        // 3b. Normal 1-level descent
        u = f;
        f = f.offset(ftop);
        ftop = f.top();
        flev = bddlevofvar(ftop);
    }

    // Select result
    ZDD result;
    if (last == 0) {
        result = f;
    } else {
        if (flev == lev)
            result = f;
        else
            result = u;
    }

    // Restore complement if original was complemented
    if (comp) result = ~result;
    return result;
}

void ZDD::set_zskip() const {
    // Propagate bddnull: nothing to do
    if (root == bddnull) return;

    // If complemented, work on the raw node instead.
    // The skip structure is the same regardless of complement.
    if (root & BDD_COMP_FLAG) {
        ZDD raw = ZDD_ID(root & ~static_cast<bddp>(BDD_COMP_FLAG));
        raw.set_zskip();
        return;
    }

    // Early exit: level <= 4
    bddvar tv = top();
    bddvar tlev = bddlevofvar(tv);
    if (tlev <= 4) return;

    // Already cached?
    bddp cached = bddrcache(BDD_OP_ZSKIP, root, root);
    if (cached != bddnull) return;

    BDD_RecurGuard guard;

    // Recurse on 0-branch (OffSet) first
    ZDD f0 = offset(tv);
    f0.set_zskip();

    // Compute skip target
    bddvar n = zlevnum(tlev);
    ZDD skip_target = zlev(n, 1);

    // Avoid self-loop
    if (skip_target.root == root) {
        skip_target = f0;
    }

    // Recurse on 1-branch (OnSet0)
    ZDD f1 = onset0(tv);
    f1.set_zskip();

    // Write to cache AFTER all recursion completes, so that if an
    // exception occurs during subtree processing, the root entry
    // is not left in a partially-constructed state.
    bddwcache(BDD_OP_ZSKIP, root, root, skip_target.root);
}

bigint::BigInt ZDD::get_sum(const std::vector<int>& weights) const {
    return bddweightsum(root, weights);
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

// --- WeightedSampleMemo ---

WeightedSampleMemo::WeightedSampleMemo(
    const ZDD& f, const std::vector<double>& weights, WeightMode mode)
    : f_(f.id()), stored_(false), mode_(mode), weights_(weights) {}

// --- Weighted sampling precomputation ---

// Count sets reachable from f (as double). Memoizes on f_raw.
// Complement adjustment applied when f has complement flag.
static double ws_count_rec(bddp f, std::unordered_map<bddp, double>& memo) {
    if (f == bddempty) return 0.0;
    if (f == bddsingle) return 1.0;

    BDD_RecurGuard guard;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    auto it = memo.find(f_raw);
    double c;
    if (it != memo.end()) {
        c = it->second;
    } else {
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);
        c = ws_count_rec(lo, memo) + ws_count_rec(hi, memo);
        memo[f_raw] = c;
    }

    if (comp) {
        c += bddhasempty(f_raw) ? -1.0 : 1.0;
    }
    return c;
}

// Sum of (sum-of-weights) over all sets in F(f). Complement-invariant.
static double ws_total_sum_rec(
    bddp f,
    const std::vector<double>& weights,
    WeightMemoMap& sum_memo,
    std::unordered_map<bddp, double>& count_memo)
{
    if (f == bddempty || f == bddsingle) return 0.0;

    BDD_RecurGuard guard;

    bddp f_raw = f & ~BDD_COMP_FLAG;
    auto it = sum_memo.find(f_raw);
    if (it != sum_memo.end()) return it->second;

    bddvar var = node_var(f_raw);
    bddp lo = node_lo(f_raw);
    bddp hi = node_hi(f_raw);

    double s_lo = ws_total_sum_rec(lo, weights, sum_memo, count_memo);
    double s_hi = ws_total_sum_rec(hi, weights, sum_memo, count_memo);
    double c_hi = ws_count_rec(hi, count_memo);

    double result = s_lo + s_hi + c_hi * weights[var];
    sum_memo[f_raw] = result;
    return result;
}

// Sum of (product-of-weights) over all sets in F(f). NOT complement-invariant.
static double ws_total_prod_rec(
    bddp f,
    const std::vector<double>& weights,
    WeightMemoMap& prod_memo)
{
    if (f == bddempty) return 0.0;
    if (f == bddsingle) return 1.0;

    BDD_RecurGuard guard;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    auto it = prod_memo.find(f_raw);
    double val;
    if (it != prod_memo.end()) {
        val = it->second;
    } else {
        bddvar var = node_var(f_raw);
        bddp lo = node_lo(f_raw);
        bddp hi = node_hi(f_raw);

        double p_lo = ws_total_prod_rec(lo, weights, prod_memo);
        double p_hi = ws_total_prod_rec(hi, weights, prod_memo);

        val = p_lo + weights[var] * p_hi;
        prod_memo[f_raw] = val;
    }

    if (comp) {
        val += bddhasempty(f_raw) ? -1.0 : 1.0;
    }
    return val;
}

// --- ZDD::boltzmann_weights ---

std::vector<double> ZDD::boltzmann_weights(
    const std::vector<double>& weights, double beta)
{
    std::vector<double> tw(weights.size());
    if (!tw.empty()) tw[0] = 0.0;
    for (size_t i = 1; i < weights.size(); ++i) {
        double v = std::exp(-beta * weights[i]);
        if (!std::isfinite(v)) {
            throw std::invalid_argument(
                "boltzmann_weights: non-finite transformed weight at index "
                + std::to_string(i)
                + " (check beta and weights values)");
        }
        tw[i] = v;
    }
    return tw;
}

// --- ZDD::weighted_sample_impl ---

std::vector<bddvar> ZDD::weighted_sample_impl(
    const std::vector<double>& weights, WeightMode mode,
    std::function<double(double)> rand_func,
    WeightedSampleMemo& memo)
{
    if (root == bddnull) {
        throw std::invalid_argument("weighted_sample: null ZDD");
    }
    if (root == bddempty) {
        throw std::invalid_argument(
            "weighted_sample: cannot sample from empty family");
    }
    if (memo.f() != root) {
        throw std::invalid_argument(
            "weighted_sample: memo was created for a different ZDD");
    }
    if (memo.mode() != mode) {
        throw std::invalid_argument(
            "weighted_sample: memo mode mismatch");
    }

    // Validate weights size against max variable in support
    {
        std::vector<bddvar> sup = bddsupport_vec(root);
        bddvar max_var = 0;
        for (bddvar v : sup) {
            if (v > max_var) max_var = v;
        }
        if (max_var > 0 && weights.size() <= static_cast<size_t>(max_var)) {
            throw std::invalid_argument(
                "weighted_sample: weights.size() must be > max variable in support");
        }
    }

    // Validate non-negative finite weights
    for (size_t i = 1; i < weights.size(); ++i) {
        if (!std::isfinite(weights[i]) || weights[i] < 0.0) {
            throw std::invalid_argument(
                "weighted_sample: invalid weight at index "
                + std::to_string(i)
                + " (must be non-negative and finite)");
        }
    }

    // Validate weights match memo's weights
    if (weights != memo.weights()) {
        throw std::invalid_argument(
            "weighted_sample: weights do not match memo's weights");
    }

    // Populate memo if needed
    if (!memo.stored()) {
        if (mode == WeightMode::Sum) {
            ws_total_sum_rec(root, weights,
                             memo.weight_map(), memo.count_map());
        } else {
            ws_total_prod_rec(root, weights, memo.weight_map());
        }
        memo.mark_stored();
    }

    // Special case: family is exactly {∅}
    if (root == bddsingle) {
        if (mode == WeightMode::Sum) {
            // w(∅) = 0 → total = 0; but only 1 set, return it
            return std::vector<bddvar>();
        } else {
            // w(∅) = 1 → OK
            return std::vector<bddvar>();
        }
    }

    bddp f = root;
    bool f_has_empty = bddhasempty(f);
    std::vector<bddvar> result;

    if (mode == WeightMode::Sum) {
        // Sum mode: w(∅) = 0, so ∅ can never be sampled (P=0)
        if (f_has_empty) {
            f = bddnot(f);  // remove ∅
        }

        // Compute total weight for validation
        double total = ws_total_sum_rec(f, weights,
                                        memo.weight_map(), memo.count_map());
        if (total <= 0.0) {
            throw std::invalid_argument(
                "weighted_sample: total weight is zero (Sum mode)");
        }

        double bias = 0.0;
        while (f != bddsingle && f != bddempty) {
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp f_raw = f & ~BDD_COMP_FLAG;

            bddvar var = node_var(f_raw);
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            if (comp) lo = bddnot(lo);

            double c_lo = ws_count_rec(lo, memo.count_map());
            double c_hi = ws_count_rec(hi, memo.count_map());
            double s_lo = ws_total_sum_rec(lo, weights,
                                           memo.weight_map(), memo.count_map());
            double s_hi = ws_total_sum_rec(hi, weights,
                                           memo.weight_map(), memo.count_map());

            double W_lo = c_lo * bias + s_lo;
            double W_hi = c_hi * (bias + weights[var]) + s_hi;
            double W_total = W_lo + W_hi;

            double r = rand_func(W_total);
            if (r < W_hi) {
                result.push_back(var);
                bias += weights[var];
                f = hi;
            } else {
                f = lo;
            }
        }

    } else {
        // Product mode: w(∅) = 1
        if (f_has_empty) {
            bddp f_no_empty = bddnot(f);
            double total_no_empty = ws_total_prod_rec(
                f_no_empty, weights, memo.weight_map());
            double total_with_empty = total_no_empty + 1.0;

            if (total_with_empty <= 0.0) {
                throw std::invalid_argument(
                    "weighted_sample: total weight is zero (Product mode)");
            }

            double r = rand_func(total_with_empty);
            if (r < 1.0) {
                return result;  // sampled ∅
            }
            f = f_no_empty;
        }

        double total = ws_total_prod_rec(f, weights, memo.weight_map());
        if (total <= 0.0) {
            throw std::invalid_argument(
                "weighted_sample: total weight is zero (Product mode)");
        }

        while (f != bddsingle && f != bddempty) {
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp f_raw = f & ~BDD_COMP_FLAG;

            bddvar var = node_var(f_raw);
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            if (comp) lo = bddnot(lo);

            double W_lo = ws_total_prod_rec(lo, weights, memo.weight_map());
            double W_hi = weights[var] *
                          ws_total_prod_rec(hi, weights, memo.weight_map());
            double W_total = W_lo + W_hi;

            double r = rand_func(W_total);
            if (r < W_hi) {
                result.push_back(var);
                f = hi;
            } else {
                f = lo;
            }
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

// --- ZDD::cost_bound_le ---

ZDD ZDD::cost_bound_le(const std::vector<int>& weights, long long b,
                        CostBoundMemo& memo) const {
    return ZDD_ID(bddcostbound_le(root, weights, b, memo));
}

ZDD ZDD::cost_bound_le(const std::vector<int>& weights, long long b) const {
    CostBoundMemo memo;
    return ZDD_ID(bddcostbound_le(root, weights, b, memo));
}

// --- ZDD::cost_bound_ge ---

ZDD ZDD::cost_bound_ge(const std::vector<int>& weights, long long b,
                        CostBoundMemo& memo) const {
    return ZDD_ID(bddcostbound_ge(root, weights, b, memo));
}

ZDD ZDD::cost_bound_ge(const std::vector<int>& weights, long long b) const {
    CostBoundMemo memo;
    return ZDD_ID(bddcostbound_ge(root, weights, b, memo));
}

// --- ZDD::cost_bound_eq ---

ZDD ZDD::cost_bound_eq(const std::vector<int>& weights, long long b,
                        CostBoundMemo& memo) const {
    ZDD le_b = cost_bound_le(weights, b, memo);
    if (b == LLONG_MIN) return le_b;  // no sum < LLONG_MIN, so le(b-1) is empty
    ZDD le_b1 = cost_bound_le(weights, b - 1, memo);
    return le_b - le_b1;
}

ZDD ZDD::cost_bound_eq(const std::vector<int>& weights, long long b) const {
    CostBoundMemo memo;
    return cost_bound_eq(weights, b, memo);
}

// --- ZDD::cost_bound_range ---

ZDD ZDD::cost_bound_range(const std::vector<int>& weights, long long lo,
                           long long hi, CostBoundMemo& memo) const {
    ZDD le_hi = cost_bound_le(weights, hi, memo);
    if (lo == LLONG_MIN) return le_hi;  // no sum < LLONG_MIN, so le(lo-1) is empty
    ZDD le_lo1 = cost_bound_le(weights, lo - 1, memo);
    return le_hi - le_lo1;
}

ZDD ZDD::cost_bound_range(const std::vector<int>& weights, long long lo,
                           long long hi) const {
    CostBoundMemo memo;
    return cost_bound_range(weights, lo, hi, memo);
}

// --- ZDD::size_le / size_ge ---

ZDD ZDD::size_le(int k) const {
    std::vector<int> weights(bddvarused() + 1, 1);
    weights[0] = 0;
    return cost_bound_le(weights, static_cast<long long>(k));
}

ZDD ZDD::size_ge(int k) const {
    std::vector<int> weights(bddvarused() + 1, 1);
    weights[0] = 0;
    return cost_bound_ge(weights, static_cast<long long>(k));
}

ZDD ZDD::supersets_of(const std::vector<bddvar>& s) const {
    return ZDD_ID(bddsupersets_of(root, s));
}

ZDD ZDD::subsets_of(const std::vector<bddvar>& s) const {
    return ZDD_ID(bddsubsets_of(root, s));
}

ZDD ZDD::project(const std::vector<bddvar>& vars) const {
    return ZDD_ID(bddproject(root, vars));
}

// --- ZDD::rank / unrank ---

int64_t ZDD::rank(const std::vector<bddvar>& s) const {
    return bddrank(root, s);
}

bigint::BigInt ZDD::exact_rank(const std::vector<bddvar>& s) const {
    return bddexactrank(root, s);
}

bigint::BigInt ZDD::exact_rank(const std::vector<bddvar>& s,
                               ZddCountMemo& memo) const {
    if (memo.f() != root) {
        throw std::invalid_argument(
            "exact_rank: memo was created for a different ZDD");
    }
    return bddexactrank(root, s, memo.map());
}

std::vector<bddvar> ZDD::unrank(int64_t order) const {
    return bddunrank(root, order);
}

std::vector<bddvar> ZDD::exact_unrank(const bigint::BigInt& order) const {
    return bddexactunrank(root, order);
}

std::vector<bddvar> ZDD::exact_unrank(const bigint::BigInt& order,
                                      ZddCountMemo& memo) const {
    if (memo.f() != root) {
        throw std::invalid_argument(
            "exact_unrank: memo was created for a different ZDD");
    }
    return bddexactunrank(root, order, memo.map());
}

// --- ZDD::get_k_sets ---

ZDD ZDD::get_k_sets(int64_t k) const {
    return ZDD_ID(bddgetksets(root, k));
}

ZDD ZDD::get_k_sets(const bigint::BigInt& k) const {
    return ZDD_ID(bddgetksets(root, k));
}

ZDD ZDD::get_k_sets(const bigint::BigInt& k, ZddCountMemo& memo) const {
    if (memo.f() != root) {
        throw std::invalid_argument(
            "get_k_sets: memo was created for a different ZDD");
    }
    return ZDD_ID(bddgetksets(root, k, memo.map()));
}

// --- ZDD::get_k_lightest / get_k_heaviest ---

ZDD ZDD::get_k_lightest(int64_t k, const std::vector<int>& weights,
                         int strict) const {
    return ZDD_ID(bddgetklightest(root, k, weights, strict));
}

ZDD ZDD::get_k_lightest(const bigint::BigInt& k,
                         const std::vector<int>& weights,
                         int strict) const {
    return ZDD_ID(bddgetklightest(root, k, weights, strict));
}

ZDD ZDD::get_k_heaviest(int64_t k, const std::vector<int>& weights,
                         int strict) const {
    return ZDD_ID(bddgetkheaviest(root, k, weights, strict));
}

ZDD ZDD::get_k_heaviest(const bigint::BigInt& k,
                         const std::vector<int>& weights,
                         int strict) const {
    return ZDD_ID(bddgetkheaviest(root, k, weights, strict));
}
