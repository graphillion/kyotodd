#ifndef KYOTODD_MTBDD_H
#define KYOTODD_MTBDD_H

#include "bdd_types.h"
#include "bdd_base.h"
#include "bdd_ops.h"
#include "bdd_internal.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include "bigint.hpp"

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
        static MTBDDTerminalTable<T>* inst = nullptr;
        static bool registered = false;
        if (inst == nullptr) {
            inst = new MTBDDTerminalTable<T>();
            registered = false;
        }
        if (!registered) {
            mtbdd_register_terminal_table(inst);
            registered = true;
        }
        return *inst;
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

// --- MTZDD count helpers (non-zero path counting) ---
// MTZDD has no complement edges, so counting is straightforward:
// zero terminal → 0, non-zero terminal → 1, internal → count(lo) + count(hi).

static inline double mtzdd_count_rec(
    bddp f, std::unordered_map<bddp, double>& memo) {
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

static inline bigint::BigInt mtzdd_exact_count_rec(
    bddp f, std::unordered_map<bddp, bigint::BigInt>& memo) {
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

// --- MTZDD count helpers (target-specific terminal counting) ---

static inline double mtzdd_count_for_terminal_rec(
    bddp f, bddp target, std::unordered_map<bddp, double>& memo) {
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

static inline bigint::BigInt mtzdd_exact_count_for_terminal_rec(
    bddp f, bddp target, std::unordered_map<bddp, bigint::BigInt>& memo) {
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

// --- MTBDD<T> class (Multi-Terminal BDD) ---

template<typename T>
class MTBDD : public DDBase {
public:
    MTBDD() : DDBase() {
        root = MTBDDTerminalTable<T>::make_terminal(
            MTBDDTerminalTable<T>::instance().zero_index());
    }

    MTBDD(const MTBDD& other) : DDBase(other) {}
    MTBDD(MTBDD&& other) : DDBase(std::move(other)) {}

    MTBDD& operator=(const MTBDD& other) {
        DDBase::operator=(other);
        return *this;
    }

    MTBDD& operator=(MTBDD&& other) {
        DDBase::operator=(std::move(other));
        return *this;
    }

    // --- Static factories ---

    static MTBDD terminal(const T& value) {
        auto& table = MTBDDTerminalTable<T>::instance();
        uint64_t idx = table.get_or_insert(value);
        return MTBDD(MTBDDTerminalTable<T>::make_terminal(idx));
    }

    static MTBDD ite(bddvar v, const MTBDD& high, const MTBDD& low) {
        MTBDD result;
        result.root = mtbdd_getnode(v, low.root, high.root);
        return result;
    }

    static MTBDD from_bdd(const BDD& bdd,
                          const T& zero_val = T{},
                          const T& one_val = T{1}) {
        auto& table = MTBDDTerminalTable<T>::instance();
        bddp zero_t = MTBDDTerminalTable<T>::make_terminal(
            table.get_or_insert(zero_val));
        bddp one_t = MTBDDTerminalTable<T>::make_terminal(
            table.get_or_insert(one_val));

        static uint8_t conv_op = mtbdd_alloc_op_code();
        MTBDD result;
        result.root = bdd_gc_guard([&]() -> bddp {
            return mtbdd_from_bdd_rec<T>(bdd.get_id(), zero_t, one_t, conv_op);
        });
        return result;
    }

    // --- Query ---

    T terminal_value() const {
        if (!(root & BDD_CONST_FLAG)) {
            throw std::logic_error("MTBDD::terminal_value: not a terminal node");
        }
        uint64_t idx = MTBDDTerminalTable<T>::terminal_index(root);
        return MTBDDTerminalTable<T>::instance().get_value(idx);
    }

    /** @brief Check if the terminal value equals T{1}. Hides DDBase::is_one(). */
    bool is_one() const {
        if (!(root & BDD_CONST_FLAG)) return false;
        uint64_t idx = MTBDDTerminalTable<T>::terminal_index(root);
        return MTBDDTerminalTable<T>::instance().get_value(idx) == T{1};
    }

    bool operator==(const MTBDD& other) const { return root == other.root; }
    bool operator!=(const MTBDD& other) const { return root != other.root; }

    // --- Evaluation ---

    T evaluate(const std::vector<int>& assignment) const {
        bddp f = root;
        while (!(f & BDD_CONST_FLAG)) {
            bddvar v = node_var(f);
            if (v >= assignment.size()) {
                throw std::invalid_argument(
                    "MTBDD::evaluate: assignment too short");
            }
            f = assignment[v] ? node_hi(f) : node_lo(f);
        }
        uint64_t idx = MTBDDTerminalTable<T>::terminal_index(f);
        return MTBDDTerminalTable<T>::instance().get_value(idx);
    }

    // --- Generic apply ---

    template<typename BinOp>
    MTBDD apply(const MTBDD& other, BinOp op) const {
        static uint8_t apply_op = mtbdd_alloc_op_code();
        MTBDD result;
        result.root = bdd_gc_guard([&]() -> bddp {
            return mtbdd_apply_rec<T, BinOp>(root, other.root, op, apply_op);
        });
        return result;
    }

    // --- Binary operators ---

    MTBDD operator+(const MTBDD& other) const {
        return apply(other, [](const T& a, const T& b) { return a + b; });
    }
    MTBDD operator-(const MTBDD& other) const {
        return apply(other, [](const T& a, const T& b) { return a - b; });
    }
    MTBDD operator*(const MTBDD& other) const {
        return apply(other, [](const T& a, const T& b) { return a * b; });
    }
    MTBDD& operator+=(const MTBDD& other) { *this = *this + other; return *this; }
    MTBDD& operator-=(const MTBDD& other) { *this = *this - other; return *this; }
    MTBDD& operator*=(const MTBDD& other) { *this = *this * other; return *this; }

    static MTBDD min(const MTBDD& a, const MTBDD& b) {
        return a.apply(b, [](const T& x, const T& y) { return x < y ? x : y; });
    }
    static MTBDD max(const MTBDD& a, const MTBDD& b) {
        return a.apply(b, [](const T& x, const T& y) { return x < y ? y : x; });
    }

    // --- ITE (3-operand: this is condition) ---

    MTBDD ite(const MTBDD& then_case, const MTBDD& else_case) const {
        // Terminal fast paths
        if (root & BDD_CONST_FLAG) {
            auto& table = MTBDDTerminalTable<T>::instance();
            T val = table.get_value(MTBDDTerminalTable<T>::terminal_index(root));
            return (val == T{}) ? else_case : then_case;
        }
        if (then_case.root == else_case.root) return then_case;

        static uint8_t ite_op = mtbdd_alloc_op_code();
        MTBDD result;
        result.root = bdd_gc_guard([&]() -> bddp {
            return mtbdd_ite_rec<T>(root, then_case.root, else_case.root, ite_op);
        });
        return result;
    }

    // --- Terminal table access ---

    static MTBDDTerminalTable<T>& terminals() {
        return MTBDDTerminalTable<T>::instance();
    }

    static bddp zero_terminal() {
        return MTBDDTerminalTable<T>::make_terminal(0);
    }

    // --- Binary export/import ---
    void export_binary(std::ostream& strm) const;
    void export_binary(const char* filename) const;
    static MTBDD import_binary(std::istream& strm);
    static MTBDD import_binary(const char* filename);

    // --- SVG export ---
    void save_svg(const char* filename, const SvgParams& params) const;
    void save_svg(const char* filename) const;
    void save_svg(std::ostream& strm, const SvgParams& params) const;
    void save_svg(std::ostream& strm) const;
    std::string save_svg(const SvgParams& params) const;
    std::string save_svg() const;

private:
    explicit MTBDD(bddp p) : DDBase() {
        root = p;
    }

    template<typename T2, typename B> friend bddp mtbdd_apply_rec(bddp, bddp, B&, uint8_t);
};

template<typename T>
using ADD = MTBDD<T>;

// --- MTZDD enumerate helper ---

template<typename T>
static void mtzdd_enumerate_rec(
    bddp f,
    std::vector<bddvar>& current,
    std::vector<std::pair<std::vector<bddvar>, T> >& result)
{
    if (f & BDD_CONST_FLAG) {
        uint64_t idx = MTBDDTerminalTable<T>::terminal_index(f);
        T val = MTBDDTerminalTable<T>::instance().get_value(idx);
        if (val != T{}) {
            result.push_back(std::make_pair(current, val));
        }
        return;
    }
    bddvar var = node_var(f);
    mtzdd_enumerate_rec<T>(node_lo(f), current, result);
    current.push_back(var);
    mtzdd_enumerate_rec<T>(node_hi(f), current, result);
    current.pop_back();
}

// --- MTZDD to ZDD threshold conversion helper ---

template<typename T>
static bddp mtzdd_to_zdd_rec(
    bddp f,
    const std::unordered_set<bddp>& pred_terminals,
    uint8_t cache_op)
{
    BDD_RecurGuard guard;

    if (f & BDD_CONST_FLAG) {
        return pred_terminals.count(f) ? bddsingle : bddempty;
    }

    bddp cached = bddrcache(cache_op, f, 0);
    if (cached != bddnull) return cached;

    bddvar var = node_var(f);
    bddp lo = mtzdd_to_zdd_rec<T>(node_lo(f), pred_terminals, cache_op);
    bddp hi = mtzdd_to_zdd_rec<T>(node_hi(f), pred_terminals, cache_op);
    bddp result = ZDD::getnode_raw(var, lo, hi);

    bddwcache(cache_op, f, 0, result);
    return result;
}

// --- MTZDD<T> class (Multi-Terminal ZDD) ---

template<typename T>
class MTZDD : public DDBase {
public:
    MTZDD() : DDBase() {
        root = MTBDDTerminalTable<T>::make_terminal(
            MTBDDTerminalTable<T>::instance().zero_index());
    }

    MTZDD(const MTZDD& other) : DDBase(other) {}
    MTZDD(MTZDD&& other) : DDBase(std::move(other)) {}

    MTZDD& operator=(const MTZDD& other) {
        DDBase::operator=(other);
        return *this;
    }

    MTZDD& operator=(MTZDD&& other) {
        DDBase::operator=(std::move(other));
        return *this;
    }

    // --- Static factories ---

    static MTZDD terminal(const T& value) {
        auto& table = MTBDDTerminalTable<T>::instance();
        uint64_t idx = table.get_or_insert(value);
        return MTZDD(MTBDDTerminalTable<T>::make_terminal(idx));
    }

    static MTZDD ite(bddvar v, const MTZDD& high, const MTZDD& low) {
        MTZDD result;
        result.root = mtzdd_getnode(v, low.root, high.root);
        return result;
    }

    static MTZDD from_zdd(const ZDD& zdd,
                          const T& zero_val = T{},
                          const T& one_val = T{1}) {
        auto& table = MTBDDTerminalTable<T>::instance();
        bddp zero_t = MTBDDTerminalTable<T>::make_terminal(
            table.get_or_insert(zero_val));
        bddp one_t = MTBDDTerminalTable<T>::make_terminal(
            table.get_or_insert(one_val));

        static uint8_t conv_op = mtbdd_alloc_op_code();
        MTZDD result;
        result.root = bdd_gc_guard([&]() -> bddp {
            return mtzdd_from_zdd_rec<T>(zdd.get_id(), zero_t, one_t, conv_op);
        });
        return result;
    }

    // --- Query ---

    T terminal_value() const {
        if (!(root & BDD_CONST_FLAG)) {
            throw std::logic_error("MTZDD::terminal_value: not a terminal node");
        }
        uint64_t idx = MTBDDTerminalTable<T>::terminal_index(root);
        return MTBDDTerminalTable<T>::instance().get_value(idx);
    }

    /** @brief Check if the terminal value equals T{1}. Hides DDBase::is_one(). */
    bool is_one() const {
        if (!(root & BDD_CONST_FLAG)) return false;
        uint64_t idx = MTBDDTerminalTable<T>::terminal_index(root);
        return MTBDDTerminalTable<T>::instance().get_value(idx) == T{1};
    }

    bool operator==(const MTZDD& other) const { return root == other.root; }
    bool operator!=(const MTZDD& other) const { return root != other.root; }

    // --- Cofactor ---

    /** @brief Fix variable v to 0. Returns sub-MTZDD for v=0. */
    MTZDD cofactor0(bddvar v) const {
        return MTZDD(mtzdd_cofactor0(root, v));
    }

    /** @brief Fix variable v to 1. Returns sub-MTZDD for v=1. */
    MTZDD cofactor1(bddvar v) const {
        return MTZDD(mtzdd_cofactor1(root, v));
    }

    // --- Empty assignment check ---

    /** @brief Check if the empty assignment (all variables = 0) maps to a non-zero value. */
    bool has_empty() const {
        bddp f = root;
        while (!(f & BDD_CONST_FLAG)) {
            f = node_lo(f);
        }
        uint64_t idx = MTBDDTerminalTable<T>::terminal_index(f);
        return MTBDDTerminalTable<T>::instance().get_value(idx) != T{};
    }

    // --- Support variables ---

    /** @brief Return all variable numbers appearing in the MTZDD, sorted by level (highest first). */
    std::vector<bddvar> support_vars() const {
        return bddsupport_vec(root);
    }

    // --- Threshold / ZDD conversion ---

    /** @brief Extract paths where terminal value > val, as ZDD. */
    ZDD threshold_gt(const T& val) const {
        auto& table = MTBDDTerminalTable<T>::instance();
        std::unordered_set<bddp> pred_terminals;
        for (uint64_t i = 0; i < table.size(); ++i) {
            if (table.get_value(i) > val) {
                pred_terminals.insert(MTBDDTerminalTable<T>::make_terminal(i));
            }
        }
        if (pred_terminals.empty()) return ZDD(0);
        static uint8_t op = mtbdd_alloc_op_code();
        bddp result = bdd_gc_guard([&]() -> bddp {
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, op);
        });
        return ZDD_ID(result);
    }

    /** @brief Extract paths where terminal value >= val, as ZDD. */
    ZDD threshold_ge(const T& val) const {
        auto& table = MTBDDTerminalTable<T>::instance();
        std::unordered_set<bddp> pred_terminals;
        for (uint64_t i = 0; i < table.size(); ++i) {
            if (!(table.get_value(i) < val)) {
                pred_terminals.insert(MTBDDTerminalTable<T>::make_terminal(i));
            }
        }
        if (pred_terminals.empty()) return ZDD(0);
        static uint8_t op = mtbdd_alloc_op_code();
        bddp result = bdd_gc_guard([&]() -> bddp {
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, op);
        });
        return ZDD_ID(result);
    }

    /** @brief Extract paths where terminal value == val, as ZDD. */
    ZDD threshold_eq(const T& val) const {
        auto& table = MTBDDTerminalTable<T>::instance();
        std::unordered_set<bddp> pred_terminals;
        for (uint64_t i = 0; i < table.size(); ++i) {
            if (table.get_value(i) == val) {
                pred_terminals.insert(MTBDDTerminalTable<T>::make_terminal(i));
            }
        }
        if (pred_terminals.empty()) return ZDD(0);
        static uint8_t op = mtbdd_alloc_op_code();
        bddp result = bdd_gc_guard([&]() -> bddp {
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, op);
        });
        return ZDD_ID(result);
    }

    /** @brief Extract paths where terminal value < val, as ZDD. */
    ZDD threshold_lt(const T& val) const {
        auto& table = MTBDDTerminalTable<T>::instance();
        std::unordered_set<bddp> pred_terminals;
        for (uint64_t i = 0; i < table.size(); ++i) {
            if (table.get_value(i) < val) {
                pred_terminals.insert(MTBDDTerminalTable<T>::make_terminal(i));
            }
        }
        if (pred_terminals.empty()) return ZDD(0);
        static uint8_t op = mtbdd_alloc_op_code();
        bddp result = bdd_gc_guard([&]() -> bddp {
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, op);
        });
        return ZDD_ID(result);
    }

    /** @brief Extract paths where terminal value <= val, as ZDD. */
    ZDD threshold_le(const T& val) const {
        auto& table = MTBDDTerminalTable<T>::instance();
        std::unordered_set<bddp> pred_terminals;
        for (uint64_t i = 0; i < table.size(); ++i) {
            if (!(table.get_value(i) > val)) {
                pred_terminals.insert(MTBDDTerminalTable<T>::make_terminal(i));
            }
        }
        if (pred_terminals.empty()) return ZDD(0);
        static uint8_t op = mtbdd_alloc_op_code();
        bddp result = bdd_gc_guard([&]() -> bddp {
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, op);
        });
        return ZDD_ID(result);
    }

    /** @brief Extract paths where terminal value != val, as ZDD. */
    ZDD threshold_ne(const T& val) const {
        auto& table = MTBDDTerminalTable<T>::instance();
        std::unordered_set<bddp> pred_terminals;
        for (uint64_t i = 0; i < table.size(); ++i) {
            if (!(table.get_value(i) == val)) {
                pred_terminals.insert(MTBDDTerminalTable<T>::make_terminal(i));
            }
        }
        if (pred_terminals.empty()) return ZDD(0);
        static uint8_t op = mtbdd_alloc_op_code();
        bddp result = bdd_gc_guard([&]() -> bddp {
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, op);
        });
        return ZDD_ID(result);
    }

    /** @brief Extract all non-zero paths as ZDD. Equivalent to threshold_ne(T{}). */
    ZDD to_zdd() const {
        auto& table = MTBDDTerminalTable<T>::instance();
        std::unordered_set<bddp> pred_terminals;
        for (uint64_t i = 1; i < table.size(); ++i) {
            pred_terminals.insert(MTBDDTerminalTable<T>::make_terminal(i));
        }
        if (pred_terminals.empty()) return ZDD(0);
        static uint8_t op = mtbdd_alloc_op_code();
        bddp result = bdd_gc_guard([&]() -> bddp {
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, op);
        });
        return ZDD_ID(result);
    }

    // --- Enumeration ---

    /** @brief Enumerate all non-zero paths as (variable set, terminal value) pairs. */
    std::vector<std::pair<std::vector<bddvar>, T> > enumerate() const {
        std::vector<std::pair<std::vector<bddvar>, T> > result;
        std::vector<bddvar> current;
        mtzdd_enumerate_rec<T>(root, current, result);
        for (size_t i = 0; i < result.size(); ++i) {
            std::sort(result[i].first.begin(), result[i].first.end());
        }
        return result;
    }

    // --- Display ---

    /** @brief Print all non-zero paths to the stream. */
    void print_sets(std::ostream& os) const;

    /** @brief Print all non-zero paths with custom variable names. */
    void print_sets(std::ostream& os,
                    const std::vector<std::string>& var_name_map) const;

    /** @brief Return a string representation of all non-zero paths. */
    std::string to_str() const;

    // --- Counting (non-zero paths) ---

    /** @brief Count the number of non-zero terminal paths (double). */
    double count() const {
        std::unordered_map<bddp, double> memo;
        return mtzdd_count_rec(root, memo);
    }

    /** @brief Count the number of non-zero terminal paths (exact BigInt). */
    bigint::BigInt exact_count() const {
        std::unordered_map<bddp, bigint::BigInt> memo;
        return mtzdd_exact_count_rec(root, memo);
    }

    /** @brief Count paths leading to the specific terminal value (double). */
    double count(const T& terminal) const {
        auto& table = MTBDDTerminalTable<T>::instance();
        uint64_t idx;
        if (!table.find_index(terminal, idx)) return 0.0;
        bddp target = MTBDDTerminalTable<T>::make_terminal(idx);
        std::unordered_map<bddp, double> memo;
        return mtzdd_count_for_terminal_rec(root, target, memo);
    }

    /** @brief Count paths leading to the specific terminal value (exact BigInt). */
    bigint::BigInt exact_count(const T& terminal) const {
        auto& table = MTBDDTerminalTable<T>::instance();
        uint64_t idx;
        if (!table.find_index(terminal, idx)) return bigint::BigInt(0);
        bddp target = MTBDDTerminalTable<T>::make_terminal(idx);
        std::unordered_map<bddp, bigint::BigInt> memo;
        return mtzdd_exact_count_for_terminal_rec(root, target, memo);
    }

    // --- Generic apply ---

    template<typename BinOp>
    MTZDD apply(const MTZDD& other, BinOp op) const {
        static uint8_t apply_op = mtbdd_alloc_op_code();
        MTZDD result;
        result.root = bdd_gc_guard([&]() -> bddp {
            return mtzdd_apply_rec<T, BinOp>(root, other.root, op, apply_op);
        });
        return result;
    }

    // --- Binary operators ---

    MTZDD operator+(const MTZDD& other) const {
        return apply(other, [](const T& a, const T& b) { return a + b; });
    }
    MTZDD operator-(const MTZDD& other) const {
        return apply(other, [](const T& a, const T& b) { return a - b; });
    }
    MTZDD operator*(const MTZDD& other) const {
        return apply(other, [](const T& a, const T& b) { return a * b; });
    }
    MTZDD& operator+=(const MTZDD& other) { *this = *this + other; return *this; }
    MTZDD& operator-=(const MTZDD& other) { *this = *this - other; return *this; }
    MTZDD& operator*=(const MTZDD& other) { *this = *this * other; return *this; }

    static MTZDD min(const MTZDD& a, const MTZDD& b) {
        return a.apply(b, [](const T& x, const T& y) { return x < y ? x : y; });
    }
    static MTZDD max(const MTZDD& a, const MTZDD& b) {
        return a.apply(b, [](const T& x, const T& y) { return x < y ? y : x; });
    }

    // --- ITE (3-operand: this is condition) ---

    MTZDD ite(const MTZDD& then_case, const MTZDD& else_case) const {
        if (root & BDD_CONST_FLAG) {
            auto& table = MTBDDTerminalTable<T>::instance();
            T val = table.get_value(MTBDDTerminalTable<T>::terminal_index(root));
            if (val == T{}) return else_case;
            // Non-zero terminal: shortcircuit only if both branches are terminals
            // (no zero-suppressed variables to handle in g/h)
            if ((then_case.root & BDD_CONST_FLAG) &&
                (else_case.root & BDD_CONST_FLAG))
                return then_case;
            // Fall through: g/h may have zero-suppressed variables
        }
        if (then_case.root == else_case.root) return then_case;

        static uint8_t ite_op = mtbdd_alloc_op_code();
        MTZDD result;
        result.root = bdd_gc_guard([&]() -> bddp {
            return mtzdd_ite_rec<T>(root, then_case.root, else_case.root, ite_op);
        });
        return result;
    }

    // --- Evaluation ---

    T evaluate(const std::vector<int>& assignment) const {
        auto& table = MTBDDTerminalTable<T>::instance();
        bddp f = root;
        bddvar num_vars = bddvarused();

        // Start from the highest level to check ALL zero-suppressed
        // variables, including those above the root node's level.
        bddvar top_level = num_vars;

        // Traverse from top level down to level 1.
        // Even after reaching a terminal, continue checking remaining
        // levels for zero-suppressed variables set to 1.
        for (bddvar lev = top_level; lev >= 1; --lev) {
            bddvar v = level2var[lev];

            if (f & BDD_CONST_FLAG) {
                // Already at a terminal, but check zero-suppressed vars
                if (v < assignment.size() && assignment[v]) {
                    return table.get_value(0);  // T{} (zero value)
                }
                continue;
            }

            bddvar f_var = node_var(f);

            if (f_var == v) {
                // Variable is present in the MTZDD
                if (v >= assignment.size()) {
                    throw std::invalid_argument(
                        "MTZDD::evaluate: assignment too short");
                }
                f = assignment[v] ? node_hi(f) : node_lo(f);
            } else {
                // Variable v is zero-suppressed (not in MTZDD)
                // Implicit hi = zero_terminal
                if (v < assignment.size() && assignment[v]) {
                    return table.get_value(0);  // T{} (zero value)
                }
                // assignment[v] == 0: follow implicit lo (= f unchanged)
            }
        }

        if (!(f & BDD_CONST_FLAG)) {
            throw std::logic_error(
                "MTZDD::evaluate: non-terminal after full traversal");
        }
        uint64_t idx = MTBDDTerminalTable<T>::terminal_index(f);
        return table.get_value(idx);
    }

    // --- Terminal table access ---

    static MTBDDTerminalTable<T>& terminals() {
        return MTBDDTerminalTable<T>::instance();
    }

    static bddp zero_terminal() {
        return MTBDDTerminalTable<T>::make_terminal(0);
    }

    // --- Binary export/import ---
    void export_binary(std::ostream& strm) const;
    void export_binary(const char* filename) const;
    static MTZDD import_binary(std::istream& strm);
    static MTZDD import_binary(const char* filename);

    // --- SVG export ---
    void save_svg(const char* filename, const SvgParams& params) const;
    void save_svg(const char* filename) const;
    void save_svg(std::ostream& strm, const SvgParams& params) const;
    void save_svg(std::ostream& strm) const;
    std::string save_svg(const SvgParams& params) const;
    std::string save_svg() const;

private:
    explicit MTZDD(bddp p) : DDBase() {
        root = p;
    }

    template<typename T2, typename B> friend bddp mtzdd_apply_rec(bddp, bddp, B&, uint8_t);
};

// ========================================================================
//  MTBDD/MTZDD binary export/import inline implementations
// ========================================================================

#include <cstring>
#include <fstream>
#include <algorithm>
#include <unordered_set>

namespace mtbdd_binary_detail {

inline void mb_encode_le16(uint8_t* buf, uint16_t v) {
    buf[0] = v & 0xFF; buf[1] = (v >> 8) & 0xFF;
}
inline void mb_encode_le32(uint8_t* buf, uint32_t v) {
    for (int i = 0; i < 4; i++) buf[i] = (v >> (8*i)) & 0xFF;
}
inline void mb_encode_le64(uint8_t* buf, uint64_t v) {
    for (int i = 0; i < 8; i++) buf[i] = (v >> (8*i)) & 0xFF;
}
inline uint32_t mb_decode_le32(const uint8_t* buf) {
    uint32_t v = 0;
    for (int i = 0; i < 4; i++) v |= static_cast<uint32_t>(buf[i]) << (8*i);
    return v;
}
inline uint64_t mb_decode_le64(const uint8_t* buf) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v |= static_cast<uint64_t>(buf[i]) << (8*i);
    return v;
}

inline void mb_write_bytes(std::ostream& strm, const void* data, size_t n) {
    strm.write(static_cast<const char*>(data), n);
}
inline bool mb_read_bytes(std::istream& strm, void* data, size_t n) {
    strm.read(static_cast<char*>(data), n);
    return !strm.fail();
}
inline void mb_write_id_le(std::ostream& strm, uint64_t id, int bytes) {
    uint8_t buf[8] = {0};
    for (int i = 0; i < bytes; i++) buf[i] = (id >> (8*i)) & 0xFF;
    mb_write_bytes(strm, buf, bytes);
}
inline uint64_t mb_read_id_le(std::istream& strm, int bytes) {
    uint8_t buf[8] = {0};
    if (!mb_read_bytes(strm, buf, bytes))
        throw std::runtime_error("mtbdd binary import: unexpected end of input");
    uint64_t id = 0;
    for (int i = 0; i < bytes; i++) id |= static_cast<uint64_t>(buf[i]) << (8*i);
    return id;
}
inline uint8_t mb_bits_for_max_id(uint64_t max_id) {
    if (max_id <= 0xFF) return 8;
    if (max_id <= 0xFFFF) return 16;
    if (max_id <= 0xFFFFFFFFULL) return 32;
    return 64;
}

template<typename T>
inline void mb_write_value_le(std::ostream& strm, const T& val) {
    uint64_t bits = 0;
    std::memcpy(&bits, &val, sizeof(T));
    uint8_t buf[8];
    for (size_t i = 0; i < sizeof(T); i++) buf[i] = static_cast<uint8_t>((bits >> (8*i)) & 0xFF);
    mb_write_bytes(strm, buf, sizeof(T));
}

template<typename T>
inline T mb_read_value_le(std::istream& strm) {
    uint8_t buf[8] = {0};
    if (!mb_read_bytes(strm, buf, sizeof(T)))
        throw std::runtime_error("mtbdd binary import: truncated terminal values");
    uint64_t bits = 0;
    for (size_t i = 0; i < sizeof(T); i++) bits |= static_cast<uint64_t>(buf[i]) << (8*i);
    T val;
    std::memcpy(&val, &bits, sizeof(T));
    return val;
}

// dd_type constants for MTBDD/MTZDD
static const uint8_t DD_TYPE_MTBDD = 4;
static const uint8_t DD_TYPE_MTZDD = 5;

template<typename T>
inline void mtbdd_export_binary_impl(std::ostream& strm, bddp f, uint8_t dd_type) {
    if (f == bddnull)
        throw std::invalid_argument("mtbdd binary export: null node");

    MTBDDTerminalTable<T>& table = MTBDDTerminalTable<T>::instance();

    // Collect all physical nodes and terminal bddp values via DFS
    std::unordered_set<bddp> visited;
    std::vector<bddp> all_nodes;
    std::unordered_set<bddp> terminal_set;

    std::vector<bddp> stack;
    if (f & BDD_CONST_FLAG) {
        terminal_set.insert(f);
    } else {
        stack.push_back(f);
        visited.insert(f);
    }

    while (!stack.empty()) {
        bddp node = stack.back();
        stack.pop_back();
        all_nodes.push_back(node);
        bddp lo = node_lo(node);
        bddp hi = node_hi(node);
        bddp children[2] = {lo, hi};
        for (int c = 0; c < 2; c++) {
            if (children[c] & BDD_CONST_FLAG) {
                terminal_set.insert(children[c]);
            } else if (visited.insert(children[c]).second) {
                stack.push_back(children[c]);
            }
        }
    }

    // Sort by level ascending
    std::sort(all_nodes.begin(), all_nodes.end(),
              [](bddp a, bddp b) {
                  return var2level[node_var(a)] < var2level[node_var(b)];
              });

    // Build terminal list: index 0 = zero terminal, rest sorted by index
    bddp zero_t = MTBDDTerminalTable<T>::make_terminal(0);
    std::vector<bddp> terminals;
    terminals.push_back(zero_t);
    terminal_set.erase(zero_t);
    // Sort remaining terminals by their terminal table index for determinism
    std::vector<bddp> rest(terminal_set.begin(), terminal_set.end());
    std::sort(rest.begin(), rest.end());
    for (size_t i = 0; i < rest.size(); i++) {
        terminals.push_back(rest[i]);
    }

    uint32_t t = static_cast<uint32_t>(terminals.size());
    std::unordered_map<bddp, uint64_t> terminal_id_map;
    for (uint32_t i = 0; i < t; i++) {
        terminal_id_map[terminals[i]] = i;
    }

    uint64_t total_nodes = all_nodes.size();
    bddvar max_level = 0;
    for (size_t i = 0; i < all_nodes.size(); i++) {
        bddvar lev = var2level[node_var(all_nodes[i])];
        if (lev > max_level) max_level = lev;
    }
    if (max_level == 0 && bddvarused() > 0) max_level = bddvarused();

    // Build non-terminal ID mapping
    std::unordered_map<bddp, uint64_t> id_map;
    for (uint64_t i = 0; i < total_nodes; i++) {
        id_map[all_nodes[i]] = t + i;
    }

    // Map any bddp to binary ID
    auto to_bin_id = [&](bddp edge) -> uint64_t {
        if (edge & BDD_CONST_FLAG) {
            typename std::unordered_map<bddp, uint64_t>::const_iterator it = terminal_id_map.find(edge);
            if (it == terminal_id_map.end())
                throw std::logic_error("mtbdd binary export: terminal not in map");
            return it->second;
        }
        typename std::unordered_map<bddp, uint64_t>::const_iterator it = id_map.find(edge);
        if (it == id_map.end())
            throw std::logic_error("mtbdd binary export: node not in id_map");
        return it->second;
    };

    uint64_t max_id = (t > 0) ? t - 1 : 0;
    if (total_nodes > 0) {
        uint64_t last_id = t + total_nodes - 1;
        if (last_id > max_id) max_id = last_id;
    }
    uint64_t root_bin_id = to_bin_id(f);
    if (root_bin_id > max_id) max_id = root_bin_id;

    uint8_t bits = mb_bits_for_max_id(max_id);
    int id_bytes = bits / 8;

    // --- Write magic ---
    uint8_t magic[3] = {0x42, 0x44, 0x44};
    mb_write_bytes(strm, magic, 3);

    // --- Write header (91 bytes) ---
    uint8_t header[91];
    std::memset(header, 0, sizeof(header));
    header[0] = 1;  // version
    header[1] = dd_type;
    mb_encode_le16(header + 2, 2);   // number_of_arcs
    mb_encode_le32(header + 4, t);   // number_of_terminals
    header[8] = 0;   // bits_for_level (unused)
    header[9] = bits; // bits_for_id
    header[10] = 0;  // use_negative_arcs = 0
    mb_encode_le64(header + 11, max_level);
    mb_encode_le64(header + 19, 1);  // number_of_roots
    header[27] = static_cast<uint8_t>(sizeof(T));  // terminal_value_size
    mb_write_bytes(strm, header, 91);

    // --- Write terminal values ---
    for (uint32_t i = 0; i < t; i++) {
        uint64_t idx = MTBDDTerminalTable<T>::terminal_index(terminals[i]);
        mb_write_value_le<T>(strm, table.get_value(idx));
    }

    // --- Write level counts ---
    std::vector<uint64_t> level_counts(max_level, 0);
    for (size_t i = 0; i < all_nodes.size(); i++) {
        bddvar lev = var2level[node_var(all_nodes[i])];
        level_counts[lev - 1]++;
    }
    for (bddvar l = 0; l < max_level; l++) {
        uint8_t buf[8];
        mb_encode_le64(buf, level_counts[l]);
        mb_write_bytes(strm, buf, 8);
    }

    // --- Write root ID ---
    mb_write_id_le(strm, root_bin_id, id_bytes);

    // --- Write nodes ---
    for (size_t i = 0; i < all_nodes.size(); i++) {
        bddp node = all_nodes[i];
        mb_write_id_le(strm, to_bin_id(node_lo(node)), id_bytes);
        mb_write_id_le(strm, to_bin_id(node_hi(node)), id_bytes);
    }

    if (strm.fail())
        throw std::runtime_error("mtbdd binary export: write error");
}

template<typename T>
inline bddp mtbdd_import_binary_impl(std::istream& strm,
                                      bddp (*make_node)(bddvar, bddp, bddp),
                                      uint8_t expected_type) {
    // --- Read magic ---
    uint8_t magic[3];
    if (!mb_read_bytes(strm, magic, 3) ||
        magic[0] != 0x42 || magic[1] != 0x44 || magic[2] != 0x44)
        throw std::runtime_error("mtbdd binary import: invalid magic bytes");

    // --- Read header ---
    uint8_t header[91];
    if (!mb_read_bytes(strm, header, 91))
        throw std::runtime_error("mtbdd binary import: truncated header");

    uint8_t version = header[0];
    if (version != 1)
        throw std::runtime_error("mtbdd binary import: unsupported version");

    uint8_t dd_type = header[1];
    if (expected_type != 0 && dd_type != expected_type)
        throw std::runtime_error("mtbdd binary import: type mismatch");

    uint32_t num_terminals = mb_decode_le32(header + 4);
    uint8_t bits_for_id = header[9];
    uint64_t max_level = mb_decode_le64(header + 11);
    uint64_t num_roots = mb_decode_le64(header + 19);
    uint8_t terminal_value_size = header[27];

    if (bits_for_id == 0 || bits_for_id % 8 != 0)
        throw std::runtime_error("mtbdd binary import: invalid bits_for_id");
    if (num_roots < 1)
        throw std::runtime_error("mtbdd binary import: number_of_roots < 1");
    if (terminal_value_size != 0 && terminal_value_size != sizeof(T))
        throw std::runtime_error("mtbdd binary import: terminal_value_size mismatch");

    int id_bytes = bits_for_id / 8;
    uint32_t t = num_terminals;

    // --- Read terminal values ---
    MTBDDTerminalTable<T>& table = MTBDDTerminalTable<T>::instance();
    std::vector<bddp> terminal_bddps(t);
    for (uint32_t i = 0; i < t; i++) {
        T val = mb_read_value_le<T>(strm);
        uint64_t idx = table.get_or_insert(val);
        terminal_bddps[i] = MTBDDTerminalTable<T>::make_terminal(idx);
    }

    // --- Read level counts ---
    std::vector<uint64_t> level_counts(max_level);
    for (uint64_t l = 0; l < max_level; l++) {
        uint8_t buf[8];
        if (!mb_read_bytes(strm, buf, 8))
            throw std::runtime_error("mtbdd binary import: truncated level counts");
        level_counts[l] = mb_decode_le64(buf);
    }

    uint64_t total_nodes = 0;
    for (uint64_t l = 0; l < max_level; l++) total_nodes += level_counts[l];

    // --- Read root ID ---
    uint64_t root_id = mb_read_id_le(strm, id_bytes);

    // --- Read nodes ---
    struct BinNode { uint64_t lo, hi; };
    std::vector<BinNode> bin_nodes(total_nodes);
    for (uint64_t i = 0; i < total_nodes; i++) {
        bin_nodes[i].lo = mb_read_id_le(strm, id_bytes);
        bin_nodes[i].hi = mb_read_id_le(strm, id_bytes);
    }

    // Ensure variables exist
    while (bddvarused() < max_level) bddnewvar();

    // --- Reconstruct nodes (topological sort) ---
    std::unordered_map<uint64_t, bddp> id_map;

    auto is_resolved = [&](uint64_t bin_id) -> bool {
        if (bin_id < t) return true;
        return id_map.count(bin_id) != 0;
    };

    auto resolve = [&](uint64_t bin_id) -> bddp {
        if (bin_id < t) {
            if (bin_id >= terminal_bddps.size())
                throw std::runtime_error("mtbdd binary import: terminal ID out of range");
            return terminal_bddps[static_cast<size_t>(bin_id)];
        }
        typename std::unordered_map<uint64_t, bddp>::const_iterator it = id_map.find(bin_id);
        if (it == id_map.end())
            throw std::runtime_error("mtbdd binary import: undefined node reference");
        return it->second;
    };

    // Pre-compute levels
    std::vector<bddvar> node_levels(total_nodes);
    {
        uint64_t idx = 0;
        for (uint64_t l = 0; l < max_level; l++) {
            for (uint64_t c = 0; c < level_counts[l]; c++) {
                node_levels[idx++] = static_cast<bddvar>(l + 1);
            }
        }
    }

    // Worklist-based topological sort
    std::vector<int> pending(total_nodes, 0);
    std::unordered_map<uint64_t, std::vector<uint64_t> > dependents;
    for (uint64_t i = 0; i < total_nodes; i++) {
        if (!is_resolved(bin_nodes[i].lo)) {
            pending[i]++;
            dependents[bin_nodes[i].lo].push_back(i);
        }
        if (!is_resolved(bin_nodes[i].hi)) {
            pending[i]++;
            dependents[bin_nodes[i].hi].push_back(i);
        }
    }

    std::vector<uint64_t> ready;
    for (uint64_t i = 0; i < total_nodes; i++) {
        if (pending[i] == 0) ready.push_back(i);
    }

    uint64_t created = 0;
    while (!ready.empty()) {
        uint64_t i = ready.back();
        ready.pop_back();

        bddvar var = bddvaroflev(node_levels[i]);
        bddp lo = resolve(bin_nodes[i].lo);
        bddp hi = resolve(bin_nodes[i].hi);
        uint64_t bin_id = t + i;
        id_map[bin_id] = make_node(var, lo, hi);
        created++;

        typename std::unordered_map<uint64_t, std::vector<uint64_t> >::iterator dit = dependents.find(bin_id);
        if (dit != dependents.end()) {
            for (size_t j = 0; j < dit->second.size(); j++) {
                uint64_t dep = dit->second[j];
                if (--pending[dep] == 0) ready.push_back(dep);
            }
        }
    }

    if (created < total_nodes)
        throw std::runtime_error("mtbdd binary import: circular dependency");

    return resolve(root_id);
}

} // namespace mtbdd_binary_detail

// --- MTBDD<T> binary export/import ---

template<typename T>
inline void MTBDD<T>::export_binary(std::ostream& strm) const {
    mtbdd_binary_detail::mtbdd_export_binary_impl<T>(
        strm, root, mtbdd_binary_detail::DD_TYPE_MTBDD);
}

template<typename T>
inline void MTBDD<T>::export_binary(const char* filename) const {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs)
        throw std::runtime_error("MTBDD::export_binary: cannot open file");
    export_binary(ofs);
}

template<typename T>
inline MTBDD<T> MTBDD<T>::import_binary(std::istream& strm) {
    bddp p = mtbdd_binary_detail::mtbdd_import_binary_impl<T>(
        strm, mtbdd_getnode_raw, mtbdd_binary_detail::DD_TYPE_MTBDD);
    MTBDD result;
    result.root = p;
    return result;
}

template<typename T>
inline MTBDD<T> MTBDD<T>::import_binary(const char* filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
        throw std::runtime_error("MTBDD::import_binary: cannot open file");
    return import_binary(ifs);
}

// --- MTZDD<T> binary export/import ---

template<typename T>
inline void MTZDD<T>::export_binary(std::ostream& strm) const {
    mtbdd_binary_detail::mtbdd_export_binary_impl<T>(
        strm, root, mtbdd_binary_detail::DD_TYPE_MTZDD);
}

template<typename T>
inline void MTZDD<T>::export_binary(const char* filename) const {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs)
        throw std::runtime_error("MTZDD::export_binary: cannot open file");
    export_binary(ofs);
}

template<typename T>
inline MTZDD<T> MTZDD<T>::import_binary(std::istream& strm) {
    bddp p = mtbdd_binary_detail::mtbdd_import_binary_impl<T>(
        strm, mtzdd_getnode_raw, mtbdd_binary_detail::DD_TYPE_MTZDD);
    MTZDD result;
    result.root = p;
    return result;
}

template<typename T>
inline MTZDD<T> MTZDD<T>::import_binary(const char* filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs)
        throw std::runtime_error("MTZDD::import_binary: cannot open file");
    return import_binary(ifs);
}

// ========================================================================
//  MTBDD/MTZDD save_svg inline implementations
// ========================================================================

#include "svg_export.h"
#include <sstream>

// Helper: build terminal_name_map by DFS from root.
template<typename T>
static SvgParams mtbdd_build_svg_params(bddp root, const SvgParams& params) {
    SvgParams p = params;
    std::vector<bddp> stack;
    std::unordered_set<bddp> visited;
    stack.push_back(root);
    while (!stack.empty()) {
        bddp f = stack.back();
        stack.pop_back();
        if (f & BDD_CONST_FLAG) {
            if (p.terminal_name_map.count(f) == 0) {
                uint64_t idx = MTBDDTerminalTable<T>::terminal_index(f);
                T val = MTBDDTerminalTable<T>::instance().get_value(idx);
                std::ostringstream oss;
                oss << val;
                p.terminal_name_map[f] = oss.str();
            }
            continue;
        }
        if (!visited.insert(f).second) continue;
        stack.push_back(node_lo(f));
        stack.push_back(node_hi(f));
    }
    return p;
}

template<typename T>
inline void MTBDD<T>::save_svg(const char* filename, const SvgParams& params) const {
    mtbdd_save_svg(filename, root, mtbdd_build_svg_params<T>(root, params));
}
template<typename T>
inline void MTBDD<T>::save_svg(const char* filename) const {
    save_svg(filename, SvgParams());
}
template<typename T>
inline void MTBDD<T>::save_svg(std::ostream& strm, const SvgParams& params) const {
    mtbdd_save_svg(strm, root, mtbdd_build_svg_params<T>(root, params));
}
template<typename T>
inline void MTBDD<T>::save_svg(std::ostream& strm) const {
    save_svg(strm, SvgParams());
}
template<typename T>
inline std::string MTBDD<T>::save_svg(const SvgParams& params) const {
    return mtbdd_save_svg(root, mtbdd_build_svg_params<T>(root, params));
}
template<typename T>
inline std::string MTBDD<T>::save_svg() const {
    return save_svg(SvgParams());
}

template<typename T>
inline void MTZDD<T>::save_svg(const char* filename, const SvgParams& params) const {
    mtzdd_save_svg(filename, root, mtbdd_build_svg_params<T>(root, params));
}
template<typename T>
inline void MTZDD<T>::save_svg(const char* filename) const {
    save_svg(filename, SvgParams());
}
template<typename T>
inline void MTZDD<T>::save_svg(std::ostream& strm, const SvgParams& params) const {
    mtzdd_save_svg(strm, root, mtbdd_build_svg_params<T>(root, params));
}
template<typename T>
inline void MTZDD<T>::save_svg(std::ostream& strm) const {
    save_svg(strm, SvgParams());
}
template<typename T>
inline std::string MTZDD<T>::save_svg(const SvgParams& params) const {
    return mtzdd_save_svg(root, mtbdd_build_svg_params<T>(root, params));
}
template<typename T>
inline std::string MTZDD<T>::save_svg() const {
    return save_svg(SvgParams());
}

// ========================================================================
//  MTZDD print_sets / to_str inline implementations
// ========================================================================

template<typename T>
inline void MTZDD<T>::print_sets(std::ostream& os) const {
    std::vector<std::pair<std::vector<bddvar>, T> > paths = enumerate();
    for (size_t i = 0; i < paths.size(); ++i) {
        if (i > 0) os << ", ";
        os << "{";
        for (size_t j = 0; j < paths[i].first.size(); ++j) {
            if (j > 0) os << ",";
            os << paths[i].first[j];
        }
        os << "} -> " << paths[i].second;
    }
}

template<typename T>
inline void MTZDD<T>::print_sets(
    std::ostream& os,
    const std::vector<std::string>& var_name_map) const
{
    std::vector<std::pair<std::vector<bddvar>, T> > paths = enumerate();
    for (size_t i = 0; i < paths.size(); ++i) {
        if (i > 0) os << ", ";
        os << "{";
        for (size_t j = 0; j < paths[i].first.size(); ++j) {
            if (j > 0) os << ",";
            bddvar v = paths[i].first[j];
            if (v < var_name_map.size() && !var_name_map[v].empty()) {
                os << var_name_map[v];
            } else {
                os << v;
            }
        }
        os << "} -> " << paths[i].second;
    }
}

template<typename T>
inline std::string MTZDD<T>::to_str() const {
    std::ostringstream oss;
    print_sets(oss);
    return oss.str();
}

#endif
