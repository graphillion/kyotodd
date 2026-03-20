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
#include "bdd_internal.h"
#include "bigint.hpp"
#include <iostream>

// DDBase inline definitions (bddtop/bddsize declared in bdd_base.h)
inline bddvar DDBase::top() const { return bddtop(root); }
inline uint64_t DDBase::raw_size() const { return bddsize(root); }

inline bddp DDBase::raw_child0(bddp f) {
    if (f == bddnull)
        throw std::invalid_argument("raw_child0: null node");
    if (f & BDD_CONST_FLAG)
        throw std::invalid_argument("raw_child0: terminal node");
    return node_lo(f);
}

inline bddp DDBase::raw_child1(bddp f) {
    if (f == bddnull)
        throw std::invalid_argument("raw_child1: null node");
    if (f & BDD_CONST_FLAG)
        throw std::invalid_argument("raw_child1: terminal node");
    return node_hi(f);
}

inline bddp DDBase::raw_child(bddp f, int child) {
    if (child == 0) return raw_child0(f);
    if (child == 1) return raw_child1(f);
    throw std::invalid_argument("raw_child: child must be 0 or 1");
}

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

inline double BDD::count(bddvar n) const {
    return bddcount(root, n);
}

inline bigint::BigInt BDD::exact_count(bddvar n) const {
    return bddexactcount(root, n);
}

inline bigint::BigInt BDD::exact_count(bddvar n, BddCountMemo& memo) const {
    if (memo.f() != root) {
        throw std::invalid_argument(
            "exact_count: memo was created for a different BDD");
    }
    if (memo.n() != n) {
        throw std::invalid_argument(
            "exact_count: memo was created with a different n");
    }
    if (memo.stored()) {
        return bddexactcount(root, n, memo.map());
    }
    bigint::BigInt result = bddexactcount(root, n, memo.map());
    memo.mark_stored();
    return result;
}

inline void BDD::Export(FILE* strm) const {
    bddp p = root;
    bddexport(strm, &p, 1);
}

inline void BDD::Export(std::ostream& strm) const {
    bddp p = root;
    bddexport(strm, &p, 1);
}

inline void BDD::export_sapporo(FILE* strm) const {
    bdd_export_sapporo(strm, root);
}

inline void BDD::export_sapporo(std::ostream& strm) const {
    bdd_export_sapporo(strm, root);
}

inline BDD BDD::import_sapporo(FILE* strm) {
    return BDD_ID(bdd_import_sapporo(strm));
}

inline BDD BDD::import_sapporo(std::istream& strm) {
    return BDD_ID(bdd_import_sapporo(strm));
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

// BDD child accessor functions (static versions returning bddp)

inline bddp BDD::child0(bddp f) {
    if (f == bddnull)
        throw std::invalid_argument("child0: null node");
    if (f & BDD_CONST_FLAG)
        throw std::invalid_argument("child0: terminal node");
    bddp lo = node_lo(f);
    if (f & BDD_COMP_FLAG) lo = bddnot(lo);
    return lo;
}

inline bddp BDD::child1(bddp f) {
    if (f == bddnull)
        throw std::invalid_argument("child1: null node");
    if (f & BDD_CONST_FLAG)
        throw std::invalid_argument("child1: terminal node");
    bddp hi = node_hi(f);
    if (f & BDD_COMP_FLAG) hi = bddnot(hi);
    return hi;
}

inline bddp BDD::child(bddp f, int child) {
    if (child == 0) return child0(f);
    if (child == 1) return child1(f);
    throw std::invalid_argument("child: child must be 0 or 1");
}

// BDD child accessor functions (member versions returning BDD)

inline BDD BDD::raw_child0() const {
    BDD b(0);
    b.root = DDBase::raw_child0(root);
    return b;
}

inline BDD BDD::raw_child1() const {
    BDD b(0);
    b.root = DDBase::raw_child1(root);
    return b;
}

inline BDD BDD::raw_child(int child) const {
    BDD b(0);
    b.root = DDBase::raw_child(root, child);
    return b;
}

inline BDD BDD::child0() const {
    BDD b(0);
    b.root = BDD::child0(root);
    return b;
}

inline BDD BDD::child1() const {
    BDD b(0);
    b.root = BDD::child1(root);
    return b;
}

inline BDD BDD::child(int child) const {
    BDD b(0);
    b.root = BDD::child(root, child);
    return b;
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

inline void ZDD::export_sapporo(FILE* strm) const {
    zdd_export_sapporo(strm, root);
}

inline void ZDD::export_sapporo(std::ostream& strm) const {
    zdd_export_sapporo(strm, root);
}

inline ZDD ZDD::import_sapporo(FILE* strm) {
    return ZDD_ID(zdd_import_sapporo(strm));
}

inline ZDD ZDD::import_sapporo(std::istream& strm) {
    return ZDD_ID(zdd_import_sapporo(strm));
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

// ZDD Graphillion format save/load

inline void ZDD::export_graphillion(FILE* strm, int offset) const {
    zdd_export_graphillion(strm, root, offset);
}

inline void ZDD::export_graphillion(std::ostream& strm, int offset) const {
    zdd_export_graphillion(strm, root, offset);
}

inline ZDD ZDD::import_graphillion(FILE* strm, int offset) {
    return ZDD_ID(zdd_import_graphillion(strm, offset));
}

inline ZDD ZDD::import_graphillion(std::istream& strm, int offset) {
    return ZDD_ID(zdd_import_graphillion(strm, offset));
}

// ZDD child accessor functions (static versions returning bddp)

inline bddp ZDD::child0(bddp f) {
    if (f == bddnull)
        throw std::invalid_argument("child0: null node");
    if (f & BDD_CONST_FLAG)
        throw std::invalid_argument("child0: terminal node");
    bddp lo = node_lo(f);
    if (f & BDD_COMP_FLAG) lo = bddnot(lo);
    return lo;
}

inline bddp ZDD::child1(bddp f) {
    if (f == bddnull)
        throw std::invalid_argument("child1: null node");
    if (f & BDD_CONST_FLAG)
        throw std::invalid_argument("child1: terminal node");
    return node_hi(f);
}

inline bddp ZDD::child(bddp f, int child) {
    if (child == 0) return child0(f);
    if (child == 1) return child1(f);
    throw std::invalid_argument("child: child must be 0 or 1");
}

// ZDD child accessor functions (member versions returning ZDD)

inline ZDD ZDD::raw_child0() const {
    ZDD z(0);
    z.root = DDBase::raw_child0(root);
    return z;
}

inline ZDD ZDD::raw_child1() const {
    ZDD z(0);
    z.root = DDBase::raw_child1(root);
    return z;
}

inline ZDD ZDD::raw_child(int child) const {
    ZDD z(0);
    z.root = DDBase::raw_child(root, child);
    return z;
}

inline ZDD ZDD::child0() const {
    ZDD z(0);
    z.root = ZDD::child0(root);
    return z;
}

inline ZDD ZDD::child1() const {
    ZDD z(0);
    z.root = ZDD::child1(root);
    return z;
}

inline ZDD ZDD::child(int child) const {
    ZDD z(0);
    z.root = ZDD::child(root, child);
    return z;
}

template<typename RNG>
std::vector<bddvar> ZDD::uniform_sample(RNG& rng, ZddCountMemo& memo) {
    return uniform_sample_impl(
        [&rng](const bigint::BigInt& upper) {
            return bigint::uniform_random(upper, rng);
        }, memo);
}

template<typename RNG>
std::vector<bddvar> BDD::uniform_sample(RNG& rng, bddvar n, BddCountMemo& memo) {
    return uniform_sample_impl(
        [&rng](const bigint::BigInt& upper) {
            return bigint::uniform_random(upper, rng);
        },
        n, memo);
}

template<typename RNG>
ZDD ZDD::random_family(bddvar n, RNG& rng) {
    // Ensure variables exist
    while (bdd_varcount < n) {
        bddnewvar();
    }
    std::uniform_int_distribution<int> coin(0, 1);
    // Recursive lambda: build random family over variables {v, v+1, ..., n}
    std::function<bddp(bddvar)> build = [&](bddvar v) -> bddp {
        if (v > n) {
            return coin(rng) ? bddsingle : bddempty;
        }
        bddp lo = build(v + 1);
        bddp hi = build(v + 1);
        return getznode(v, lo, hi);
    };
    return ZDD_ID(build(1));
}

#include "unreduced_dd.h"
#include "qdd.h"

#endif
