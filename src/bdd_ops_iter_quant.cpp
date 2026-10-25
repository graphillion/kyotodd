#include "bdd.h"
#include "bdd_internal.h"
#include <algorithm>
#include <vector>

namespace kyotodd {

// PRECONDITION: Must be invoked under a bdd_gc_guard scope. See bddand_iter.
// Shares the BDD_OP_AND_EXIST cache slots with bddandexist_rec. Reductions to
// a single AND or a single quantification are delegated to bddand_iter /
// bddexist_iter, and the OR at a quantified level goes through bddand_iter
// via De Morgan, so the whole traversal stays stack-safe.
bddp bddandexist_iter(bddp f, bddp g, bddp cube) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    enum class Combine : uint8_t { MakeNode, ReduceOr };
    struct Frame {
        bddp f;          // Operand f (after normalization)
        bddp g;          // Operand g (after normalization)
        bddp cube;       // Variable set (after skipping, used as cache key)
        bddvar top_var;  // Variable for getnode_raw (MakeNode only)
        bddp f_hi;       // hi cofactor of f (saved for GOT_LO phase)
        bddp g_hi;       // hi cofactor of g (saved for GOT_LO phase)
        bddp cube_hi;    // Cube passed to the hi subproblem
        bddp lo_result;  // Result of lo subproblem
        Combine combine;
        Phase phase;
    };

    auto make_frame = [](bddp ff, bddp gg, bddp cc) -> Frame {
        Frame fr;
        fr.f = ff;
        fr.g = gg;
        fr.cube = cc;
        fr.top_var = 0;
        fr.f_hi = 0;
        fr.g_hi = 0;
        fr.cube_hi = 0;
        fr.lo_result = bddnull;
        fr.combine = Combine::MakeNode;
        fr.phase = Phase::ENTER;
        return fr;
    };

    bddp result = bddnull;
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(make_frame(f, g, cube));

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp cf = frame.f;
            bddp cg = frame.g;
            bddp cc = frame.cube & ~BDD_COMP_FLAG;

            // Terminal cases
            if (cf == bddfalse || cg == bddfalse || cf == bddnot(cg)) {
                result = bddfalse; stack.pop_back(); break;
            }
            // Reductions to a single AND or a single quantification
            if (cc == bddfalse) {
                result = bddand_iter(cf, cg); stack.pop_back(); break;
            }
            if (cf == bddtrue || cf == cg) {
                result = bddexist_iter(cg, cc); stack.pop_back(); break;
            }
            if (cg == bddtrue) {
                result = bddexist_iter(cf, cc); stack.pop_back(); break;
            }

            // Normalize operand order (AND is commutative)
            if (cf > cg) { bddp tmp = cf; cf = cg; cg = tmp; }

            bddvar f_var = node_var(cf);
            bddvar g_var = node_var(cg);
            bddvar f_level = var2level[f_var];
            bddvar g_level = var2level[g_var];
            bddvar top_level = std::max(f_level, g_level);
            bddvar top_var = (f_level >= g_level) ? f_var : g_var;

            // Variables of the cube above both operands do not occur in f AND g
            while (cc != bddfalse && var2level[node_var(cc)] > top_level) {
                cc = node_lo(cc);
            }
            if (cc == bddfalse) {
                result = bddand_iter(cf, cg); stack.pop_back(); break;
            }

            bddp cached = bddrcache3(BDD_OP_AND_EXIST, cf, cg, cc);
            if (cached != bddnull) { result = cached; stack.pop_back(); break; }

            frame.f = cf;
            frame.g = cg;
            frame.cube = cc;

            // Cofactors with respect to the top variable
            bddp f_lo = cf, f_hi = cf, g_lo = cg, g_hi = cg;
            if (f_level == top_level) {
                f_lo = node_lo(cf);
                f_hi = node_hi(cf);
                if (cf & BDD_COMP_FLAG) { f_lo = bddnot(f_lo); f_hi = bddnot(f_hi); }
            }
            if (g_level == top_level) {
                g_lo = node_lo(cg);
                g_hi = node_hi(cg);
                if (cg & BDD_COMP_FLAG) { g_lo = bddnot(g_lo); g_hi = bddnot(g_hi); }
            }

            bddp child_cube;
            if (var2level[node_var(cc)] == top_level) {
                // Top variable is quantified: combine via OR
                child_cube = node_lo(cc);
                frame.combine = Combine::ReduceOr;
            } else {
                // Top variable is kept: combine via make-node
                child_cube = cc;
                frame.combine = Combine::MakeNode;
                frame.top_var = top_var;
            }
            frame.f_hi = f_hi;
            frame.g_hi = g_hi;
            frame.cube_hi = child_cube;
            frame.phase = Phase::GOT_LO;
            stack.push_back(make_frame(f_lo, g_lo, child_cube));
            break;
        }

        case Phase::GOT_LO: {
            if (frame.combine == Combine::ReduceOr && result == bddtrue) {
                // lo OR hi is already true; skip the hi branch
                bddwcache3(BDD_OP_AND_EXIST, frame.f, frame.g, frame.cube, bddtrue);
                stack.pop_back();
                break;
            }
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;
            Frame child = make_frame(frame.f_hi, frame.g_hi, frame.cube_hi);
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
            bddwcache3(BDD_OP_AND_EXIST, frame.f, frame.g, frame.cube, combined);
            result = combined;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

} // namespace kyotodd
