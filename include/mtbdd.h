#ifndef KYOTODD_MTBDD_H
#define KYOTODD_MTBDD_H

#include "bdd_types.h"
#include "bdd_base.h"
#include "bdd_ops.h"
#include "bdd_internal.h"
#include <vector>
#include <unordered_map>
#include <stdexcept>

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

    // Normalize operand order for commutative cache reuse
    if (f > g) { bddp tmp = f; f = g; g = tmp; }

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

    if (f > g) { bddp tmp = f; f = g; g = tmp; }

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

    if (f_term) {
        auto& table = MTBDDTerminalTable<T>::instance();
        T f_val = table.get_value(MTBDDTerminalTable<T>::terminal_index(f));
        return (f_val == T{}) ? h : g;
    }

    if (g == h) return g;

    bddp cached = bddrcache3(cache_op, f, g, h);
    if (cached != bddnull) return cached;

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

    bddp zero_t = BDD_CONST_FLAG | 0;

    // Cofactors: f uses BDD semantics (condition), g/h use ZDD semantics
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

    // Resolve complement (ZDD: only lo is affected)
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp fn = f & ~BDD_COMP_FLAG;

    bddp zt = comp ? one_t : zero_t;
    bddp ot = comp ? zero_t : one_t;

    bddp cached = bddrcache3(cache_op, fn, zt, ot);
    if (cached != bddnull) return cached;

    bddvar v = node_var(fn);
    bddp raw_lo = node_lo(fn);  // always non-complemented (ZDD invariant)
    bddp raw_hi = node_hi(fn);  // never complemented in ZDD

    // ZDD complement affects only lo. Since we resolved complement by
    // swapping zt/ot, the complement on lo in the recursion is handled
    // by propagating zt/ot correctly.
    bddp mt_lo = mtzdd_from_zdd_rec<T>(raw_lo, zt, ot, cache_op);
    bddp mt_hi = mtzdd_from_zdd_rec<T>(raw_hi, zt, ot, cache_op);

    bddp result = mtzdd_getnode_raw(v, mt_lo, mt_hi);
    bddwcache3(cache_op, fn, zt, ot, result);
    return result;
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

private:
    explicit MTBDD(bddp p) : DDBase() {
        root = p;
    }

    template<typename T2, typename B> friend bddp mtbdd_apply_rec(bddp, bddp, B&, uint8_t);
};

template<typename T>
using ADD = MTBDD<T>;

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

    bool operator==(const MTZDD& other) const { return root == other.root; }
    bool operator!=(const MTZDD& other) const { return root != other.root; }

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
            return (val == T{}) ? else_case : then_case;
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

        // Determine the top level of the MTZDD
        bddvar top_level = 0;
        if (!(f & BDD_CONST_FLAG)) {
            top_level = var2level[node_var(f)];
        }

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

private:
    explicit MTZDD(bddp p) : DDBase() {
        root = p;
    }

    template<typename T2, typename B> friend bddp mtzdd_apply_rec(bddp, bddp, B&, uint8_t);
};

#endif
