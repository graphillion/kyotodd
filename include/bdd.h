#ifndef KYOTODD_BDD_H
#define KYOTODD_BDD_H

#include "bdd_types.h"
#include "bdd_base.h"
#include "bdd_ops.h"
#include "bdd_io.h"

inline BDD BDD::operator&(const BDD& other) const {
    BDD b(0);
    b.root = bddand(root, other.root);
    return b;
}

inline BDD& BDD::operator&=(const BDD& other) {
    root = bddand(root, other.root);
    return *this;
}

#endif
