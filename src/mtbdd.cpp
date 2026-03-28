#include "mtbdd.h"
#include "bdd_base.h"
#include <vector>
#include <stdexcept>

// --- Dynamic op code allocation ---

uint8_t mtbdd_next_op_code = 70;  // start past all static codes (max existing: 67)

uint8_t mtbdd_alloc_op_code() {
    if (mtbdd_next_op_code == 255) {
        throw std::overflow_error(
            "mtbdd: operation cache op code space exhausted");
    }
    return mtbdd_next_op_code++;
}

// --- Global terminal table registry ---

static std::vector<MTBDDTerminalTableBase*>& mtbdd_terminal_registry() {
    static auto* instance = new std::vector<MTBDDTerminalTableBase*>();
    return *instance;
}

void mtbdd_register_terminal_table(MTBDDTerminalTableBase* table) {
    mtbdd_terminal_registry().push_back(table);
}

void mtbdd_clear_all_terminal_tables() {
    for (auto* t : mtbdd_terminal_registry()) {
        t->clear();
    }
    // Do NOT reset mtbdd_next_op_code here.
    // Static op code variables (e.g. MTBDD::apply's static apply_op)
    // persist across bddfinal()/bddinit() cycles. Reusing their codes
    // for different operations would cause cache collisions.
}

// ========================================================================
//  MTZDD cofactor operations
// ========================================================================

// cofactor0: fix variable v to 0 (ZDD semantics)
// - For zero-suppressed variable (not in MTZDD): return f unchanged (v=0 is default)
// - At matching variable: return lo branch
// - Otherwise: recurse both branches

static uint8_t mtzdd_cofactor0_op() {
    static uint8_t op = mtbdd_alloc_op_code();
    return op;
}

static bddp mtzdd_cofactor0_rec(bddp f, bddvar v) {
    BDD_RecurGuard guard;

    // Terminal: no variables to cofactor
    if (f & BDD_CONST_FLAG) return f;

    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];
    bddvar v_level = var2level[v];

    // v is above f's top variable: v is zero-suppressed, v=0 is default
    if (f_level < v_level) return f;

    // Cache lookup
    bddp cached = bddrcache(mtzdd_cofactor0_op(), f, static_cast<bddp>(v));
    if (cached != bddnull) return cached;

    bddp result;
    if (f_var == v) {
        // Take lo branch (sets where v=0)
        result = node_lo(f);
    } else {
        // f_level > v_level: v is below, recurse into both branches
        bddp lo = mtzdd_cofactor0_rec(node_lo(f), v);
        bddp hi = mtzdd_cofactor0_rec(node_hi(f), v);
        result = mtzdd_getnode_raw(f_var, lo, hi);
    }

    bddwcache(mtzdd_cofactor0_op(), f, static_cast<bddp>(v), result);
    return result;
}

bddp mtzdd_cofactor0(bddp f, bddvar v) {
    if (f == bddnull)
        throw std::invalid_argument("mtzdd_cofactor0: null node");
    if (v < 1 || v > bddvarused())
        throw std::invalid_argument("mtzdd_cofactor0: var out of range");
    if (f & BDD_CONST_FLAG) return f;

    return bdd_gc_guard([&]() -> bddp { return mtzdd_cofactor0_rec(f, v); });
}

// cofactor1: fix variable v to 1 (ZDD semantics)
// - For zero-suppressed variable (not in MTZDD): return zero terminal
//   (implicit hi = zero_terminal)
// - At matching variable: return hi branch
// - Otherwise: recurse both branches

static uint8_t mtzdd_cofactor1_op() {
    static uint8_t op = mtbdd_alloc_op_code();
    return op;
}

static bddp mtzdd_cofactor1_rec(bddp f, bddvar v) {
    BDD_RecurGuard guard;

    // Terminal: zero-suppressed variable set to 1 → zero
    if (f & BDD_CONST_FLAG) return BDD_CONST_FLAG;  // zero terminal

    bddvar f_var = node_var(f);
    bddvar f_level = var2level[f_var];
    bddvar v_level = var2level[v];

    // v is above f's top variable: v is zero-suppressed, v=1 → zero
    if (f_level < v_level) return BDD_CONST_FLAG;  // zero terminal

    // Cache lookup
    bddp cached = bddrcache(mtzdd_cofactor1_op(), f, static_cast<bddp>(v));
    if (cached != bddnull) return cached;

    bddp result;
    if (f_var == v) {
        // Take hi branch (sets where v=1, with v removed)
        result = node_hi(f);
    } else {
        // f_level > v_level: v is below, recurse into both branches
        bddp lo = mtzdd_cofactor1_rec(node_lo(f), v);
        bddp hi = mtzdd_cofactor1_rec(node_hi(f), v);
        result = mtzdd_getnode_raw(f_var, lo, hi);
    }

    bddwcache(mtzdd_cofactor1_op(), f, static_cast<bddp>(v), result);
    return result;
}

bddp mtzdd_cofactor1(bddp f, bddvar v) {
    if (f == bddnull)
        throw std::invalid_argument("mtzdd_cofactor1: null node");
    if (v < 1 || v > bddvarused())
        throw std::invalid_argument("mtzdd_cofactor1: var out of range");
    if (f & BDD_CONST_FLAG) return BDD_CONST_FLAG;  // zero terminal

    return bdd_gc_guard([&]() -> bddp { return mtzdd_cofactor1_rec(f, v); });
}
