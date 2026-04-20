#include "bdd.h"
#include "bdd_internal.h"
#include <cstdint>
#include <vector>

namespace kyotodd {

// =====================================================================
// Group A: Binary filter ops — disjoin / jointjoin / restrict / permit /
//                             nonsup / nonsub
// =====================================================================

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. Intermediate bddp
// values on the iteration stack are not registered as GC roots, so GC must be
// deferred (bdd_gc_depth > 0) for the duration of this call. The public
// wrapper bdddisjoin() in zdd_adv_filter.cpp satisfies this precondition.
bddp bdddisjoin_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI_A, GOT_HI_B };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bool same_var;
        bddp f_lo, f_hi, g_lo, g_hi;
        bddp lo_result, hi_a;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.g = g;
    init.top_var = 0;
    init.same_var = false;
    init.f_lo = 0; init.f_hi = 0; init.g_lo = 0; init.g_hi = 0;
    init.lo_result = bddnull; init.hi_a = bddnull;
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
            frame.f = cf; frame.g = cg;

            bddp cached = bddrcache(BDD_OP_DISJOIN, cf, cg);
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
                frame.top_var = f_var;
                frame.same_var = false;
                frame.f_lo = node_lo(cf); frame.f_hi = node_hi(cf);
                if (f_comp) { frame.f_lo = bddnot(frame.f_lo); }
                frame.g_lo = cg; frame.g_hi = cg;
            } else if (g_level > f_level) {
                frame.top_var = g_var;
                frame.same_var = false;
                frame.g_lo = node_lo(cg); frame.g_hi = node_hi(cg);
                if (g_comp) { frame.g_lo = bddnot(frame.g_lo); }
                frame.f_lo = cf; frame.f_hi = cf;
            } else {
                frame.top_var = f_var;
                frame.same_var = true;
                frame.f_lo = node_lo(cf); frame.f_hi = node_hi(cf);
                if (f_comp) { frame.f_lo = bddnot(frame.f_lo); }
                frame.g_lo = node_lo(cg); frame.g_hi = node_hi(cg);
                if (g_comp) { frame.g_lo = bddnot(frame.g_lo); }
            }

            frame.phase = Phase::GOT_LO;
            Frame child;
            child.f = frame.f_lo; child.g = frame.g_lo;
            child.top_var = 0; child.same_var = false;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.hi_a = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            if (!frame.same_var) {
                frame.phase = Phase::GOT_HI_B;
                Frame child;
                child.f = frame.f_hi; child.g = frame.g_hi;
                child.top_var = 0; child.same_var = false;
                child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
                child.lo_result = bddnull; child.hi_a = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else {
                frame.phase = Phase::GOT_HI_A;
                Frame child;
                child.f = frame.f_lo; child.g = frame.g_hi;
                child.top_var = 0; child.same_var = false;
                child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
                child.lo_result = bddnull; child.hi_a = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            }
            break;
        }

        case Phase::GOT_HI_A: {
            frame.hi_a = result;
            frame.phase = Phase::GOT_HI_B;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g_lo;
            child.top_var = 0; child.same_var = false;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.hi_a = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_B: {
            bddp hi_result;
            if (frame.same_var) {
                bddp hi_b = result;
                hi_result = bddunion_iter(frame.hi_a, hi_b);
            } else {
                hi_result = result;
            }
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_result);
            bddwcache(BDD_OP_DISJOIN, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bdddisjoin_iter.
bddp bddjointjoin_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI_A, GOT_HI_B };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bool same_var;
        bddp f_lo, f_hi, g_lo, g_hi;
        bddp lo_result, hi_a;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.g = g;
    init.top_var = 0;
    init.same_var = false;
    init.f_lo = 0; init.f_hi = 0; init.g_lo = 0; init.g_hi = 0;
    init.lo_result = bddnull; init.hi_a = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;
            if (cf == bddempty || cg == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cf == bddsingle) { result = bddempty; stack.pop_back(); break; }
            if (cg == bddsingle) { result = bddempty; stack.pop_back(); break; }
            if (cf > cg) { bddp tmp = cf; cf = cg; cg = tmp; }
            frame.f = cf; frame.g = cg;

            bddp cached = bddrcache(BDD_OP_JOINTJOIN, cf, cg);
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
                frame.top_var = f_var;
                frame.same_var = false;
                frame.f_lo = node_lo(cf); frame.f_hi = node_hi(cf);
                if (f_comp) { frame.f_lo = bddnot(frame.f_lo); }
                frame.g_lo = cg; frame.g_hi = cg;
            } else if (g_level > f_level) {
                frame.top_var = g_var;
                frame.same_var = false;
                frame.g_lo = node_lo(cg); frame.g_hi = node_hi(cg);
                if (g_comp) { frame.g_lo = bddnot(frame.g_lo); }
                frame.f_lo = cf; frame.f_hi = cf;
            } else {
                frame.top_var = f_var;
                frame.same_var = true;
                frame.f_lo = node_lo(cf); frame.f_hi = node_hi(cf);
                if (f_comp) { frame.f_lo = bddnot(frame.f_lo); }
                frame.g_lo = node_lo(cg); frame.g_hi = node_hi(cg);
                if (g_comp) { frame.g_lo = bddnot(frame.g_lo); }
            }

            frame.phase = Phase::GOT_LO;
            Frame child;
            child.f = frame.f_lo; child.g = frame.g_lo;
            child.top_var = 0; child.same_var = false;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.hi_a = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            if (!frame.same_var) {
                frame.phase = Phase::GOT_HI_B;
                Frame child;
                child.f = frame.f_hi; child.g = frame.g_hi;
                child.top_var = 0; child.same_var = false;
                child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
                child.lo_result = bddnull; child.hi_a = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            } else {
                frame.phase = Phase::GOT_HI_A;
                Frame child;
                child.f = frame.f_lo; child.g = frame.g_hi;
                child.top_var = 0; child.same_var = false;
                child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
                child.lo_result = bddnull; child.hi_a = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            }
            break;
        }

        case Phase::GOT_HI_A: {
            frame.hi_a = result;
            frame.phase = Phase::GOT_HI_B;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g_lo;
            child.top_var = 0; child.same_var = false;
            child.f_lo = 0; child.f_hi = 0; child.g_lo = 0; child.g_hi = 0;
            child.lo_result = bddnull; child.hi_a = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_B: {
            bddp hi_result;
            if (frame.same_var) {
                bddp hi_b = result;
                // hi_c = join(f_hi, g_hi) — always qualifies (v ∈ both)
                bddp hi_c = bddjoin_iter(frame.f_hi, frame.g_hi);
                hi_result = bddunion_iter(bddunion_iter(frame.hi_a, hi_b), hi_c);
            } else {
                hi_result = result;
            }
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi_result);
            bddwcache(BDD_OP_JOINTJOIN, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bdddisjoin_iter.
bddp bddrestrict_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_PASS, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bool is_pass;
        bddp f_hi;
        bddp g_hi_arg;
        bddp lo_result;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f; init.g = g;
    init.top_var = 0; init.is_pass = false;
    init.f_hi = 0; init.g_hi_arg = 0;
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
            if (cg == bddsingle) { result = cf; stack.pop_back(); break; }
            if (cf == cg) { result = cf; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_RESTRICT, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_const = (cf & BDD_CONST_FLAG) != 0;
            bool g_const = (cg & BDD_CONST_FLAG) != 0;
            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;
            bddvar f_var = f_const ? 0 : node_var(cf);
            bddvar g_var = g_const ? 0 : node_var(cg);
            bddvar f_level = f_const ? 0 : var2level[f_var];
            bddvar g_level = g_const ? 0 : var2level[g_var];

            bddp child_f, child_g;
            if (f_level > g_level) {
                // G has no f_var: B⊆A works for both branches of F
                bddp f_lo = node_lo(cf);
                bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                frame.top_var = f_var;
                frame.is_pass = false;
                frame.f_hi = f_hi;
                frame.g_hi_arg = cg;
                child_f = f_lo; child_g = cg;
                frame.phase = Phase::GOT_LO;
            } else if (g_level > f_level) {
                // F has no g_var: skip to G_lo
                bddp g_lo = node_lo(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.is_pass = true;
                child_f = cf; child_g = g_lo;
                frame.phase = Phase::GOT_PASS;
            } else {
                // Same top variable
                bddp f_lo = node_lo(cf);
                bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                bddp g_lo = node_lo(cg);
                bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.top_var = f_var;
                frame.is_pass = false;
                frame.f_hi = f_hi;
                frame.g_hi_arg = bddunion_iter(g_lo, g_hi);
                child_f = f_lo; child_g = g_lo;
                frame.phase = Phase::GOT_LO;
            }

            Frame child;
            child.f = child_f; child.g = child_g;
            child.top_var = 0; child.is_pass = false;
            child.f_hi = 0; child.g_hi_arg = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_PASS: {
            bddwcache(BDD_OP_RESTRICT, frame.f, frame.g, result);
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g_hi_arg;
            child.top_var = 0; child.is_pass = false;
            child.f_hi = 0; child.g_hi_arg = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, result);
            bddwcache(BDD_OP_RESTRICT, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bdddisjoin_iter.
bddp bddpermit_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_PASS, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bool is_pass;
        bddp f_hi;
        bddp g_hi_arg;
        bddp lo_result;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f; init.g = g;
    init.top_var = 0; init.is_pass = false;
    init.f_hi = 0; init.g_hi_arg = 0;
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
            if (cf == bddsingle) { result = bddsingle; stack.pop_back(); break; }
            if (cf == cg) { result = cf; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_PERMIT, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_const = (cf & BDD_CONST_FLAG) != 0;
            bool g_const = (cg & BDD_CONST_FLAG) != 0;
            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;
            bddvar f_var = f_const ? 0 : node_var(cf);
            bddvar g_var = g_const ? 0 : node_var(cg);
            bddvar f_level = f_const ? 0 : var2level[f_var];
            bddvar g_level = g_const ? 0 : var2level[g_var];

            bddp child_f, child_g;
            if (f_level > g_level) {
                // G has no f_var: A with f_var can't be ⊆ any B
                bddp f_lo = node_lo(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                frame.is_pass = true;
                child_f = f_lo; child_g = cg;
                frame.phase = Phase::GOT_PASS;
            } else if (g_level > f_level) {
                // F has no g_var: A⊆(T∪{g_var}) iff A⊆T
                bddp g_lo = node_lo(cg);
                bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.is_pass = true;
                child_f = cf; child_g = bddunion_iter(g_lo, g_hi);
                frame.phase = Phase::GOT_PASS;
            } else {
                // Same top variable
                bddp f_lo = node_lo(cf);
                bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                bddp g_lo = node_lo(cg);
                bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.top_var = f_var;
                frame.is_pass = false;
                frame.f_hi = f_hi;
                frame.g_hi_arg = g_hi;
                child_f = f_lo; child_g = bddunion_iter(g_lo, g_hi);
                frame.phase = Phase::GOT_LO;
            }

            Frame child;
            child.f = child_f; child.g = child_g;
            child.top_var = 0; child.is_pass = false;
            child.f_hi = 0; child.g_hi_arg = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_PASS: {
            bddwcache(BDD_OP_PERMIT, frame.f, frame.g, result);
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g_hi_arg;
            child.top_var = 0; child.is_pass = false;
            child.f_hi = 0; child.g_hi_arg = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, result);
            bddwcache(BDD_OP_PERMIT, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bdddisjoin_iter.
bddp bddremove_supersets_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_PASS, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bool is_pass;
        bddp f_hi;
        bddp g_hi_arg;
        bddp lo_result;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f; init.g = g;
    init.top_var = 0; init.is_pass = false;
    init.f_hi = 0; init.g_hi_arg = 0;
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
            if (cg == bddsingle) { result = bddempty; stack.pop_back(); break; }
            if (cf == cg) { result = bddempty; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_REMOVE_SUPERSETS, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_const = (cf & BDD_CONST_FLAG) != 0;
            bool g_const = (cg & BDD_CONST_FLAG) != 0;
            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;
            bddvar f_var = f_const ? 0 : node_var(cf);
            bddvar g_var = g_const ? 0 : node_var(cg);
            bddvar f_level = f_const ? 0 : var2level[f_var];
            bddvar g_level = g_const ? 0 : var2level[g_var];

            bddp child_f, child_g;
            if (f_level > g_level) {
                bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                frame.top_var = f_var;
                frame.is_pass = false;
                frame.f_hi = f_hi;
                frame.g_hi_arg = cg;
                child_f = f_lo; child_g = cg;
                frame.phase = Phase::GOT_LO;
            } else if (g_level > f_level) {
                bddp g_lo = node_lo(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.is_pass = true;
                child_f = cf; child_g = g_lo;
                frame.phase = Phase::GOT_PASS;
            } else {
                bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.top_var = f_var;
                frame.is_pass = false;
                frame.f_hi = f_hi;
                frame.g_hi_arg = bddunion_iter(g_lo, g_hi);
                child_f = f_lo; child_g = g_lo;
                frame.phase = Phase::GOT_LO;
            }

            Frame child;
            child.f = child_f; child.g = child_g;
            child.top_var = 0; child.is_pass = false;
            child.f_hi = 0; child.g_hi_arg = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_PASS: {
            bddwcache(BDD_OP_REMOVE_SUPERSETS, frame.f, frame.g, result);
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;
            Frame child;
            child.f = frame.f_hi; child.g = frame.g_hi_arg;
            child.top_var = 0; child.is_pass = false;
            child.f_hi = 0; child.g_hi_arg = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, result);
            bddwcache(BDD_OP_REMOVE_SUPERSETS, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bdddisjoin_iter.
bddp bddremove_subsets_iter(bddp f, bddp g) {
    enum class Phase : uint8_t { ENTER, GOT_PASS, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp g;
        bddvar top_var;
        bool is_pass;
        bool hi_from_f;  // true when hi = f_hi directly (f_level > g_level case)
        bddp f_hi;       // used either as hi-directly or as next-child arg
        bddp g_hi_arg;   // g argument for GOT_HI child
        bddp lo_result;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f; init.g = g;
    init.top_var = 0; init.is_pass = false; init.hi_from_f = false;
    init.f_hi = 0; init.g_hi_arg = 0;
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
            if (cf == bddsingle) { result = bddempty; stack.pop_back(); break; }
            if (cf == cg) { result = bddempty; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_REMOVE_SUBSETS, cf, cg);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_const = (cf & BDD_CONST_FLAG) != 0;
            bool g_const = (cg & BDD_CONST_FLAG) != 0;
            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bool g_comp = (cg & BDD_COMP_FLAG) != 0;
            bddvar f_var = f_const ? 0 : node_var(cf);
            bddvar g_var = g_const ? 0 : node_var(cg);
            bddvar f_level = f_const ? 0 : var2level[f_var];
            bddvar g_level = g_const ? 0 : var2level[g_var];

            bddp child_f, child_g;
            if (f_level > g_level) {
                // hi = f_hi directly (all of F_hi qualify)
                bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                frame.top_var = f_var;
                frame.is_pass = false;
                frame.hi_from_f = true;
                frame.f_hi = f_hi;
                frame.g_hi_arg = 0;
                child_f = f_lo; child_g = cg;
                frame.phase = Phase::GOT_LO;
            } else if (g_level > f_level) {
                // skip to union(g_lo, g_hi)
                bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.is_pass = true;
                child_f = cf; child_g = bddunion_iter(g_lo, g_hi);
                frame.phase = Phase::GOT_PASS;
            } else {
                // Same top variable
                bddp f_lo = node_lo(cf); bddp f_hi = node_hi(cf);
                if (f_comp) { f_lo = bddnot(f_lo); }
                bddp g_lo = node_lo(cg); bddp g_hi = node_hi(cg);
                if (g_comp) { g_lo = bddnot(g_lo); }
                frame.top_var = f_var;
                frame.is_pass = false;
                frame.hi_from_f = false;
                frame.f_hi = f_hi;
                frame.g_hi_arg = g_hi;
                child_f = f_lo; child_g = bddunion_iter(g_lo, g_hi);
                frame.phase = Phase::GOT_LO;
            }

            Frame child;
            child.f = child_f; child.g = child_g;
            child.top_var = 0; child.is_pass = false; child.hi_from_f = false;
            child.f_hi = 0; child.g_hi_arg = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_PASS: {
            bddwcache(BDD_OP_REMOVE_SUBSETS, frame.f, frame.g, result);
            stack.pop_back();
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            if (frame.hi_from_f) {
                bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, frame.f_hi);
                bddwcache(BDD_OP_REMOVE_SUBSETS, frame.f, frame.g, combined);
                result = combined;
                stack.pop_back();
            } else {
                frame.phase = Phase::GOT_HI;
                Frame child;
                child.f = frame.f_hi; child.g = frame.g_hi_arg;
                child.top_var = 0; child.is_pass = false; child.hi_from_f = false;
                child.f_hi = 0; child.g_hi_arg = 0;
                child.lo_result = bddnull;
                child.phase = Phase::ENTER;
                stack.push_back(child);
            }
            break;
        }

        case Phase::GOT_HI: {
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, result);
            bddwcache(BDD_OP_REMOVE_SUBSETS, frame.f, frame.g, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// Group B: Unary filter ops — maximal / minimal / minhit / closure
// =====================================================================

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bdddisjoin_iter.
bddp bddmaximal_iter(bddp f) {
    enum class Phase : uint8_t { ENTER, GOT_HI, GOT_LO_TMP };
    struct Frame {
        bddp f;
        bddvar top_var;
        bddp f_lo, f_hi;
        bddp hi_result;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.top_var = 0;
    init.f_lo = 0; init.f_hi = 0;
    init.hi_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cf == bddsingle) { result = bddsingle; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_MAXIMAL, cf, 0);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }

            frame.top_var = f_var;
            frame.f_lo = f_lo;
            frame.f_hi = f_hi;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = f_hi;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0;
            child.hi_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI: {
            frame.hi_result = result;
            frame.phase = Phase::GOT_LO_TMP;
            Frame child;
            child.f = frame.f_lo;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0;
            child.hi_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO_TMP: {
            // lo = bddremove_subsets(maximal(f_lo), f_hi)
            bddp lo = bddremove_subsets_iter(result, frame.f_hi);
            bddp combined = ZDD::getnode_raw(frame.top_var, lo, frame.hi_result);
            bddwcache(BDD_OP_MAXIMAL, frame.f, 0, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bdddisjoin_iter.
bddp bddminimal_iter(bddp f) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI_TMP };
    struct Frame {
        bddp f;
        bddvar top_var;
        bddp f_lo, f_hi;
        bddp lo_result;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
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

            bddp cached = bddrcache(BDD_OP_MINIMAL, cf, 0);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }

            frame.top_var = f_var;
            frame.f_lo = f_lo;
            frame.f_hi = f_hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = f_lo;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI_TMP;
            Frame child;
            child.f = frame.f_hi;
            child.top_var = 0;
            child.f_lo = 0; child.f_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_HI_TMP: {
            // hi = bddremove_supersets(minimal(f_hi), f_lo)
            bddp hi = bddremove_supersets_iter(result, frame.f_lo);
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.lo_result, hi);
            bddwcache(BDD_OP_MINIMAL, frame.f, 0, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bdddisjoin_iter.
bddp bddminhit_iter(bddp f) {
    enum class Phase : uint8_t { ENTER, GOT_P, GOT_Q };
    struct Frame {
        bddp f;
        bddvar top_var;
        bddp f_lo;
        bddp P;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.top_var = 0;
    init.f_lo = 0;
    init.P = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf == bddempty) { result = bddsingle; stack.pop_back(); break; }
            if (cf == bddsingle) { result = bddempty; stack.pop_back(); break; }
            if (bddhasempty(cf)) { result = bddempty; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_MINHIT, cf, 0);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }

            frame.top_var = f_var;
            frame.f_lo = f_lo;
            frame.phase = Phase::GOT_P;

            // First child: minhit(union(f_hi, f_lo))
            bddp union_fg = bddunion_iter(f_hi, f_lo);
            Frame child;
            child.f = union_fg;
            child.top_var = 0;
            child.f_lo = 0;
            child.P = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_P: {
            frame.P = result;
            frame.phase = Phase::GOT_Q;
            Frame child;
            child.f = frame.f_lo;
            child.top_var = 0;
            child.f_lo = 0;
            child.P = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_Q: {
            bddp Q = result;
            bddp Q_filt = bddremove_supersets_iter(Q, frame.P);
            bddp combined = ZDD::getnode_raw(frame.top_var, frame.P, Q_filt);
            bddwcache(BDD_OP_MINHIT, frame.f, 0, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bdddisjoin_iter.
bddp bddclosure_iter(bddp f) {
    enum class Phase : uint8_t { ENTER, GOT_C0, GOT_C1 };
    struct Frame {
        bddp f;
        bddvar top_var;
        bddp f_hi;
        bddp C0;
        Phase phase;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = f;
    init.top_var = 0;
    init.f_hi = 0;
    init.C0 = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            if (cf == bddempty) { result = bddempty; stack.pop_back(); break; }
            if (cf == bddsingle) { result = bddsingle; stack.pop_back(); break; }

            bddp cached = bddrcache(BDD_OP_CLOSURE, cf, 0);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            bool f_comp = (cf & BDD_COMP_FLAG) != 0;
            bddvar f_var = node_var(cf);
            bddp f_lo = node_lo(cf);
            bddp f_hi = node_hi(cf);
            if (f_comp) { f_lo = bddnot(f_lo); }

            frame.top_var = f_var;
            frame.f_hi = f_hi;
            frame.phase = Phase::GOT_C0;

            Frame child;
            child.f = f_lo;
            child.top_var = 0;
            child.f_hi = 0;
            child.C0 = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_C0: {
            frame.C0 = result;
            frame.phase = Phase::GOT_C1;
            Frame child;
            child.f = frame.f_hi;
            child.top_var = 0;
            child.f_hi = 0;
            child.C0 = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }

        case Phase::GOT_C1: {
            bddp C1 = result;
            bddp lo = bddunion_iter(frame.C0, C1);
            bddp hi = C1;
            bddp combined = ZDD::getnode_raw(frame.top_var, lo, hi);
            bddwcache(BDD_OP_CLOSURE, frame.f, 0, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

} // namespace kyotodd
