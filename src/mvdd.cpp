#include "bdd.h"
#include "mvdd.h"
#include <algorithm>
#include <sstream>
#include <unordered_set>

// ============================================================
//  MVDDVarInfo
// ============================================================

MVDDVarInfo::MVDDVarInfo() : mvdd_var(0), k(0), dd_vars() {}

MVDDVarInfo::MVDDVarInfo(bddvar mv, int kval, const std::vector<bddvar>& dvars)
    : mvdd_var(mv), k(kval), dd_vars(dvars) {}

// ============================================================
//  MVDDVarTable
// ============================================================

MVDDVarTable::MVDDVarTable(int k) : k_(k) {
    if (k < 2 || k > 65536) {
        throw std::invalid_argument(
            "MVDDVarTable: k must be >= 2 and <= 65536");
    }
}

int MVDDVarTable::k() const { return k_; }

bddvar MVDDVarTable::mvdd_var_count() const {
    return static_cast<bddvar>(var_map_.size());
}

bddvar MVDDVarTable::register_var(const std::vector<bddvar>& dd_vars) {
    if (static_cast<int>(dd_vars.size()) != k_ - 1) {
        throw std::invalid_argument(
            "MVDDVarTable::register_var: dd_vars size must be k-1");
    }

    // Check for duplicates within dd_vars and reuse of already-registered DD vars
    for (int i = 0; i < static_cast<int>(dd_vars.size()); ++i) {
        bddvar dv = dd_vars[i];
        // Check duplicate within this call
        for (int j = 0; j < i; ++j) {
            if (dd_vars[j] == dv) {
                throw std::invalid_argument(
                    "MVDDVarTable::register_var: duplicate DD variable " +
                    std::to_string(dv));
            }
        }
        // Check already registered to another MV var
        if (dv < dd_to_mvdd_var_.size() && dd_to_mvdd_var_[dv] != 0) {
            throw std::invalid_argument(
                "MVDDVarTable::register_var: DD variable " +
                std::to_string(dv) + " is already registered");
        }
    }

    // Check that dd_vars are in increasing BDD level order
    for (int i = 1; i < static_cast<int>(dd_vars.size()); ++i) {
        if (bddlevofvar(dd_vars[i]) <= bddlevofvar(dd_vars[i - 1])) {
            throw std::invalid_argument(
                "MVDDVarTable::register_var: dd_vars must be in increasing level order");
        }
    }

    bddvar mv = static_cast<bddvar>(var_map_.size()) + 1;
    var_map_.push_back(MVDDVarInfo(mv, k_, dd_vars));

    for (int i = 0; i < static_cast<int>(dd_vars.size()); ++i) {
        bddvar dv = dd_vars[i];
        if (dv >= dd_to_mvdd_var_.size()) {
            dd_to_mvdd_var_.resize(dv + 1, 0);
            dd_to_index_.resize(dv + 1, -1);
        }
        dd_to_mvdd_var_[dv] = mv;
        dd_to_index_[dv] = i;
    }
    return mv;
}

const std::vector<bddvar>& MVDDVarTable::dd_vars_of(bddvar mv) const {
    if (mv < 1 || mv > static_cast<bddvar>(var_map_.size())) {
        throw std::out_of_range("MVDDVarTable::dd_vars_of: mv out of range");
    }
    return var_map_[mv - 1].dd_vars;
}

bddvar MVDDVarTable::mvdd_var_of(bddvar dv) const {
    if (dv >= dd_to_mvdd_var_.size()) return 0;
    return dd_to_mvdd_var_[dv];
}

int MVDDVarTable::dd_var_index(bddvar dv) const {
    if (dv >= dd_to_index_.size()) return -1;
    return dd_to_index_[dv];
}

const MVDDVarInfo& MVDDVarTable::var_info(bddvar mv) const {
    if (mv < 1 || mv > static_cast<bddvar>(var_map_.size())) {
        throw std::out_of_range("MVDDVarTable::var_info: mv out of range");
    }
    return var_map_[mv - 1];
}

bddvar MVDDVarTable::get_top_dd_var(bddvar mv) const {
    if (mv < 1 || mv > static_cast<bddvar>(var_map_.size())) {
        throw std::out_of_range("MVDDVarTable::get_top_dd_var: mv out of range");
    }
    const std::vector<bddvar>& dvars = var_map_[mv - 1].dd_vars;
    if (dvars.empty()) return 0;
    return dvars[0];
}

// ============================================================
//  Helper: validate that all DD variables in a BDD/ZDD are registered
// ============================================================

static void validate_dd_vars_registered(bddp root, const MVDDVarTable* table,
                                         const char* context) {
    if ((root & BDD_CONST_FLAG) || root == bddnull) return;

    std::unordered_set<bddp> visited;
    std::vector<bddp> stack;
    bddp rn = root & ~BDD_COMP_FLAG;
    stack.push_back(rn);
    visited.insert(rn);

    while (!stack.empty()) {
        bddp fn = stack.back();
        stack.pop_back();

        bddvar dv = node_var(fn);
        if (table->mvdd_var_of(dv) == 0) {
            throw std::invalid_argument(
                std::string(context) + ": DD variable " +
                std::to_string(dv) + " is not registered in the var table");
        }

        bddp children[] = { node_lo(fn), node_hi(fn) };
        for (bddp c : children) {
            if (c & BDD_CONST_FLAG) continue;
            bddp cn = c & ~BDD_COMP_FLAG;
            if (visited.find(cn) == visited.end()) {
                visited.insert(cn);
                stack.push_back(cn);
            }
        }
    }
}

// ============================================================
//  MVBDD helper: count MVDD-level entry nodes in a BDD/ZDD graph
// ============================================================

static uint64_t mvdd_count_entries(bddp root, const MVDDVarTable* table) {
    if ((root & BDD_CONST_FLAG) || root == bddnull) return 0;

    bddp rn = root & ~BDD_COMP_FLAG;
    std::unordered_set<bddp> visited;
    std::unordered_set<bddp> entries;

    entries.insert(rn);
    visited.insert(rn);

    std::vector<bddp> stack;
    stack.push_back(rn);

    while (!stack.empty()) {
        bddp fn = stack.back();
        stack.pop_back();

        bddvar mv = table->mvdd_var_of(node_var(fn));

        bddp children[] = { node_lo(fn), node_hi(fn) };
        for (bddp c : children) {
            if (c & BDD_CONST_FLAG) continue;
            bddp cn = c & ~BDD_COMP_FLAG;

            bddvar cmv = table->mvdd_var_of(node_var(cn));
            if (cmv != mv) {
                entries.insert(cn);
            }

            if (visited.find(cn) == visited.end()) {
                visited.insert(cn);
                stack.push_back(cn);
            }
        }
    }

    return entries.size();
}

// ============================================================
//  MVBDD
// ============================================================

// --- Private bddp constructor ---

MVBDD::MVBDD(std::shared_ptr<MVDDVarTable> table, bddp p)
    : DDBase(), var_table_(table)
{
    root = p;
}

// --- Constructors ---

MVBDD::MVBDD() : DDBase(-1), var_table_(nullptr) {}

MVBDD::MVBDD(int k, bool value)
    : DDBase(value ? 1 : 0),
      var_table_(std::make_shared<MVDDVarTable>(k))
{
}

MVBDD::MVBDD(std::shared_ptr<MVDDVarTable> table, const BDD& bdd)
    : DDBase(), var_table_(table)
{
    root = bdd.get_id();
    if (table) {
        validate_dd_vars_registered(root, table.get(), "MVBDD(table, BDD)");
    }
}

MVBDD::MVBDD(const MVBDD& other)
    : DDBase(other), var_table_(other.var_table_)
{
}

MVBDD::MVBDD(MVBDD&& other)
    : DDBase(std::move(other)), var_table_(std::move(other.var_table_))
{
}

MVBDD& MVBDD::operator=(const MVBDD& other) {
    DDBase::operator=(other);
    var_table_ = other.var_table_;
    return *this;
}

MVBDD& MVBDD::operator=(MVBDD&& other) {
    DDBase::operator=(std::move(other));
    var_table_ = std::move(other.var_table_);
    return *this;
}

// --- Factory methods ---

MVBDD MVBDD::zero(std::shared_ptr<MVDDVarTable> table) {
    return MVBDD(table, bddfalse);
}

MVBDD MVBDD::one(std::shared_ptr<MVDDVarTable> table) {
    return MVBDD(table, bddtrue);
}

MVBDD MVBDD::singleton(const MVBDD& base, bddvar mv, int value) {
    if (!base.var_table_) {
        throw std::invalid_argument("MVBDD::singleton: base has no var table");
    }
    int k = base.var_table_->k();
    if (value < 0 || value >= k) {
        throw std::invalid_argument("MVBDD::singleton: value out of range");
    }
    const std::vector<bddvar>& dvars = base.var_table_->dd_vars_of(mv);

    // Build bottom-up: dd_vars[0] is lowest level
    bddp result = bddtrue;

    if (value == 0) {
        // All dd_vars must be 0: chain of (var, result, false)
        for (int i = 0; i < k - 1; ++i) {
            result = BDD::getnode(dvars[i], result, bddfalse);
        }
    } else {
        // dd_vars[0..value-2] must be 0, dd_vars[value-1] must be 1
        for (int i = 0; i < value - 1; ++i) {
            result = BDD::getnode(dvars[i], result, bddfalse);
        }
        result = BDD::getnode(dvars[value - 1], bddfalse, result);
        // dd_vars[value..k-2] are don't care (skipped by BDD jump rule)
    }

    return base.make_result(result);
}

// --- Variable management ---

bddvar MVBDD::new_var() {
    if (!var_table_) {
        throw std::logic_error("MVBDD::new_var: no var table");
    }
    int k = var_table_->k();
    std::vector<bddvar> dvars;
    dvars.reserve(k - 1);
    for (int i = 0; i < k - 1; ++i) {
        dvars.push_back(bddnewvar());
    }
    return var_table_->register_var(dvars);
}

int MVBDD::k() const {
    if (!var_table_) return 0;
    return var_table_->k();
}

std::shared_ptr<MVDDVarTable> MVBDD::var_table() const {
    return var_table_;
}

// --- ITE construction ---

MVBDD MVBDD::ite(const MVBDD& base, bddvar mv,
                  const std::vector<MVBDD>& children) {
    if (!base.var_table_) {
        throw std::invalid_argument("MVBDD::ite: base has no var table");
    }
    int k = base.var_table_->k();
    if (static_cast<int>(children.size()) != k) {
        throw std::invalid_argument("MVBDD::ite: children size must equal k");
    }
    if (mv < 1 || mv > base.var_table_->mvdd_var_count()) {
        throw std::invalid_argument("MVBDD::ite: mv out of range");
    }
    for (int i = 0; i < k; ++i) {
        if (children[i].var_table_.get() != base.var_table_.get()) {
            throw std::invalid_argument(
                "MVBDD::ite: children must share the same var table");
        }
    }

    // Build using BDD operations: result = OR(literal_i AND children[i])
    // This correctly handles any level ordering between mv and children.
    bddp result = bddfalse;
    for (int i = 0; i < k; ++i) {
        MVBDD lit = singleton(base, mv, i);
        result = bddor(result, bddand(lit.root, children[i].root));
    }

    return base.make_result(result);
}

MVBDD MVBDD::ite(const MVBDD& base, bddvar mv,
                  std::initializer_list<MVBDD> children) {
    return ite(base, mv, std::vector<MVBDD>(children));
}

// --- Node reference ---

bddp MVBDD::get_id() const { return root; }

// --- Child node access ---

MVBDD MVBDD::child(int value) const {
    if (root == bddnull) {
        throw std::invalid_argument("MVBDD::child: null node");
    }
    if (root & BDD_CONST_FLAG) {
        throw std::invalid_argument("MVBDD::child: terminal node");
    }
    if (!var_table_) {
        throw std::logic_error("MVBDD::child: no var table");
    }

    bddvar mv = top_var();
    int k = var_table_->k();
    if (value < 0 || value >= k) {
        throw std::invalid_argument("MVBDD::child: value out of range");
    }

    const std::vector<bddvar>& dvars = var_table_->dd_vars_of(mv);

    // Traverse from highest-level dd_var downward
    bddp f = root;

    if (value == 0) {
        // Take lo at every dd_var level
        for (int i = k - 2; i >= 0; --i) {
            if (f & BDD_CONST_FLAG) break;
            if (node_var(f) != dvars[i]) continue;
            f = BDD::child0(f);
        }
    } else {
        // Take lo from dd_vars[k-2] down to dd_vars[value]
        for (int i = k - 2; i >= value; --i) {
            if (f & BDD_CONST_FLAG) break;
            if (node_var(f) != dvars[i]) continue;
            f = BDD::child0(f);
        }
        // Take hi at dd_vars[value-1]
        if (!(f & BDD_CONST_FLAG) && node_var(f) == dvars[value - 1]) {
            f = BDD::child1(f);
        }
        // Take lo at remaining lower dd_vars (must be 0 in the encoding)
        for (int i = value - 2; i >= 0; --i) {
            if (f & BDD_CONST_FLAG) break;
            if (node_var(f) != dvars[i]) continue;
            f = BDD::child0(f);
        }
    }

    return make_result(f);
}

bddvar MVBDD::top_var() const {
    if (root == bddnull || (root & BDD_CONST_FLAG)) return 0;
    if (!var_table_) return 0;
    bddvar dv = node_var(root);
    bddvar mv = var_table_->mvdd_var_of(dv);
    if (mv == 0) {
        throw std::logic_error(
            "MVBDD::top_var: root DD variable " + std::to_string(dv) +
            " is not registered in the var table");
    }
    return mv;
}

// --- Boolean operations ---

MVBDD MVBDD::operator&(const MVBDD& other) const {
    check_compatible(other);
    return make_result(bddand(root, other.root));
}

MVBDD MVBDD::operator|(const MVBDD& other) const {
    check_compatible(other);
    return make_result(bddor(root, other.root));
}

MVBDD MVBDD::operator^(const MVBDD& other) const {
    check_compatible(other);
    return make_result(bddxor(root, other.root));
}

MVBDD MVBDD::operator~() const {
    return make_result(bddnot(root));
}

MVBDD& MVBDD::operator&=(const MVBDD& other) {
    check_compatible(other);
    root = bddand(root, other.root);
    return *this;
}

MVBDD& MVBDD::operator|=(const MVBDD& other) {
    check_compatible(other);
    root = bddor(root, other.root);
    return *this;
}

MVBDD& MVBDD::operator^=(const MVBDD& other) {
    check_compatible(other);
    root = bddxor(root, other.root);
    return *this;
}

// --- Evaluation ---

bool MVBDD::evaluate(const std::vector<int>& assignment) const {
    if (!var_table_) {
        throw std::logic_error("MVBDD::evaluate: no var table");
    }
    if (assignment.size() != var_table_->mvdd_var_count()) {
        throw std::invalid_argument("MVBDD::evaluate: assignment size mismatch");
    }
    int k = var_table_->k();
    for (size_t i = 0; i < assignment.size(); ++i) {
        if (assignment[i] < 0 || assignment[i] >= k) {
            throw std::invalid_argument("MVBDD::evaluate: value out of range");
        }
    }

    bddp f = root;
    while (!(f & BDD_CONST_FLAG)) {
        bddvar dv = node_var(f);
        bddvar mv = var_table_->mvdd_var_of(dv);
        int idx = var_table_->dd_var_index(dv);
        int val = assignment[mv - 1];

        bool take_hi = (val > 0 && idx == val - 1);
        f = take_hi ? BDD::child1(f) : BDD::child0(f);
    }
    return f == bddtrue;
}

// --- Conversion ---

BDD MVBDD::to_bdd() const {
    return BDD_ID(root);
}

MVBDD MVBDD::from_bdd(const MVBDD& base, const BDD& bdd) {
    if (!base.var_table_) {
        throw std::invalid_argument("MVBDD::from_bdd: base has no var table");
    }
    validate_dd_vars_registered(bdd.get_id(), base.var_table_.get(),
                                "MVBDD::from_bdd");
    return MVBDD(base.var_table_, bdd.get_id());
}

// --- Terminal checks ---

bool MVBDD::is_zero() const { return root == bddfalse; }
bool MVBDD::is_one() const { return root == bddtrue; }
bool MVBDD::is_terminal() const { return (root & BDD_CONST_FLAG) != 0; }

// --- Node count ---

uint64_t MVBDD::mvbdd_node_count() const {
    if (!var_table_) return 0;
    return mvdd_count_entries(root, var_table_.get());
}

uint64_t MVBDD::size() const {
    return raw_size();
}

// --- Comparison ---

bool MVBDD::operator==(const MVBDD& other) const {
    return var_table_.get() == other.var_table_.get() && root == other.root;
}

bool MVBDD::operator!=(const MVBDD& other) const {
    return !(*this == other);
}

// --- Private helpers ---

void MVBDD::check_compatible(const MVBDD& other) const {
    if (var_table_.get() != other.var_table_.get()) {
        throw std::invalid_argument(
            "MVBDD: incompatible var tables");
    }
}

MVBDD MVBDD::make_result(bddp p) const {
    return MVBDD(var_table_, p);
}

// ============================================================
//  MVZDD
// ============================================================

// --- Private bddp constructor ---

MVZDD::MVZDD(std::shared_ptr<MVDDVarTable> table, bddp p)
    : DDBase(), var_table_(table)
{
    root = p;
}

// --- Constructors ---

MVZDD::MVZDD() : DDBase(-1), var_table_(nullptr) {}

MVZDD::MVZDD(int k, bool value)
    : DDBase(value ? 1 : 0),
      var_table_(std::make_shared<MVDDVarTable>(k))
{
}

MVZDD::MVZDD(std::shared_ptr<MVDDVarTable> table, const ZDD& zdd)
    : DDBase(), var_table_(table)
{
    root = zdd.get_id();
    if (table) {
        validate_dd_vars_registered(root, table.get(), "MVZDD(table, ZDD)");
    }
}

MVZDD::MVZDD(const MVZDD& other)
    : DDBase(other), var_table_(other.var_table_)
{
}

MVZDD::MVZDD(MVZDD&& other)
    : DDBase(std::move(other)), var_table_(std::move(other.var_table_))
{
}

MVZDD& MVZDD::operator=(const MVZDD& other) {
    DDBase::operator=(other);
    var_table_ = other.var_table_;
    return *this;
}

MVZDD& MVZDD::operator=(MVZDD&& other) {
    DDBase::operator=(std::move(other));
    var_table_ = std::move(other.var_table_);
    return *this;
}

// --- Factory methods ---

MVZDD MVZDD::zero(std::shared_ptr<MVDDVarTable> table) {
    return MVZDD(table, bddfalse);
}

MVZDD MVZDD::one(std::shared_ptr<MVDDVarTable> table) {
    return MVZDD(table, bddtrue);
}

MVZDD MVZDD::singleton(const MVZDD& base, bddvar mv, int value) {
    if (!base.var_table_) {
        throw std::invalid_argument("MVZDD::singleton: base has no var table");
    }
    int k = base.var_table_->k();
    if (value < 0 || value >= k) {
        throw std::invalid_argument("MVZDD::singleton: value out of range");
    }
    // Validate mv range (dd_vars_of throws std::out_of_range for invalid mv)
    const std::vector<bddvar>& dvars = base.var_table_->dd_vars_of(mv);

    if (value == 0) {
        // All dd_vars are 0 → the set is {} → family {∅} = bddsingle
        return base.make_result(bddsingle);
    }

    // value > 0: the set is {dd_vars[value-1]}
    bddp result = ZDD::getnode(dvars[value - 1], bddempty, bddsingle);
    return base.make_result(result);
}

// --- Variable management ---

bddvar MVZDD::new_var() {
    if (!var_table_) {
        throw std::logic_error("MVZDD::new_var: no var table");
    }
    int k = var_table_->k();
    std::vector<bddvar> dvars;
    dvars.reserve(k - 1);
    for (int i = 0; i < k - 1; ++i) {
        dvars.push_back(bddnewvar());
    }
    return var_table_->register_var(dvars);
}

int MVZDD::k() const {
    if (!var_table_) return 0;
    return var_table_->k();
}

std::shared_ptr<MVDDVarTable> MVZDD::var_table() const {
    return var_table_;
}

// --- ITE construction ---

MVZDD MVZDD::ite(const MVZDD& base, bddvar mv,
                  const std::vector<MVZDD>& children) {
    if (!base.var_table_) {
        throw std::invalid_argument("MVZDD::ite: base has no var table");
    }
    int k = base.var_table_->k();
    if (static_cast<int>(children.size()) != k) {
        throw std::invalid_argument("MVZDD::ite: children size must equal k");
    }
    if (mv < 1 || mv > base.var_table_->mvdd_var_count()) {
        throw std::invalid_argument("MVZDD::ite: mv out of range");
    }
    for (int i = 0; i < k; ++i) {
        if (children[i].var_table_.get() != base.var_table_.get()) {
            throw std::invalid_argument(
                "MVZDD::ite: children must share the same var table");
        }
    }

    const std::vector<bddvar>& dvars = base.var_table_->dd_vars_of(mv);

    // Build using ZDD operations.
    // First strip all dd_vars of mv from each child (project out mv).
    // Then for value 0: keep sets unchanged (no dd_vars of mv).
    // For value i > 0: add dd_vars[i-1] to all sets via Change.
    // Finally take the union of all.

    // Helper: strip all dd_vars of mv from a ZDD by projecting them out.
    // Offset(f, v) = sets without v; OnSet0(f, v) = sets with v, v removed.
    // Union of both = all sets with v removed.
    auto strip_mv_dvars = [&dvars, k](bddp f) -> bddp {
        for (int j = 0; j < k - 1; ++j) {
            f = bddunion(bddoffset(f, dvars[j]),
                         bddonset0(f, dvars[j]));
        }
        return f;
    };

    bddp result = strip_mv_dvars(children[0].root);
    for (int i = 1; i < k; ++i) {
        bddp stripped = strip_mv_dvars(children[i].root);
        bddp modified = bddchange(stripped, dvars[i - 1]);
        result = bddunion(result, modified);
    }

    return base.make_result(result);
}

MVZDD MVZDD::ite(const MVZDD& base, bddvar mv,
                  std::initializer_list<MVZDD> children) {
    return ite(base, mv, std::vector<MVZDD>(children));
}

// --- Node reference ---

bddp MVZDD::get_id() const { return root; }

// --- Child node access ---

MVZDD MVZDD::child(int value) const {
    if (root == bddnull) {
        throw std::invalid_argument("MVZDD::child: null node");
    }
    if (root & BDD_CONST_FLAG) {
        throw std::invalid_argument("MVZDD::child: terminal node");
    }
    if (!var_table_) {
        throw std::logic_error("MVZDD::child: no var table");
    }

    bddvar mv = top_var();
    int k = var_table_->k();
    if (value < 0 || value >= k) {
        throw std::invalid_argument("MVZDD::child: value out of range");
    }

    const std::vector<bddvar>& dvars = var_table_->dd_vars_of(mv);

    bddp f = root;

    if (value == 0) {
        // Take lo at every dd_var level
        for (int i = k - 2; i >= 0; --i) {
            if (f & BDD_CONST_FLAG) break;
            if (node_var(f) != dvars[i]) continue;
            f = ZDD::child0(f);
        }
    } else {
        // Take lo from dd_vars[k-2] down to dd_vars[value]
        for (int i = k - 2; i >= value; --i) {
            if (f & BDD_CONST_FLAG) break;
            if (node_var(f) != dvars[i]) continue;
            f = ZDD::child0(f);
        }
        // Take hi at dd_vars[value-1]
        if (!(f & BDD_CONST_FLAG) && node_var(f) == dvars[value - 1]) {
            f = ZDD::child1(f);
        } else {
            // Zero-suppressed → hi is bddempty
            f = bddempty;
        }
    }

    return make_result(f);
}

bddvar MVZDD::top_var() const {
    if (root == bddnull || (root & BDD_CONST_FLAG)) return 0;
    if (!var_table_) return 0;
    bddvar dv = node_var(root);
    bddvar mv = var_table_->mvdd_var_of(dv);
    if (mv == 0) {
        throw std::logic_error(
            "MVZDD::top_var: root DD variable " + std::to_string(dv) +
            " is not registered in the var table");
    }
    return mv;
}

// --- Set family operations ---

MVZDD MVZDD::operator+(const MVZDD& other) const {
    check_compatible(other);
    return make_result(bddunion(root, other.root));
}

MVZDD MVZDD::operator-(const MVZDD& other) const {
    check_compatible(other);
    return make_result(bddsubtract(root, other.root));
}

MVZDD MVZDD::operator&(const MVZDD& other) const {
    check_compatible(other);
    return make_result(bddintersec(root, other.root));
}

MVZDD& MVZDD::operator+=(const MVZDD& other) {
    check_compatible(other);
    root = bddunion(root, other.root);
    return *this;
}

MVZDD& MVZDD::operator-=(const MVZDD& other) {
    check_compatible(other);
    root = bddsubtract(root, other.root);
    return *this;
}

MVZDD& MVZDD::operator&=(const MVZDD& other) {
    check_compatible(other);
    root = bddintersec(root, other.root);
    return *this;
}

// --- Counting ---

double MVZDD::count() const {
    return bddcount(root);
}

bigint::BigInt MVZDD::exact_count() const {
    return bddexactcount(root);
}

// --- Evaluation ---

bool MVZDD::evaluate(const std::vector<int>& assignment) const {
    if (!var_table_) {
        throw std::logic_error("MVZDD::evaluate: no var table");
    }
    if (assignment.size() != var_table_->mvdd_var_count()) {
        throw std::invalid_argument("MVZDD::evaluate: assignment size mismatch");
    }
    int kv = var_table_->k();

    // Build DD-level assignment: dd_assign[v] = 0 or 1
    bddvar num_vars = bddvarused();
    std::vector<int> dd_assign(num_vars + 1, 0);

    for (bddvar mv = 1; mv <= var_table_->mvdd_var_count(); ++mv) {
        int val = assignment[mv - 1];
        if (val < 0 || val >= kv) {
            throw std::invalid_argument("MVZDD::evaluate: value out of range");
        }
        if (val > 0) {
            const std::vector<bddvar>& dvars = var_table_->dd_vars_of(mv);
            dd_assign[dvars[val - 1]] = 1;
        }
    }

    // Traverse ZDD level by level (handles zero-suppressed variables)
    bddp f = root;
    for (bddvar lev = num_vars; lev >= 1; --lev) {
        bddvar v = bddvaroflev(lev);

        if (f & BDD_CONST_FLAG) {
            // Terminal reached. Check zero-suppressed vars
            if (v <= num_vars && dd_assign[v]) {
                return false;
            }
            continue;
        }

        bddvar f_var = node_var(f);

        if (f_var == v) {
            if (dd_assign[v]) {
                f = ZDD::child1(f);
            } else {
                f = ZDD::child0(f);
            }
        } else {
            // Zero-suppressed: hi = bddempty
            if (dd_assign[v]) {
                return false;
            }
        }
    }

    return f == bddtrue;
}

// --- Enumeration / display ---

std::vector<std::vector<int>> MVZDD::enumerate() const {
    if (!var_table_) return std::vector<std::vector<int>>();

    ZDD z = to_zdd();
    std::vector<std::vector<bddvar>> raw_sets = z.enumerate();

    bddvar n = var_table_->mvdd_var_count();
    std::vector<std::vector<int>> result;
    result.reserve(raw_sets.size());

    for (size_t s = 0; s < raw_sets.size(); ++s) {
        std::vector<int> assign(n, 0);
        for (size_t j = 0; j < raw_sets[s].size(); ++j) {
            bddvar dv = raw_sets[s][j];
            bddvar mv = var_table_->mvdd_var_of(dv);
            int idx = var_table_->dd_var_index(dv);
            if (mv >= 1 && mv <= n) {
                assign[mv - 1] = idx + 1;
            }
        }
        result.push_back(assign);
    }
    return result;
}

void MVZDD::print_sets(std::ostream& os) const {
    std::vector<std::vector<int>> sets = enumerate();
    for (size_t i = 0; i < sets.size(); ++i) {
        os << "{";
        for (size_t j = 0; j < sets[i].size(); ++j) {
            if (j > 0) os << ",";
            os << sets[i][j];
        }
        os << "}\n";
    }
}

void MVZDD::print_sets(std::ostream& os,
                        const std::vector<std::string>& var_names) const {
    std::vector<std::vector<int>> sets = enumerate();
    for (size_t i = 0; i < sets.size(); ++i) {
        os << "{";
        for (size_t j = 0; j < sets[i].size(); ++j) {
            if (j > 0) os << ", ";
            if (j < var_names.size()) {
                os << var_names[j] << "=" << sets[i][j];
            } else {
                os << "x" << (j + 1) << "=" << sets[i][j];
            }
        }
        os << "}\n";
    }
}

std::string MVZDD::to_str() const {
    std::ostringstream oss;
    print_sets(oss);
    return oss.str();
}

// --- Conversion ---

ZDD MVZDD::to_zdd() const {
    return ZDD_ID(root);
}

MVZDD MVZDD::from_zdd(const MVZDD& base, const ZDD& zdd) {
    if (!base.var_table_) {
        throw std::invalid_argument("MVZDD::from_zdd: base has no var table");
    }
    validate_dd_vars_registered(zdd.get_id(), base.var_table_.get(),
                                "MVZDD::from_zdd");
    return MVZDD(base.var_table_, zdd.get_id());
}

// --- Terminal checks ---

bool MVZDD::is_zero() const { return root == bddfalse; }
bool MVZDD::is_one() const { return root == bddtrue; }
bool MVZDD::is_terminal() const { return (root & BDD_CONST_FLAG) != 0; }

// --- Node count ---

uint64_t MVZDD::mvzdd_node_count() const {
    if (!var_table_) return 0;
    return mvdd_count_entries(root, var_table_.get());
}

uint64_t MVZDD::size() const {
    return raw_size();
}

// --- Comparison ---

bool MVZDD::operator==(const MVZDD& other) const {
    return var_table_.get() == other.var_table_.get() && root == other.root;
}

bool MVZDD::operator!=(const MVZDD& other) const {
    return !(*this == other);
}

// --- Private helpers ---

void MVZDD::check_compatible(const MVZDD& other) const {
    if (var_table_.get() != other.var_table_.get()) {
        throw std::invalid_argument(
            "MVZDD: incompatible var tables");
    }
}

MVZDD MVZDD::make_result(bddp p) const {
    return MVZDD(var_table_, p);
}
