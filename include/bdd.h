#ifndef KYOTODD_BDD_H
#define KYOTODD_BDD_H

/**
 * @note Thread safety: This library is NOT thread-safe. All BDD/ZDD
 * operations use shared global state (node table, unique tables, cache,
 * GC roots) without synchronization. All calls must be made from a
 * single thread, or externally serialized by the caller.
 */

#include "bdd_types.h"
#include "bdd_base.h"
#include "bdd_ops.h"
#include "bdd_io.h"
#include <iostream>

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
    b.root = bddlshiftb(root, shift);
    return b;
}

inline BDD& BDD::operator<<=(bddvar shift) {
    root = bddlshiftb(root, shift);
    return *this;
}

inline BDD BDD::operator>>(bddvar shift) const {
    BDD b(0);
    b.root = bddrshiftb(root, shift);
    return b;
}

inline BDD& BDD::operator>>=(bddvar shift) {
    root = bddrshiftb(root, shift);
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

inline uint64_t BDD::raw_size() const {
    return bddsize(root);
}

inline uint64_t BDD::plain_size() const {
    return bddplainsize(root, false);
}

inline uint64_t BDD::raw_size(const std::vector<BDD>& v) {
    std::vector<bddp> roots;
    roots.reserve(v.size());
    for (const auto& b : v) roots.push_back(b.GetID());
    return bddrawsize(roots);
}

inline uint64_t BDD::plain_size(const std::vector<BDD>& v) {
    std::vector<bddp> roots;
    roots.reserve(v.size());
    for (const auto& b : v) roots.push_back(b.GetID());
    return bddplainsize(roots, false);
}

inline BDD BDD::Ite(const BDD& f, const BDD& g, const BDD& h) {
    BDD b(0);
    b.root = bddite(f.root, g.root, h.root);
    return b;
}

inline BDD BDD::Swap(bddvar v1, bddvar v2) const {
    BDD b(0);
    b.root = bddswap(root, v1, v2);
    return b;
}

inline BDD BDD::Smooth(bddvar v) const {
    BDD b(0);
    b.root = bddsmooth(root, v);
    return b;
}

inline BDD BDD::Spread(int k) const {
    BDD b(0);
    b.root = bddspread(root, k);
    return b;
}

inline void BDD::Export(FILE* strm) const {
    bddp p = root;
    bddexport(strm, &p, 1);
}

inline void BDD::Export(std::ostream& strm) const {
    bddp p = root;
    bddexport(strm, &p, 1);
}

inline bddvar BDD::Top() const {
    return bddtop(root);
}

inline void BDD::Print() const {
    bddvar v = bddtop(root);
    bddvar lev = bddlevofvar(v);
    std::cout << "[ " << root
              << " Var:" << v << "(" << lev << ")"
              << " Size:" << bddsize(root)
              << " ]";
    std::cout.flush();
}

inline void BDD::XPrint0() const {
    bddgraph0(root);
}

inline void BDD::XPrint() const {
    bddgraph(root);
}

// ZDD member functions

inline ZDD ZDD::operator~() const {
    ZDD z(0);
    z.root = bddnot(root);
    return z;
}

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

inline ZDD ZDD::Intersec(const ZDD& other) const {
    ZDD z(0);
    z.root = bddintersec(root, other.root);
    return z;
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

inline ZDD ZDD::operator<<(bddvar s) const {
    ZDD z(0);
    z.root = bddlshiftz(root, s);
    return z;
}

inline ZDD& ZDD::operator<<=(bddvar s) {
    root = bddlshiftz(root, s);
    return *this;
}

inline ZDD ZDD::operator>>(bddvar s) const {
    ZDD z(0);
    z.root = bddrshiftz(root, s);
    return z;
}

inline ZDD& ZDD::operator>>=(bddvar s) {
    root = bddrshiftz(root, s);
    return *this;
}

// ZDD high-level member functions

inline ZDD ZDD::Meet(const ZDD& other) const {
    ZDD z(0);
    z.root = bddmeet(root, other.root);
    return z;
}

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

inline double ZDD::count() const {
    return bddcount(root);
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

inline ZDD ZDD::Support() const {
    ZDD z(0);
    z.root = bddsupport(root);
    return z;
}

inline bddvar ZDD::Top() const {
    return bddtop(root);
}

inline uint64_t ZDD::Size() const {
    return bddsize(root);
}

inline uint64_t ZDD::raw_size() const {
    return bddsize(root);
}

inline uint64_t ZDD::plain_size() const {
    return bddplainsize(root, true);
}

inline uint64_t ZDD::raw_size(const std::vector<ZDD>& v) {
    std::vector<bddp> roots;
    roots.reserve(v.size());
    for (const auto& z : v) roots.push_back(z.GetID());
    return bddrawsize(roots);
}

inline uint64_t ZDD::plain_size(const std::vector<ZDD>& v) {
    std::vector<bddp> roots;
    roots.reserve(v.size());
    for (const auto& z : v) roots.push_back(z.GetID());
    return bddplainsize(roots, true);
}

inline uint64_t ZDD::Lit() const {
    return bddlit(root);
}

inline uint64_t ZDD::Len() const {
    return bddlen(root);
}

inline char* ZDD::CardMP16(char* s) const {
    return bddcardmp16(root, s);
}

inline void ZDD::Export(FILE* strm) const {
    bddp p = root;
    bddexport(strm, &p, 1);
}

inline void ZDD::Export(std::ostream& strm) const {
    bddp p = root;
    bddexport(strm, &p, 1);
}

inline void ZDD::XPrint() const {
    bddgraph(root);
}

inline int ZDD::IsPoly() const {
    return bddispoly(root);
}

inline ZDD ZDD::Swap(bddvar v1, bddvar v2) const {
    ZDD z(0);
    z.root = bddswapz(root, v1, v2);
    return z;
}

inline int ZDD::ImplyChk(bddvar v1, bddvar v2) const {
    return bddimplychk(root, v1, v2);
}

inline int ZDD::CoImplyChk(bddvar v1, bddvar v2) const {
    return bddcoimplychk(root, v1, v2);
}

inline ZDD ZDD::PermitSym(int n) const {
    ZDD z(0);
    z.root = bddpermitsym(root, n);
    return z;
}

inline ZDD ZDD::Always() const {
    ZDD z(0);
    z.root = bddalways(root);
    return z;
}

inline int ZDD::SymChk(bddvar v1, bddvar v2) const {
    return bddsymchk(root, v1, v2);
}

inline ZDD ZDD::ImplySet(bddvar v) const {
    ZDD z(0);
    z.root = bddimplyset(root, v);
    return z;
}

inline ZDD ZDD::SymGrp() const {
    ZDD z(0);
    z.root = bddsymgrp(root);
    return z;
}

inline ZDD ZDD::SymGrpNaive() const {
    ZDD z(0);
    z.root = bddsymgrpnaive(root);
    return z;
}

inline ZDD ZDD::SymSet(bddvar v) const {
    ZDD z(0);
    z.root = bddsymset(root, v);
    return z;
}

inline ZDD ZDD::CoImplySet(bddvar v) const {
    ZDD z(0);
    z.root = bddcoimplyset(root, v);
    return z;
}

inline ZDD ZDD::Divisor() const {
    ZDD z(0);
    z.root = bdddivisor(root);
    return z;
}

#endif
