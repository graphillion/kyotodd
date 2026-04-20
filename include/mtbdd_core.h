#ifndef KYOTODD_MTBDD_CORE_H
#define KYOTODD_MTBDD_CORE_H

// Internal header: terminal table, apply/ITE templates, from_bdd/from_zdd
// conversion, and MTZDD counting helpers for the MTBDD/MTZDD templates.
//
// End users should include "mtbdd.h" (umbrella), not this header directly.

#include "bdd_types.h"
#include "bdd_base.h"
#include "bdd_ops.h"
#include "bdd_internal.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include "bigint.hpp"
#include "svg_export.h"
#include <cstring>
#include <fstream>
#include <algorithm>
#include <sstream>

namespace kyotodd {

// --- Forward declarations for non-template functions (defined in mtbdd.cpp) ---

void mtbdd_register_terminal_table(class MTBDDTerminalTableBase* table);
void mtbdd_clear_all_terminal_tables();

// --- Terminal table base class (non-template, for type erasure) ---

class MTBDDTerminalTableBase {
public:
    virtual ~MTBDDTerminalTableBase() {}
    virtual uint64_t size() const = 0;
    virtual void clear() = 0;
};

// --- Terminal table (one singleton per type T) ---

template<typename T>
class MTBDDTerminalTable : public MTBDDTerminalTableBase {
public:
    static MTBDDTerminalTable<T>& instance() {
        // C++11 magic static: thread-safe initialization, no leak.
        // The registration side-effect is performed exactly once via the
        // static `reg` variable's initializer.
        static MTBDDTerminalTable<T> inst;
        static int reg = (mtbdd_register_terminal_table(&inst), 0);
        (void)reg;
        return inst;
    }

    uint64_t get_or_insert(const T& value) {
        auto it = value_to_index_.find(value);
        if (it != value_to_index_.end()) {
            return it->second;
        }
        uint64_t index = values_.size();
        values_.push_back(value);
        value_to_index_[value] = index;
        return index;
    }

    const T& get_value(uint64_t index) const {
        if (index >= values_.size()) {
            throw std::out_of_range("MTBDDTerminalTable::get_value: index out of range");
        }
        return values_[index];
    }

    bool contains(const T& value) const {
        return value_to_index_.find(value) != value_to_index_.end();
    }

    bool find_index(const T& value, uint64_t& index) const {
        auto it = value_to_index_.find(value);
        if (it == value_to_index_.end()) return false;
        index = it->second;
        return true;
    }

    uint64_t size() const override {
        return values_.size();
    }

    uint64_t zero_index() const {
        return 0;
    }

    void clear() override {
        values_.clear();
        value_to_index_.clear();
        // Re-register the zero value at index 0
        values_.push_back(T{});
        value_to_index_[T{}] = 0;
    }

    static bddp make_terminal(uint64_t index) {
        return BDD_CONST_FLAG | index;
    }

    static uint64_t terminal_index(bddp p) {
        return p & ~BDD_CONST_FLAG;
    }

private:
    MTBDDTerminalTable() {
        // Index 0 is always the zero value T{}
        values_.push_back(T{});
        value_to_index_[T{}] = 0;
    }

    std::vector<T> values_;
    std::unordered_map<T, uint64_t> value_to_index_;
};

// --- Forward declarations for getnode (defined in bdd_base.cpp) ---

bddp mtbdd_getnode_raw(bddvar var, bddp lo, bddp hi);
bddp mtbdd_getnode(bddvar var, bddp lo, bddp hi);
bddp mtzdd_getnode_raw(bddvar var, bddp lo, bddp hi);
bddp mtzdd_getnode(bddvar var, bddp lo, bddp hi);
uint8_t mtbdd_alloc_op_code();

// --- Forward declarations for MTZDD cofactor (defined in mtbdd.cpp) ---

bddp mtzdd_cofactor0(bddp f, bddvar v);
bddp mtzdd_cofactor1(bddp f, bddvar v);

// --- Forward declarations for MTZDD cofactor iter (defined in mtbdd_iter.cpp) ---

bddp mtzdd_cofactor0_iter(bddp f, bddvar v, uint8_t op_code);
bddp mtzdd_cofactor1_iter(bddp f, bddvar v, uint8_t op_code);

// --- Apply templates (BDD cofactoring) ---

template<typename T, typename BinOp>
static bddp mtbdd_apply_rec(bddp f, bddp g, BinOp& op, uint8_t cache_op) {
    BDD_RecurGuard guard;

    bool f_term = (f & BDD_CONST_FLAG) != 0;
    bool g_term = (g & BDD_CONST_FLAG) != 0;

    if (f_term && g_term) {
        auto& table = MTBDDTerminalTable<T>::instance();
        T result_val = op(
            table.get_value(MTBDDTerminalTable<T>::terminal_index(f)),
            table.get_value(MTBDDTerminalTable<T>::terminal_index(g)));
        return MTBDDTerminalTable<T>::make_terminal(
            table.get_or_insert(result_val));
    }

    // Do NOT normalize operand order — the operation may be non-commutative
    // (e.g. subtraction). Cache entries for (f,g) and (g,f) are distinct.

    bddp cached = bddrcache(cache_op, f, g);
    if (cached != bddnull) return cached;

    bddvar f_var = f_term ? 0 : node_var(f);
    bddvar g_var = g_term ? 0 : node_var(g);
    bddvar f_level = f_term ? 0 : var2level[f_var];
    bddvar g_level = g_term ? 0 : var2level[g_var];

    bddvar top_var;
    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        top_var = f_var;
        f_lo = node_lo(f); f_hi = node_hi(f);
        g_lo = g; g_hi = g;  // BDD: pass through both
    } else if (g_level > f_level) {
        top_var = g_var;
        f_lo = f; f_hi = f;
        g_lo = node_lo(g); g_hi = node_hi(g);
    } else {
        top_var = f_var;
        f_lo = node_lo(f); f_hi = node_hi(f);
        g_lo = node_lo(g); g_hi = node_hi(g);
    }

    bddp lo = mtbdd_apply_rec<T, BinOp>(f_lo, g_lo, op, cache_op);
    bddp hi = mtbdd_apply_rec<T, BinOp>(f_hi, g_hi, op, cache_op);

    bddp result = mtbdd_getnode_raw(top_var, lo, hi);
    bddwcache(cache_op, f, g, result);
    return result;
}

// Iterative counterpart to mtbdd_apply_rec. Explicit-stack implementation
// for stack-safety on deep MTBDDs. Shares cache with _rec via cache_op.
// PRECONDITION: caller holds a bdd_gc_guard scope.
template<typename T, typename BinOp>
static bddp mtbdd_apply_iter(bddp root_f, bddp root_g, BinOp& op, uint8_t cache_op) {
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

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root_f;
    init.g = root_g;
    init.top_var = 0;
    init.f_hi = 0;
    init.g_hi = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            bddp g = frame.g;
            bool f_term = (f & BDD_CONST_FLAG) != 0;
            bool g_term = (g & BDD_CONST_FLAG) != 0;

            if (f_term && g_term) {
                auto& table = MTBDDTerminalTable<T>::instance();
                T result_val = op(
                    table.get_value(MTBDDTerminalTable<T>::terminal_index(f)),
                    table.get_value(MTBDDTerminalTable<T>::terminal_index(g)));
                result = MTBDDTerminalTable<T>::make_terminal(
                    table.get_or_insert(result_val));
                stack.pop_back();
                break;
            }

            bddp cached = bddrcache(cache_op, f, g);
            if (cached != bddnull) {
                result = cached;
                stack.pop_back();
                break;
            }

            bddvar f_var = f_term ? 0 : node_var(f);
            bddvar g_var = g_term ? 0 : node_var(g);
            bddvar f_level = f_term ? 0 : var2level[f_var];
            bddvar g_level = g_term ? 0 : var2level[g_var];

            bddvar top_var;
            bddp f_lo, f_hi, g_lo, g_hi;

            if (f_level > g_level) {
                top_var = f_var;
                f_lo = node_lo(f); f_hi = node_hi(f);
                g_lo = g; g_hi = g;
            } else if (g_level > f_level) {
                top_var = g_var;
                f_lo = f; f_hi = f;
                g_lo = node_lo(g); g_hi = node_hi(g);
            } else {
                top_var = f_var;
                f_lo = node_lo(f); f_hi = node_hi(f);
                g_lo = node_lo(g); g_hi = node_hi(g);
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
            bddp hi = result;
            bddp r = mtbdd_getnode_raw(frame.top_var, frame.lo_result, hi);
            bddwcache(cache_op, frame.f, frame.g, r);
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// --- Apply templates (ZDD cofactoring) ---

template<typename T, typename BinOp>
static bddp mtzdd_apply_rec(bddp f, bddp g, BinOp& op, uint8_t cache_op) {
    BDD_RecurGuard guard;

    bool f_term = (f & BDD_CONST_FLAG) != 0;
    bool g_term = (g & BDD_CONST_FLAG) != 0;

    if (f_term && g_term) {
        auto& table = MTBDDTerminalTable<T>::instance();
        T result_val = op(
            table.get_value(MTBDDTerminalTable<T>::terminal_index(f)),
            table.get_value(MTBDDTerminalTable<T>::terminal_index(g)));
        return MTBDDTerminalTable<T>::make_terminal(
            table.get_or_insert(result_val));
    }

    // Do NOT normalize operand order — the operation may be non-commutative
    // (e.g. subtraction). Cache entries for (f,g) and (g,f) are distinct.

    bddp cached = bddrcache(cache_op, f, g);
    if (cached != bddnull) return cached;

    bddvar f_var = f_term ? 0 : node_var(f);
    bddvar g_var = g_term ? 0 : node_var(g);
    bddvar f_level = f_term ? 0 : var2level[f_var];
    bddvar g_level = g_term ? 0 : var2level[g_var];

    bddp zero_t = BDD_CONST_FLAG | 0;
    bddvar top_var;
    bddp f_lo, f_hi, g_lo, g_hi;

    if (f_level > g_level) {
        top_var = f_var;
        f_lo = node_lo(f); f_hi = node_hi(f);
        g_lo = g; g_hi = zero_t;  // ZDD: hi of missing var is zero
    } else if (g_level > f_level) {
        top_var = g_var;
        f_lo = f; f_hi = zero_t;
        g_lo = node_lo(g); g_hi = node_hi(g);
    } else {
        top_var = f_var;
        f_lo = node_lo(f); f_hi = node_hi(f);
        g_lo = node_lo(g); g_hi = node_hi(g);
    }

    bddp lo = mtzdd_apply_rec<T, BinOp>(f_lo, g_lo, op, cache_op);
    bddp hi = mtzdd_apply_rec<T, BinOp>(f_hi, g_hi, op, cache_op);

    bddp result = mtzdd_getnode_raw(top_var, lo, hi);
    bddwcache(cache_op, f, g, result);
    return result;
}

// Iterative counterpart to mtzdd_apply_rec.
// PRECONDITION: caller holds a bdd_gc_guard scope.
template<typename T, typename BinOp>
static bddp mtzdd_apply_iter(bddp root_f, bddp root_g, BinOp& op, uint8_t cache_op) {
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

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root_f;
    init.g = root_g;
    init.top_var = 0;
    init.f_hi = 0;
    init.g_hi = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            bddp g = frame.g;
            bool f_term = (f & BDD_CONST_FLAG) != 0;
            bool g_term = (g & BDD_CONST_FLAG) != 0;

            if (f_term && g_term) {
                auto& table = MTBDDTerminalTable<T>::instance();
                T result_val = op(
                    table.get_value(MTBDDTerminalTable<T>::terminal_index(f)),
                    table.get_value(MTBDDTerminalTable<T>::terminal_index(g)));
                result = MTBDDTerminalTable<T>::make_terminal(
                    table.get_or_insert(result_val));
                stack.pop_back();
                break;
            }

            bddp cached = bddrcache(cache_op, f, g);
            if (cached != bddnull) {
                result = cached;
                stack.pop_back();
                break;
            }

            bddvar f_var = f_term ? 0 : node_var(f);
            bddvar g_var = g_term ? 0 : node_var(g);
            bddvar f_level = f_term ? 0 : var2level[f_var];
            bddvar g_level = g_term ? 0 : var2level[g_var];

            bddp zero_t = BDD_CONST_FLAG | 0;
            bddvar top_var;
            bddp f_lo, f_hi, g_lo, g_hi;

            if (f_level > g_level) {
                top_var = f_var;
                f_lo = node_lo(f); f_hi = node_hi(f);
                g_lo = g; g_hi = zero_t;
            } else if (g_level > f_level) {
                top_var = g_var;
                f_lo = f; f_hi = zero_t;
                g_lo = node_lo(g); g_hi = node_hi(g);
            } else {
                top_var = f_var;
                f_lo = node_lo(f); f_hi = node_hi(f);
                g_lo = node_lo(g); g_hi = node_hi(g);
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
            bddp hi = result;
            bddp r = mtzdd_getnode_raw(frame.top_var, frame.lo_result, hi);
            bddwcache(cache_op, frame.f, frame.g, r);
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// --- ITE templates ---

template<typename T>
static bddp mtbdd_ite_rec(bddp f, bddp g, bddp h, uint8_t cache_op) {
    BDD_RecurGuard guard;

    bool f_term = (f & BDD_CONST_FLAG) != 0;

    // Terminal cases: f is a terminal
    if (f_term) {
        auto& table = MTBDDTerminalTable<T>::instance();
        T f_val = table.get_value(MTBDDTerminalTable<T>::terminal_index(f));
        return (f_val == T{}) ? h : g;
    }

    if (g == h) return g;

    bddp cached = bddrcache3(cache_op, f, g, h);
    if (cached != bddnull) return cached;

    // f is non-terminal MTBDD (no complement edges)
    bool g_term = (g & BDD_CONST_FLAG) != 0;
    bool h_term = (h & BDD_CONST_FLAG) != 0;

    bddvar f_var = node_var(f);
    bddvar g_var = g_term ? 0 : node_var(g);
    bddvar h_var = h_term ? 0 : node_var(h);
    bddvar f_level = var2level[f_var];
    bddvar g_level = g_term ? 0 : var2level[g_var];
    bddvar h_level = h_term ? 0 : var2level[h_var];

    bddvar top_level = f_level;
    if (g_level > top_level) top_level = g_level;
    if (h_level > top_level) top_level = h_level;
    bddvar top_var = (f_level == top_level) ? f_var :
                     (g_level == top_level) ? g_var : h_var;

    // Cofactors (BDD semantics: missing var → pass through)
    bddp f_lo, f_hi;
    if (f_level == top_level) {
        f_lo = node_lo(f); f_hi = node_hi(f);
    } else {
        f_lo = f; f_hi = f;
    }

    bddp g_lo, g_hi;
    if (g_level == top_level) {
        g_lo = node_lo(g); g_hi = node_hi(g);
    } else {
        g_lo = g; g_hi = g;
    }

    bddp h_lo, h_hi;
    if (h_level == top_level) {
        h_lo = node_lo(h); h_hi = node_hi(h);
    } else {
        h_lo = h; h_hi = h;
    }

    bddp lo = mtbdd_ite_rec<T>(f_lo, g_lo, h_lo, cache_op);
    bddp hi = mtbdd_ite_rec<T>(f_hi, g_hi, h_hi, cache_op);

    bddp result = mtbdd_getnode_raw(top_var, lo, hi);
    bddwcache3(cache_op, f, g, h, result);
    return result;
}

// Iterative counterpart to mtbdd_ite_rec (BDD cofactoring, 3-operand).
// PRECONDITION: caller holds a bdd_gc_guard scope.
template<typename T>
static bddp mtbdd_ite_iter(bddp root_f, bddp root_g, bddp root_h, uint8_t cache_op) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f, g, h;
        bddvar top_var;
        bddp f_hi, g_hi, h_hi;
        bddp lo_result;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root_f; init.g = root_g; init.h = root_h;
    init.top_var = 0;
    init.f_hi = 0; init.g_hi = 0; init.h_hi = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            bddp g = frame.g;
            bddp h = frame.h;
            bool f_term = (f & BDD_CONST_FLAG) != 0;

            if (f_term) {
                auto& table = MTBDDTerminalTable<T>::instance();
                T f_val = table.get_value(MTBDDTerminalTable<T>::terminal_index(f));
                result = (f_val == T{}) ? h : g;
                stack.pop_back();
                break;
            }
            if (g == h) { result = g; stack.pop_back(); break; }

            bddp cached = bddrcache3(cache_op, f, g, h);
            if (cached != bddnull) {
                result = cached;
                stack.pop_back();
                break;
            }

            bool g_term = (g & BDD_CONST_FLAG) != 0;
            bool h_term = (h & BDD_CONST_FLAG) != 0;

            bddvar f_var = node_var(f);
            bddvar g_var = g_term ? 0 : node_var(g);
            bddvar h_var = h_term ? 0 : node_var(h);
            bddvar f_level = var2level[f_var];
            bddvar g_level = g_term ? 0 : var2level[g_var];
            bddvar h_level = h_term ? 0 : var2level[h_var];

            bddvar top_level = f_level;
            if (g_level > top_level) top_level = g_level;
            if (h_level > top_level) top_level = h_level;
            bddvar top_var = (f_level == top_level) ? f_var :
                             (g_level == top_level) ? g_var : h_var;

            bddp f_lo, f_hi;
            if (f_level == top_level) {
                f_lo = node_lo(f); f_hi = node_hi(f);
            } else {
                f_lo = f; f_hi = f;
            }

            bddp g_lo, g_hi;
            if (g_level == top_level) {
                g_lo = node_lo(g); g_hi = node_hi(g);
            } else {
                g_lo = g; g_hi = g;
            }

            bddp h_lo, h_hi;
            if (h_level == top_level) {
                h_lo = node_lo(h); h_hi = node_hi(h);
            } else {
                h_lo = h; h_hi = h;
            }

            frame.top_var = top_var;
            frame.f_hi = f_hi;
            frame.g_hi = g_hi;
            frame.h_hi = h_hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = f_lo; child.g = g_lo; child.h = h_lo;
            child.top_var = 0;
            child.f_hi = 0; child.g_hi = 0; child.h_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = frame.f_hi; child.g = frame.g_hi; child.h = frame.h_hi;
            child.top_var = 0;
            child.f_hi = 0; child.g_hi = 0; child.h_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp hi = result;
            bddp r = mtbdd_getnode_raw(frame.top_var, frame.lo_result, hi);
            bddwcache3(cache_op, frame.f, frame.g, frame.h, r);
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

template<typename T>
static bddp mtzdd_ite_rec(bddp f, bddp g, bddp h, uint8_t cache_op) {
    BDD_RecurGuard guard;

    bool f_term = (f & BDD_CONST_FLAG) != 0;

    // Terminal fast path for condition
    if (f_term) {
        auto& table = MTBDDTerminalTable<T>::instance();
        T f_val = table.get_value(MTBDDTerminalTable<T>::terminal_index(f));
        if (f_val == T{}) return h;  // zero condition → always else
        // Non-zero terminal condition: shortcircuit only if g and h are
        // also terminals (no zero-suppressed variables to handle)
        if ((g & BDD_CONST_FLAG) && (h & BDD_CONST_FLAG)) return g;
        // Otherwise fall through: g/h may have zero-suppressed variables
        // where the condition should evaluate to zero.
    }

    if (g == h) return g;

    bddp cached = bddrcache3(cache_op, f, g, h);
    if (cached != bddnull) return cached;

    bool g_term = (g & BDD_CONST_FLAG) != 0;
    bool h_term = (h & BDD_CONST_FLAG) != 0;

    bddvar f_var = f_term ? 0 : node_var(f);
    bddvar g_var = g_term ? 0 : node_var(g);
    bddvar h_var = h_term ? 0 : node_var(h);
    bddvar f_level = f_term ? 0 : var2level[f_var];
    bddvar g_level = g_term ? 0 : var2level[g_var];
    bddvar h_level = h_term ? 0 : var2level[h_var];

    bddvar top_level = f_level;
    if (g_level > top_level) top_level = g_level;
    if (h_level > top_level) top_level = h_level;
    bddvar top_var = (f_level == top_level) ? f_var :
                     (g_level == top_level) ? g_var : h_var;

    bddp zero_t = BDD_CONST_FLAG | 0;

    // Cofactors: all three (f, g, h) use ZDD semantics
    bddp f_lo, f_hi;
    if (f_term) {
        f_lo = f; f_hi = zero_t;  // terminal condition, missing var → zero
    } else if (f_level == top_level) {
        f_lo = node_lo(f); f_hi = node_hi(f);
    } else {
        f_lo = f; f_hi = zero_t;  // ZDD: missing var → hi=zero
    }

    bddp g_lo, g_hi;
    if (g_level == top_level) {
        g_lo = node_lo(g); g_hi = node_hi(g);
    } else {
        g_lo = g; g_hi = zero_t;  // ZDD: missing var → hi=zero
    }

    bddp h_lo, h_hi;
    if (h_level == top_level) {
        h_lo = node_lo(h); h_hi = node_hi(h);
    } else {
        h_lo = h; h_hi = zero_t;
    }

    bddp lo = mtzdd_ite_rec<T>(f_lo, g_lo, h_lo, cache_op);
    bddp hi = mtzdd_ite_rec<T>(f_hi, g_hi, h_hi, cache_op);

    bddp result = mtzdd_getnode_raw(top_var, lo, hi);
    bddwcache3(cache_op, f, g, h, result);
    return result;
}

// Iterative counterpart to mtzdd_ite_rec (ZDD cofactoring, 3-operand).
// PRECONDITION: caller holds a bdd_gc_guard scope.
template<typename T>
static bddp mtzdd_ite_iter(bddp root_f, bddp root_g, bddp root_h, uint8_t cache_op) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f, g, h;
        bddvar top_var;
        bddp f_hi, g_hi, h_hi;
        bddp lo_result;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root_f; init.g = root_g; init.h = root_h;
    init.top_var = 0;
    init.f_hi = 0; init.g_hi = 0; init.h_hi = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            bddp g = frame.g;
            bddp h = frame.h;
            bool f_term = (f & BDD_CONST_FLAG) != 0;

            if (f_term) {
                auto& table = MTBDDTerminalTable<T>::instance();
                T f_val = table.get_value(MTBDDTerminalTable<T>::terminal_index(f));
                if (f_val == T{}) { result = h; stack.pop_back(); break; }
                if ((g & BDD_CONST_FLAG) && (h & BDD_CONST_FLAG)) {
                    result = g;
                    stack.pop_back();
                    break;
                }
                // Fall through: g/h may have zero-suppressed variables
            }
            if (g == h) { result = g; stack.pop_back(); break; }

            bddp cached = bddrcache3(cache_op, f, g, h);
            if (cached != bddnull) {
                result = cached;
                stack.pop_back();
                break;
            }

            bool g_term = (g & BDD_CONST_FLAG) != 0;
            bool h_term = (h & BDD_CONST_FLAG) != 0;

            bddvar f_var = f_term ? 0 : node_var(f);
            bddvar g_var = g_term ? 0 : node_var(g);
            bddvar h_var = h_term ? 0 : node_var(h);
            bddvar f_level = f_term ? 0 : var2level[f_var];
            bddvar g_level = g_term ? 0 : var2level[g_var];
            bddvar h_level = h_term ? 0 : var2level[h_var];

            bddvar top_level = f_level;
            if (g_level > top_level) top_level = g_level;
            if (h_level > top_level) top_level = h_level;
            bddvar top_var = (f_level == top_level) ? f_var :
                             (g_level == top_level) ? g_var : h_var;

            bddp zero_t = BDD_CONST_FLAG | 0;

            bddp f_lo, f_hi;
            if (f_term) {
                f_lo = f; f_hi = zero_t;
            } else if (f_level == top_level) {
                f_lo = node_lo(f); f_hi = node_hi(f);
            } else {
                f_lo = f; f_hi = zero_t;
            }

            bddp g_lo, g_hi;
            if (g_level == top_level) {
                g_lo = node_lo(g); g_hi = node_hi(g);
            } else {
                g_lo = g; g_hi = zero_t;
            }

            bddp h_lo, h_hi;
            if (h_level == top_level) {
                h_lo = node_lo(h); h_hi = node_hi(h);
            } else {
                h_lo = h; h_hi = zero_t;
            }

            frame.top_var = top_var;
            frame.f_hi = f_hi;
            frame.g_hi = g_hi;
            frame.h_hi = h_hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = f_lo; child.g = g_lo; child.h = h_lo;
            child.top_var = 0;
            child.f_hi = 0; child.g_hi = 0; child.h_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = frame.f_hi; child.g = frame.g_hi; child.h = frame.h_hi;
            child.top_var = 0;
            child.f_hi = 0; child.g_hi = 0; child.h_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp hi = result;
            bddp r = mtzdd_getnode_raw(frame.top_var, frame.lo_result, hi);
            bddwcache3(cache_op, frame.f, frame.g, frame.h, r);
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// --- from_bdd / from_zdd conversion templates ---

template<typename T>
static bddp mtbdd_from_bdd_rec(bddp f, bddp zero_t, bddp one_t, uint8_t cache_op) {
    BDD_RecurGuard guard;

    // Terminal cases
    if (f == bddfalse) return zero_t;
    if (f == bddtrue) return one_t;

    // Resolve complement: strip complement flag, swap terminals
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp fn = f & ~BDD_COMP_FLAG;

    bddp zt = comp ? one_t : zero_t;
    bddp ot = comp ? zero_t : one_t;

    // Cache lookup (key on non-complemented node + terminal assignment)
    bddp cached = bddrcache3(cache_op, fn, zt, ot);
    if (cached != bddnull) return cached;

    bddvar v = node_var(fn);
    bddp raw_lo = node_lo(fn);  // always non-complemented (BDD invariant)
    bddp raw_hi = node_hi(fn);  // may be complemented

    bddp mt_lo = mtbdd_from_bdd_rec<T>(raw_lo, zt, ot, cache_op);
    bddp mt_hi = mtbdd_from_bdd_rec<T>(raw_hi, zt, ot, cache_op);

    bddp result = mtbdd_getnode_raw(v, mt_lo, mt_hi);
    bddwcache3(cache_op, fn, zt, ot, result);
    return result;
}

// Iterative counterpart to mtbdd_from_bdd_rec.
// Absorbs BDD complement edges into terminal mapping (zt/ot swap).
// PRECONDITION: caller holds a bdd_gc_guard scope.
template<typename T>
static bddp mtbdd_from_bdd_iter(bddp root_f, bddp root_zero_t, bddp root_one_t, uint8_t cache_op) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;          // input edge (possibly complemented); set to fn after ENTER
        bddp zt;         // effective zero terminal
        bddp ot;         // effective one terminal
        bddvar v;
        bddp lo_result;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root_f;
    init.zt = root_zero_t;
    init.ot = root_one_t;
    init.v = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            bddp zt = frame.zt;
            bddp ot = frame.ot;

            if (f == bddfalse) { result = zt; stack.pop_back(); break; }
            if (f == bddtrue) { result = ot; stack.pop_back(); break; }

            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp fn = f & ~BDD_COMP_FLAG;
            bddp eff_zt = comp ? ot : zt;
            bddp eff_ot = comp ? zt : ot;

            bddp cached = bddrcache3(cache_op, fn, eff_zt, eff_ot);
            if (cached != bddnull) {
                result = cached;
                stack.pop_back();
                break;
            }

            bddvar v = node_var(fn);
            frame.f = fn;
            frame.zt = eff_zt;
            frame.ot = eff_ot;
            frame.v = v;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = node_lo(fn);
            child.zt = eff_zt;
            child.ot = eff_ot;
            child.v = 0;
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
            child.zt = frame.zt;
            child.ot = frame.ot;
            child.v = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp hi = result;
            bddp r = mtbdd_getnode_raw(frame.v, frame.lo_result, hi);
            bddwcache3(cache_op, frame.f, frame.zt, frame.ot, r);
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

template<typename T>
static bddp mtzdd_from_zdd_rec(bddp f, bddp zero_t, bddp one_t, uint8_t cache_op) {
    BDD_RecurGuard guard;

    if (f == bddempty) return zero_t;
    if (f == bddsingle) return one_t;

    // ZDD complement edge affects only lo, not hi.
    // Instead of absorbing complement into terminal mapping (which would
    // incorrectly apply to hi as well), propagate the complement bit
    // to the effective lo child and cache on the original (f, zero_t, one_t).
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp fn = f & ~BDD_COMP_FLAG;

    bddp cached = bddrcache3(cache_op, f, zero_t, one_t);
    if (cached != bddnull) return cached;

    bddvar v = node_var(fn);
    bddp raw_lo = node_lo(fn);  // always non-complemented (ZDD invariant)
    bddp raw_hi = node_hi(fn);  // never complemented in ZDD

    // ZDD complement: ~(v, lo, hi) = (v, ~lo, hi)
    // Toggle complement bit on lo when parent edge was complemented.
    bddp eff_lo = comp ? (raw_lo ^ BDD_COMP_FLAG) : raw_lo;

    bddp mt_lo = mtzdd_from_zdd_rec<T>(eff_lo, zero_t, one_t, cache_op);
    bddp mt_hi = mtzdd_from_zdd_rec<T>(raw_hi, zero_t, one_t, cache_op);

    bddp result = mtzdd_getnode_raw(v, mt_lo, mt_hi);
    bddwcache3(cache_op, f, zero_t, one_t, result);
    return result;
}

// Iterative counterpart to mtzdd_from_zdd_rec.
// ZDD complement is propagated (not absorbed); cache key uses original f.
// PRECONDITION: caller holds a bdd_gc_guard scope.
template<typename T>
static bddp mtzdd_from_zdd_iter(bddp root_f, bddp zero_t, bddp one_t, uint8_t cache_op) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;          // original (possibly complemented) input edge
        bddvar v;
        bddp raw_hi;     // unchanged hi child (ZDD: complement doesn't affect hi)
        bddp lo_result;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root_f;
    init.v = 0;
    init.raw_hi = 0;
    init.lo_result = bddnull;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bddp result = bddnull;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f == bddempty) { result = zero_t; stack.pop_back(); break; }
            if (f == bddsingle) { result = one_t; stack.pop_back(); break; }

            bool comp = (f & BDD_COMP_FLAG) != 0;
            bddp fn = f & ~BDD_COMP_FLAG;

            bddp cached = bddrcache3(cache_op, f, zero_t, one_t);
            if (cached != bddnull) {
                result = cached;
                stack.pop_back();
                break;
            }

            bddvar v = node_var(fn);
            bddp raw_lo = node_lo(fn);
            bddp raw_hi = node_hi(fn);
            // ZDD complement: toggle complement bit on lo only
            bddp eff_lo = comp ? (raw_lo ^ BDD_COMP_FLAG) : raw_lo;

            frame.v = v;
            frame.raw_hi = raw_hi;
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = eff_lo;
            child.v = 0;
            child.raw_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_result = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = frame.raw_hi;
            child.v = 0;
            child.raw_hi = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp hi = result;
            bddp r = mtzdd_getnode_raw(frame.v, frame.lo_result, hi);
            bddwcache3(cache_op, frame.f, zero_t, one_t, r);
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// --- MTZDD count helpers (non-zero path counting) ---
// MTZDD has no complement edges, so counting is straightforward:
// zero terminal → 0, non-zero terminal → 1, internal → count(lo) + count(hi).

static inline double mtzdd_count_rec(
    bddp f, std::unordered_map<bddp, double>& memo) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) {
        uint64_t idx = f & ~BDD_CONST_FLAG;
        return (idx == 0) ? 0.0 : 1.0;
    }

    auto it = memo.find(f);
    if (it != memo.end()) return it->second;

    bddp lo = node_lo(f);
    bddp hi = node_hi(f);
    double count = mtzdd_count_rec(lo, memo) + mtzdd_count_rec(hi, memo);
    memo[f] = count;
    return count;
}

// Iterative counterpart to mtzdd_count_rec.
static inline double mtzdd_count_iter(
    bddp root, std::unordered_map<bddp, double>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        double lo_val;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root;
    init.lo_val = 0.0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    double result = 0.0;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f & BDD_CONST_FLAG) {
                uint64_t idx = f & ~BDD_CONST_FLAG;
                result = (idx == 0) ? 0.0 : 1.0;
                stack.pop_back();
                break;
            }
            auto it = memo.find(f);
            if (it != memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = node_lo(f);
            child.lo_val = 0.0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_val = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = node_hi(frame.f);
            child.lo_val = 0.0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            double total = frame.lo_val + result;
            memo[frame.f] = total;
            result = total;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

static inline bigint::BigInt mtzdd_exact_count_rec(
    bddp f, std::unordered_map<bddp, bigint::BigInt>& memo) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) {
        uint64_t idx = f & ~BDD_CONST_FLAG;
        return (idx == 0) ? bigint::BigInt(0) : bigint::BigInt(1);
    }

    auto it = memo.find(f);
    if (it != memo.end()) return it->second;

    bddp lo = node_lo(f);
    bddp hi = node_hi(f);
    bigint::BigInt count = mtzdd_exact_count_rec(lo, memo)
                         + mtzdd_exact_count_rec(hi, memo);
    memo[f] = count;
    return count;
}

// Iterative counterpart to mtzdd_exact_count_rec.
static inline bigint::BigInt mtzdd_exact_count_iter(
    bddp root, std::unordered_map<bddp, bigint::BigInt>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bigint::BigInt lo_val;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root;
    init.lo_val = bigint::BigInt(0);
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bigint::BigInt result(0);
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f & BDD_CONST_FLAG) {
                uint64_t idx = f & ~BDD_CONST_FLAG;
                result = (idx == 0) ? bigint::BigInt(0) : bigint::BigInt(1);
                stack.pop_back();
                break;
            }
            auto it = memo.find(f);
            if (it != memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = node_lo(f);
            child.lo_val = bigint::BigInt(0);
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_val = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = node_hi(frame.f);
            child.lo_val = bigint::BigInt(0);
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bigint::BigInt total = frame.lo_val + result;
            memo[frame.f] = total;
            result = total;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

// --- MTZDD count helpers (target-specific terminal counting) ---
//
// WARNING: memo is keyed only on the node id; the target terminal is not
// part of the key. Callers MUST pass a fresh memo for each distinct target
// (or the target-specific subtree counts will be reused for unrelated
// targets, producing wrong numbers).

static inline double mtzdd_count_for_terminal_rec(
    bddp f, bddp target, std::unordered_map<bddp, double>& memo) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) {
        return (f == target) ? 1.0 : 0.0;
    }

    auto it = memo.find(f);
    if (it != memo.end()) return it->second;

    bddp lo = node_lo(f);
    bddp hi = node_hi(f);
    double count = mtzdd_count_for_terminal_rec(lo, target, memo)
                 + mtzdd_count_for_terminal_rec(hi, target, memo);
    memo[f] = count;
    return count;
}

// Iterative counterpart to mtzdd_count_for_terminal_rec.
static inline double mtzdd_count_for_terminal_iter(
    bddp root, bddp target, std::unordered_map<bddp, double>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        double lo_val;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root;
    init.lo_val = 0.0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    double result = 0.0;
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f & BDD_CONST_FLAG) {
                result = (f == target) ? 1.0 : 0.0;
                stack.pop_back();
                break;
            }
            auto it = memo.find(f);
            if (it != memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = node_lo(f);
            child.lo_val = 0.0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_val = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = node_hi(frame.f);
            child.lo_val = 0.0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            double total = frame.lo_val + result;
            memo[frame.f] = total;
            result = total;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}

static inline bigint::BigInt mtzdd_exact_count_for_terminal_rec(
    bddp f, bddp target, std::unordered_map<bddp, bigint::BigInt>& memo) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) {
        return (f == target) ? bigint::BigInt(1) : bigint::BigInt(0);
    }

    auto it = memo.find(f);
    if (it != memo.end()) return it->second;

    bddp lo = node_lo(f);
    bddp hi = node_hi(f);
    bigint::BigInt count = mtzdd_exact_count_for_terminal_rec(lo, target, memo)
                         + mtzdd_exact_count_for_terminal_rec(hi, target, memo);
    memo[f] = count;
    return count;
}

// Iterative counterpart to mtzdd_exact_count_for_terminal_rec.
static inline bigint::BigInt mtzdd_exact_count_for_terminal_iter(
    bddp root, bddp target, std::unordered_map<bddp, bigint::BigInt>& memo) {
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bigint::BigInt lo_val;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root;
    init.lo_val = bigint::BigInt(0);
    init.phase = Phase::ENTER;
    stack.push_back(init);

    bigint::BigInt result(0);
    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f & BDD_CONST_FLAG) {
                result = (f == target) ? bigint::BigInt(1) : bigint::BigInt(0);
                stack.pop_back();
                break;
            }
            auto it = memo.find(f);
            if (it != memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = node_lo(f);
            child.lo_val = bigint::BigInt(0);
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            frame.lo_val = result;
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = node_hi(frame.f);
            child.lo_val = bigint::BigInt(0);
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bigint::BigInt total = frame.lo_val + result;
            memo[frame.f] = total;
            result = total;
            stack.pop_back();
            break;
        }
        }
    }
    return result;
}


} // namespace kyotodd

#endif
