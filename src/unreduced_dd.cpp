#include "bdd.h"

// --- UnreducedBDD implementation ---

UnreducedBDD UnreducedBDD::getnode(bddvar var, const UnreducedBDD& lo,
                                 const UnreducedBDD& hi) {
    bddp lo_id = lo.root;
    bddp hi_id = hi.root;

    // If both children are reduced and lo != hi, delegate to getnode
    // (which applies complement normalization and uses the unique table)
    if (lo_id != hi_id &&
        bddp_is_reduced(lo_id) && bddp_is_reduced(hi_id)) {
        UnreducedBDD result(0);
        result.root = bdd_gc_guard([&]() -> bddp {
            return BDD::getnode_raw(var, lo_id, hi_id);
        });
        return result;
    }

    // Unreduced path: no complement normalization, no unique table
    UnreducedBDD result(0);
    result.root = bdd_gc_guard([&]() -> bddp {
        bddp node_id = allocate_node();
        node_write(node_id, var, lo_id, hi_id);
        // reduced flag defaults to 0 (unreduced) — do not set it
        return node_id;
    });
    return result;
}

// --- Child accessors (static bddp versions, BDD complement semantics) ---

bddp UnreducedBDD::child0(bddp f) {
    if (f == bddnull)
        throw std::invalid_argument("child0: null node");
    if (f & BDD_CONST_FLAG)
        throw std::invalid_argument("child0: terminal node");
    bddp lo = node_lo(f);
    if (f & BDD_COMP_FLAG) lo = bddnot(lo);
    return lo;
}

bddp UnreducedBDD::child1(bddp f) {
    if (f == bddnull)
        throw std::invalid_argument("child1: null node");
    if (f & BDD_CONST_FLAG)
        throw std::invalid_argument("child1: terminal node");
    bddp hi = node_hi(f);
    if (f & BDD_COMP_FLAG) hi = bddnot(hi);
    return hi;
}

bddp UnreducedBDD::child(bddp f, int child) {
    if (child == 0) return child0(f);
    if (child == 1) return child1(f);
    throw std::invalid_argument("child: child must be 0 or 1");
}

// --- Child accessors (member versions) ---

UnreducedBDD UnreducedBDD::raw_child0() const {
    UnreducedBDD r(0);
    r.root = DDBase::raw_child0(root);
    return r;
}

UnreducedBDD UnreducedBDD::raw_child1() const {
    UnreducedBDD r(0);
    r.root = DDBase::raw_child1(root);
    return r;
}

UnreducedBDD UnreducedBDD::raw_child(int child) const {
    UnreducedBDD r(0);
    r.root = DDBase::raw_child(root, child);
    return r;
}

UnreducedBDD UnreducedBDD::child0() const {
    UnreducedBDD r(0);
    r.root = UnreducedBDD::child0(root);
    return r;
}

UnreducedBDD UnreducedBDD::child1() const {
    UnreducedBDD r(0);
    r.root = UnreducedBDD::child1(root);
    return r;
}

UnreducedBDD UnreducedBDD::child(int child) const {
    UnreducedBDD r(0);
    r.root = UnreducedBDD::child(root, child);
    return r;
}

// --- Child mutation ---

void UnreducedBDD::set_child0(const UnreducedBDD& c) {
    if (root == bddnull)
        throw std::invalid_argument("set_child0: null node");
    if (root & BDD_CONST_FLAG)
        throw std::invalid_argument("set_child0: terminal node");
    if (root & BDD_COMP_FLAG)
        throw std::invalid_argument("set_child0: complement reference");
    if (node_is_reduced(root))
        throw std::invalid_argument("set_child0: node is reduced");
    bddp hi = node_hi(root);
    node_write(root, node_var(root), c.root, hi);
}

void UnreducedBDD::set_child1(const UnreducedBDD& c) {
    if (root == bddnull)
        throw std::invalid_argument("set_child1: null node");
    if (root & BDD_CONST_FLAG)
        throw std::invalid_argument("set_child1: terminal node");
    if (root & BDD_COMP_FLAG)
        throw std::invalid_argument("set_child1: complement reference");
    if (node_is_reduced(root))
        throw std::invalid_argument("set_child1: node is reduced");
    bddp lo = node_lo(root);
    node_write(root, node_var(root), lo, c.root);
}

// --- Operators ---

UnreducedBDD UnreducedBDD::operator~() const {
    UnreducedBDD r(0);
    r.root = bddnot(root);
    return r;
}

// --- Query ---

bool UnreducedBDD::is_reduced() const {
    if (root == bddnull) return false;
    if (root & BDD_CONST_FLAG) return true;
    return node_is_reduced(root & ~BDD_COMP_FLAG);
}

// --- Reduction ---

static bddp reduce_bdd_impl(bddp f, std::unordered_map<bddp, bddp>& memo) {
    BDD_RecurGuard guard;
    if (f == bddnull)
        throw std::invalid_argument("reduce: null node encountered");
    if (f & BDD_CONST_FLAG) return f;
    if (bddp_is_reduced(f)) return f;

    bddp base = f & ~BDD_COMP_FLAG;
    bool comp = (f & BDD_COMP_FLAG) != 0;

    auto it = memo.find(base);
    if (it != memo.end())
        return comp ? bddnot(it->second) : it->second;

    // Unreduced nodes are not normalized, so use raw stored values
    bddp lo = node_lo(base);
    bddp hi = node_hi(base);

    bddp rlo = reduce_bdd_impl(lo, memo);
    bddp rhi = reduce_bdd_impl(hi, memo);

    bddp result = BDD::getnode_raw(node_var(base), rlo, rhi);
    memo[base] = result;
    return comp ? bddnot(result) : result;
}

BDD UnreducedBDD::reduce() const {
    bddp result = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        return reduce_bdd_impl(root, memo);
    });
    return BDD_ID(result);
}

// --- UnreducedZDD implementation ---

UnreducedZDD UnreducedZDD::getnode(bddvar var, const UnreducedZDD& lo,
                                 const UnreducedZDD& hi) {
    bddp lo_id = lo.root;
    bddp hi_id = hi.root;

    // If both children are reduced and hi != bddempty, delegate to getznode
    if (hi_id != bddempty &&
        bddp_is_reduced(lo_id) && bddp_is_reduced(hi_id)) {
        UnreducedZDD result(0);
        result.root = bdd_gc_guard([&]() -> bddp {
            return ZDD::getnode_raw(var, lo_id, hi_id);
        });
        return result;
    }

    // Unreduced path: no complement normalization, no unique table
    UnreducedZDD result(0);
    result.root = bdd_gc_guard([&]() -> bddp {
        bddp node_id = allocate_node();
        node_write(node_id, var, lo_id, hi_id);
        return node_id;
    });
    return result;
}

// --- ZDD Child accessors (static bddp versions, ZDD complement semantics) ---

bddp UnreducedZDD::child0(bddp f) {
    if (f == bddnull)
        throw std::invalid_argument("child0: null node");
    if (f & BDD_CONST_FLAG)
        throw std::invalid_argument("child0: terminal node");
    bddp lo = node_lo(f);
    if (f & BDD_COMP_FLAG) lo = bddnot(lo);
    return lo;
}

bddp UnreducedZDD::child1(bddp f) {
    if (f == bddnull)
        throw std::invalid_argument("child1: null node");
    if (f & BDD_CONST_FLAG)
        throw std::invalid_argument("child1: terminal node");
    // ZDD: complement does NOT affect hi
    return node_hi(f);
}

bddp UnreducedZDD::child(bddp f, int child) {
    if (child == 0) return child0(f);
    if (child == 1) return child1(f);
    throw std::invalid_argument("child: child must be 0 or 1");
}

// --- ZDD Child accessors (member versions) ---

UnreducedZDD UnreducedZDD::raw_child0() const {
    UnreducedZDD r(0);
    r.root = DDBase::raw_child0(root);
    return r;
}

UnreducedZDD UnreducedZDD::raw_child1() const {
    UnreducedZDD r(0);
    r.root = DDBase::raw_child1(root);
    return r;
}

UnreducedZDD UnreducedZDD::raw_child(int child) const {
    UnreducedZDD r(0);
    r.root = DDBase::raw_child(root, child);
    return r;
}

UnreducedZDD UnreducedZDD::child0() const {
    UnreducedZDD r(0);
    r.root = UnreducedZDD::child0(root);
    return r;
}

UnreducedZDD UnreducedZDD::child1() const {
    UnreducedZDD r(0);
    r.root = UnreducedZDD::child1(root);
    return r;
}

UnreducedZDD UnreducedZDD::child(int child) const {
    UnreducedZDD r(0);
    r.root = UnreducedZDD::child(root, child);
    return r;
}

// --- ZDD Child mutation ---

void UnreducedZDD::set_child0(const UnreducedZDD& c) {
    if (root == bddnull)
        throw std::invalid_argument("set_child0: null node");
    if (root & BDD_CONST_FLAG)
        throw std::invalid_argument("set_child0: terminal node");
    if (root & BDD_COMP_FLAG)
        throw std::invalid_argument("set_child0: complement reference");
    if (node_is_reduced(root))
        throw std::invalid_argument("set_child0: node is reduced");
    bddp hi = node_hi(root);
    node_write(root, node_var(root), c.root, hi);
}

void UnreducedZDD::set_child1(const UnreducedZDD& c) {
    if (root == bddnull)
        throw std::invalid_argument("set_child1: null node");
    if (root & BDD_CONST_FLAG)
        throw std::invalid_argument("set_child1: terminal node");
    if (root & BDD_COMP_FLAG)
        throw std::invalid_argument("set_child1: complement reference");
    if (node_is_reduced(root))
        throw std::invalid_argument("set_child1: node is reduced");
    bddp lo = node_lo(root);
    node_write(root, node_var(root), lo, c.root);
}

// --- ZDD Operators ---

UnreducedZDD UnreducedZDD::operator~() const {
    UnreducedZDD r(0);
    r.root = bddnot(root);
    return r;
}

// --- ZDD Query ---

bool UnreducedZDD::is_reduced() const {
    if (root == bddnull) return false;
    if (root & BDD_CONST_FLAG) return true;
    return node_is_reduced(root & ~BDD_COMP_FLAG);
}

// --- ZDD Reduction ---

static bddp reduce_zdd_impl(bddp f, std::unordered_map<bddp, bddp>& memo) {
    BDD_RecurGuard guard;
    if (f == bddnull)
        throw std::invalid_argument("reduce: null node encountered");
    if (f & BDD_CONST_FLAG) return f;
    if (bddp_is_reduced(f)) return f;

    bddp base = f & ~BDD_COMP_FLAG;
    bool comp = (f & BDD_COMP_FLAG) != 0;

    auto it = memo.find(base);
    if (it != memo.end())
        return comp ? bddnot(it->second) : it->second;

    bddp lo = node_lo(base);
    bddp hi = node_hi(base);

    bddp rlo = reduce_zdd_impl(lo, memo);
    bddp rhi = reduce_zdd_impl(hi, memo);

    bddp result = ZDD::getnode_raw(node_var(base), rlo, rhi);
    memo[base] = result;
    return comp ? bddnot(result) : result;
}

ZDD UnreducedZDD::reduce() const {
    bddp result = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        return reduce_zdd_impl(root, memo);
    });
    return ZDD_ID(result);
}
