#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <algorithm>
#include <climits>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace kyotodd {

namespace {
// Forward declarations (definitions appear later in this file).
long long costbound_safe_add(long long a, long long b);
long long costbound_safe_sub(long long a, long long b);
}

// =====================================================================
// bddweightsum_iter — Template D + memo.
//
// Mirrors bddweightsum_rec: the sum is complement-invariant (∅ has
// weight 0), so we strip the complement bit up front and key the memo
// on the non-complement node id. count_memo is shared with
// bddexactcount_iter for the per-node hi-subtree cardinality.
//
// PRECONDITION: f is a well-formed bddp; memos are caller-owned.
// =====================================================================
bigint::BigInt bddweightsum_iter(
    bddp f, const std::vector<int>& weights,
    CountMemoMap& sum_memo, CountMemoMap& count_memo)
{
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f_raw;     // cache key (complement stripped in ENTER)
        bddvar var;
        bddp lo, hi;    // raw children (weightsum is complement-invariant)
        bigint::BigInt sum_lo;
        Phase phase;
    };

    bigint::BigInt result(0);
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
                result = bigint::BigInt(0);
                stack.pop_back();
                break;
            }

            bddp f_raw = cf & ~BDD_COMP_FLAG;
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
            frame.lo = lo;
            frame.hi = hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f_raw = lo;
            child.var = 0;
            child.lo = 0; child.hi = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.sum_lo = std::move(result);
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
            bigint::BigInt sum_hi = std::move(result);
            bigint::BigInt count_hi = bddexactcount_iter(frame.hi, count_memo);
            bigint::BigInt r = frame.sum_lo + sum_hi
                             + bigint::BigInt(weights[frame.var]) * count_hi;
            sum_memo[frame.f_raw] = r;
            result = std::move(r);
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddminweight_iter — Template D (unary, long long return).
//
// Memo is keyed on the full bddp (including the complement bit) to
// match bddminweight_rec exactly. Complement is propagated to the lo
// child per ZDD semantics before recursing.
// =====================================================================
long long bddminweight_iter(
    bddp f, const std::vector<int>& weights,
    std::unordered_map<bddp, long long>& memo)
{
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f_in;           // memo key
        bddvar var;
        bddp lo, hi;         // lo has complement absorbed
        long long lo_val;
        Phase phase;
    };

    long long result = 0;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f_in = f;
    init.var = 0;
    init.lo = 0; init.hi = 0;
    init.lo_val = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f_in;
            if (cf == bddempty) { result = LLONG_MAX; stack.pop_back(); break; }
            if (cf == bddsingle) { result = 0; stack.pop_back(); break; }

            auto it = memo.find(cf);
            if (it != memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;
            bddvar var = node_var(f_raw);
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            if (comp) lo = bddnot(lo);

            frame.var = var;
            frame.lo = lo;
            frame.hi = hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f_in = lo;
            child.var = 0;
            child.lo = 0; child.hi = 0;
            child.lo_val = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_val = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f_in = frame.hi;
            child.var = 0;
            child.lo = 0; child.hi = 0;
            child.lo_val = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            long long hi_val = result;
            long long hi_total = costbound_safe_add(
                static_cast<long long>(weights[frame.var]), hi_val);
            long long r = std::min(frame.lo_val, hi_total);
            memo[frame.f_in] = r;
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddmaxweight_iter — Template D (unary, long long return).
//
// Structural twin of bddminweight_iter with max() in place of min() and
// LLONG_MIN as the empty-family sentinel.
// =====================================================================
long long bddmaxweight_iter(
    bddp f, const std::vector<int>& weights,
    std::unordered_map<bddp, long long>& memo)
{
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f_in;
        bddvar var;
        bddp lo, hi;
        long long lo_val;
        Phase phase;
    };

    long long result = 0;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f_in = f;
    init.var = 0;
    init.lo = 0; init.hi = 0;
    init.lo_val = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f_in;
            if (cf == bddempty) { result = LLONG_MIN; stack.pop_back(); break; }
            if (cf == bddsingle) { result = 0; stack.pop_back(); break; }

            auto it = memo.find(cf);
            if (it != memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;
            bddvar var = node_var(f_raw);
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            if (comp) lo = bddnot(lo);

            frame.var = var;
            frame.lo = lo;
            frame.hi = hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f_in = lo;
            child.var = 0;
            child.lo = 0; child.hi = 0;
            child.lo_val = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_val = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f_in = frame.hi;
            child.var = 0;
            child.lo = 0; child.hi = 0;
            child.lo_val = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            long long hi_val = result;
            long long hi_total = costbound_safe_add(
                static_cast<long long>(weights[frame.var]), hi_val);
            long long r = std::max(frame.lo_val, hi_total);
            memo[frame.f_in] = r;
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// bddcostbound_iter — Template B-style (unary + bound, bddp return) with
// interval memo.
//
// Mirrors bddcostbound_rec: each node produces a {h, aw, rb} triple and
// inserts into the shared interval memo. The bound b threads through
// the hi-recursion as b - weights[var].
//
// PRECONDITION: must be invoked under a bdd_gc_guard scope. The public
// wrapper handles validation, memo binding, and terminal fast-paths;
// this helper assumes f is non-terminal.
// =====================================================================
namespace {

long long costbound_safe_add(long long a, long long b) {
    if (a == LLONG_MIN || b == LLONG_MIN) return LLONG_MIN;
    if (a == LLONG_MAX || b == LLONG_MAX) return LLONG_MAX;
    if (b > 0 && a > LLONG_MAX - b) return LLONG_MAX;
    if (b < 0 && a < LLONG_MIN - b) return LLONG_MIN;
    return a + b;
}

long long costbound_safe_sub(long long a, long long b) {
    if (a == LLONG_MIN) return LLONG_MIN;
    if (a == LLONG_MAX) return LLONG_MAX;
    if (b == LLONG_MIN) return LLONG_MAX;
    if (b == LLONG_MAX) return LLONG_MIN;
    if (b > 0 && a < LLONG_MIN + b) return LLONG_MIN;
    if (b < 0 && a > LLONG_MAX + b) return LLONG_MAX;
    return a - b;
}

} // namespace

bddp bddcostbound_iter(bddp f, const std::vector<int>& weights, long long b,
                        CostBoundMemo& memo)
{
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f_in;          // memo key (preserves complement)
        long long b;
        bddvar var;
        bddp lo, hi;        // lo has complement absorbed
        long long cx;
        bddp r0_h;          // lo result (saved at GOT_LO)
        long long r0_aw;
        long long r0_rb;
        Phase phase;
    };

    // Top-level return values for the innermost result (multi-valued).
    bddp result_h = bddempty;
    long long result_aw = LLONG_MIN;
    long long result_rb = LLONG_MAX;

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f_in = f;
    init.b = b;
    init.var = 0;
    init.lo = 0; init.hi = 0;
    init.cx = 0;
    init.r0_h = bddempty;
    init.r0_aw = LLONG_MIN;
    init.r0_rb = LLONG_MAX;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f_in;
            long long cb = frame.b;

            // Terminal cases mirror bddcostbound_rec.
            if (cf == bddempty) {
                result_h = bddempty;
                result_aw = LLONG_MIN;
                result_rb = LLONG_MAX;
                stack.pop_back();
                break;
            }
            if (cf == bddsingle) {
                if (cb >= 0) {
                    result_h = bddsingle;
                    result_aw = 0;
                    result_rb = LLONG_MAX;
                } else {
                    result_h = bddempty;
                    result_aw = LLONG_MIN;
                    result_rb = 0;
                }
                stack.pop_back();
                break;
            }

            // Interval-memo lookup.
            bddp cached_h;
            long long cached_aw, cached_rb;
            if (memo.lookup(cf, cb, cached_h, cached_aw, cached_rb)) {
                result_h = cached_h;
                result_aw = cached_aw;
                result_rb = cached_rb;
                stack.pop_back();
                break;
            }

            // Decompose f_in = <var, lo, hi> with ZDD complement handling.
            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;
            bddvar var = node_var(f_raw);
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            if (comp) lo = bddnot(lo);

            frame.var = var;
            frame.lo = lo;
            frame.hi = hi;
            frame.cx = static_cast<long long>(weights[var]);
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f_in = lo;
            child.b = cb;
            child.var = 0;
            child.lo = 0; child.hi = 0;
            child.cx = 0;
            child.r0_h = bddempty;
            child.r0_aw = LLONG_MIN;
            child.r0_rb = LLONG_MAX;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.r0_h = result_h;
            frame.r0_aw = result_aw;
            frame.r0_rb = result_rb;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f_in = frame.hi;
            child.b = costbound_safe_sub(frame.b, frame.cx);
            child.var = 0;
            child.lo = 0; child.hi = 0;
            child.cx = 0;
            child.r0_h = bddempty;
            child.r0_aw = LLONG_MIN;
            child.r0_rb = LLONG_MAX;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp r1_h = result_h;
            long long r1_aw = result_aw;
            long long r1_rb = result_rb;

            bddp h = ZDD::getnode_raw(frame.var, frame.r0_h, r1_h);
            long long aw =
                std::max(frame.r0_aw, costbound_safe_add(r1_aw, frame.cx));
            long long rb =
                std::min(frame.r0_rb, costbound_safe_add(r1_rb, frame.cx));

            memo.insert(frame.f_in, aw, rb, h);

            result_h = h;
            result_aw = aw;
            result_rb = rb;
            stack.pop_back();
            break;
        }
        }
    }
    return result_h;
}

} // namespace kyotodd
