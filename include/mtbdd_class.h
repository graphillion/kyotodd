#ifndef KYOTODD_MTBDD_CLASS_H
#define KYOTODD_MTBDD_CLASS_H

// Internal header: MTBDD<T> / MTZDD<T> class definitions plus supporting
// helpers (enumerate, threshold conversion).
//
// End users should include "mtbdd.h" (umbrella), not this header directly.

#include "mtbdd_core.h"

namespace kyotodd {

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
            if (use_iter_1op(bdd.id())) {
                return mtbdd_from_bdd_iter<T>(bdd.id(), zero_t, one_t, conv_op);
            }
            return mtbdd_from_bdd_rec<T>(bdd.id(), zero_t, one_t, conv_op);
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
            if (use_iter_2op(root, other.root)) {
                return mtbdd_apply_iter<T, BinOp>(root, other.root, op, apply_op);
            }
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
            if (use_iter_3op(root, then_case.root, else_case.root)) {
                return mtbdd_ite_iter<T>(root, then_case.root, else_case.root, ite_op);
            }
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
    BDD_RecurGuard guard;
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

// Iterative counterpart to mtzdd_enumerate_rec (side-effect, Template F).
template<typename T>
static void mtzdd_enumerate_iter(
    bddp root,
    std::vector<bddvar>& current,
    std::vector<std::pair<std::vector<bddvar>, T> >& result)
{
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddvar var;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root;
    init.var = 0;
    init.phase = Phase::ENTER;
    stack.push_back(init);

    while (!stack.empty()) {
        Frame& frame = stack.back();
        switch (frame.phase) {
        case Phase::ENTER: {
            bddp f = frame.f;
            if (f & BDD_CONST_FLAG) {
                uint64_t idx = MTBDDTerminalTable<T>::terminal_index(f);
                T val = MTBDDTerminalTable<T>::instance().get_value(idx);
                if (val != T{}) {
                    result.push_back(std::make_pair(current, val));
                }
                stack.pop_back();
                break;
            }
            frame.var = node_var(f);
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = node_lo(f);
            child.var = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_LO: {
            current.push_back(frame.var);
            frame.phase = Phase::GOT_HI;

            Frame child;
            child.f = node_hi(frame.f);
            child.var = 0;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            current.pop_back();
            stack.pop_back();
            break;
        }
        }
    }
}

// --- MTZDD to ZDD threshold conversion helper ---

template<typename T>
static bddp mtzdd_to_zdd_rec(
    bddp f,
    const std::unordered_set<bddp>& pred_terminals,
    std::unordered_map<bddp, bddp>& memo)
{
    BDD_RecurGuard guard;

    if (f & BDD_CONST_FLAG) {
        return pred_terminals.count(f) ? bddsingle : bddempty;
    }

    std::unordered_map<bddp, bddp>::iterator it = memo.find(f);
    if (it != memo.end()) return it->second;

    bddvar var = node_var(f);
    bddp lo = mtzdd_to_zdd_rec<T>(node_lo(f), pred_terminals, memo);
    bddp hi = mtzdd_to_zdd_rec<T>(node_hi(f), pred_terminals, memo);
    bddp result = ZDD::getnode_raw(var, lo, hi);

    memo[f] = result;
    return result;
}

// Iterative counterpart to mtzdd_to_zdd_rec.
// PRECONDITION: caller holds a bdd_gc_guard scope.
template<typename T>
static bddp mtzdd_to_zdd_iter(
    bddp root_f,
    const std::unordered_set<bddp>& pred_terminals,
    std::unordered_map<bddp, bddp>& memo)
{
    enum class Phase : uint8_t { ENTER, GOT_LO, GOT_HI };
    struct Frame {
        bddp f;
        bddvar var;
        bddp lo_result;
        Phase phase;
    };

    std::vector<Frame> stack;
    stack.reserve(64);

    Frame init;
    init.f = root_f;
    init.var = 0;
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
                result = pred_terminals.count(f) ? bddsingle : bddempty;
                stack.pop_back();
                break;
            }
            std::unordered_map<bddp, bddp>::iterator it = memo.find(f);
            if (it != memo.end()) {
                result = it->second;
                stack.pop_back();
                break;
            }
            frame.var = node_var(f);
            frame.phase = Phase::GOT_LO;

            Frame child;
            child.f = node_lo(f);
            child.var = 0;
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
            child.var = 0;
            child.lo_result = bddnull;
            child.phase = Phase::ENTER;
            stack.push_back(child);
            break;
        }
        case Phase::GOT_HI: {
            bddp hi = result;
            bddp r = ZDD::getnode_raw(frame.var, frame.lo_result, hi);
            memo[frame.f] = r;
            result = r;
            stack.pop_back();
            break;
        }
        }
    }
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
            if (use_iter_1op(zdd.id())) {
                return mtzdd_from_zdd_iter<T>(zdd.id(), zero_t, one_t, conv_op);
            }
            return mtzdd_from_zdd_rec<T>(zdd.id(), zero_t, one_t, conv_op);
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
        std::unordered_map<bddp, bddp> memo;
        bddp result = bdd_gc_guard([&]() -> bddp {
            if (use_iter_1op(root)) {
                return mtzdd_to_zdd_iter<T>(root, pred_terminals, memo);
            }
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, memo);
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
        std::unordered_map<bddp, bddp> memo;
        bddp result = bdd_gc_guard([&]() -> bddp {
            if (use_iter_1op(root)) {
                return mtzdd_to_zdd_iter<T>(root, pred_terminals, memo);
            }
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, memo);
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
        std::unordered_map<bddp, bddp> memo;
        bddp result = bdd_gc_guard([&]() -> bddp {
            if (use_iter_1op(root)) {
                return mtzdd_to_zdd_iter<T>(root, pred_terminals, memo);
            }
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, memo);
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
        std::unordered_map<bddp, bddp> memo;
        bddp result = bdd_gc_guard([&]() -> bddp {
            if (use_iter_1op(root)) {
                return mtzdd_to_zdd_iter<T>(root, pred_terminals, memo);
            }
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, memo);
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
        std::unordered_map<bddp, bddp> memo;
        bddp result = bdd_gc_guard([&]() -> bddp {
            if (use_iter_1op(root)) {
                return mtzdd_to_zdd_iter<T>(root, pred_terminals, memo);
            }
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, memo);
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
        std::unordered_map<bddp, bddp> memo;
        bddp result = bdd_gc_guard([&]() -> bddp {
            if (use_iter_1op(root)) {
                return mtzdd_to_zdd_iter<T>(root, pred_terminals, memo);
            }
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, memo);
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
        std::unordered_map<bddp, bddp> memo;
        bddp result = bdd_gc_guard([&]() -> bddp {
            if (use_iter_1op(root)) {
                return mtzdd_to_zdd_iter<T>(root, pred_terminals, memo);
            }
            return mtzdd_to_zdd_rec<T>(root, pred_terminals, memo);
        });
        return ZDD_ID(result);
    }

    // --- Enumeration ---

    /** @brief Enumerate all non-zero paths as (variable set, terminal value) pairs. */
    std::vector<std::pair<std::vector<bddvar>, T> > enumerate() const {
        std::vector<std::pair<std::vector<bddvar>, T> > result;
        std::vector<bddvar> current;
        if (use_iter_1op(root)) {
            mtzdd_enumerate_iter<T>(root, current, result);
        } else {
            mtzdd_enumerate_rec<T>(root, current, result);
        }
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
        if (use_iter_1op(root)) {
            return mtzdd_count_iter(root, memo);
        }
        return mtzdd_count_rec(root, memo);
    }

    /** @brief Count the number of non-zero terminal paths (exact BigInt). */
    bigint::BigInt exact_count() const {
        std::unordered_map<bddp, bigint::BigInt> memo;
        if (use_iter_1op(root)) {
            return mtzdd_exact_count_iter(root, memo);
        }
        return mtzdd_exact_count_rec(root, memo);
    }

    /** @brief Count paths leading to the specific terminal value (double). */
    double count(const T& terminal) const {
        auto& table = MTBDDTerminalTable<T>::instance();
        uint64_t idx;
        if (!table.find_index(terminal, idx)) return 0.0;
        bddp target = MTBDDTerminalTable<T>::make_terminal(idx);
        std::unordered_map<bddp, double> memo;
        if (use_iter_1op(root)) {
            return mtzdd_count_for_terminal_iter(root, target, memo);
        }
        return mtzdd_count_for_terminal_rec(root, target, memo);
    }

    /** @brief Count paths leading to the specific terminal value (exact BigInt). */
    bigint::BigInt exact_count(const T& terminal) const {
        auto& table = MTBDDTerminalTable<T>::instance();
        uint64_t idx;
        if (!table.find_index(terminal, idx)) return bigint::BigInt(0);
        bddp target = MTBDDTerminalTable<T>::make_terminal(idx);
        std::unordered_map<bddp, bigint::BigInt> memo;
        if (use_iter_1op(root)) {
            return mtzdd_exact_count_for_terminal_iter(root, target, memo);
        }
        return mtzdd_exact_count_for_terminal_rec(root, target, memo);
    }

    // --- Generic apply ---

    template<typename BinOp>
    MTZDD apply(const MTZDD& other, BinOp op) const {
        static uint8_t apply_op = mtbdd_alloc_op_code();
        MTZDD result;
        result.root = bdd_gc_guard([&]() -> bddp {
            if (use_iter_2op(root, other.root)) {
                return mtzdd_apply_iter<T, BinOp>(root, other.root, op, apply_op);
            }
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
            if (use_iter_3op(root, then_case.root, else_case.root)) {
                return mtzdd_ite_iter<T>(root, then_case.root, else_case.root, ite_op);
            }
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


} // namespace kyotodd

#endif
