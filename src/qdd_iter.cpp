#include "bdd.h"
#include "bdd_internal.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace kyotodd {

// =====================================================================
// QDD -> BDD (explicit-stack). Matches qdd_to_bdd_rec semantics:
//   - memo keyed on f with complement stripped; value is the
//     non-complemented result, flipped at return time based on f's comp.
//   - BDD::getnode_raw applies the jump rule (lo == hi -> lo).
// PRECONDITION: caller holds a bdd_gc_guard scope.
// =====================================================================
bddp qdd_to_bdd_iter(bddp root, std::unordered_map<bddp, bddp>& memo) {
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
            if (f & BDD_CONST_FLAG) { result = f; stack.pop_back(); break; }
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
            bddp r = BDD::getnode_raw(node_var(frame.base), frame.lo_result, rhi);
            memo[frame.base] = r;
            result = frame.comp ? bddnot(r) : r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// QDD -> ZDD (explicit-stack). Matches qdd_to_zdd_rec semantics:
//   - memo keyed on f (complement-inclusive) because complement is
//     resolved into lo/hi before recursing (BDD semantics: both flip).
//   - ZDD::getnode_raw applies zero-suppression.
// PRECONDITION: caller holds a bdd_gc_guard scope.
// =====================================================================
bddp qdd_to_zdd_iter(bddp root, std::unordered_map<bddp, bddp>& memo) {
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
            if (f == bddfalse) { result = bddempty; stack.pop_back(); break; }
            if (f == bddtrue) { result = bddsingle; stack.pop_back(); break; }
            auto it = memo.find(f);
            if (it != memo.end()) { result = it->second; stack.pop_back(); break; }

            bddp base = f & ~BDD_COMP_FLAG;
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp lo = node_lo(base);
            bddp hi = node_hi(base);
            if (comp) { lo = bddnot(lo); hi = bddnot(hi); }

            frame.f = f;
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
            bddp zhi = result;
            bddp r = ZDD::getnode_raw(node_var(frame.base), frame.lo_result, zhi);
            memo[frame.f] = r;
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// BDD -> QDD (explicit-stack). Matches bdd_to_qdd_rec semantics:
//   - memo keyed on base (complement stripped). Complement is applied at
//     return time.
//   - After combining children, fills skipped levels via qdd_fill_levels
//     (BDD don't-care semantics).
// PRECONDITION: caller holds a bdd_gc_guard scope.
// =====================================================================
bddp bdd_to_qdd_iter(bddp root, std::unordered_map<bddp, bddp>& memo) {
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
            if (f & BDD_CONST_FLAG) { result = f; stack.pop_back(); break; }
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
            bddp qhi = result;
            bddp qlo = frame.lo_result;
            bddvar node_level = var2level[node_var(frame.base)];
            bddp filled_lo = qdd_fill_levels(qlo, bddp_level(qlo), node_level - 1);
            bddp filled_hi = qdd_fill_levels(qhi, bddp_level(qhi), node_level - 1);
            bddp r = QDD::getnode_raw(node_var(frame.base), filled_lo, filled_hi);
            memo[frame.base] = r;
            result = frame.comp ? bddnot(r) : r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// ZDD -> QDD (explicit-stack). Matches zdd_to_qdd_rec semantics:
//   - memo keyed on f (complement-inclusive). ZDD complement affects
//     only lo, resolved before recursing.
//   - After combining children, fills skipped levels via
//     qdd_fill_levels_zdd (ZDD zero-suppression semantics).
// PRECONDITION: caller holds a bdd_gc_guard scope.
// =====================================================================
bddp zdd_to_qdd_iter(bddp root, std::unordered_map<bddp, bddp>& memo) {
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
            if (f == bddempty) { result = bddfalse; stack.pop_back(); break; }
            if (f == bddtrue) { result = bddtrue; stack.pop_back(); break; }
            auto it = memo.find(f);
            if (it != memo.end()) { result = it->second; stack.pop_back(); break; }

            bddp base = f & ~BDD_COMP_FLAG;
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp lo = node_lo(base);
            bddp hi = node_hi(base);
            if (comp) lo = bddnot(lo);  // ZDD: only lo affected

            frame.f = f;
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
            bddp qhi = result;
            bddp qlo = frame.lo_result;
            bddvar node_level = var2level[node_var(frame.base)];
            bddp filled_lo = qdd_fill_levels_zdd(qlo, bddp_level(qlo), node_level - 1);
            bddp filled_hi = qdd_fill_levels_zdd(qhi, bddp_level(qhi), node_level - 1);
            bddp r = QDD::getnode_raw(node_var(frame.base), filled_lo, filled_hi);
            memo[frame.f] = r;
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// ZDD -> BDD (explicit-stack). Matches zdd_to_bdd_rec semantics:
//   - memo keyed on f (complement-inclusive). ZDD complement toggles lo.
//   - SOURCE (ZDD) child levels must be captured BEFORE recursing into
//     children, because BDD jump rule may collapse intermediate nodes.
//   - fill_zdd_levels_as_bdd inserts BDD "hi=false" nodes for suppressed
//     ZDD levels.
// PRECONDITION: caller holds a bdd_gc_guard scope.
// =====================================================================
bddp zdd_to_bdd_iter(bddp root, std::unordered_map<bddp, bddp>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp base;
        bddp hi_child;
        bddvar lo_src_level;
        bddvar hi_src_level;
        bddp lo_result;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root;
    init.base = 0;
    init.hi_child = 0;
    init.lo_src_level = 0;
    init.hi_src_level = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f == bddempty) { result = bddfalse; stack.pop_back(); break; }
            if (f == bddsingle) { result = bddtrue; stack.pop_back(); break; }
            auto it = memo.find(f);
            if (it != memo.end()) { result = it->second; stack.pop_back(); break; }

            bddp base = f & ~BDD_COMP_FLAG;
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp lo = node_lo(base);
            bddp hi = node_hi(base);
            if (comp) lo = bddnot(lo);

            frame.f = f;
            frame.base = base;
            frame.hi_child = hi;
            frame.lo_src_level = bddp_level(lo);
            frame.hi_src_level = bddp_level(hi);
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = lo;
            child.base = 0;
            child.hi_child = 0;
            child.lo_src_level = 0;
            child.hi_src_level = 0;
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
            child.lo_src_level = 0;
            child.hi_src_level = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp bdd_hi = result;
            bddp bdd_lo = frame.lo_result;
            bddvar node_level = var2level[node_var(frame.base)];
            bdd_lo = fill_zdd_levels_as_bdd(bdd_lo, frame.lo_src_level, node_level - 1);
            bdd_hi = fill_zdd_levels_as_bdd(bdd_hi, frame.hi_src_level, node_level - 1);
            bddp r = BDD::getnode_raw(node_var(frame.base), bdd_lo, bdd_hi);
            memo[frame.f] = r;
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// BDD -> ZDD (explicit-stack). Matches bdd_to_zdd_rec semantics:
//   - memo keyed on f (complement-inclusive). BDD complement flips both
//     children, resolved before recursing.
//   - SOURCE (BDD) child levels captured before recursion (ZDD
//     zero-suppression may collapse nodes).
//   - fill_bdd_levels_as_zdd inserts ZDD "both-same" nodes for skipped
//     BDD don't-care levels.
// PRECONDITION: caller holds a bdd_gc_guard scope.
// =====================================================================
bddp bdd_to_zdd_iter(bddp root, std::unordered_map<bddp, bddp>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddp base;
        bddp hi_child;
        bddvar lo_src_level;
        bddvar hi_src_level;
        bddp lo_result;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root;
    init.base = 0;
    init.hi_child = 0;
    init.lo_src_level = 0;
    init.hi_src_level = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f == bddfalse) { result = bddempty; stack.pop_back(); break; }
            if (f == bddtrue) { result = bddsingle; stack.pop_back(); break; }
            auto it = memo.find(f);
            if (it != memo.end()) { result = it->second; stack.pop_back(); break; }

            bddp base = f & ~BDD_COMP_FLAG;
            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp lo = node_lo(base);
            bddp hi = node_hi(base);
            if (comp) { lo = bddnot(lo); hi = bddnot(hi); }

            frame.f = f;
            frame.base = base;
            frame.hi_child = hi;
            frame.lo_src_level = bddp_level(lo);
            frame.hi_src_level = bddp_level(hi);
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = lo;
            child.base = 0;
            child.hi_child = 0;
            child.lo_src_level = 0;
            child.hi_src_level = 0;
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
            child.lo_src_level = 0;
            child.hi_src_level = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp zdd_hi = result;
            bddp zdd_lo = frame.lo_result;
            bddvar node_level = var2level[node_var(frame.base)];
            zdd_lo = fill_bdd_levels_as_zdd(zdd_lo, frame.lo_src_level, node_level - 1);
            zdd_hi = fill_bdd_levels_as_zdd(zdd_hi, frame.hi_src_level, node_level - 1);
            bddp r = ZDD::getnode_raw(node_var(frame.base), zdd_lo, zdd_hi);
            memo[frame.f] = r;
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

} // namespace kyotodd
