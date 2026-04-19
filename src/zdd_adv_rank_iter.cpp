#include "bdd.h"
#include "bdd_internal.h"
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace kyotodd {

namespace {

struct BddpIdxPairHash {
    std::size_t operator()(const std::pair<bddp, std::size_t>& p) const {
        std::size_t h = std::hash<bddp>()(p.first);
        h ^= std::hash<std::size_t>()(p.second)
             + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};
typedef std::unordered_map<std::pair<bddp, std::size_t>, bddp,
                           BddpIdxPairHash>
    BddpIdxMemo;

} // namespace

// =====================================================================
// bddsupersets_of_iter — explicit-stack rewrite of bddsupersets_of_rec.
//
// The recursion in zdd_adv_rank.cpp:
//   - terminal: f == empty → empty; idx == |s| → f; f == single → empty
//   - s_level > f_level → empty (s contains a var zero-suppressed here)
//   - s_level == f_level → node(f_var, empty, rec(f_hi, idx+1))
//   - s_level <  f_level → node(f_var, rec(f_lo, idx), rec(f_hi, idx))
//
// PRECONDITION: Must be invoked under a bdd_gc_guard scope. Intermediate
// bddp values on the iteration stack and in the local memo are not
// registered as GC roots, so GC must be deferred for the duration of this
// call. The public wrapper bddsupersets_of() satisfies this precondition.
// The caller must supply the already-sorted variable vector (descending by
// level, deduplicated) that the public wrapper prepares.
// =====================================================================
bddp bddsupersets_of_iter(bddp f, const std::vector<bddvar>& sorted_s) {
    enum class Phase : uint8_t { ENTER, GOT_HI_ONLY, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        std::size_t idx;
        bddvar top_var;
        bddp f_lo, f_hi;
        bddp lo_result;
        Phase phase;
    };

    BddpIdxMemo memo;
    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f; init.idx = 0;
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
            if (cf == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (frame.idx == sorted_s.size()) {
                result = cf;
                stack.pop_back();
                break;
            }
            if (cf == bddsingle) { result = bddempty; stack.pop_back(); break; }

            auto key = std::make_pair(cf, frame.idx);
            auto it = memo.find(key);
            if (it != memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;

            bddvar f_var = node_var(f_raw);
            bddvar f_level = var2level[f_var];
            bddvar s_var = sorted_s[frame.idx];
            bddvar s_level = var2level[s_var];

            if (s_level > f_level) {
                memo[key] = bddempty;
                result = bddempty;
                stack.pop_back();
                break;
            }

            bddp f_lo = node_lo(f_raw);
            bddp f_hi = node_hi(f_raw);
            if (comp) f_lo = bddnot(f_lo);

            frame.top_var = f_var;
            frame.f_lo = f_lo;
            frame.f_hi = f_hi;

            if (s_level == f_level) {
                frame.phase = Phase::GOT_HI_ONLY;
                Frame child;
                child.f = f_hi; child.idx = frame.idx + 1;
                child.top_var = 0;
                child.f_lo = 0; child.f_hi = 0;
                child.lo_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else {
                frame.phase = Phase::GOT_LO;
                Frame child;
                child.f = f_lo; child.idx = frame.idx;
                child.top_var = 0;
                child.f_lo = 0; child.f_hi = 0;
                child.lo_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            }
            break;
        }

        case Phase::GOT_HI_ONLY: {
            bddp combined = ZDD::getnode_raw(frame.top_var, bddempty, result);
            memo[std::make_pair(frame.f, frame.idx)] = combined;
            result = combined;
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f = frame.f_hi; child.idx = frame.idx;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp combined =
                ZDD::getnode_raw(frame.top_var, frame.lo_result, result);
            memo[std::make_pair(frame.f, frame.idx)] = combined;
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddsubsets_of_iter — explicit-stack rewrite of bddsubsets_of_rec.
//
// The recursion:
//   - terminal: f == empty → empty; f == single → single
//   - idx == |s| → hasempty(f) ? single : empty
//   - s_level > f_level → rec(f, idx+1)      (tail-call: drop s[idx])
//   - s_level == f_level → node(f_var, rec(f_lo, idx+1), rec(f_hi, idx+1))
//   - s_level <  f_level → rec(f_lo, idx)    (tail-call: f_hi not allowed)
//
// The two tail-call branches are implemented by rewriting the current
// frame in place and restarting ENTER, so the stack never grows for
// tail recursion.
//
// PRECONDITION: See bddsupersets_of_iter.
// =====================================================================
bddp bddsubsets_of_iter(bddp f, const std::vector<bddvar>& sorted_s) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        std::size_t idx;
        bddp key_f;          // memo key is (key_f, key_idx) — captured before
        std::size_t key_idx; // tail rewrites; only used for two-child case
        bddvar top_var;
        bddp f_lo, f_hi;
        bddp lo_result;
        Phase phase;
    };

    BddpIdxMemo memo;
    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f; init.idx = 0;
    init.key_f = 0; init.key_idx = 0;
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
            if (cf == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cf == bddsingle) { result = bddsingle; stack.pop_back(); break; }
            if (frame.idx == sorted_s.size()) {
                result = bddhasempty(cf) ? bddsingle : bddempty;
                stack.pop_back();
                break;
            }

            auto key = std::make_pair(cf, frame.idx);
            auto it = memo.find(key);
            if (it != memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;

            bddvar f_var = node_var(f_raw);
            bddvar f_level = var2level[f_var];
            bddvar s_var = sorted_s[frame.idx];
            bddvar s_level = var2level[s_var];

            bddp f_lo = node_lo(f_raw);
            bddp f_hi = node_hi(f_raw);
            if (comp) f_lo = bddnot(f_lo);

            if (s_level > f_level) {
                // Tail-call rec(f, idx+1): rewrite frame, re-enter.
                frame.idx = frame.idx + 1;
                // phase stays ENTER — loop will re-dispatch this frame.
                break;
            }

            if (s_level < f_level) {
                // Tail-call rec(f_lo, idx).
                frame.f = f_lo;
                // idx unchanged, phase stays ENTER.
                break;
            }

            // s_level == f_level: both children, store memo key for later.
            frame.key_f = cf;
            frame.key_idx = frame.idx;
            frame.top_var = f_var;
            frame.f_lo = f_lo;
            frame.f_hi = f_hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = f_lo; child.idx = frame.idx + 1;
            child.key_f = 0; child.key_idx = 0;
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
            child.f = frame.f_hi; child.idx = frame.key_idx + 1;
            child.key_f = 0; child.key_idx = 0;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp combined =
                ZDD::getnode_raw(frame.top_var, frame.lo_result, result);
            memo[std::make_pair(frame.key_f, frame.key_idx)] = combined;
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddproject_iter — explicit-stack rewrite of bddproject_rec.
//
// The recursion always descends to both children; the difference between
// the two f_var cases is only in how to combine the child results:
//   - f_var ∈ proj_vars: result = union(lo_proj, hi_proj)
//   - otherwise         : result = node(f_var, lo_proj, hi_proj)
//
// PRECONDITION: See bddsupersets_of_iter. Additionally invokes
// bddunion_iter when f_var is projected, so the same GC-guard requirement
// on bddunion_iter applies.
// =====================================================================
bddp bddproject_iter(bddp f, const std::vector<bddvar>& vars) {
    std::unordered_set<bddvar> proj_vars(vars.begin(), vars.end());
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddvar top_var;
        bool projected;
        bddp f_lo, f_hi;
        bddp lo_result;
        Phase phase;
    };

    std::unordered_map<bddp, bddp> memo;
    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.top_var = 0; init.projected = false;
    init.f_lo = 0; init.f_hi = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cf == bddsingle) { result = bddsingle; stack.pop_back(); break; }

            auto it = memo.find(cf);
            if (it != memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;

            bddvar f_var = node_var(f_raw);
            bddp f_lo = node_lo(f_raw);
            bddp f_hi = node_hi(f_raw);
            if (comp) f_lo = bddnot(f_lo);

            frame.top_var = f_var;
            frame.projected = (proj_vars.count(f_var) != 0);
            frame.f_lo = f_lo;
            frame.f_hi = f_hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = f_lo;
            child.top_var = 0; child.projected = false;
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
            child.f = frame.f_hi;
            child.top_var = 0; child.projected = false;
            child.f_lo = 0; child.f_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp combined;
            if (frame.projected) {
                combined = bddunion_iter(frame.lo_result, result);
            } else {
                combined =
                    ZDD::getnode_raw(frame.top_var, frame.lo_result, result);
            }
            memo[frame.f] = combined;
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

} // namespace kyotodd
