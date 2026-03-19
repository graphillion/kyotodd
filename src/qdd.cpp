#include "bdd.h"
#include "bdd_internal.h"
#include <stdexcept>
#include <unordered_map>

const QDD QDD::False(0);
const QDD QDD::True(1);
const QDD QDD::Null(-1);

QDD QDD::node(bddvar var, const QDD& lo, const QDD& hi) {
    QDD q(0);
    q.root = bdd_gc_guard([&]() -> bddp {
        return getqnode(var, lo.root, hi.root);
    });
    return q;
}

// --- QDD to BDD conversion ---

static bddp qdd_to_bdd_impl(bddp f, std::unordered_map<bddp, bddp>& memo) {
    BDD_RecurGuard guard;
    if (f & BDD_CONST_FLAG) return f;

    bddp base = f & ~BDD_COMP_FLAG;
    bool comp = (f & BDD_COMP_FLAG) != 0;

    auto it = memo.find(base);
    if (it != memo.end())
        return comp ? bddnot(it->second) : it->second;

    bddp lo = node_lo(base);
    bddp hi = node_hi(base);

    bddp rlo = qdd_to_bdd_impl(lo, memo);
    bddp rhi = qdd_to_bdd_impl(hi, memo);

    bddp result = getnode(node_var(base), rlo, rhi);  // jump rule applied
    memo[base] = result;
    return comp ? bddnot(result) : result;
}

BDD QDD::to_bdd() const {
    bddp result = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        return qdd_to_bdd_impl(root, memo);
    });
    return BDD_ID(result);
}

// --- QDD to ZDD conversion ---

static bddp qdd_to_zdd_impl(bddp f, std::unordered_map<bddp, bddp>& memo) {
    BDD_RecurGuard guard;
    if (f == bddfalse) return bddempty;
    if (f == bddtrue) return bddsingle;

    auto it = memo.find(f);
    if (it != memo.end())
        return it->second;

    // Resolve BDD complement to get explicit lo/hi
    bddp base = f & ~BDD_COMP_FLAG;
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp lo = node_lo(base);
    bddp hi = node_hi(base);
    if (comp) {
        lo = bddnot(lo);
        hi = bddnot(hi);
    }

    bddp zlo = qdd_to_zdd_impl(lo, memo);
    bddp zhi = qdd_to_zdd_impl(hi, memo);

    bddp result = getznode(node_var(base), zlo, zhi);
    memo[f] = result;
    return result;
}

ZDD QDD::to_zdd() const {
    bddp result = bdd_gc_guard([&]() -> bddp {
        std::unordered_map<bddp, bddp> memo;
        return qdd_to_zdd_impl(root, memo);
    });
    return ZDD_ID(result);
}

QDD QDD_ID(bddp p) {
    if (p != bddnull && !(p & BDD_CONST_FLAG)) {
        if (!bddp_is_reduced(p)) {
            throw std::invalid_argument("QDD_ID: node is not reduced");
        }
    }
    QDD q(0);
    q.root = p;
    return q;
}
