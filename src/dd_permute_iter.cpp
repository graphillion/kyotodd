// Iterative (explicit-stack) counterparts of the variable substitution
// operations in src/dd_permute.cpp. They share the BDD_OP_PERMUTE /
// BDD_OP_PERMUTEZ cache slots and the perm_id key with the _rec versions,
// so results are interchangeable.
//
// PRECONDITION for every function here: the caller has entered a
// bdd_gc_guard scope (intermediate results on the explicit stack are not
// GC roots).

#include "bdd.h"
#include "bdd_internal.h"
#include <vector>

namespace kyotodd {

namespace {

enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };

struct Frame {
    bddp fn;          // non-complemented node (set in ENTER)
    bool comp;        // complement to apply to the final result
    bddp lo_result;   // substituted lo child (set in GOT_LO)
    Phase phase;
};

inline Frame make_frame(bddp f) {
    Frame fr;
    fr.fn = f;
    fr.comp = false;
    fr.lo_result = bddnull;
    fr.phase = Phase::ENTER;
    return fr;
}

// Shared traversal. `make_node(w, r_lo, r_hi)` rebuilds a node for the new
// variable w from already-substituted children using the _iter primitives
// of the respective DD type, so the whole operation stays stack-safe.
template<typename MakeNode>
bddp permute_iter_impl(bddp root, const std::vector<bddvar>& perm,
                       bddvar min_level, bddp perm_id, uint8_t op,
                       MakeNode make_node) {
    std::vector<Frame> stack;
    stack.reserve(64);
    stack.push_back(make_frame(root));

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.fn;
            if (f & BDD_CONST_FLAG) {
                result = f;
                stack.pop_back();
                break;
            }
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp fn = f & ~static_cast<bddp>(BDD_COMP_FLAG);
            bddvar f_var = node_var(fn);
            if (var2level[f_var] < min_level) {
                result = f;
                stack.pop_back();
                break;
            }
            bddp cached = bddrcache(op, fn, perm_id);
            if (cached != bddnull) {
                result = comp ? bddnot(cached) : cached;
                stack.pop_back();
                break;
            }
            frame.fn = fn;
            frame.comp = comp;
            frame.phase = Phase::GOT_LO;
            stack.push_back(make_frame(node_lo(fn)));
            break;
        }

        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;
            stack.push_back(make_frame(node_hi(frame.fn)));
            break;
        }

        case Phase::GOT_HI: {
            bddp r_hi = result;
            bddp r_lo = frame.lo_result;
            bddp fn = frame.fn;
            bool comp = frame.comp;
            bddvar w = perm[node_var(fn)];
            // Pop before the (possibly allocating) combine step; the frame
            // reference is not used afterwards.
            stack.pop_back();
            bddp combined = make_node(w, r_lo, r_hi);
            bddwcache(op, fn, perm_id, combined);
            result = comp ? bddnot(combined) : combined;
            break;
        }
        }
    }
    return result;
}

struct MakeBddNode {
    bddp operator()(bddvar w, bddp r_lo, bddp r_hi) const {
        bddvar wl = var2level[w];
        if (wl > bddp_level(r_lo) && wl > bddp_level(r_hi)) {
            return BDD::getnode_raw(w, r_lo, r_hi);
        }
        return bddite_iter(bddprime(w), r_hi, r_lo);
    }
};

struct MakeZddNode {
    // { S ∪ {w} : S ∈ g }; see zdd_attach in src/dd_permute.cpp.
    static bddp attach(bddp g, bddvar w) {
        bddp g_off = bddoffset_iter(g, w);
        if (g_off == g) return bddchange_iter(g, w);
        return bddunion_iter(bddchange_iter(g_off, w), bddonset_iter(g, w));
    }
    bddp operator()(bddvar w, bddp r_lo, bddp r_hi) const {
        bddvar wl = var2level[w];
        if (wl > bddp_level(r_lo) && wl > bddp_level(r_hi)) {
            return ZDD::getnode_raw(w, r_lo, r_hi);
        }
        return bddunion_iter(r_lo, attach(r_hi, w));
    }
};

}  // namespace

bddp bddpermute_iter(bddp f, const std::vector<bddvar>& perm,
                     bddvar min_level, bddp perm_id) {
    return permute_iter_impl(f, perm, min_level, perm_id, BDD_OP_PERMUTE,
                             MakeBddNode());
}

bddp bddpermutez_iter(bddp f, const std::vector<bddvar>& perm,
                      bddvar min_level, bddp perm_id) {
    return permute_iter_impl(f, perm, min_level, perm_id, BDD_OP_PERMUTEZ,
                             MakeZddNode());
}

}  // namespace kyotodd
