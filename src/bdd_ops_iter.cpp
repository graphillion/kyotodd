#include "bdd.h"
#include "bdd_internal.h"
#include <vector>

namespace kyotodd {


namespace {

enum class IterPhase : uint8_t {
    ENTER,    // Check terminals/cache, decompose, push lo subproblem
    GOT_LO,  // lo result ready, push hi subproblem
    GOT_HI   // Both results ready, combine and return
};

struct IterFrame {
    bddp f;           // Operand f (after normalization)
    bddp g;           // Operand g (after normalization)
    bddvar top_var;   // Variable for getnode_raw
    bddp f_hi;        // hi cofactor of f (saved for GOT_LO phase)
    bddp g_hi;        // hi cofactor of g (saved for GOT_LO phase)
    bddp lo_result;   // Result of lo subproblem
    IterPhase phase;   // Current phase
};

} // anonymous namespace

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. Intermediate bddp
// values on the iteration stack are not registered as GC roots, so GC must be
// deferred (bdd_gc_depth > 0) for the duration of this call. The public wrapper
// bddand() in bdd_ops.cpp satisfies this precondition.
bddp bddand_iter(bddp f, bddp g) {
    bddp result = bddnull;

    std::vector<IterFrame> stack;
    stack.reserve(64);

    // Push the initial problem
    IterFrame init;
    init.f = f;
    init.g = g;
    init.top_var = 0;
    init.f_hi = 0;
    init.g_hi = 0;
    init.lo_result = bddnull;
    init.phase = IterPhase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        IterFrame& frame = stack.back();

        switch (frame.phase) {
        case IterPhase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;

            // Terminal cases
            if (cf == bddfalse || cg == bddfalse) {
                result = bddfalse; stack.pop_back(); break;
            }
            if (cf == bddtrue) {
                result = cg; stack.pop_back(); break;
            }
            if (cg == bddtrue) {
                result = cf; stack.pop_back(); break;
            }
            if (cf == cg) {
                result = cf; stack.pop_back(); break;
            }
            if (cf == bddnot(cg)) {
                result = bddfalse; stack.pop_back(); break;
            }

            // Normalize operand order (AND is commutative)
            if (cf > cg) {
                bddp tmp = cf; cf = cg; cg = tmp;
                frame.f = cf;
                frame.g = cg;
            }

            // Cache lookup
            bddp cached = bddrcache(BDD_OP_AND, cf, cg);
            if (cached != bddnull) {
                result = cached; stack.pop_back(); break;
            }

            // Cofactoring
            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;

            bddvar f_var = node_var(cf);
            bddvar g_var = node_var(cg);
            bddvar f_level = var2level[f_var];
            bddvar g_level = var2level[g_var];

            bddp f_lo, f_hi, g_lo, g_hi;

            if (f_level > g_level) {
                frame.top_var = f_var;
                f_lo = node_lo(cf);
                f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
                g_lo = cg;
                g_hi = cg;
            } else if (g_level > f_level) {
                frame.top_var = g_var;
                f_lo = cf;
                f_hi = cf;
                g_lo = node_lo(cg);
                g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
            } else {
                frame.top_var = f_var;
                f_lo = node_lo(cf);
                f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
                g_lo = node_lo(cg);
                g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
            }

            // Save hi cofactors for GOT_LO phase
            frame.f_hi = f_hi;
            frame.g_hi = g_hi;

            // Transition to GOT_LO (will process lo result on return)
            frame.phase = IterPhase::GOT_LO;

            // Push lo subproblem
            IterFrame lo_frame;
            lo_frame.f = f_lo;
            lo_frame.g = g_lo;
            lo_frame.top_var = 0;
            lo_frame.f_hi = 0;
            lo_frame.g_hi = 0;
            lo_frame.lo_result = bddnull;
            lo_frame.phase = IterPhase::ENTER;
            stack.push_back(lo_frame);
            // frame reference is now invalid after push_back
            break;
        }

        case IterPhase::GOT_LO: {
            // 'result' holds the lo subproblem result
            frame.lo_result = result;

            // Transition to GOT_HI
            frame.phase = IterPhase::GOT_HI;

            // Push hi subproblem
            IterFrame hi_frame;
            hi_frame.f = frame.f_hi;
            hi_frame.g = frame.g_hi;
            hi_frame.top_var = 0;
            hi_frame.f_hi = 0;
            hi_frame.g_hi = 0;
            hi_frame.lo_result = bddnull;
            hi_frame.phase = IterPhase::ENTER;
            stack.push_back(hi_frame);
            // frame reference is now invalid after push_back
            break;
        }

        case IterPhase::GOT_HI: {
            // 'result' holds the hi subproblem result
            bddp hi_result = result;

            // Combine lo and hi results
            result = BDD::getnode_raw(frame.top_var, frame.lo_result, hi_result);

            // Cache insert
            bddwcache(BDD_OP_AND, frame.f, frame.g, result);

            stack.pop_back();
            break;
        }
        } // switch
    } // while

    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. Intermediate bddp
// values on the iteration stack are not registered as GC roots, so GC must be
// deferred (bdd_gc_depth > 0) for the duration of this call. The public wrapper
// bddxor() in bdd_ops.cpp satisfies this precondition.
bddp bddxor_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;            // Post-normalization operand (non-complemented, used as cache key)
        bddp g;            // Post-normalization operand (non-complemented, used as cache key)
        bddvar top_var;
        bddp f_hi;
        bddp g_hi;
        bddp lo_result;
        bool comp;         // Apply bddnot() to the final result if true
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.g = g;
    init.top_var = 0;
    init.f_hi = 0;
    init.g_hi = 0;
    init.lo_result = bddnull;
    init.comp = false;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();

        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;

            // Terminal cases
            if (cf == bddfalse) { result = cg; stack.pop_back(); break; }
            if (cf == bddtrue) { result = bddnot(cg); stack.pop_back(); break; }
            if (cg == bddfalse) { result = cf; stack.pop_back(); break; }
            if (cg == bddtrue) { result = bddnot(cf); stack.pop_back(); break; }
            if (cf == cg) { result = bddfalse; stack.pop_back(); break; }
            if (cf == bddnot(cg)) { result = bddtrue; stack.pop_back(); break; }

            // Strip complement edges and track parity
            // XOR(~a, b) = ~XOR(a, b), XOR(a, ~b) = ~XOR(a, b)
            bool comp = false;
            if (cf & BDD_COMP_FLAG) { cf ^= BDD_COMP_FLAG; comp = !comp; }
            if (cg & BDD_COMP_FLAG) { cg ^= BDD_COMP_FLAG; comp = !comp; }

            // Normalize operand order (XOR is commutative)
            if (cf > cg) { bddp tmp = cf; cf = cg; cg = tmp; }

            frame.f = cf;
            frame.g = cg;
            frame.comp = comp;

            // Cache lookup
            bddp cached = bddrcache(BDD_OP_XOR, cf, cg);
            if (cached != bddnull) {
                result = comp ? bddnot(cached) : cached;
                stack.pop_back();
                break;
            }

            // Cofactoring (both operands are non-complemented, non-terminal)
            bddvar f_var = node_var(cf);
            bddvar g_var = node_var(cg);
            bddvar f_level = var2level[f_var];
            bddvar g_level = var2level[g_var];

            bddp f_lo, f_hi, g_lo, g_hi;
            if (f_level > g_level) {
                frame.top_var = f_var;
                f_lo = node_lo(cf);
                f_hi = node_hi(cf);
                g_lo = cg;
                g_hi = cg;
            } else if (g_level > f_level) {
                frame.top_var = g_var;
                f_lo = cf;
                f_hi = cf;
                g_lo = node_lo(cg);
                g_hi = node_hi(cg);
            } else {
                frame.top_var = f_var;
                f_lo = node_lo(cf);
                f_hi = node_hi(cf);
                g_lo = node_lo(cg);
                g_hi = node_hi(cg);
            }

            frame.f_hi = f_hi;
            frame.g_hi = g_hi;
            frame.phase = Phase::GOT_LO;

            Frame lo_frame;
            lo_frame.f = f_lo;
            lo_frame.g = g_lo;
            lo_frame.top_var = 0;
            lo_frame.f_hi = 0;
            lo_frame.g_hi = 0;
            lo_frame.lo_result = bddnull;
            lo_frame.comp = false;
            lo_frame.phase = Phase::ENTER;
            stack.push_back(lo_frame);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;

            Frame hi_frame;
            hi_frame.f = frame.f_hi;
            hi_frame.g = frame.g_hi;
            hi_frame.top_var = 0;
            hi_frame.f_hi = 0;
            hi_frame.g_hi = 0;
            hi_frame.lo_result = bddnull;
            hi_frame.comp = false;
            hi_frame.phase = Phase::ENTER;
            stack.push_back(hi_frame);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi_result = result;
            bddp combined = BDD::getnode_raw(frame.top_var, frame.lo_result, hi_result);
            // Cache insert stores the result for normalized (non-comp) operands
            bddwcache(BDD_OP_XOR, frame.f, frame.g, combined);
            result = frame.comp ? bddnot(combined) : combined;
            stack.pop_back();
            break;
        }
        } // switch
    } // while

    return result;
}

} // namespace kyotodd
