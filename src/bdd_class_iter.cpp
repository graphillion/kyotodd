#include "bdd.h"
#include "bdd_internal.h"
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace kyotodd {

// =====================================================================
// enumerate_iter — explicit-stack rewrite of enumerate_rec.
//
// DFS over a ZDD, emitting each reached bddsingle as a copy of the
// current path (variables taken via 1-edges). Same visit order as
// enumerate_rec (lo-first, then hi with top variable pushed).
//
// PRECONDITION: see header. enumerate_iter does not allocate new DD
// nodes, so GC cannot fire during the call.
// =====================================================================
void enumerate_iter(bddp root, std::vector<bddvar>& current,
                    std::vector<std::vector<bddvar>>& result) {
    enum class Phase : uint8_t { ENTER, AFTER_LO, AFTER_HI };
    struct Frame {
        bddp f;
        bddvar var;
        bddp hi;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root; init.var = 0; init.hi = 0; init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f == bddempty) { stack.pop_back(); break; }
            if (f == bddsingle) {
                result.push_back(current);
                stack.pop_back();
                break;
            }
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp f_raw = f & ~BDD_COMP_FLAG;
            bddvar var = node_var(f_raw);
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            if (comp) lo = bddnot(lo);

            frame.var = var;
            frame.hi = hi;
            frame.phase = Phase::AFTER_LO;

            Frame child;
            child.f = lo; child.var = 0; child.hi = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::AFTER_LO: {
            current.push_back(frame.var);
            bddp hi = frame.hi;
            frame.phase = Phase::AFTER_HI;

            Frame child;
            child.f = hi; child.var = 0; child.hi = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::AFTER_HI: {
            current.pop_back();
            stack.pop_back();
            break;
        }
        }
    }
}

// =====================================================================
// combination_iter — explicit-stack rewrite of combination_rec.
//
// Build the ZDD of all k-element subsets selected from sorted_vars[idx..).
// Recursion decomposes at sorted_vars[idx]:
//   lo = rec(idx+1, k)     (skip this variable)
//   hi = rec(idx+1, k-1)   (take this variable)
// then result = ZDD::getnode_raw(v, lo, hi).
// =====================================================================
bddp combination_iter(const std::vector<bddvar>& sorted_vars,
                      size_t idx, bddvar k) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        size_t idx;
        bddvar k;
        bddvar v;
        bddp lo;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.idx = idx; init.k = k;
    init.v = 0; init.lo = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddvar remaining =
                static_cast<bddvar>(sorted_vars.size() - frame.idx);
            if (frame.k == 0) {
                result = bddsingle;
                stack.pop_back();
                break;
            }
            if (remaining < frame.k) {
                result = bddempty;
                stack.pop_back();
                break;
            }
            if (remaining == frame.k) {
                bddp f = bddsingle;
                for (size_t i = frame.idx; i < sorted_vars.size(); ++i) {
                    f = bddchange(f, sorted_vars[i]);
                }
                result = f;
                stack.pop_back();
                break;
            }
            frame.v = sorted_vars[frame.idx];
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.idx = frame.idx + 1; child.k = frame.k;
            child.v = 0; child.lo = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.idx = frame.idx + 1; child.k = frame.k - 1;
            child.v = 0; child.lo = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp hi = result;
            result = ZDD::getnode_raw(frame.v, frame.lo, hi);
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// print_sets_iter — explicit-stack rewrite of print_sets_rec.
//
// Same traversal shape as enumerate_iter: lo-first DFS with the top
// variable pushed before descending into hi. Each bddsingle reached
// emits the current path using (delim1, delim2, var_name_map).
// =====================================================================
void print_sets_iter(std::ostream& os, bddp root,
                     std::vector<bddvar>& current,
                     bool& first_set,
                     const std::string& delim1,
                     const std::string& delim2,
                     const std::vector<std::string>* var_name_map) {
    enum class Phase : uint8_t { ENTER, AFTER_LO, AFTER_HI };
    struct Frame {
        bddp f;
        bddvar var;
        bddp hi;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root; init.var = 0; init.hi = 0; init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f == bddempty) { stack.pop_back(); break; }
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
                stack.pop_back();
                break;
            }
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp f_raw = f & ~BDD_COMP_FLAG;
            bddvar var = node_var(f_raw);
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            if (comp) lo = bddnot(lo);

            frame.var = var;
            frame.hi = hi;
            frame.phase = Phase::AFTER_LO;

            Frame child;
            child.f = lo; child.var = 0; child.hi = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::AFTER_LO: {
            current.push_back(frame.var);
            bddp hi = frame.hi;
            frame.phase = Phase::AFTER_HI;

            Frame child;
            child.f = hi; child.var = 0; child.hi = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::AFTER_HI: {
            current.pop_back();
            stack.pop_back();
            break;
        }
        }
    }
}

// =====================================================================
// print_pla_iter — explicit-stack rewrite of print_pla_rec.
//
// Enumerates product terms by iterating over every variable level from
// lev down to 0. At each level: set cube[lev-1]='1' and recurse on
// bddonset0(f, var); then cube[lev-1]='0' and recurse on bddoffset.
// Returns false if any bddnull is encountered (propagated eagerly).
// =====================================================================
bool print_pla_iter(bddp root, bddvar root_lev, std::string& cube) {
    enum class Phase : uint8_t { ENTER, GOT_FIRST, GOT_SECOND };
    struct Frame {
        bddp f;
        bddvar lev;
        bddvar var;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root; init.lev = root_lev; init.var = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bool result = true;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            bddvar lev = frame.lev;
            if (f == bddnull) {
                result = false;
                stack.pop_back();
                break;
            }
            if (f == bddempty) {
                result = true;
                stack.pop_back();
                break;
            }
            if (lev == 0) {
                char out_ch = (f != bddempty) ? '1' : '~';
                std::cout << cube << " " << out_ch << "\n";
                std::cout.flush();
                result = true;
                stack.pop_back();
                break;
            }
            bddvar var = bddvaroflev(lev);
            frame.var = var;
            cube[lev - 1] = '1';
            frame.phase = Phase::GOT_FIRST;

            Frame child;
            child.f = bddonset0(f, var);
            child.lev = lev - 1;
            child.var = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_FIRST: {
            if (!result) {
                // Propagate failure: drop this frame; outer pop unwinds.
                stack.pop_back();
                break;
            }
            bddp f = frame.f;
            bddvar lev = frame.lev;
            bddvar var = frame.var;
            cube[lev - 1] = '0';
            frame.phase = Phase::GOT_SECOND;

            Frame child;
            child.f = bddoffset(f, var);
            child.lev = lev - 1;
            child.var = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_SECOND: {
            // result already reflects success/failure of the second child.
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// ws_count_iter — explicit-stack rewrite of ws_count_rec.
//
// Memoized count of sets reachable from f. Memo keyed on f_raw
// (complement stripped) so that both f and ~f share the memo entry;
// complement adjustment is applied per-call using bddhasempty(f_raw).
// =====================================================================
double ws_count_iter(bddp root,
                     std::unordered_map<bddp, double>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp f_raw;
        bddp hi;
        double c_lo;
        bool comp;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root; init.f_raw = 0; init.hi = 0; init.c_lo = 0.0;
    init.comp = false; init.phase = Phase::ENTER;
    stack.push_back(init);

    double result = 0.0;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f == bddempty) {
                result = 0.0;
                stack.pop_back();
                break;
            }
            if (f == bddsingle) {
                result = 1.0;
                stack.pop_back();
                break;
            }
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp f_raw = f & ~BDD_COMP_FLAG;
            auto it = memo.find(f_raw);
            if (it != memo.end()) {
                double c = it->second;
                if (comp) {
                    c += bddhasempty(f_raw) ? -1.0 : 1.0;
                }
                result = c;
                stack.pop_back();
                break;
            }
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            frame.comp = comp;
            frame.f_raw = f_raw;
            frame.hi = hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = lo; child.f_raw = 0; child.hi = 0;
            child.c_lo = 0.0; child.comp = false;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.c_lo = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = frame.hi; child.f_raw = 0; child.hi = 0;
            child.c_lo = 0.0; child.comp = false;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            double c = frame.c_lo + result;
            memo[frame.f_raw] = c;
            if (frame.comp) {
                c += bddhasempty(frame.f_raw) ? -1.0 : 1.0;
            }
            result = c;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// ws_total_sum_iter — explicit-stack rewrite of ws_total_sum_rec.
//
// Sum of (sum-of-weights) over all sets of F(f). Complement-invariant
// (sum_memo is keyed on f_raw with the raw value stored directly).
// Uses ws_count_iter to obtain c_hi per node.
// =====================================================================
double ws_total_sum_iter(bddp root,
                         const std::vector<double>& weights,
                         WeightMemoMap& sum_memo,
                         std::unordered_map<bddp, double>& count_memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp f_raw;
        bddvar var;
        bddp hi;
        double s_lo;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root; init.f_raw = 0; init.var = 0; init.hi = 0;
    init.s_lo = 0.0; init.phase = Phase::ENTER;
    stack.push_back(init);

    double result = 0.0;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f == bddempty || f == bddsingle) {
                result = 0.0;
                stack.pop_back();
                break;
            }
            bddp f_raw = f & ~BDD_COMP_FLAG;
            auto it = sum_memo.find(f_raw);
            if (it != sum_memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }
            bddvar var = node_var(f_raw);
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            frame.f_raw = f_raw;
            frame.var = var;
            frame.hi = hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = lo; child.f_raw = 0; child.var = 0; child.hi = 0;
            child.s_lo = 0.0; child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.s_lo = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = frame.hi; child.f_raw = 0; child.var = 0; child.hi = 0;
            child.s_lo = 0.0; child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            double s_hi = result;
            bddp hi_copy = frame.hi;
            double c_hi = ws_count_iter(hi_copy, count_memo);
            double val = frame.s_lo + s_hi + c_hi * weights[frame.var];
            sum_memo[frame.f_raw] = val;
            result = val;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// ws_total_prod_iter — explicit-stack rewrite of ws_total_prod_rec.
//
// Sum of (product-of-weights) over all sets of F(f). NOT
// complement-invariant: prod_memo stores the raw value (as if f had
// no complement); callers with complement receive the stored value
// plus a ±1 adjustment depending on bddhasempty(f_raw).
// =====================================================================
double ws_total_prod_iter(bddp root,
                          const std::vector<double>& weights,
                          WeightMemoMap& prod_memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp f_raw;
        bddvar var;
        bddp hi;
        double p_lo;
        bool comp;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root; init.f_raw = 0; init.var = 0; init.hi = 0;
    init.p_lo = 0.0; init.comp = false; init.phase = Phase::ENTER;
    stack.push_back(init);

    double result = 0.0;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f == bddempty) {
                result = 0.0;
                stack.pop_back();
                break;
            }
            if (f == bddsingle) {
                result = 1.0;
                stack.pop_back();
                break;
            }
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp f_raw = f & ~BDD_COMP_FLAG;
            auto it = prod_memo.find(f_raw);
            if (it != prod_memo.end()) {
                double val = it->second;
                if (comp) {
                    val += bddhasempty(f_raw) ? -1.0 : 1.0;
                }
                result = val;
                stack.pop_back();
                break;
            }
            bddvar var = node_var(f_raw);
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            frame.comp = comp;
            frame.f_raw = f_raw;
            frame.var = var;
            frame.hi = hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = lo; child.f_raw = 0; child.var = 0; child.hi = 0;
            child.p_lo = 0.0; child.comp = false;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.p_lo = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = frame.hi; child.f_raw = 0; child.var = 0; child.hi = 0;
            child.p_lo = 0.0; child.comp = false;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            double p_hi = result;
            double val = frame.p_lo + weights[frame.var] * p_hi;
            prod_memo[frame.f_raw] = val;
            if (frame.comp) {
                val += bddhasempty(frame.f_raw) ? -1.0 : 1.0;
            }
            result = val;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

} // namespace kyotodd
