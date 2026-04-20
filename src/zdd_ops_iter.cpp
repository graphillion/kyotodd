#include "bdd.h"
#include "bdd_internal.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace kyotodd {

// =====================================================================
// Group A: Unary + var (Template B) — offset / onset / onset0 / change
// =====================================================================

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. Intermediate bddp
// values on the iteration stack are not registered as GC roots, so GC must be
// deferred (bdd_gc_depth > 0) for the duration of this call. The public
// wrapper bddoffset() in zdd_ops.cpp satisfies this precondition.
bddp bddoffset_iter(bddp f, bddvar var) {
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

    bddvar v_level = var2level[var];

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

            bddp cached = bddrcache(BDD_OP_OFFSET, cf, static_cast<bddp>(var));
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }  // ZDD: only lo is complemented

            if (f_var == var) {
                bddwcache(BDD_OP_OFFSET, cf, static_cast<bddp>(var), f_lo);
                result = f_lo; stack.pop_back(); break;
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
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_result);
            bddwcache(BDD_OP_OFFSET, frame.f, static_cast<bddp>(var), combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddoffset_iter.
bddp bddonset_iter(bddp f, bddvar var) {
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

    bddvar v_level = var2level[var];

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf & BDD_CONST_FLAG) { result = bddempty; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddvar f_level = var2level[f_var];

            if (f_level < v_level) { result = bddempty; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_ONSET, cf, static_cast<bddp>(var));
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }

            if (f_var == var) {
                bddp r = ZDD::getnode_raw(var, bddempty, f_hi);
                bddwcache(BDD_OP_ONSET, cf, static_cast<bddp>(var), r);
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
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_result);
            bddwcache(BDD_OP_ONSET, frame.f, static_cast<bddp>(var), combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddoffset_iter.
bddp bddonset0_iter(bddp f, bddvar var) {
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

    bddvar v_level = var2level[var];

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf & BDD_CONST_FLAG) { result = bddempty; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddvar f_level = var2level[f_var];

            if (f_level < v_level) { result = bddempty; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_ONSET0, cf, static_cast<bddp>(var));
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }

            if (f_var == var) {
                bddwcache(BDD_OP_ONSET0, cf, static_cast<bddp>(var), f_hi);
                result = f_hi; stack.pop_back(); break;
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
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_result);
            bddwcache(BDD_OP_ONSET0, frame.f, static_cast<bddp>(var), combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddoffset_iter.
// Caller must have already performed variable auto-expansion (bddnewvar) if
// needed so that var <= bdd_varcount.
bddp bddchange_iter(bddp f, bddvar var) {
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

    bddvar v_level = var2level[var];

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cf == bddsingle) {
                result = ZDD::getnode_raw(var, bddempty, bddsingle);
                stack.pop_back(); break;
            }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddvar f_level = var2level[f_var];

            if (f_level < v_level) {
                // var is above f's top: add var to every set
                result = ZDD::getnode_raw(var, bddempty, cf);
                stack.pop_back(); break;
            }

            bddp cached = bddrcache(BDD_OP_CHANGE, cf, static_cast<bddp>(var));
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }

            if (f_var == var) {
                bddp r = ZDD::getnode_raw(var, f_hi, f_lo);
                bddwcache(BDD_OP_CHANGE, cf, static_cast<bddp>(var), r);
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
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_result);
            bddwcache(BDD_OP_CHANGE, frame.f, static_cast<bddp>(var), combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// Group B: Simple binary (Template A) — union / intersec / subtract / symdiff
// =====================================================================

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddoffset_iter.
bddp bddunion_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
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

            if (cf == bddempty) { result = cg; stack.pop_back(); break; }
            if (cg == bddempty) { result = cf; stack.pop_back(); break; }
            if (cf == cg) { result = cf; stack.pop_back(); break; }

            if (cf > cg) { bddp tmp = cf; cf = cg; cg = tmp; }
            frame.f = cf;
            frame.g = cg;

            bddp cached = bddrcache(BDD_OP_UNION, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_const = (cf & BDD_CONST_FLAG) != 0;
            bool g_const = (cg & BDD_CONST_FLAG) != 0;
            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;

            bddvar f_var = f_const ? 0 : node_var(cf);
            bddvar g_var = g_const ? 0 : node_var(cg);
            bddvar f_level = f_const ? 0 : var2level[f_var];
            bddvar g_level = g_const ? 0 : var2level[g_var];

            bddvar top_var;
            bddp f_lo, f_hi, g_lo, g_hi;
            if (f_level > g_level) {
                top_var = f_var;
                f_lo = node_lo(cf); f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                g_lo = cg; g_hi = bddempty;
            } else if (g_level > f_level) {
                top_var = g_var;
                f_lo = cf; f_hi = bddempty;
                g_lo = node_lo(cg); g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
            } else {
                top_var = f_var;
                f_lo = node_lo(cf); f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                g_lo = node_lo(cg); g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
            }

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
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_r);
            bddwcache(BDD_OP_UNION, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddoffset_iter.
bddp bddintersec_iter(bddp f, bddp g) {
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

            if (cf == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cg == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cf == cg) { result = cf; stack.pop_back(); break; }

            if (cf > cg) { bddp tmp = cf; cf = cg; cg = tmp; }
            frame.f = cf;
            frame.g = cg;

            bddp cached = bddrcache(BDD_OP_INTERSEC, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_const = (cf & BDD_CONST_FLAG) != 0;
            bool g_const = (cg & BDD_CONST_FLAG) != 0;
            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;

            bddvar f_var = f_const ? 0 : node_var(cf);
            bddvar g_var = g_const ? 0 : node_var(cg);
            bddvar f_level = f_const ? 0 : var2level[f_var];
            bddvar g_level = g_const ? 0 : var2level[g_var];

            if (f_level > g_level) {
                bddp f_lo = node_lo(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                frame.phase = Phase::GOT_PASS;
                Frame child;
                child.f = f_lo;
                child.g = cg;
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else if (g_level > f_level) {
                bddp g_lo = node_lo(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.phase = Phase::GOT_PASS;
                Frame child;
                child.f = cf;
                child.g = g_lo;
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else {
                bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }

                frame.top_var = f_var;
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
            bddwcache(BDD_OP_INTERSEC, frame.f, frame.g, result);
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
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_r);
            bddwcache(BDD_OP_INTERSEC, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddoffset_iter.
bddp bddsubtract_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_PASS, GOT_LO_SAME, GOT_HI_SAME, GOT_LO_FHI };
    // Combining strategy for each decomposition case:
    //   GOT_PASS   : just cache and return (single recursion result)
    //   GOT_LO_FHI : f_level > g_level; one recurse on (f_lo,g), f_hi untouched
    //   GOT_LO_SAME: same top var; recursed on (f_lo,g_lo), then (f_hi,g_hi)
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

            if (cf == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cg == bddempty) { result = cf; stack.pop_back(); break; }
            if (cf == cg) { result = bddempty; stack.pop_back(); break; }

            // Cache lookup (subtract is not commutative, no order normalization)
            bddp cached = bddrcache(BDD_OP_SUBTRACT, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_const = (cf & BDD_CONST_FLAG) != 0;
            bool g_const = (cg & BDD_CONST_FLAG) != 0;
            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;

            bddvar f_var = f_const ? 0 : node_var(cf);
            bddvar g_var = g_const ? 0 : node_var(cg);
            bddvar f_level = f_const ? 0 : var2level[f_var];
            bddvar g_level = g_const ? 0 : var2level[g_var];

            if (f_level > g_level) {
                bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                frame.top_var = f_var;
                frame.f_hi = f_hi;
                frame.phase = Phase::GOT_LO_FHI;
                Frame child;
                child.f = f_lo;
                child.g = cg;
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else if (g_level > f_level) {
                bddp g_lo = node_lo(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.phase = Phase::GOT_PASS;
                Frame child;
                child.f = cf;
                child.g = g_lo;
                child.top_var = 0;
                child.f_hi = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else {
                bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.top_var = f_var;
                frame.f_hi = f_hi;
                frame.g_hi = g_hi;
                frame.phase = Phase::GOT_LO_SAME;
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
            bddwcache(BDD_OP_SUBTRACT, frame.f, frame.g, result);
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO_FHI: {
            bddp lo = result;
            bddp combined = ZDD::getnode_raw(frame.top_var, lo, frame.f_hi);
            bddwcache(BDD_OP_SUBTRACT, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO_SAME: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI_SAME;
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

        case Phase::GOT_HI_SAME: {
            bddp hi_r = result;
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_r);
            bddwcache(BDD_OP_SUBTRACT, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddoffset_iter.
bddp bddsymdiff_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
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

            if (cf == bddempty) { result = cg; stack.pop_back(); break; }
            if (cg == bddempty) { result = cf; stack.pop_back(); break; }
            if (cf == cg) { result = bddempty; stack.pop_back(); break; }

            if (cf > cg) { bddp tmp = cf; cf = cg; cg = tmp; }
            frame.f = cf;
            frame.g = cg;

            bddp cached = bddrcache(BDD_OP_SYMDIFF, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_const = (cf & BDD_CONST_FLAG) != 0;
            bool g_const = (cg & BDD_CONST_FLAG) != 0;
            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;

            bddvar f_var = f_const ? 0 : node_var(cf);
            bddvar g_var = g_const ? 0 : node_var(cg);
            bddvar f_level = f_const ? 0 : var2level[f_var];
            bddvar g_level = g_const ? 0 : var2level[g_var];

            bddvar top_var;
            bddp f_lo, f_hi, g_lo, g_hi;
            if (f_level > g_level) {
                top_var = f_var;
                f_lo = node_lo(cf); f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                g_lo = cg; g_hi = bddempty;
            } else if (g_level > f_level) {
                top_var = g_var;
                f_lo = cf; f_hi = bddempty;
                g_lo = node_lo(cg); g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
            } else {
                top_var = f_var;
                f_lo = node_lo(cf); f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                g_lo = node_lo(cg); g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
            }

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
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_r);
            bddwcache(BDD_OP_SYMDIFF, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// Group C: bool binary — issubset / isdisjoint
// =====================================================================

// These do not create new DD nodes, so they do not require bdd_gc_guard.
// They use a local SubsetMemoMap (shared layout with zdd_ops.cpp).

namespace {

typedef std::unordered_map<bddp, std::unordered_map<bddp, bool>> BoolMemoMap;

// Helper to look up the memo. Returns pointer to bool inside memo, or nullptr.
const bool* memo_get(const BoolMemoMap& memo, bddp f, bddp g) {
    auto it_f = memo.find(f);
    if (it_f == memo.end()) return nullptr;
    auto it_g = it_f->second.find(g);
    if (it_g == it_f->second.end()) return nullptr;
    return &it_g->second;
}

} // anonymous namespace

// PRECONDITION: No bdd_gc_guard required. See Group C header above — this
// predicate traverses the existing DAG without creating new DD nodes.
bool bddissubset_iter(bddp f, bddp g) {
    // Short-circuit trivial cases up-front (same as public wrapper).
    if (f == bddempty) return true;
    if (f == g) return true;
    if (g == bddempty) return false;

    enum class Phase : uint8_t { ENTER, GOT_HI, GOT_LO };
    struct Frame {
        bddp f;
        bddp g;
        bddp f_lo;
        bddp g_lo;
        Phase phase;
    };

    bool result = false;
    std::vector<Frame> stack;
    stack.reserve(64);
    BoolMemoMap memo;

    Frame init;
    init.f = f;
    init.g = g;
    init.f_lo = 0;
    init.g_lo = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;

            if (cf == bddempty) { result = true; stack.pop_back(); break; }
            if (cg == bddempty) { result = false; stack.pop_back(); break; }
            if (cf == cg) { result = true; stack.pop_back(); break; }
            if (cf == bddsingle) { result = bddhasempty(cg); stack.pop_back(); break; }
            if (cg == bddsingle) { result = false; stack.pop_back(); break; }

            const bool* cached = memo_get(memo, cf, cg);
            if (cached) { result = *cached; stack.pop_back(); break; }

            bool f_const = (cf & BDD_CONST_FLAG) != 0;
            bool g_const = (cg & BDD_CONST_FLAG) != 0;
            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;

            bddvar f_var = f_const ? 0 : node_var(cf);
            bddvar g_var = g_const ? 0 : node_var(cg);
            bddvar f_level = f_const ? 0 : var2level[f_var];
            bddvar g_level = g_const ? 0 : var2level[g_var];

            if (f_level > g_level) {
                // f has var not in g → f_hi ⊆ ∅ is false (f_hi is non-empty).
                memo[cf][cg] = false;
                result = false; stack.pop_back(); break;
            }
            if (g_level > f_level) {
                // Single recurse on (f, g_lo). Mark parent frame as
                // pass-through (f_lo=g_lo=0 is the sentinel): when the child
                // returns in GOT_HI phase, we just memoize and propagate.
                bddp g_lo = node_lo(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.f_lo = 0;
                frame.g_lo = 0;
                frame.phase = Phase::GOT_HI;
                Frame child;
                child.f = cf;
                child.g = g_lo;
                child.f_lo = 0;
                child.g_lo = 0;
                child.phase = Phase::ENTER;
                stack.push_back(child);
                break;
            }

            // Same top variable: check hi first (early termination)
            bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }
            bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
            if (g_comp) { g_lo = bddnot(g_lo); }

            frame.f_lo = f_lo;
            frame.g_lo = g_lo;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = f_hi;
            child.g = g_hi;
            child.f_lo = 0;
            child.g_lo = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            // If this frame is a "single-recurse pass-through" (f_lo == 0 && g_lo == 0),
            // just memoize and return the sub-result.
            if (frame.f_lo == 0 && frame.g_lo == 0) {
                memo[frame.f][frame.g] = result;
                stack.pop_back();
                break;
            }
            // Otherwise: got hi result from the same-var case.
            if (!result) {
                // Early termination: hi failed → whole result is false
                memo[frame.f][frame.g] = false;
                stack.pop_back();
                break;
            }
            // hi passed; now check lo
            frame.phase = Phase::GOT_LO;
            Frame child;
            child.f = frame.f_lo;
            child.g = frame.g_lo;
            child.f_lo = 0;
            child.g_lo = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            memo[frame.f][frame.g] = result;
            stack.pop_back();
            break;
        }
        }
    }

    return result;
}

// PRECONDITION: No bdd_gc_guard required. See Group C header above — this
// predicate traverses the existing DAG without creating new DD nodes.
bool bddisdisjoint_iter(bddp f, bddp g) {
    if (f == bddempty || g == bddempty) return true;
    if (f == g) return false;

    enum class Phase : uint8_t { ENTER, GOT_HI, GOT_LO };
    struct Frame {
        bddp f;
        bddp g;
        bddp f_lo;
        bddp g_lo;
        Phase phase;
    };

    bool result = false;
    std::vector<Frame> stack;
    stack.reserve(64);
    BoolMemoMap memo;

    Frame init;
    init.f = f;
    init.g = g;
    init.f_lo = 0;
    init.g_lo = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;

            if (cf == bddempty || cg == bddempty) { result = true; stack.pop_back(); break; }
            if (cf == cg) { result = false; stack.pop_back(); break; }
            if (cf == bddsingle) { result = !bddhasempty(cg); stack.pop_back(); break; }
            if (cg == bddsingle) { result = !bddhasempty(cf); stack.pop_back(); break; }

            if (cf > cg) { bddp tmp = cf; cf = cg; cg = tmp; }
            frame.f = cf;
            frame.g = cg;

            const bool* cached = memo_get(memo, cf, cg);
            if (cached) { result = *cached; stack.pop_back(); break; }

            bool f_const = (cf & BDD_CONST_FLAG) != 0;
            bool g_const = (cg & BDD_CONST_FLAG) != 0;
            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;

            bddvar f_var = f_const ? 0 : node_var(cf);
            bddvar g_var = g_const ? 0 : node_var(cg);
            bddvar f_level = f_const ? 0 : var2level[f_var];
            bddvar g_level = g_const ? 0 : var2level[g_var];

            if (f_level > g_level) {
                // g has no var at f's top. Check f_lo ∩ g disjoint?
                bddp f_lo = node_lo(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                frame.phase = Phase::GOT_HI;  // pass-through
                // f_lo/g_lo == 0 marker indicates single-recurse
                frame.f_lo = 0;
                frame.g_lo = 0;
                Frame child;
                child.f = f_lo;
                child.g = cg;
                child.f_lo = 0;
                child.g_lo = 0;
                child.phase = Phase::ENTER;
                stack.push_back(child);
                break;
            }
            if (g_level > f_level) {
                bddp g_lo = node_lo(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.phase = Phase::GOT_HI;
                frame.f_lo = 0;
                frame.g_lo = 0;
                Frame child;
                child.f = cf;
                child.g = g_lo;
                child.f_lo = 0;
                child.g_lo = 0;
                child.phase = Phase::ENTER;
                stack.push_back(child);
                break;
            }

            // Same var: check both hi and lo.
            bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }
            bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
            if (g_comp) { g_lo = bddnot(g_lo); }

            frame.f_lo = f_lo;
            frame.g_lo = g_lo;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = f_hi;
            child.g = g_hi;
            child.f_lo = 0;
            child.g_lo = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            // Pass-through case for asymmetric levels
            if (frame.f_lo == 0 && frame.g_lo == 0) {
                memo[frame.f][frame.g] = result;
                stack.pop_back();
                break;
            }
            if (!result) {
                memo[frame.f][frame.g] = false;
                stack.pop_back();
                break;
            }
            frame.phase = Phase::GOT_LO;
            Frame child;
            child.f = frame.f_lo;
            child.g = frame.g_lo;
            child.f_lo = 0;
            child.g_lo = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            memo[frame.f][frame.g] = result;
            stack.pop_back();
            break;
        }
        }
    }

    return result;
}

} // namespace kyotodd
