#include "bdd.h"
#include "bdd_internal.h"
#include <cstdint>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace kyotodd {

// =====================================================================
// Iterative BDD complement expansion (explicit-stack). Matches
// expand_bdd_rec semantics:
//   - memo keyed on f (complement-inclusive); value is the fully
//     expanded (non-complemented) unreduced node ID.
//   - BDD complement: both lo and hi are flipped before recursing.
//   - Creates raw unreduced nodes via allocate_node/node_write; the
//     reduced flag is NOT set.
// PRECONDITION: caller holds a bdd_gc_guard scope (GC suppressed while
// intermediate bddp values are on the stack and in memo).
// =====================================================================
bddp expand_bdd_iter(bddp root, std::unordered_map<bddp, bddp>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp base;
        bddp hi_child;
        bddp lo_result;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root;
    init.base = 0;
    init.hi_child = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f & BDD_CONST_FLAG) { result = f; stack.pop_back(); break; }
            auto it = memo.find(f);
            if (it != memo.end()) { result = it->second; stack.pop_back(); break; }

            bddp base = f & ~BDD_COMP_FLAG;
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp lo = node_lo(base);
            bddp hi = node_hi(base);
            if (comp) { lo = bddnot(lo); hi = bddnot(hi); }

            frame.base = base;
            frame.hi_child = hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = lo;
            child.base = 0;
            child.hi_child = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = frame.hi_child;
            child.base = 0;
            child.hi_child = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp ehi = result;
            bddp elo = frame.lo_result;
            bddp node_id = allocate_node();
            node_write(node_id, node_var(frame.base), elo, ehi);
            // Do not set reduced flag — this is an unreduced node
            memo[frame.f] = node_id;
            result = node_id;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// Iterative ZDD complement expansion (explicit-stack). Matches
// expand_zdd_rec semantics:
//   - memo keyed on f (complement-inclusive).
//   - ZDD complement: only lo is flipped; hi unchanged.
// PRECONDITION: caller holds a bdd_gc_guard scope.
// =====================================================================
bddp expand_zdd_iter(bddp root, std::unordered_map<bddp, bddp>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp base;
        bddp hi_child;
        bddp lo_result;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root;
    init.base = 0;
    init.hi_child = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f & BDD_CONST_FLAG) { result = f; stack.pop_back(); break; }
            auto it = memo.find(f);
            if (it != memo.end()) { result = it->second; stack.pop_back(); break; }

            bddp base = f & ~BDD_COMP_FLAG;
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp lo = node_lo(base);
            bddp hi = node_hi(base);
            if (comp) lo = bddnot(lo);  // ZDD: only lo affected

            frame.base = base;
            frame.hi_child = hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = lo;
            child.base = 0;
            child.hi_child = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = frame.hi_child;
            child.base = 0;
            child.hi_child = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp ehi = result;
            bddp elo = frame.lo_result;
            bddp node_id = allocate_node();
            node_write(node_id, node_var(frame.base), elo, ehi);
            memo[frame.f] = node_id;
            result = node_id;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// Iterative reduce (explicit-stack). Matches reduce_rec semantics:
//   - memo keyed on `base` (complement stripped); value is the
//     non-complemented reduced result. Complement is applied at
//     return time via bddnot.
//   - Short-circuit: terminals and already-reduced subtrees are
//     returned as-is (no memo store).
//   - make_node applies the target semantics (BDD jump rule or ZDD
//     zero-suppression) via BDD::getnode_raw / ZDD::getnode_raw.
// PRECONDITION: caller holds a bdd_gc_guard scope.
// =====================================================================
bddp reduce_iter(bddp root, std::unordered_map<bddp, bddp>& memo,
                 bddp (*make_node)(bddvar, bddp, bddp)) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp base;
        bool comp;
        bddp lo_result;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root;
    init.base = 0;
    init.comp = false;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f == bddnull)
                throw std::invalid_argument("reduce: null node encountered");
            if (f & BDD_CONST_FLAG) { result = f; stack.pop_back(); break; }
            if (bddp_is_reduced(f)) { result = f; stack.pop_back(); break; }

            bddp base = f & ~BDD_COMP_FLAG;
            bool comp = (f & BDD_COMP_FLAG) != 0;

            auto it = memo.find(base);
            if (it != memo.end()) {
                result = comp ? bddnot(it->second) : it->second;
                stack.pop_back();
                break;
            }

            frame.base = base;
            frame.comp = comp;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = node_lo(base);
            child.base = 0;
            child.comp = false;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = node_hi(frame.base);
            child.base = 0;
            child.comp = false;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp rhi = result;
            bddp rlo = frame.lo_result;
            bddp r = make_node(node_var(frame.base), rlo, rhi);
            memo[frame.base] = r;
            result = frame.comp ? bddnot(r) : r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

} // namespace kyotodd
