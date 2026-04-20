#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace kyotodd {

// BDDLEN_MAX / BDDLIT_MAX / BDDCARD_MAX mirror the saturation bounds
// in zdd_adv_count.cpp.
static const uint64_t BDDLEN_MAX_ITER = (UINT64_C(1) << 39) - 1;
static const uint64_t BDDLIT_MAX_ITER = (UINT64_C(1) << 39) - 1;
static const uint64_t BDDCARD_MAX_ITER = (UINT64_C(1) << 39) - 1;

// =====================================================================
// bddchoose_iter — Template B (unary + int, bddp return, cache-memoized)
//
// PRECONDITION: Must be invoked under a bdd_gc_guard scope. The public
// wrapper strips the complement bit and handles k == 0 / terminal cases
// up front, so this helper assumes f is a non-terminal, non-complemented
// bddp and k > 0 (cache key compatibility with bddchoose_rec).
// =====================================================================
bddp bddchoose_iter(bddp f, int k) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;          // non-complement node id (cache key)
        int k;
        bddvar top_var;
        bddp f_lo;       // lo child with complement absorbed
        bddp f_hi;       // hi child
        bddp lo_result;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f; init.k = k;
    init.top_var = 0;
    init.f_lo = 0; init.f_hi = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            int ck = frame.k;
            if (cf == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (ck < 0) { result = bddempty; stack.pop_back(); break; }
            if (cf == bddsingle) {
                result = (ck == 0) ? bddsingle : bddempty;
                stack.pop_back();
                break;
            }
            if (ck == 0) {
                result = bddhasempty(cf) ? bddsingle : bddempty;
                stack.pop_back();
                break;
            }

            // k > 0, non-terminal: strip complement for cache/structure access
            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;

            bddp cached = bddrcache(BDD_OP_CHOOSE, f_raw, static_cast<bddp>(ck));
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bddvar var = node_var(f_raw);
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            if (comp) lo = bddnot(lo);

            // Cache key is f_raw; store it so GOT_HI writes the same key
            frame.f = f_raw;
            frame.top_var = var;
            frame.f_lo = lo;
            frame.f_hi = hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = lo; child.k = ck;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f = frame.f_hi; child.k = frame.k - 1;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi_result = result;
            bddp combined = ZDD::getnode_raw(
                frame.top_var, frame.lo_result, hi_result);
            bddwcache(BDD_OP_CHOOSE, frame.f,
                      static_cast<bddp>(frame.k), combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddminsize_iter — Template D (unary, uint64_t return)
//
// Mirrors bddminsize_rec: the cache key is the full bddp (including the
// complement bit), matching the recursive implementation.
//
// PRECONDITION: Caller holds bdd_gc_guard (no new nodes are created so
// this is a conservative requirement that matches other _iter helpers).
// =====================================================================
uint64_t bddminsize_iter(bddp f) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f_orig;     // input (complement-preserving, cache key)
        bddp f_lo;       // lo with complement absorbed
        bddp f_hi;
        uint64_t min0;
        Phase phase;
    };

    uint64_t result = 0;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f_orig = f;
    init.f_lo = 0; init.f_hi = 0;
    init.min0 = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f_orig;
            if (cf == bddempty) { result = UINT64_MAX; stack.pop_back(); break; }
            if (cf == bddsingle) { result = 0; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_MINSIZE, cf, 0);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            if (comp) lo = bddnot(lo);

            frame.f_lo = lo; frame.f_hi = hi;
            frame.phase = Phase::GOT_LO;
            Frame child;
            child.f_orig = lo;
            child.f_lo = 0; child.f_hi = 0;
            child.min0 = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.min0 = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f_orig = frame.f_hi;
            child.f_lo = 0; child.f_hi = 0;
            child.min0 = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            uint64_t min1 = result;
            if (min1 != UINT64_MAX) {
                if (min1 >= BDDLEN_MAX_ITER) {
                    min1 = BDDLEN_MAX_ITER;
                } else {
                    min1 += 1;
                }
            }
            uint64_t r = (frame.min0 < min1) ? frame.min0 : min1;
            bddwcache(BDD_OP_MINSIZE, frame.f_orig, 0, r);
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddcount_iter — ZDD double counting (Template D, memo-backed).
//
// Mirrors bddcount_rec's structure: memo stores the non-complement count
// indexed by f_raw, and complement adjustment is re-applied after each
// lookup (it toggles ∅ membership, which changes the count by ±1).
//
// PRECONDITION: Caller holds bdd_gc_guard (no new nodes are created so
// this is a conservative requirement that matches other _iter helpers).
// =====================================================================
double bddcount_iter(bddp f, std::unordered_map<bddp, double>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f_raw;
        bool comp;
        bddp lo, hi;
        double lo_count;
        Phase phase;
    };

    double result = 0.0;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f_raw = f;           // ENTER will normalize
    init.comp = false;
    init.lo = 0; init.hi = 0;
    init.lo_count = 0.0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f_raw;  // actually raw-or-complemented input
            if (cf == bddempty) { result = 0.0; stack.pop_back(); break; }
            if (cf == bddsingle) { result = 1.0; stack.pop_back(); break; }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;

            auto it = memo.find(f_raw);
            if (it != memo.end()) {
                double count = it->second;
                if (comp) {
                    count += bddhasempty(f_raw) ? -1.0 : 1.0;
                }
                result = count;
                stack.pop_back();
                break;
            }

            frame.f_raw = f_raw;
            frame.comp = comp;
            frame.lo = node_lo(f_raw);
            frame.hi = node_hi(f_raw);
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f_raw = frame.lo;
            child.comp = false;
            child.lo = 0; child.hi = 0;
            child.lo_count = 0.0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_count = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f_raw = frame.hi;
            child.comp = false;
            child.lo = 0; child.hi = 0;
            child.lo_count = 0.0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            double hi_count = result;
            double count = frame.lo_count + hi_count;
            memo[frame.f_raw] = count;
            if (frame.comp) {
                count += bddhasempty(frame.f_raw) ? -1.0 : 1.0;
            }
            result = count;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddexactcount_iter — ZDD BigInt counting (Template D + CountMemoMap).
//
// Exposed via bdd_internal.h (non-static) so zdd_adv_count_iter.cpp's
// bddelmfreq_iter / bddcountintersec_iter can share the memo table.
//
// PRECONDITION: Caller holds bdd_gc_guard (no new nodes are created so
// this is a conservative requirement that matches other _iter helpers).
// =====================================================================
bigint::BigInt bddexactcount_iter(
    bddp f, std::unordered_map<bddp, bigint::BigInt>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f_raw;
        bool comp;
        bddp lo, hi;
        bigint::BigInt lo_count;
        Phase phase;
    };

    bigint::BigInt result(0);
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f_raw = f;
    init.comp = false;
    init.lo = 0; init.hi = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f_raw;
            if (cf == bddempty) { result = bigint::BigInt(0); stack.pop_back(); break; }
            if (cf == bddsingle) { result = bigint::BigInt(1); stack.pop_back(); break; }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;

            auto it = memo.find(f_raw);
            if (it != memo.end()) {
                bigint::BigInt count = it->second;
                if (comp) {
                    if (bddhasempty(f_raw)) count -= bigint::BigInt(1);
                    else count += bigint::BigInt(1);
                }
                result = count;
                stack.pop_back();
                break;
            }

            frame.f_raw = f_raw;
            frame.comp = comp;
            frame.lo = node_lo(f_raw);
            frame.hi = node_hi(f_raw);
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f_raw = frame.lo;
            child.comp = false;
            child.lo = 0; child.hi = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_count = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f_raw = frame.hi;
            child.comp = false;
            child.lo = 0; child.hi = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bigint::BigInt hi_count = result;
            bigint::BigInt count = frame.lo_count + hi_count;
            memo[frame.f_raw] = count;
            if (frame.comp) {
                if (bddhasempty(frame.f_raw)) count -= bigint::BigInt(1);
                else count += bigint::BigInt(1);
            }
            result = count;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddprofile_iter — ZDD set-size distribution (Template D + vector memo).
//
// The memo stores the uncomplemented profile at f_raw. Complement
// adjustment (toggle ∅ membership ↔ profile[0] ± 1) is applied after
// every memo read and after fresh computation.
//
// PRECONDITION: Caller holds bdd_gc_guard (no new nodes are created so
// this is a conservative requirement that matches other _iter helpers).
// =====================================================================
std::vector<bigint::BigInt> bddprofile_iter(
    bddp f,
    std::unordered_map<bddp, std::vector<bigint::BigInt>>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f_raw;
        bool comp;
        bddp lo, hi;
        std::vector<bigint::BigInt> lo_profile;
        Phase phase;
    };

    std::vector<bigint::BigInt> result;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f_raw = f;
    init.comp = false;
    init.lo = 0; init.hi = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    auto apply_comp_adjust = [](std::vector<bigint::BigInt>& r,
                                bddp f_raw_key, bool comp) {
        if (!comp) return;
        if (r.empty()) {
            r.push_back(bigint::BigInt(1));
        } else if (bddhasempty(f_raw_key)) {
            r[0] -= bigint::BigInt(1);
            while (!r.empty() && r.back().is_zero()) r.pop_back();
        } else {
            r[0] += bigint::BigInt(1);
        }
    };

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f_raw;
            if (cf == bddempty) {
                result.clear();
                stack.pop_back();
                break;
            }
            if (cf == bddsingle) {
                result.assign(1, bigint::BigInt(1));
                stack.pop_back();
                break;
            }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;

            auto it = memo.find(f_raw);
            if (it != memo.end()) {
                result = it->second;
                apply_comp_adjust(result, f_raw, comp);
                stack.pop_back();
                break;
            }

            frame.f_raw = f_raw;
            frame.comp = comp;
            frame.lo = node_lo(f_raw);
            frame.hi = node_hi(f_raw);
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f_raw = frame.lo;
            child.comp = false;
            child.lo = 0; child.hi = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_profile = std::move(result);
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f_raw = frame.hi;
            child.comp = false;
            child.lo = 0; child.hi = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            std::vector<bigint::BigInt> hi_profile = std::move(result);
            // hi-profile shifts by +1 (every set in hi contains current var)
            size_t max_len = std::max(frame.lo_profile.size(),
                                      hi_profile.size() + 1);
            std::vector<bigint::BigInt> combined(max_len);
            for (size_t i = 0; i < frame.lo_profile.size(); ++i) {
                combined[i] += frame.lo_profile[i];
            }
            for (size_t i = 0; i < hi_profile.size(); ++i) {
                combined[i + 1] += hi_profile[i];
            }
            memo[frame.f_raw] = combined;  // non-complement version
            apply_comp_adjust(combined, frame.f_raw, frame.comp);
            result = std::move(combined);
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddelmfreq_iter — ZDD per-variable frequency (Template D + vector memo,
//                                               plus external count memo).
//
// Complement edges only toggle ∅ membership which has no elements, so
// the frequency is identical for f and ~f. The recursive body therefore
// does not branch on the complement bit; we simply key the memo on f_raw.
//
// PRECONDITION: Caller holds bdd_gc_guard (no new nodes are created so
// this is a conservative requirement that matches other _iter helpers).
// =====================================================================
std::vector<bigint::BigInt> bddelmfreq_iter(
    bddp f,
    std::unordered_map<bddp, std::vector<bigint::BigInt>>& freq_memo,
    std::unordered_map<bddp, bigint::BigInt>& count_memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f_raw;
        bddvar var;
        bddp lo, hi;
        std::vector<bigint::BigInt> lo_freq;
        Phase phase;
    };

    std::vector<bigint::BigInt> result;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f_raw = f;
    init.var = 0;
    init.lo = 0; init.hi = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f_raw;
            if (cf == bddempty || cf == bddsingle) {
                result.clear();
                stack.pop_back();
                break;
            }

            bddp f_raw = cf & ~BDD_COMP_FLAG;
            auto it = freq_memo.find(f_raw);
            if (it != freq_memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }

            bddvar var = node_var(f_raw);
            frame.f_raw = f_raw;
            frame.var = var;
            frame.lo = node_lo(f_raw);
            frame.hi = node_hi(f_raw);
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f_raw = frame.lo;
            child.var = 0;
            child.lo = 0; child.hi = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_freq = std::move(result);
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f_raw = frame.hi;
            child.var = 0;
            child.lo = 0; child.hi = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            std::vector<bigint::BigInt> hi_freq = std::move(result);

            size_t result_size = static_cast<size_t>(frame.var) + 1;
            if (frame.lo_freq.size() > result_size) result_size = frame.lo_freq.size();
            if (hi_freq.size() > result_size) result_size = hi_freq.size();

            std::vector<bigint::BigInt> combined(result_size);
            for (size_t i = 0; i < frame.lo_freq.size(); ++i) {
                combined[i] += frame.lo_freq[i];
            }
            for (size_t i = 0; i < hi_freq.size(); ++i) {
                combined[i] += hi_freq[i];
            }
            combined[frame.var] += bddexactcount_iter(frame.hi, count_memo);

            freq_memo[frame.f_raw] = combined;
            result = std::move(combined);
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddcountintersec_iter — |F ∩ G| without materializing intersection.
//
// Structure is Template A-ish (binary DD walk, commutative normalization,
// pair memo) but the return value is a BigInt (Template D).
//
// PRECONDITION: Caller holds bdd_gc_guard (no new nodes are created so
// this is a conservative requirement that matches other _iter helpers).
// =====================================================================
bigint::BigInt bddcountintersec_iter(
    bddp f, bddp g,
    std::unordered_map<bddp,
        std::unordered_map<bddp, bigint::BigInt>>& pair_memo,
    std::unordered_map<bddp, bigint::BigInt>& count_memo) {
    enum class Phase : uint8_t { ENTER, GOT_FIRST, GOT_SECOND };
    struct Frame {
        bddp f, g;             // post-canonicalisation cache key
        bool two_children;     // same_top case needs two sub-results
        bddp child2_f;         // second child args (only when two_children)
        bddp child2_g;
        bigint::BigInt first_result;
        Phase phase;
    };

    bigint::BigInt result(0);
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f; init.g = g;
    init.two_children = false;
    init.child2_f = 0; init.child2_g = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;
            if (cf == bddempty || cg == bddempty) {
                result = bigint::BigInt(0);
                stack.pop_back();
                break;
            }
            if (cf == bddsingle && cg == bddsingle) {
                result = bigint::BigInt(1);
                stack.pop_back();
                break;
            }
            if (cf == bddsingle) {
                result = bddhasempty(cg) ? bigint::BigInt(1) : bigint::BigInt(0);
                stack.pop_back();
                break;
            }
            if (cg == bddsingle) {
                result = bddhasempty(cf) ? bigint::BigInt(1) : bigint::BigInt(0);
                stack.pop_back();
                break;
            }
            if (cf == cg) {
                result = bddexactcount_iter(cf, count_memo);
                stack.pop_back();
                break;
            }

            // Canonical order (intersection is commutative)
            if (cf > cg) { bddp t = cf; cf = cg; cg = t; }
            frame.f = cf; frame.g = cg;

            auto it_f = pair_memo.find(cf);
            if (it_f != pair_memo.end()) {
                auto it_g = it_f->second.find(cg);
                if (it_g != it_f->second.end()) {
                    result = it_g->second;
                    stack.pop_back();
                    break;
                }
            }

            bool f_const = (cf & BDD_CONST_FLAG) != 0;
            bool g_const = (cg & BDD_CONST_FLAG) != 0;
            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;
            bddvar f_var = f_const ? 0 : node_var(cf);
            bddvar g_var = g_const ? 0 : node_var(cg);
            bddvar f_level = f_const ? 0 : var2level[f_var];
            bddvar g_level = g_const ? 0 : var2level[g_var];

            bddp child1_f, child1_g;
            if (f_level > g_level) {
                bddp f_lo = node_lo(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                frame.two_children = false;
                child1_f = f_lo; child1_g = cg;
            } else if (g_level > f_level) {
                bddp g_lo = node_lo(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.two_children = false;
                child1_f = cf; child1_g = g_lo;
            } else {
                // Same top variable
                bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.two_children = true;
                frame.child2_f = f_hi; frame.child2_g = g_hi;
                child1_f = f_lo; child1_g = g_lo;
            }
            frame.phase = Phase::GOT_FIRST;

            Frame child;
            child.f = child1_f; child.g = child1_g;
            child.two_children = false;
            child.child2_f = 0; child.child2_g = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_FIRST: {
            if (!frame.two_children) {
                pair_memo[frame.f][frame.g] = result;
                stack.pop_back();
                break;
            }
            frame.first_result = std::move(result);
            frame.phase = Phase::GOT_SECOND;
            Frame child;
            child.f = frame.child2_f; child.g = frame.child2_g;
            child.two_children = false;
            child.child2_f = 0; child.child2_g = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_SECOND: {
            bigint::BigInt total = frame.first_result + result;
            pair_memo[frame.f][frame.g] = total;
            result = std::move(total);
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddlen_iter — Template D (unary, uint64 return)
//
// Returns the maximum element size across the family represented by f.
// Uses BDD_OP_LEN cache. Complement is invariant because ∅ has size 0.
//
// PRECONDITION: Caller holds bdd_gc_guard (no new nodes are created so
// this is a conservative requirement that matches other _iter helpers).
// =====================================================================
uint64_t bddlen_iter(bddp root_f) {
    if (root_f == bddnull) return 0;
    if (root_f == bddempty) return 0;
    if (root_f == bddsingle) return 0;

    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f_raw;
        uint64_t lo_len;
        Phase phase;
    };

    uint64_t result = 0;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f_raw = root_f & ~BDD_COMP_FLAG;
    init.lo_len = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f_raw = frame.f_raw;
            bddp cached = bddrcache(BDD_OP_LEN, f_raw, 0);
            if (cached != bddnull) {
                result = static_cast<uint64_t>(cached);
                stack.pop_back();
                break;
            }
            bddp lo = node_lo(f_raw);
            frame.phase = Phase::GOT_LO;
            if (lo == bddnull || lo == bddempty || lo == bddsingle) {
                result = 0;
                break;
            }
            Frame child;
            child.f_raw = lo & ~BDD_COMP_FLAG;
            child.lo_len = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_len = result;
            bddp hi = node_hi(frame.f_raw);
            frame.phase = Phase::GOT_HI;
            if (hi == bddnull || hi == bddempty || hi == bddsingle) {
                result = 0;
                break;
            }
            Frame child;
            child.f_raw = hi & ~BDD_COMP_FLAG;
            child.lo_len = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            uint64_t hi_len = result;
            // len(f) = max(len(lo), len(hi) + 1), saturated at BDDLEN_MAX.
            if (hi_len >= BDDLEN_MAX_ITER) {
                hi_len = BDDLEN_MAX_ITER;
            } else {
                hi_len += 1;
            }
            uint64_t len = frame.lo_len > hi_len ? frame.lo_len : hi_len;
            bddwcache(BDD_OP_LEN, frame.f_raw, 0, len);
            result = len;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddlit_iter — Template D variant (unary, uint64 return)
//
// Returns the total literal count (= sum over sets of their sizes).
// The recursion formula is
//   lit(f) = lit(f0) + lit(f1) + card(f1)
// so each frame computes both lit and card bottom-up and reuses the
// BDD_OP_LIT / BDD_OP_CARD caches. The complement edge only toggles the
// ∅ membership, which affects card (±1) but not lit. Stored cache values
// are the complement-free "raw" quantities; the complement adjustment is
// applied when a parent consumes a child edge.
//
// PRECONDITION: Caller holds bdd_gc_guard.
// =====================================================================
uint64_t bddlit_iter(bddp root_f) {
    if (root_f == bddnull) return 0;
    if (root_f == bddempty) return 0;
    if (root_f == bddsingle) return 0;

    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f_raw;
        uint64_t lit_lo;
        uint64_t card_lo;
        Phase phase;
    };

    auto adjust_card = [](bddp edge, uint64_t raw_card) -> uint64_t {
        if (edge == bddnull || edge == bddempty) return 0;
        if (edge == bddsingle) return 1;
        if ((edge & BDD_COMP_FLAG) == 0) return raw_card;
        bddp raw = edge & ~BDD_COMP_FLAG;
        if (bddhasempty(raw)) {
            return raw_card > 0 ? raw_card - 1 : 0;
        }
        if (raw_card >= BDDCARD_MAX_ITER) return BDDCARD_MAX_ITER;
        return raw_card + 1;
    };

    // result_lit / result_card carry the RAW values (no complement
    // adjustment) of the most recently completed frame.
    uint64_t result_lit = 0, result_card = 0;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f_raw = root_f & ~BDD_COMP_FLAG;
    init.lit_lo = 0; init.card_lo = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f_raw = frame.f_raw;
            bddp cached_lit = bddrcache(BDD_OP_LIT, f_raw, 0);
            bddp cached_card = bddrcache(BDD_OP_CARD, f_raw, 0);
            if (cached_lit != bddnull && cached_card != bddnull) {
                result_lit = static_cast<uint64_t>(cached_lit);
                result_card = static_cast<uint64_t>(cached_card);
                stack.pop_back();
                break;
            }
            bddp lo = node_lo(f_raw);
            frame.phase = Phase::GOT_LO;
            if (lo == bddnull || lo == bddempty) {
                result_lit = 0; result_card = 0;
                break;
            }
            if (lo == bddsingle) {
                result_lit = 0; result_card = 1;
                break;
            }
            Frame child;
            child.f_raw = lo & ~BDD_COMP_FLAG;
            child.lit_lo = 0; child.card_lo = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            bddp lo = node_lo(frame.f_raw);
            frame.lit_lo = result_lit;
            frame.card_lo = adjust_card(lo, result_card);
            bddp hi = node_hi(frame.f_raw);
            frame.phase = Phase::GOT_HI;
            if (hi == bddnull || hi == bddempty) {
                result_lit = 0; result_card = 0;
                break;
            }
            if (hi == bddsingle) {
                result_lit = 0; result_card = 1;
                break;
            }
            Frame child;
            child.f_raw = hi & ~BDD_COMP_FLAG;
            child.lit_lo = 0; child.card_lo = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi = node_hi(frame.f_raw);
            uint64_t lit_hi = result_lit;
            uint64_t card_hi = adjust_card(hi, result_card);

            uint64_t lit = frame.lit_lo;
            if (lit_hi > BDDLIT_MAX_ITER - lit) lit = BDDLIT_MAX_ITER;
            else lit += lit_hi;
            if (card_hi > BDDLIT_MAX_ITER - lit) lit = BDDLIT_MAX_ITER;
            else lit += card_hi;
            if (lit > BDDLIT_MAX_ITER) lit = BDDLIT_MAX_ITER;

            uint64_t card;
            if (frame.card_lo > BDDCARD_MAX_ITER - card_hi) card = BDDCARD_MAX_ITER;
            else card = frame.card_lo + card_hi;

            bddwcache(BDD_OP_LIT, frame.f_raw, 0, lit);
            bddwcache(BDD_OP_CARD, frame.f_raw, 0, card);
            result_lit = lit;
            result_card = card;
            stack.pop_back();
            break;
        }
        }
    }
    // lit is complement-invariant, so result_lit is the answer for root_f.
    return result_lit;
}

} // namespace kyotodd
