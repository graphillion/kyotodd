#include "bdd.h"

// --- Complement expansion helpers ---

// Expand BDD/QDD complement edges: ~(var, lo, hi) = (var, ~lo, ~hi)
static bddp expand_bdd_impl(bddp f,
                             std::unordered_map<bddp, bddp>& memo) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) return f;  // terminals have no complement edge

    // Use f directly as memo key (encodes base + comp)
    auto it = memo.find(f);
    if (it != memo.end()) return it->second;

    bddp base = f & ~BDD_COMP_FLAG;
    bool comp = (f & BDD_COMP_FLAG) != 0;

    bddp lo = node_lo(base);
    bddp hi = node_hi(base);

    if (comp) {
        // BDD: ~(var, lo, hi) = (var, ~lo, ~hi)
        lo = bddnot(lo);
        hi = bddnot(hi);
    }

    bddp elo = expand_bdd_impl(lo, memo);
    bddp ehi = expand_bdd_impl(hi, memo);

    bddp node_id = allocate_node();
    node_write(node_id, node_var(base), elo, ehi);
    // Do not set reduced flag — this is an unreduced node

    memo[f] = node_id;
    return node_id;
}

// Expand ZDD complement edges: ~(var, lo, hi) = (var, ~lo, hi)
static bddp expand_zdd_impl(bddp f,
                             std::unordered_map<bddp, bddp>& memo) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) return f;

    auto it = memo.find(f);
    if (it != memo.end()) return it->second;

    bddp base = f & ~BDD_COMP_FLAG;
    bool comp = (f & BDD_COMP_FLAG) != 0;

    bddp lo = node_lo(base);
    bddp hi = node_hi(base);

    if (comp) {
        // ZDD: ~(var, lo, hi) = (var, ~lo, hi)
        lo = bddnot(lo);
        // hi unchanged
    }

    bddp elo = expand_zdd_impl(lo, memo);
    bddp ehi = expand_zdd_impl(hi, memo);

    bddp node_id = allocate_node();
    node_write(node_id, node_var(base), elo, ehi);

    memo[f] = node_id;
    return node_id;
}

// --- Complement expansion constructors ---

UnreducedDD::UnreducedDD(const BDD& bdd) : DDBase() {
    bddp id = bdd.get_id();
    if (id == bddnull || (id & BDD_CONST_FLAG)) {
        root = id;
        return;
    }
    root = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        return expand_bdd_impl(id, memo);
    });
}

UnreducedDD::UnreducedDD(const ZDD& zdd) : DDBase() {
    bddp id = zdd.get_id();
    if (id == bddnull || (id & BDD_CONST_FLAG)) {
        root = id;
        return;
    }
    root = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        return expand_zdd_impl(id, memo);
    });
}

UnreducedDD::UnreducedDD(const QDD& qdd) : DDBase() {
    bddp id = qdd.get_id();
    if (id == bddnull || (id & BDD_CONST_FLAG)) {
        root = id;
        return;
    }
    // QDD uses BDD complement semantics
    root = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        return expand_bdd_impl(id, memo);
    });
}

// --- Raw wrap ---

UnreducedDD UnreducedDD::wrap_raw(const BDD& bdd) {
    UnreducedDD r(0);
    r.root = bdd.get_id();
    return r;
}

UnreducedDD UnreducedDD::wrap_raw(const ZDD& zdd) {
    UnreducedDD r(0);
    r.root = zdd.get_id();
    return r;
}

UnreducedDD UnreducedDD::wrap_raw(const QDD& qdd) {
    UnreducedDD r(0);
    r.root = qdd.get_id();
    return r;
}

// --- Node creation ---

UnreducedDD UnreducedDD::getnode(bddvar var, const UnreducedDD& lo,
                                 const UnreducedDD& hi) {
    bddp lo_id = lo.root;
    bddp hi_id = hi.root;

    // Always create an unreduced node:
    // no complement normalization, no reduction rules, no unique table.
    UnreducedDD result(0);
    result.root = bdd_gc_guard([&]() -> bddp {
        bddp node_id = allocate_node();
        node_write(node_id, var, lo_id, hi_id);
        return node_id;
    });
    return result;
}

// --- Child accessors (member versions) ---

UnreducedDD UnreducedDD::raw_child0() const {
    UnreducedDD r(0);
    r.root = DDBase::raw_child0(root);
    return r;
}

UnreducedDD UnreducedDD::raw_child1() const {
    UnreducedDD r(0);
    r.root = DDBase::raw_child1(root);
    return r;
}

UnreducedDD UnreducedDD::raw_child(int child) const {
    UnreducedDD r(0);
    r.root = DDBase::raw_child(root, child);
    return r;
}

// --- Child mutation ---

void UnreducedDD::set_child0(const UnreducedDD& c) {
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

void UnreducedDD::set_child1(const UnreducedDD& c) {
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

UnreducedDD UnreducedDD::operator~() const {
    UnreducedDD r(0);
    r.root = bddnot(root);
    return r;
}

// --- Query ---

bool UnreducedDD::is_reduced() const {
    if (root == bddnull) return false;
    if (root & BDD_CONST_FLAG) return true;
    return node_is_reduced(root & ~BDD_COMP_FLAG);
}

// --- Reduce (common implementation) ---

typedef bddp (*getnode_raw_fn)(bddvar, bddp, bddp);

static bddp reduce_impl(bddp f, std::unordered_map<bddp, bddp>& memo,
                         getnode_raw_fn make_node) {
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

    bddp rlo = reduce_impl(lo, memo, make_node);
    bddp rhi = reduce_impl(hi, memo, make_node);

    bddp result = make_node(node_var(base), rlo, rhi);
    memo[base] = result;
    return comp ? bddnot(result) : result;
}

BDD UnreducedDD::reduce_as_bdd() const {
    bddp result = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        return reduce_impl(root, memo, BDD::getnode_raw);
    });
    return BDD_ID(result);
}

ZDD UnreducedDD::reduce_as_zdd() const {
    bddp result = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        return reduce_impl(root, memo, ZDD::getnode_raw);
    });
    return ZDD_ID(result);
}

QDD UnreducedDD::reduce_as_qdd() const {
    return reduce_as_bdd().to_qdd();
}
