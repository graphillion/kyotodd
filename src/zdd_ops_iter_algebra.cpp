// ZDD iterative operations (composite binary: div / join / product / meet / delta).
// Set ops (offset/onset/onset0/change/union/intersec/subtract/symdiff/issubset/
// isdisjoint) live in src/zdd_ops_iter.cpp.

#include "bdd.h"
#include "bdd_internal.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace kyotodd {

// =====================================================================
// Group D: Composite binary (Template A + sub-ops)
// =====================================================================

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddoffset_iter.
// At the "same top variable" branch, combines two sub-results via
// bddintersec_iter, which runs its own iterative traversal.
bddp bdddiv_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_LO_DIFF, GOT_HI_DIFF, GOT_Q1, GOT_Q0 };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bddp f_hi;          // for f_level > g_level path
        bddp f_lo;          // for same-var path
        bddp g_lo;          // for same-var path
        bddp lo_result;
        bddp q1_result;
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
    init.f_lo = 0;
    init.g_lo = 0;
    init.lo_result = bddnull;
    init.q1_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;

            if (cg == bddsingle) { result = cf; stack.pop_back(); break; }
            if (cf == bddempty || cg == bddempty) {
                result = bddempty; stack.pop_back(); break;
            }
            if (cf == bddsingle) { result = bddempty; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_DIV, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddvar g_var = node_var(cg);
            bddvar f_level = var2level[f_var];
            bddvar g_level = var2level[g_var];

            if (g_level > f_level) {
                bddwcache(BDD_OP_DIV, cf, cg, bddempty);
                result = bddempty; stack.pop_back(); break;
            }

            if (f_level > g_level) {
                bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                frame.top_var = f_var;
                frame.f_hi = f_hi;
                frame.phase = Phase::GOT_LO_DIFF;
                Frame child;
                child.f = f_lo;
                child.g = cg;
                child.top_var = 0;
                child.f_hi = 0;
                child.f_lo = 0;
                child.g_lo = 0;
                child.lo_result = bddnull;
                child.q1_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
                break;
            }

            // Same top var
            bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }
            bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
            if (g_comp) { g_lo = bddnot(g_lo); }

            frame.top_var = f_var;
            frame.f_lo = f_lo;
            frame.g_lo = g_lo;
            frame.phase = Phase::GOT_Q1;

            Frame child;
            child.f = f_hi;
            child.g = g_hi;
            child.top_var = 0;
            child.f_hi = 0;
            child.f_lo = 0;
            child.g_lo = 0;
            child.lo_result = bddnull;
            child.q1_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO_DIFF: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI_DIFF;
            Frame child;
            child.f = frame.f_hi;
            child.g = frame.g;
            child.top_var = 0;
            child.f_hi = 0;
            child.f_lo = 0;
            child.g_lo = 0;
            child.lo_result = bddnull;
            child.q1_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_DIFF: {
            bddp hi_r = result;
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_r);
            bddwcache(BDD_OP_DIV, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }

        case Phase::GOT_Q1: {
            frame.q1_result = result;
            if (frame.g_lo == bddempty) {
                bddwcache(BDD_OP_DIV, frame.f, frame.g, result);
                // result == q1_result already
                stack.pop_back();
                break;
            }
            frame.phase = Phase::GOT_Q0;
            Frame child;
            child.f = frame.f_lo;
            child.g = frame.g_lo;
            child.top_var = 0;
            child.f_hi = 0;
            child.f_lo = 0;
            child.g_lo = 0;
            child.lo_result = bddnull;
            child.q1_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_Q0: {
            bddp q0 = result;
            bddp q1 = frame.q1_result;
            bddp combined = bddintersec_iter(q0, q1);
            bddwcache(BDD_OP_DIV, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddoffset_iter.
// At same-var case, combines four sub-results via bddunion_iter.
bddp bddjoin_iter(bddp f, bddp g) {
    enum class Phase : uint8_t {
        ENTER,
        GOT_LO_F, GOT_HI_F,    // f_level > g_level path
        GOT_LO_G, GOT_HI_G,    // g_level > f_level path
        GOT_LO, GOT_HI_A, GOT_HI_B, GOT_HI_C  // same-var path
    };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bddp f_lo;
        bddp f_hi;
        bddp g_lo;
        bddp g_hi;
        bddp lo_result;
        bddp hi_a;
        bddp hi_b;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.g = g;
    init.top_var = 0;
    init.f_lo = 0;
    init.f_hi = 0;
    init.g_lo = 0;
    init.g_hi = 0;
    init.lo_result = bddnull;
    init.hi_a = bddnull;
    init.hi_b = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;

            if (cf == bddempty || cg == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cf == bddsingle) { result = cg; stack.pop_back(); break; }
            if (cg == bddsingle) { result = cf; stack.pop_back(); break; }

            if (cf > cg) { bddp tmp = cf; cf = cg; cg = tmp; }
            frame.f = cf;
            frame.g = cg;

            bddp cached = bddrcache(BDD_OP_JOIN, cf, cg);
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
                frame.phase = Phase::GOT_LO_F;
                Frame child;
                child.f = f_lo;
                child.g = cg;
                child.top_var = 0;
                child.f_lo = 0;
                child.f_hi = 0;
                child.g_lo = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.hi_a = bddnull;
                child.hi_b = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
                break;
            }
            if (g_level > f_level) {
                bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.top_var = g_var;
                frame.g_hi = g_hi;
                frame.phase = Phase::GOT_LO_G;
                Frame child;
                child.f = cf;
                child.g = g_lo;
                child.top_var = 0;
                child.f_lo = 0;
                child.f_hi = 0;
                child.g_lo = 0;
                child.g_hi = 0;
                child.lo_result = bddnull;
                child.hi_a = bddnull;
                child.hi_b = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
                break;
            }

            // Same top var
            bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }
            bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
            if (g_comp) { g_lo = bddnot(g_lo); }

            frame.top_var = f_var;
            frame.f_lo = f_lo; frame.f_hi = f_hi;
            frame.g_lo = g_lo; frame.g_hi = g_hi;
            frame.phase = Phase::GOT_LO;
            Frame child;
            child.f = f_lo; child.g = g_lo;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.hi_a = bddnull; child.hi_b = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO_F: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI_F;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.hi_a = bddnull; child.hi_b = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_F: {
            bddp hi_r = result;
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_r);
            bddwcache(BDD_OP_JOIN, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO_G: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI_G;
            Frame child;
            child.f = frame.f; child.g = frame.g_hi;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.hi_a = bddnull; child.hi_b = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_G: {
            bddp hi_r = result;
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_r);
            bddwcache(BDD_OP_JOIN, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO: {
            // result = join(f_lo, g_lo), used as lo child
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI_A;
            Frame child;
            child.f = frame.f_lo; child.g = frame.g_hi;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.hi_a = bddnull; child.hi_b = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_A: {
            frame.hi_a = result;
            frame.phase = Phase::GOT_HI_B;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g_lo;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.hi_a = bddnull; child.hi_b = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_B: {
            frame.hi_b = result;
            frame.phase = Phase::GOT_HI_C;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g_hi;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.hi_a = bddnull; child.hi_b = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_C: {
            bddp hi_c = result;
            bddp hi = bddunion_iter(bddunion_iter(frame.hi_a, frame.hi_b), hi_c);
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi);
            bddwcache(BDD_OP_JOIN, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddoffset_iter.
// Product assumes disjoint variable sets; recursion always goes down a single
// operand at a time.
bddp bddproduct_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bddp f_hi;
        bddp g_hi;
        bool dec_f;          // which operand was decomposed
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
    init.dec_f = false;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;

            if (cf == bddempty || cg == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cf == bddsingle) { result = cg; stack.pop_back(); break; }
            if (cg == bddsingle) { result = cf; stack.pop_back(); break; }

            if (cf > cg) { bddp tmp = cf; cf = cg; cg = tmp; }
            frame.f = cf;
            frame.g = cg;

            bddp cached = bddrcache(BDD_OP_PRODUCT, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;

            bddvar f_var = node_var(cf);
            bddvar g_var = node_var(cg);
            bddvar f_level = var2level[f_var];
            bddvar g_level = var2level[g_var];

            if (f_level == g_level) {
                throw std::invalid_argument(
                    "bddproduct: operands must have disjoint variable sets "
                    "(variable " + std::to_string(f_var) + " appears in both)");
            }

            if (f_level > g_level) {
                bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                frame.top_var = f_var;
                frame.f_hi = f_hi;
                frame.dec_f = true;
                frame.phase = Phase::GOT_LO;
                Frame child;
                child.f = f_lo; child.g = cg;
                child.top_var = 0;
                child.f_hi = 0; child.g_hi = 0;
                child.dec_f = false;
                child.lo_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else {
                bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.top_var = g_var;
                frame.g_hi = g_hi;
                frame.dec_f = false;
                frame.phase = Phase::GOT_LO;
                Frame child;
                child.f = cf; child.g = g_lo;
                child.top_var = 0;
                child.f_hi = 0; child.g_hi = 0;
                child.dec_f = false;
                child.lo_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            }
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            if (frame.dec_f) {
                child.f = frame.f_hi; child.g = frame.g;
            } else {
                child.f = frame.f; child.g = frame.g_hi;
            }
            child.top_var = 0;
            child.f_hi = 0; child.g_hi = 0;
            child.dec_f = false;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi_r = result;
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_r);
            bddwcache(BDD_OP_PRODUCT, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddoffset_iter.
// At asymmetric levels, combines two sub-results via bddunion_iter; at same var,
// combines three lo sub-results plus a hi via bddunion_iter as well.
bddp bddmeet_iter(bddp f, bddp g) {
    enum class Phase : uint8_t {
        ENTER,
        GOT_A_F, GOT_B_F,    // f_level > g_level
        GOT_A_G, GOT_B_G,    // g_level > f_level
        GOT_LO_A, GOT_LO_B, GOT_LO_C, GOT_HI  // same var
    };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bddp f_lo;
        bddp f_hi;
        bddp g_lo;
        bddp g_hi;
        bddp tmp_a;
        bddp tmp_b;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.g = g;
    init.top_var = 0;
    init.f_lo = 0; init.f_hi = 0;
    init.g_lo = 0; init.g_hi = 0;
    init.tmp_a = bddnull; init.tmp_b = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;

            if (cf == bddempty || cg == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cf == bddsingle) { result = bddsingle; stack.pop_back(); break; }
            if (cg == bddsingle) { result = bddsingle; stack.pop_back(); break; }

            if (cf > cg) { bddp tmp = cf; cf = cg; cg = tmp; }
            frame.f = cf;
            frame.g = cg;

            bddp cached = bddrcache(BDD_OP_MEET, cf, cg);
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
                // meet(f, g) = meet(f_lo, g) ∪ meet(f_hi, g)
                bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                frame.f_lo = f_lo; frame.f_hi = f_hi;
                frame.phase = Phase::GOT_A_F;
                Frame child;
                child.f = f_lo; child.g = cg;
                child.top_var = 0;
                child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
                child.tmp_a = bddnull; child.tmp_b = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
                break;
            }
            if (g_level > f_level) {
                bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.g_lo = g_lo; frame.g_hi = g_hi;
                frame.phase = Phase::GOT_A_G;
                Frame child;
                child.f = cf; child.g = g_lo;
                child.top_var = 0;
                child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
                child.tmp_a = bddnull; child.tmp_b = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
                break;
            }

            // Same top var
            bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }
            bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
            if (g_comp) { g_lo = bddnot(g_lo); }

            frame.top_var = f_var;
            frame.f_lo = f_lo; frame.f_hi = f_hi;
            frame.g_lo = g_lo; frame.g_hi = g_hi;
            frame.phase = Phase::GOT_LO_A;
            Frame child;
            child.f = f_lo; child.g = g_lo;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.tmp_a = bddnull; child.tmp_b = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_A_F: {
            frame.tmp_a = result;
            frame.phase = Phase::GOT_B_F;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.tmp_a = bddnull; child.tmp_b = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_B_F: {
            bddp combined = bddunion_iter(frame.tmp_a, result);
            bddwcache(BDD_OP_MEET, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }

        case Phase::GOT_A_G: {
            frame.tmp_a = result;
            frame.phase = Phase::GOT_B_G;
            Frame child;
            child.f = frame.f; child.g = frame.g_hi;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.tmp_a = bddnull; child.tmp_b = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_B_G: {
            bddp combined = bddunion_iter(frame.tmp_a, result);
            bddwcache(BDD_OP_MEET, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO_A: {
            // meet(f_lo, g_lo)
            frame.tmp_a = result;
            frame.phase = Phase::GOT_LO_B;
            Frame child;
            child.f = frame.f_lo; child.g = frame.g_hi;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.tmp_a = bddnull; child.tmp_b = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO_B: {
            frame.tmp_b = result;
            frame.phase = Phase::GOT_LO_C;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g_lo;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.tmp_a = bddnull; child.tmp_b = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO_C: {
            // result = meet(f_hi, g_lo)
            // lo = (tmp_a ∪ tmp_b) ∪ result
            bddp lo = bddunion_iter(bddunion_iter(frame.tmp_a, frame.tmp_b), result);
            // Reuse tmp_a to hold lo for later combine
            frame.tmp_a = lo;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g_hi;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.tmp_a = bddnull; child.tmp_b = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp hi_r = result;
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.tmp_a, hi_r);
            bddwcache(BDD_OP_MEET, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddoffset_iter.
bddp bdddelta_iter(bddp f, bddp g) {
    enum class Phase : uint8_t {
        ENTER,
        GOT_LO_F, GOT_HI_F,
        GOT_LO_G, GOT_HI_G,
        GOT_LO_A, GOT_LO_B, GOT_HI_A, GOT_HI_B
    };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bddp f_lo;
        bddp f_hi;
        bddp g_lo;
        bddp g_hi;
        bddp lo_result;
        bddp tmp_a;      // holds lo_a or hi_a
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.g = g;
    init.top_var = 0;
    init.f_lo = 0; init.f_hi = 0;
    init.g_lo = 0; init.g_hi = 0;
    init.lo_result = bddnull;
    init.tmp_a = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;

            if (cf == bddempty || cg == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cf == bddsingle) { result = cg; stack.pop_back(); break; }
            if (cg == bddsingle) { result = cf; stack.pop_back(); break; }

            if (cf > cg) { bddp tmp = cf; cf = cg; cg = tmp; }
            frame.f = cf;
            frame.g = cg;

            bddp cached = bddrcache(BDD_OP_DELTA, cf, cg);
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
                frame.phase = Phase::GOT_LO_F;
                Frame child;
                child.f = f_lo; child.g = cg;
                child.top_var = 0;
                child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
                child.lo_result = bddnull; child.tmp_a = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
                break;
            }
            if (g_level > f_level) {
                bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.top_var = g_var;
                frame.g_hi = g_hi;
                frame.phase = Phase::GOT_LO_G;
                Frame child;
                child.f = cf; child.g = g_lo;
                child.top_var = 0;
                child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
                child.lo_result = bddnull; child.tmp_a = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
                break;
            }

            // Same top var
            bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }
            bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
            if (g_comp) { g_lo = bddnot(g_lo); }

            frame.top_var = f_var;
            frame.f_lo = f_lo; frame.f_hi = f_hi;
            frame.g_lo = g_lo; frame.g_hi = g_hi;
            frame.phase = Phase::GOT_LO_A;
            Frame child;
            child.f = f_lo; child.g = g_lo;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.tmp_a = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO_F: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI_F;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.tmp_a = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_F: {
            bddp hi_r = result;
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_r);
            bddwcache(BDD_OP_DELTA, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO_G: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI_G;
            Frame child;
            child.f = frame.f; child.g = frame.g_hi;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.tmp_a = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_G: {
            bddp hi_r = result;
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_r);
            bddwcache(BDD_OP_DELTA, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO_A: {
            // result = delta(f_lo, g_lo)
            frame.tmp_a = result;
            frame.phase = Phase::GOT_LO_B;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g_hi;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.tmp_a = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO_B: {
            // result = delta(f_hi, g_hi); lo = tmp_a ∪ result
            bddp lo = bddunion_iter(frame.tmp_a, result);
            frame.lo_result = lo;
            frame.phase = Phase::GOT_HI_A;
            Frame child;
            child.f = frame.f_lo; child.g = frame.g_hi;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.tmp_a = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_A: {
            // result = delta(f_lo, g_hi)
            frame.tmp_a = result;
            frame.phase = Phase::GOT_HI_B;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g_lo;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.tmp_a = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_B: {
            // result = delta(f_hi, g_lo); hi = tmp_a ∪ result
            bddp hi = bddunion_iter(frame.tmp_a, result);
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi);
            bddwcache(BDD_OP_DELTA, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}


} // namespace kyotodd
