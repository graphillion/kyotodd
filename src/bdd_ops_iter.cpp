#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <cmath>
#include <unordered_map>
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

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. Intermediate bddp
// values on the iteration stack are not registered as GC roots, so GC must be
// deferred (bdd_gc_depth > 0) for the duration of this call. The public wrapper
// bddat0() in bdd_ops.cpp satisfies this precondition.
bddp bddat0_iter(bddp f, bddvar v) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddvar top_var;
        bddp f_hi;
        bddp lo_result;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.top_var = 0;
    init.f_hi = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddvar v_level = var2level[v];

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf & BDD_CONST_FLAG) { result = cf; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddvar f_level = var2level[f_var];

            if (f_level < v_level) { result = cf; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_AT0, cf, static_cast<bddp>(v));
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            if (f_var == v) {
                bddp r = node_lo(cf);
                if (f_comp) r = bddnot(r);
                bddwcache(BDD_OP_AT0, cf, static_cast<bddp>(v), r);
                result = r; stack.pop_back(); break;
            }

            // f_level > v_level: recurse into both branches
            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }

            frame.top_var = f_var;
            frame.f_hi = f_hi;
            frame.phase = Phase::GOT_LO;

            Frame lo_frame;
            lo_frame.f = f_lo;
            lo_frame.top_var = 0;
            lo_frame.f_hi = 0;
            lo_frame.lo_result = bddnull;
            lo_frame.phase = Phase::ENTER;
            stack.push_back(lo_frame);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;

            Frame hi_frame;
            hi_frame.f = frame.f_hi;
            hi_frame.top_var = 0;
            hi_frame.f_hi = 0;
            hi_frame.lo_result = bddnull;
            hi_frame.phase = Phase::ENTER;
            stack.push_back(hi_frame);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi_result = result;
            bddp combined = BDD::getnode_raw(frame.top_var, frame.lo_result, hi_result);
            bddwcache(BDD_OP_AT0, frame.f, static_cast<bddp>(v), combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddat0_iter.
bddp bddat1_iter(bddp f, bddvar v) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddvar top_var;
        bddp f_hi;
        bddp lo_result;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.top_var = 0;
    init.f_hi = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddvar v_level = var2level[v];

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf & BDD_CONST_FLAG) { result = cf; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddvar f_level = var2level[f_var];

            if (f_level < v_level) { result = cf; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_AT1, cf, static_cast<bddp>(v));
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            if (f_var == v) {
                bddp r = node_hi(cf);
                if (f_comp) r = bddnot(r);
                bddwcache(BDD_OP_AT1, cf, static_cast<bddp>(v), r);
                result = r; stack.pop_back(); break;
            }

            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }

            frame.top_var = f_var;
            frame.f_hi = f_hi;
            frame.phase = Phase::GOT_LO;

            Frame lo_frame;
            lo_frame.f = f_lo;
            lo_frame.top_var = 0;
            lo_frame.f_hi = 0;
            lo_frame.lo_result = bddnull;
            lo_frame.phase = Phase::ENTER;
            stack.push_back(lo_frame);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;

            Frame hi_frame;
            hi_frame.f = frame.f_hi;
            hi_frame.top_var = 0;
            hi_frame.f_hi = 0;
            hi_frame.lo_result = bddnull;
            hi_frame.phase = Phase::ENTER;
            stack.push_back(hi_frame);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi_result = result;
            bddp combined = BDD::getnode_raw(frame.top_var, frame.lo_result, hi_result);
            bddwcache(BDD_OP_AT1, frame.f, static_cast<bddp>(v), combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddat0_iter.
// g is a cube BDD (variable set). The same-variable case combines children
// via OR, delegated through bddand_iter to stay stack-safe.
bddp bddexist_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_PASS, GOT_LO, GOT_HI };
    enum class Combine : uint8_t { MakeNode, ReduceOr };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bddp f_hi;
        bddp g_hi;
        bddp lo_result;
        Combine combine;
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
    init.combine = Combine::MakeNode;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g & ~BDD_COMP_FLAG;
            frame.g = cg;

            if (cg == bddfalse) { result = cf; stack.pop_back(); break; }
            if (cf == bddfalse) { result = bddfalse; stack.pop_back(); break; }
            if (cf == bddtrue) { result = bddtrue; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_EXIST, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddvar f_level = var2level[f_var];
            bddvar g_var = node_var(cg);
            bddvar g_level = var2level[g_var];

            if (g_level > f_level) {
                // Skip g's top variable: recurse on (f, node_lo(g))
                frame.phase = Phase::GOT_PASS;
                Frame child;
                child.f = cf;
                child.g = node_lo(cg);
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.combine = Combine::MakeNode;
                child.phase = Phase::ENTER;
                stack.push_back(child);
                break;
            }

            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }

            if (f_level > g_level) {
                // Keep f's top variable: combine via make-node
                frame.top_var = f_var;
                frame.f_hi = f_hi;
                frame.g_hi = cg;
                frame.combine = Combine::MakeNode;
                frame.phase = Phase::GOT_LO;
                Frame child;
                child.f = f_lo;
                child.g = cg;
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.combine = Combine::MakeNode;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else {
                // Same variable: quantify out via OR
                bddp g_rest = node_lo(cg);
                frame.top_var = 0;
                frame.f_hi = f_hi;
                frame.g_hi = g_rest;
                frame.combine = Combine::ReduceOr;
                frame.phase = Phase::GOT_LO;
                Frame child;
                child.f = f_lo;
                child.g = g_rest;
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.combine = Combine::MakeNode;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            }
            break;
        }

        case Phase::GOT_PASS: {
            bddwcache(BDD_OP_EXIST, frame.f, frame.g, result);
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f = frame.f_hi;
            child.g = frame.g_hi;
            child.top_var = 0;
            child.f_hi = 0;
            child.g_hi = 0;
            child.lo_result = bddnull;
            child.combine = Combine::MakeNode;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi_r = result;
            bddp lo_r = frame.lo_result;
            bddp combined;
            if (frame.combine == Combine::MakeNode) {
                combined = BDD::getnode_raw(frame.top_var, lo_r, hi_r);
            } else {
                combined = bddnot(bddand_iter(bddnot(lo_r), bddnot(hi_r)));
            }
            bddwcache(BDD_OP_EXIST, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddat0_iter.
// Same structure as bddexist_iter but combines via AND at the match case.
bddp bdduniv_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_PASS, GOT_LO, GOT_HI };
    enum class Combine : uint8_t { MakeNode, ReduceAnd };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bddp f_hi;
        bddp g_hi;
        bddp lo_result;
        Combine combine;
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
    init.combine = Combine::MakeNode;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g & ~BDD_COMP_FLAG;
            frame.g = cg;

            if (cg == bddfalse) { result = cf; stack.pop_back(); break; }
            if (cf == bddfalse) { result = bddfalse; stack.pop_back(); break; }
            if (cf == bddtrue) { result = bddtrue; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_UNIV, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddvar f_level = var2level[f_var];
            bddvar g_var = node_var(cg);
            bddvar g_level = var2level[g_var];

            if (g_level > f_level) {
                frame.phase = Phase::GOT_PASS;
                Frame child;
                child.f = cf;
                child.g = node_lo(cg);
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.combine = Combine::MakeNode;
                child.phase = Phase::ENTER;
                stack.push_back(child);
                break;
            }

            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }

            if (f_level > g_level) {
                frame.top_var = f_var;
                frame.f_hi = f_hi;
                frame.g_hi = cg;
                frame.combine = Combine::MakeNode;
                frame.phase = Phase::GOT_LO;
                Frame child;
                child.f = f_lo;
                child.g = cg;
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.combine = Combine::MakeNode;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else {
                bddp g_rest = node_lo(cg);
                frame.top_var = 0;
                frame.f_hi = f_hi;
                frame.g_hi = g_rest;
                frame.combine = Combine::ReduceAnd;
                frame.phase = Phase::GOT_LO;
                Frame child;
                child.f = f_lo;
                child.g = g_rest;
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.combine = Combine::MakeNode;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            }
            break;
        }

        case Phase::GOT_PASS: {
            bddwcache(BDD_OP_UNIV, frame.f, frame.g, result);
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f = frame.f_hi;
            child.g = frame.g_hi;
            child.top_var = 0;
            child.f_hi = 0;
            child.g_hi = 0;
            child.lo_result = bddnull;
            child.combine = Combine::MakeNode;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi_r = result;
            bddp lo_r = frame.lo_result;
            bddp combined;
            if (frame.combine == Combine::MakeNode) {
                combined = BDD::getnode_raw(frame.top_var, lo_r, hi_r);
            } else {
                combined = bddand_iter(lo_r, hi_r);
            }
            bddwcache(BDD_OP_UNIV, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddat0_iter.
// Generalized cofactor. Terminal cases match bddcofactor; decomposition
// choose single-recurse when one of g's children is bddfalse (don't-care),
// else two recursions combined via make-node.
bddp bddcofactor_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_PASS, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bddp f_hi;
        bddp g_hi;
        bddp lo_result;
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
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;

            if (cf & BDD_CONST_FLAG) { result = cf; stack.pop_back(); break; }
            if (cg == bddfalse) { result = bddfalse; stack.pop_back(); break; }
            if (cf == bddnot(cg)) { result = bddfalse; stack.pop_back(); break; }
            if (cf == cg) { result = bddtrue; stack.pop_back(); break; }
            if (cg == bddtrue) { result = cf; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_COFACTOR, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddvar g_var = node_var(cg);
            bddvar f_level = var2level[f_var];
            bddvar g_level = var2level[g_var];

            bddvar top_var;
            bddp f_lo, f_hi, g_lo, g_hi;
            if (f_level > g_level) {
                top_var = f_var;
                f_lo = node_lo(cf); f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
                g_lo = cg; g_hi = cg;
            } else if (g_level > f_level) {
                top_var = g_var;
                f_lo = cf; f_hi = cf;
                g_lo = node_lo(cg); g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
            } else {
                top_var = f_var;
                f_lo = node_lo(cf); f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
                g_lo = node_lo(cg); g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
            }

            if (g_lo == bddfalse && g_hi != bddfalse) {
                frame.phase = Phase::GOT_PASS;
                Frame child;
                child.f = f_hi;
                child.g = g_hi;
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else if (g_hi == bddfalse && g_lo != bddfalse) {
                frame.phase = Phase::GOT_PASS;
                Frame child;
                child.f = f_lo;
                child.g = g_lo;
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else {
                frame.top_var = top_var;
                frame.f_hi = f_hi;
                frame.g_hi = g_hi;
                frame.phase = Phase::GOT_LO;
                Frame child;
                child.f = f_lo;
                child.g = g_lo;
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            }
            break;
        }

        case Phase::GOT_PASS: {
            bddwcache(BDD_OP_COFACTOR, frame.f, frame.g, result);
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f = frame.f_hi;
            child.g = frame.g_hi;
            child.top_var = 0;
            child.f_hi = 0;
            child.g_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi_r = result;
            bddp combined = BDD::getnode_raw(frame.top_var, frame.lo_result, hi_r);
            bddwcache(BDD_OP_COFACTOR, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddat0_iter.
// When f's top variable equals v, computes f|_{v=0} OR f|_{v=1} via
// ~(~f_lo AND ~f_hi). The inner AND is delegated to bddand_iter so that deep
// operands do not overflow the C++ call stack.
bddp bddsmooth_iter(bddp f, bddvar v) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddvar top_var;
        bddp f_hi;
        bddp lo_result;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.top_var = 0;
    init.f_hi = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddvar v_level = var2level[v];

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf & BDD_CONST_FLAG) { result = cf; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddvar f_level = var2level[f_var];

            if (f_level < v_level) { result = cf; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_SMOOTH, cf, static_cast<bddp>(v));
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }

            if (f_var == v) {
                // Quantify: f_lo OR f_hi, via ~(~f_lo AND ~f_hi).
                bddp r = bddnot(bddand_iter(bddnot(f_lo), bddnot(f_hi)));
                bddwcache(BDD_OP_SMOOTH, cf, static_cast<bddp>(v), r);
                result = r; stack.pop_back(); break;
            }

            frame.top_var = f_var;
            frame.f_hi = f_hi;
            frame.phase = Phase::GOT_LO;

            Frame lo_frame;
            lo_frame.f = f_lo;
            lo_frame.top_var = 0;
            lo_frame.f_hi = 0;
            lo_frame.lo_result = bddnull;
            lo_frame.phase = Phase::ENTER;
            stack.push_back(lo_frame);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;

            Frame hi_frame;
            hi_frame.f = frame.f_hi;
            hi_frame.top_var = 0;
            hi_frame.f_hi = 0;
            hi_frame.lo_result = bddnull;
            hi_frame.phase = Phase::ENTER;
            stack.push_back(hi_frame);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi_result = result;
            bddp combined = BDD::getnode_raw(frame.top_var, frame.lo_result, hi_result);
            bddwcache(BDD_OP_SMOOTH, frame.f, static_cast<bddp>(v), combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddand_iter.
// Each ENTER decomposes f into 4 sub-problems (lo/hi x k and k-1). The OR
// combining the results is delegated to bddand_iter via De Morgan.
bddp bddspread_iter(bddp f, int k) {
    enum class Phase : uint8_t { ENTER, GOT_A, GOT_B, GOT_C, GOT_D };
    struct Frame {
        bddp f;           // the BDD at this frame (cache key)
        int k;            // current k
        bddvar top_var;
        bddp f_lo;
        bddp f_hi;
        bddp r_a;         // spread(f_lo, k)
        bddp r_b;         // spread(f_hi, k)
        bddp r_c;         // spread(f_lo, k-1)
        // r_d computed from result
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.k = k;
    init.top_var = 0;
    init.f_lo = 0;
    init.f_hi = 0;
    init.r_a = 0;
    init.r_b = 0;
    init.r_c = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf & BDD_CONST_FLAG) { result = cf; stack.pop_back(); break; }
            if (frame.k == 0) { result = cf; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_SPREAD, cf, static_cast<bddp>(frame.k));
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar t = node_var(cf);
            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }

            frame.top_var = t;
            frame.f_lo = f_lo;
            frame.f_hi = f_hi;
            frame.phase = Phase::GOT_A;

            Frame child;
            child.f = f_lo;
            child.k = frame.k;
            child.top_var = 0;
            child.f_lo = 0;
            child.f_hi = 0;
            child.r_a = 0;
            child.r_b = 0;
            child.r_c = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_A: {
            frame.r_a = result;
            frame.phase = Phase::GOT_B;
            Frame child;
            child.f = frame.f_hi;
            child.k = frame.k;
            child.top_var = 0;
            child.f_lo = 0;
            child.f_hi = 0;
            child.r_a = 0;
            child.r_b = 0;
            child.r_c = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_B: {
            frame.r_b = result;
            frame.phase = Phase::GOT_C;
            Frame child;
            child.f = frame.f_lo;
            child.k = frame.k - 1;
            child.top_var = 0;
            child.f_lo = 0;
            child.f_hi = 0;
            child.r_a = 0;
            child.r_b = 0;
            child.r_c = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_C: {
            frame.r_c = result;
            frame.phase = Phase::GOT_D;
            Frame child;
            child.f = frame.f_hi;
            child.k = frame.k - 1;
            child.top_var = 0;
            child.f_lo = 0;
            child.f_hi = 0;
            child.r_a = 0;
            child.r_b = 0;
            child.r_c = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_D: {
            bddp r_d = result;
            // lo = spread(f_lo, k) | spread(f_hi, k-1)
            //    = ~(~r_a AND ~r_d)
            bddp lo = bddnot(bddand_iter(bddnot(frame.r_a), bddnot(r_d)));
            // hi = spread(f_hi, k) | spread(f_lo, k-1)
            //    = ~(~r_b AND ~r_c)
            bddp hi = bddnot(bddand_iter(bddnot(frame.r_b), bddnot(frame.r_c)));
            bddp combined = BDD::getnode_raw(frame.top_var, lo, hi);
            bddwcache(BDD_OP_SPREAD, frame.f, static_cast<bddp>(frame.k), combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddand_iter.
// At each level the combined result may need ITE to re-order variables; that
// inner ITE is delegated to bddite_iter so the whole traversal stays
// stack-safe.
bddp bddswap_iter(bddp f, bddvar v1, bddvar v2, bddvar lev1, bddvar lev2) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    enum class Combine : uint8_t { MakeNodeV1, MakeNodeVar, IteV2, IteVar };
    struct Frame {
        bddp f;           // non-complemented f (used as cache key)
        bddvar top_var;
        bddp f_hi;
        bddp lo_result;
        bool comp;        // whether result is negated
        Combine combine;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.top_var = 0;
    init.f_hi = 0;
    init.lo_result = bddnull;
    init.comp = false;
    init.combine = Combine::MakeNodeVar;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp cache_key = (static_cast<bddp>(v1) << 31) | static_cast<bddp>(v2);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf & BDD_CONST_FLAG) { result = cf; stack.pop_back(); break; }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp fn = cf & ~static_cast<bddp>(BDD_COMP_FLAG);

            bddvar f_var = node_var(fn);
            bddvar f_level = var2level[f_var];

            if (f_level < lev2) { result = cf; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_SWAP, fn, cache_key);
            if (cached != bddnull) {
                result = comp ? bddnot(cached) : cached;
                stack.pop_back(); break;
            }

            bddp f_lo = node_lo(fn);
            bddp f_hi = node_hi(fn);

            frame.f = fn;
            frame.f_hi = f_hi;
            frame.comp = comp;

            if (f_var == v2) {
                // Below lev2, children contain neither v1 nor v2.
                // Re-label v2 -> v1 directly without recursion.
                bddp combined = BDD::getnode_raw(v1, f_lo, f_hi);
                bddwcache(BDD_OP_SWAP, fn, cache_key, combined);
                result = comp ? bddnot(combined) : combined;
                stack.pop_back(); break;
            }

            if (f_var == v1) {
                frame.top_var = v2;
                frame.combine = Combine::IteV2;
            } else if (f_level > lev1) {
                frame.top_var = f_var;
                frame.combine = Combine::MakeNodeVar;
            } else {
                frame.top_var = f_var;
                frame.combine = Combine::IteVar;
            }

            frame.phase = Phase::GOT_LO;
            Frame lo_frame;
            lo_frame.f = f_lo;
            lo_frame.top_var = 0;
            lo_frame.f_hi = 0;
            lo_frame.lo_result = bddnull;
            lo_frame.comp = false;
            lo_frame.combine = Combine::MakeNodeVar;
            lo_frame.phase = Phase::ENTER;
            stack.push_back(lo_frame);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;
            Frame hi_frame;
            hi_frame.f = frame.f_hi;
            hi_frame.top_var = 0;
            hi_frame.f_hi = 0;
            hi_frame.lo_result = bddnull;
            hi_frame.comp = false;
            hi_frame.combine = Combine::MakeNodeVar;
            hi_frame.phase = Phase::ENTER;
            stack.push_back(hi_frame);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi_r = result;
            bddp lo_r = frame.lo_result;
            bddp combined;
            if (frame.combine == Combine::IteV2) {
                combined = bddite_iter(bddprime(v2), hi_r, lo_r);
            } else if (frame.combine == Combine::IteVar) {
                combined = bddite_iter(bddprime(frame.top_var), hi_r, lo_r);
            } else {
                combined = BDD::getnode_raw(frame.top_var, lo_r, hi_r);
            }
            bddwcache(BDD_OP_SWAP, frame.f, cache_key, combined);
            result = frame.comp ? bddnot(combined) : combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddand_iter.
// Reductions to 2-operand (g or h terminal) are delegated to bddand_iter so the
// entire computation stays stack-safe even when the non-terminal operands are
// deep.
bddp bddite_iter(bddp f, bddp g, bddp h) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;          // non-complemented after normalization
        bddp g;          // non-complemented after normalization
        bddp h;          // may be complemented
        bddvar top_var;
        bddp f_hi;
        bddp g_hi;
        bddp h_hi;
        bddp lo_result;
        bool comp;       // apply bddnot() to final result
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.g = g;
    init.h = h;
    init.top_var = 0;
    init.f_hi = 0;
    init.g_hi = 0;
    init.h_hi = 0;
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
            bddp ch = frame.h;

            // Terminal cases for f
            if (cf == bddtrue) { result = cg; stack.pop_back(); break; }
            if (cf == bddfalse) { result = ch; stack.pop_back(); break; }

            // g == h
            if (cg == ch) { result = cg; stack.pop_back(); break; }

            // Reduction to 2-operand (use bddand_iter for stack-safety)
            if (cg == bddtrue) {
                // OR(f, h) = ~AND(~f, ~h)
                result = bddnot(bddand_iter(bddnot(cf), bddnot(ch)));
                stack.pop_back(); break;
            }
            if (cg == bddfalse) {
                result = bddand_iter(bddnot(cf), ch);
                stack.pop_back(); break;
            }
            if (ch == bddfalse) {
                result = bddand_iter(cf, cg);
                stack.pop_back(); break;
            }
            if (ch == bddtrue) {
                // OR(~f, g) = ~AND(f, ~g)
                result = bddnot(bddand_iter(cf, bddnot(cg)));
                stack.pop_back(); break;
            }

            // All three non-terminal; normalize f
            if (cf & BDD_COMP_FLAG) {
                cf = bddnot(cf);
                bddp tmp = cg; cg = ch; ch = tmp;
            }

            // Normalize g: ensure g is non-complemented
            bool comp = false;
            if (cg & BDD_COMP_FLAG) {
                cg = bddnot(cg);
                ch = bddnot(ch);
                comp = true;
            }

            // Post-normalization terminal check
            if (cg == ch) {
                result = comp ? bddnot(cg) : cg;
                stack.pop_back(); break;
            }

            // Cache lookup
            bddp cached = bddrcache3(BDD_OP_ITE, cf, cg, ch);
            if (cached != bddnull) {
                result = comp ? bddnot(cached) : cached;
                stack.pop_back(); break;
            }

            // Determine top variable (highest level among f, g, h)
            bddvar f_var = node_var(cf);
            bddvar g_var = node_var(cg);
            bddvar f_level = var2level[f_var];
            bddvar g_level = var2level[g_var];

            bddvar top_var = (f_level >= g_level) ? f_var : g_var;
            bddvar top_level = (f_level >= g_level) ? f_level : g_level;

            bool h_comp = (ch & BDD_COMP_FLAG) != 0;
            bddvar h_var = node_var(ch);
            bddvar h_level = var2level[h_var];
            if (h_level > top_level) { top_var = h_var; top_level = h_level; }

            // Cofactors for f (non-complemented)
            bddp f_lo, f_hi;
            if (f_level == top_level) {
                f_lo = node_lo(cf);
                f_hi = node_hi(cf);
            } else {
                f_lo = cf; f_hi = cf;
            }

            // Cofactors for g (non-complemented)
            bddp g_lo, g_hi;
            if (g_level == top_level) {
                g_lo = node_lo(cg);
                g_hi = node_hi(cg);
            } else {
                g_lo = cg; g_hi = cg;
            }

            // Cofactors for h (may be complemented)
            bddp h_lo, h_hi;
            if (h_level == top_level) {
                h_lo = node_lo(ch);
                h_hi = node_hi(ch);
                if (h_comp) { h_lo = bddnot(h_lo); h_hi = bddnot(h_hi); }
            } else {
                h_lo = ch; h_hi = ch;
            }

            // Save normalized key + state for GOT_LO/GOT_HI/cache
            frame.f = cf;
            frame.g = cg;
            frame.h = ch;
            frame.top_var = top_var;
            frame.f_hi = f_hi;
            frame.g_hi = g_hi;
            frame.h_hi = h_hi;
            frame.comp = comp;
            frame.phase = Phase::GOT_LO;

            Frame lo_frame;
            lo_frame.f = f_lo;
            lo_frame.g = g_lo;
            lo_frame.h = h_lo;
            lo_frame.top_var = 0;
            lo_frame.f_hi = 0;
            lo_frame.g_hi = 0;
            lo_frame.h_hi = 0;
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
            hi_frame.h = frame.h_hi;
            hi_frame.top_var = 0;
            hi_frame.f_hi = 0;
            hi_frame.g_hi = 0;
            hi_frame.h_hi = 0;
            hi_frame.lo_result = bddnull;
            hi_frame.comp = false;
            hi_frame.phase = Phase::ENTER;
            stack.push_back(hi_frame);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi_r = result;
            bddp combined = BDD::getnode_raw(frame.top_var, frame.lo_result, hi_r);
            bddwcache3(BDD_OP_ITE, frame.f, frame.g, frame.h, combined);
            result = frame.comp ? bddnot(combined) : combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope.
// Explicit-stack counterpart of bddcount_bdd_rec. Returns the inner count
// (scaled by the top-level); the outer bddcount() re-applies the top-level
// shift. Shares the memo semantics with the recursive version.
double bddcount_bdd_iter(bddp f, bddvar n,
                         std::unordered_map<bddp, double>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;           // raw input (may be complemented)
        bddp f_raw;       // f with complement stripped
        bool comp;
        bddvar f_level;
        bddvar lo_level;
        bddvar hi_level;
        double lo_count;
        Phase phase;
    };

    double result = 0.0;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.f_raw = 0;
    init.comp = false;
    init.f_level = 0;
    init.lo_level = 0;
    init.hi_level = 0;
    init.lo_count = 0.0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf == bddfalse) { result = 0.0; stack.pop_back(); break; }
            if (cf == bddtrue) { result = 1.0; stack.pop_back(); break; }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;

            auto it = memo.find(f_raw);
            if (it != memo.end()) {
                double count = it->second;
                if (comp) {
                    bddvar lvl = var2level[node_var(f_raw)];
                    count = std::ldexp(1.0, lvl) - count;
                }
                result = count;
                stack.pop_back();
                break;
            }

            bddvar var = node_var(f_raw);
            if (var > n) {
                throw std::invalid_argument(
                    "bddcount: BDD contains variable > n");
            }
            bddvar f_level = var2level[var];
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);

            bddvar lo_level = (lo & BDD_CONST_FLAG) ? 0 : var2level[node_var(lo)];
            bddvar hi_level = (hi & BDD_CONST_FLAG) ? 0
                : var2level[node_var(hi & ~BDD_COMP_FLAG)];

            frame.f_raw = f_raw;
            frame.comp = comp;
            frame.f_level = f_level;
            frame.lo_level = lo_level;
            frame.hi_level = hi_level;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = lo;
            child.f_raw = 0;
            child.comp = false;
            child.f_level = 0;
            child.lo_level = 0;
            child.hi_level = 0;
            child.lo_count = 0.0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_count = result;
            frame.phase = Phase::GOT_HI;
            bddp hi = node_hi(frame.f_raw);
            Frame child;
            child.f = hi;
            child.f_raw = 0;
            child.comp = false;
            child.f_level = 0;
            child.lo_level = 0;
            child.hi_level = 0;
            child.lo_count = 0.0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            double hi_count = result;
            int exp_lo = static_cast<int>(frame.f_level) - 1
                       - static_cast<int>(frame.lo_level);
            int exp_hi = static_cast<int>(frame.f_level) - 1
                       - static_cast<int>(frame.hi_level);
            if (exp_lo < 0 || exp_hi < 0) {
                throw std::logic_error(
                    "bddcount: invalid BDD structure (child level >= parent level)");
            }
            double count = std::ldexp(frame.lo_count, exp_lo)
                         + std::ldexp(hi_count, exp_hi);
            memo[frame.f_raw] = count;
            if (frame.comp) {
                count = std::ldexp(1.0, frame.f_level) - count;
            }
            result = count;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope.
// BigInt variant of bddcount_bdd_iter.
bigint::BigInt bddexactcount_bdd_iter(
    bddp f, bddvar n,
    std::unordered_map<bddp, bigint::BigInt>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp f_raw;
        bool comp;
        bddvar f_level;
        bddvar lo_level;
        bddvar hi_level;
        bigint::BigInt lo_count;
        Phase phase;
    };

    bigint::BigInt result(0);
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.f_raw = 0;
    init.comp = false;
    init.f_level = 0;
    init.lo_level = 0;
    init.hi_level = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf == bddfalse) { result = bigint::BigInt(0); stack.pop_back(); break; }
            if (cf == bddtrue) { result = bigint::BigInt(1); stack.pop_back(); break; }

            bool comp = (cf & BDD_COMP_FLAG) != 0;
            bddp f_raw = cf & ~BDD_COMP_FLAG;

            auto it = memo.find(f_raw);
            if (it != memo.end()) {
                bigint::BigInt count = it->second;
                if (comp) {
                    bddvar lvl = var2level[node_var(f_raw)];
                    count = (bigint::BigInt(1) << static_cast<std::size_t>(lvl))
                          - count;
                }
                result = count;
                stack.pop_back();
                break;
            }

            bddvar var = node_var(f_raw);
            if (var > n) {
                throw std::invalid_argument(
                    "bddexactcount: BDD contains variable > n");
            }
            bddvar f_level = var2level[var];
            bddp lo = node_lo(f_raw);
            bddp hi = node_hi(f_raw);
            bddvar lo_level = (lo & BDD_CONST_FLAG) ? 0 : var2level[node_var(lo)];
            bddvar hi_level = (hi & BDD_CONST_FLAG) ? 0
                : var2level[node_var(hi & ~BDD_COMP_FLAG)];

            frame.f_raw = f_raw;
            frame.comp = comp;
            frame.f_level = f_level;
            frame.lo_level = lo_level;
            frame.hi_level = hi_level;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = lo;
            child.f_raw = 0;
            child.comp = false;
            child.f_level = 0;
            child.lo_level = 0;
            child.hi_level = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_count = result;
            frame.phase = Phase::GOT_HI;
            bddp hi = node_hi(frame.f_raw);
            Frame child;
            child.f = hi;
            child.f_raw = 0;
            child.comp = false;
            child.f_level = 0;
            child.lo_level = 0;
            child.hi_level = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bigint::BigInt hi_count = result;
            int shift_lo = static_cast<int>(frame.f_level) - 1
                         - static_cast<int>(frame.lo_level);
            int shift_hi = static_cast<int>(frame.f_level) - 1
                         - static_cast<int>(frame.hi_level);
            if (shift_lo < 0 || shift_hi < 0) {
                throw std::logic_error(
                    "bddexactcount: invalid BDD structure (child level >= parent level)");
            }
            bigint::BigInt count =
                (frame.lo_count << static_cast<std::size_t>(shift_lo))
              + (hi_count << static_cast<std::size_t>(shift_hi));
            memo[frame.f_raw] = count;
            if (frame.comp) {
                count = (bigint::BigInt(1) << static_cast<std::size_t>(frame.f_level))
                      - count;
            }
            result = count;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

} // namespace kyotodd
