#include "bdd.h"
#include "bdd_internal.h"
#include <stdexcept>

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
