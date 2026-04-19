#include "mtbdd.h"
#include "bdd_base.h"
#include "bdd_internal.h"
#include <cstdint>
#include <vector>

namespace kyotodd {

// =====================================================================
// MTZDD cofactor0 (explicit-stack). Matches mtzdd_cofactor0_rec semantics:
//   - Terminal: return f unchanged.
//   - If v is above f's top (f_level < v_level): v is zero-suppressed,
//     default value v=0 → return f unchanged.
//   - If f_var == v: return lo branch.
//   - Otherwise (f_level > v_level): recurse into both children and
//     rebuild node.
//   - Cache keyed by (op, f, static_cast<bddp>(v)) shared with _rec.
// PRECONDITION: caller holds a bdd_gc_guard scope.
// =====================================================================
bddp mtzdd_cofactor0_iter(bddp root, bddvar v, uint8_t op_code) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddvar f_var;
        bddp lo_result;
        Phase phase;
    };

    bddvar v_level = var2level[v];

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root;
    init.f_var = 0;
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

            bddvar f_var = node_var(f);
            bddvar f_level = var2level[f_var];

            if (f_level < v_level) {
                result = f;
                stack.pop_back();
                break;
            }

            bddp cached = bddrcache(op_code, f, static_cast<bddp>(v));
            if (cached != bddnull) {
                result = cached;
                stack.pop_back();
                break;
            }

            if (f_var == v) {
                bddp r = node_lo(f);
                bddwcache(op_code, f, static_cast<bddp>(v), r);
                result = r;
                stack.pop_back();
                break;
            }

            frame.f_var = f_var;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = node_lo(f);
            child.f_var = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = node_hi(frame.f);
            child.f_var = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp hi = result;
            bddp r = mtzdd_getnode_raw(frame.f_var, frame.lo_result, hi);
            bddwcache(op_code, frame.f, static_cast<bddp>(v), r);
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// =====================================================================
// MTZDD cofactor1 (explicit-stack). Matches mtzdd_cofactor1_rec semantics:
//   - Terminal: return zero terminal (BDD_CONST_FLAG, index 0).
//   - If v is above f's top (f_level < v_level): v is zero-suppressed,
//     v=1 → zero terminal.
//   - If f_var == v: return hi branch.
//   - Otherwise: recurse both children and rebuild node.
//   - Cache shared with _rec via op_code.
// PRECONDITION: caller holds a bdd_gc_guard scope.
// =====================================================================
bddp mtzdd_cofactor1_iter(bddp root, bddvar v, uint8_t op_code) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddvar f_var;
        bddp lo_result;
        Phase phase;
    };

    bddvar v_level = var2level[v];

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root;
    init.f_var = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f & BDD_CONST_FLAG) {
                result = BDD_CONST_FLAG;  // zero terminal
                stack.pop_back();
                break;
            }

            bddvar f_var = node_var(f);
            bddvar f_level = var2level[f_var];

            if (f_level < v_level) {
                result = BDD_CONST_FLAG;  // zero terminal
                stack.pop_back();
                break;
            }

            bddp cached = bddrcache(op_code, f, static_cast<bddp>(v));
            if (cached != bddnull) {
                result = cached;
                stack.pop_back();
                break;
            }

            if (f_var == v) {
                bddp r = node_hi(f);
                bddwcache(op_code, f, static_cast<bddp>(v), r);
                result = r;
                stack.pop_back();
                break;
            }

            frame.f_var = f_var;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = node_lo(f);
            child.f_var = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = node_hi(frame.f);
            child.f_var = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp hi = result;
            bddp r = mtzdd_getnode_raw(frame.f_var, frame.lo_result, hi);
            bddwcache(op_code, frame.f, static_cast<bddp>(v), r);
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

} // namespace kyotodd
