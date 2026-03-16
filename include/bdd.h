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

inline BDD BDD::operator<<(bddvar shift) const {
    BDD b(0);
    b.root = bddlshift(root, shift);
    return b;
}

inline BDD& BDD::operator<<=(bddvar shift) {
    root = bddlshift(root, shift);
    return *this;
}

inline BDD BDD::operator>>(bddvar shift) const {
    BDD b(0);
    b.root = bddrshift(root, shift);
    return b;
}

inline BDD& BDD::operator>>=(bddvar shift) {
    root = bddrshift(root, shift);
    return *this;
}

// BDD high-level member functions

inline BDD BDD::At0(bddvar v) const {
    BDD b(0);
    b.root = bddat0(root, v);
    return b;
}

inline BDD BDD::At1(bddvar v) const {
    BDD b(0);
    b.root = bddat1(root, v);
    return b;
}

inline BDD BDD::Exist(const BDD& cube) const {
    BDD b(0);
    b.root = bddexist(root, cube.root);
    return b;
}

inline BDD BDD::Exist(const std::vector<bddvar>& vars) const {
    BDD b(0);
    b.root = bddexist(root, vars);
    return b;
}

inline BDD BDD::Exist(bddvar v) const {
    BDD b(0);
    b.root = bddexistvar(root, v);
    return b;
}

inline BDD BDD::Univ(const BDD& cube) const {
    BDD b(0);
    b.root = bdduniv(root, cube.root);
    return b;
}

inline BDD BDD::Univ(const std::vector<bddvar>& vars) const {
    BDD b(0);
    b.root = bdduniv(root, vars);
    return b;
}

inline BDD BDD::Univ(bddvar v) const {
    BDD b(0);
    b.root = bddunivvar(root, v);
    return b;
}

inline BDD BDD::Cofactor(const BDD& g) const {
    BDD b(0);
    b.root = bddcofactor(root, g.root);
    return b;
}

inline BDD BDD::Support() const {
    BDD b(0);
    b.root = bddsupport(root);
    return b;
}

inline std::vector<bddvar> BDD::SupportVec() const {
    return bddsupport_vec(root);
}

inline int BDD::Imply(const BDD& g) const {
    return bddimply(root, g.root);
}

inline uint64_t BDD::Size() const {
    return bddsize(root);
}

inline BDD BDD::Ite(const BDD& f, const BDD& g, const BDD& h) {
    BDD b(0);
    b.root = bddite(f.root, g.root, h.root);
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

inline ZDD ZDD::operator/(const ZDD& other) const {
    ZDD z(0);
    z.root = bdddiv(root, other.root);
    return z;
}

inline ZDD& ZDD::operator/=(const ZDD& other) {
    root = bdddiv(root, other.root);
    return *this;
}

inline ZDD ZDD::operator^(const ZDD& other) const {
    ZDD z(0);
    z.root = bddsymdiff(root, other.root);
    return z;
}

inline ZDD& ZDD::operator^=(const ZDD& other) {
    root = bddsymdiff(root, other.root);
    return *this;
}

inline ZDD ZDD::operator*(const ZDD& other) const {
    ZDD z(0);
    z.root = bddjoin(root, other.root);
    return z;
}

inline ZDD& ZDD::operator*=(const ZDD& other) {
    root = bddjoin(root, other.root);
    return *this;
}

inline ZDD ZDD::operator%(const ZDD& other) const {
    ZDD z(0);
    z.root = bddremainder(root, other.root);
    return z;
}

inline ZDD& ZDD::operator%=(const ZDD& other) {
    root = bddremainder(root, other.root);
    return *this;
}

// ZDD high-level member functions

inline ZDD ZDD::Maximal() const {
    ZDD z(0);
    z.root = bddmaximal(root);
    return z;
}

inline ZDD ZDD::Minimal() const {
    ZDD z(0);
    z.root = bddminimal(root);
    return z;
}

inline ZDD ZDD::Minhit() const {
    ZDD z(0);
    z.root = bddminhit(root);
    return z;
}

inline ZDD ZDD::Closure() const {
    ZDD z(0);
    z.root = bddclosure(root);
    return z;
}

inline uint64_t ZDD::Card() const {
    return bddcard(root);
}

inline bigint::BigInt ZDD::ExactCount() const {
    return bddexactcount(root);
}

inline ZDD ZDD::Restrict(const ZDD& g) const {
    ZDD z(0);
    z.root = bddrestrict(root, g.root);
    return z;
}

inline ZDD ZDD::Permit(const ZDD& g) const {
    ZDD z(0);
    z.root = bddpermit(root, g.root);
    return z;
}

inline ZDD ZDD::Nonsup(const ZDD& g) const {
    ZDD z(0);
    z.root = bddnonsup(root, g.root);
    return z;
}

inline ZDD ZDD::Nonsub(const ZDD& g) const {
    ZDD z(0);
    z.root = bddnonsub(root, g.root);
    return z;
}

inline ZDD ZDD::Disjoin(const ZDD& g) const {
    ZDD z(0);
    z.root = bdddisjoin(root, g.root);
    return z;
}

inline ZDD ZDD::Jointjoin(const ZDD& g) const {
    ZDD z(0);
    z.root = bddjointjoin(root, g.root);
    return z;
}

inline ZDD ZDD::Delta(const ZDD& g) const {
    ZDD z(0);
    z.root = bdddelta(root, g.root);
    return z;
}

#endif
