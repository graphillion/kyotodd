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

inline BDD BDD::operator|(const BDD& other) const {
    BDD b(0);
    b.root = bddor(root, other.root);
    return b;
}

inline BDD& BDD::operator|=(const BDD& other) {
    root = bddor(root, other.root);
    return *this;
}

inline BDD BDD::operator^(const BDD& other) const {
    BDD b(0);
    b.root = bddxor(root, other.root);
    return b;
}

inline BDD& BDD::operator^=(const BDD& other) {
    root = bddxor(root, other.root);
    return *this;
}

inline BDD BDD::operator~() const {
    BDD b(0);
    b.root = bddnot(root);
    return b;
}

// ZDD member functions

inline ZDD ZDD::Change(bddvar var) const {
    ZDD z(0);
    z.root = bddchange(root, var);
    return z;
}

inline ZDD ZDD::Offset(bddvar var) const {
    ZDD z(0);
    z.root = bddoffset(root, var);
    return z;
}

inline ZDD ZDD::OnSet(bddvar var) const {
    ZDD z(0);
    z.root = bddonset(root, var);
    return z;
}

inline ZDD ZDD::OnSet0(bddvar var) const {
    ZDD z(0);
    z.root = bddonset0(root, var);
    return z;
}

inline ZDD ZDD::operator+(const ZDD& other) const {
    ZDD z(0);
    z.root = bddunion(root, other.root);
    return z;
}

inline ZDD& ZDD::operator+=(const ZDD& other) {
    root = bddunion(root, other.root);
    return *this;
}

inline ZDD ZDD::operator-(const ZDD& other) const {
    ZDD z(0);
    z.root = bddsubtract(root, other.root);
    return z;
}

inline ZDD& ZDD::operator-=(const ZDD& other) {
    root = bddsubtract(root, other.root);
    return *this;
}

inline ZDD ZDD::operator&(const ZDD& other) const {
    ZDD z(0);
    z.root = bddintersec(root, other.root);
    return z;
}

inline ZDD& ZDD::operator&=(const ZDD& other) {
    root = bddintersec(root, other.root);
    return *this;
}

#endif
