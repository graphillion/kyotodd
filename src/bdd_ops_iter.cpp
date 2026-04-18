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

} // namespace kyotodd
