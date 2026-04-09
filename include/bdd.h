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
#include "svg_export.h"
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

inline void BDD::export_knuth(FILE* strm, bool is_hex, int offset) const {
    bdd_export_knuth(strm, root, is_hex, offset);
}
inline void BDD::export_knuth(std::ostream& strm, bool is_hex, int offset) const {
    bdd_export_knuth(strm, root, is_hex, offset);
}
inline BDD BDD::import_knuth(FILE* strm, bool is_hex, int offset) {
    return BDD_ID(bdd_import_knuth(strm, is_hex, offset));
}
inline BDD BDD::import_knuth(std::istream& strm, bool is_hex, int offset) {
    return BDD_ID(bdd_import_knuth(strm, is_hex, offset));
}

inline void BDD::export_binary(FILE* strm) const {
    bdd_export_binary(strm, root);
}

inline void BDD::export_binary(std::ostream& strm) const {
    bdd_export_binary(strm, root);
}

inline BDD BDD::import_binary(FILE* strm, bool ignore_type) {
    return BDD_ID(bdd_import_binary(strm, ignore_type));
}

inline BDD BDD::import_binary(std::istream& strm, bool ignore_type) {
    return BDD_ID(bdd_import_binary(strm, ignore_type));
}

inline void BDD::export_binary_multi(FILE* strm, const std::vector<BDD>& bdds) {
    std::vector<bddp> roots;
    roots.reserve(bdds.size());
    for (size_t i = 0; i < bdds.size(); i++) roots.push_back(bdds[i].get_id());
    bdd_export_binary_multi(strm, roots.data(), roots.size());
}

inline void BDD::export_binary_multi(std::ostream& strm, const std::vector<BDD>& bdds) {
    std::vector<bddp> roots;
    roots.reserve(bdds.size());
    for (size_t i = 0; i < bdds.size(); i++) roots.push_back(bdds[i].get_id());
    bdd_export_binary_multi(strm, roots.data(), roots.size());
}

inline std::vector<BDD> BDD::import_binary_multi(FILE* strm, bool ignore_type) {
    std::vector<bddp> raw = bdd_import_binary_multi(strm, ignore_type);
    std::vector<BDD> result;
    result.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); i++) result.push_back(BDD_ID(raw[i]));
    return result;
}

inline std::vector<BDD> BDD::import_binary_multi(std::istream& strm, bool ignore_type) {
    std::vector<bddp> raw = bdd_import_binary_multi(strm, ignore_type);
    std::vector<BDD> result;
    result.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); i++) result.push_back(BDD_ID(raw[i]));
    return result;
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

inline void BDD::save_graphviz(FILE* strm, DrawMode mode) const {
    bdd_save_graphviz(strm, root, mode);
}

inline void BDD::save_graphviz(std::ostream& strm, DrawMode mode) const {
    bdd_save_graphviz(strm, root, mode);
}

inline void BDD::save_svg(const char* filename, const SvgParams& params) const {
    bdd_save_svg(filename, root, params);
}
inline void BDD::save_svg(const char* filename) const {
    bdd_save_svg(filename, root);
}
inline void BDD::save_svg(std::ostream& strm, const SvgParams& params) const {
    bdd_save_svg(strm, root, params);
}
inline void BDD::save_svg(std::ostream& strm) const {
    bdd_save_svg(strm, root);
}
inline std::string BDD::save_svg(const SvgParams& params) const {
    return bdd_save_svg(root, params);
}
inline std::string BDD::save_svg() const {
    return bdd_save_svg(root);
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

inline ZDD ZDD::product(const ZDD& other) const {
    ZDD z(0);
    z.root = bddproduct(root, other.root);
    return z;
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

inline std::vector<bddvar> ZDD::support_vars() const {
    return bddsupport_vec(root);
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

inline uint64_t ZDD::min_size() const {
    return bddminsize(root);
}

inline uint64_t ZDD::max_size() const {
    return bddmaxsize(root);
}

inline bool ZDD::is_disjoint(const ZDD& g) const {
    return bddisdisjoint(root, g.root);
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

inline void ZDD::export_knuth(FILE* strm, bool is_hex, int offset) const {
    zdd_export_knuth(strm, root, is_hex, offset);
}
inline void ZDD::export_knuth(std::ostream& strm, bool is_hex, int offset) const {
    zdd_export_knuth(strm, root, is_hex, offset);
}
inline ZDD ZDD::import_knuth(FILE* strm, bool is_hex, int offset) {
    return ZDD_ID(zdd_import_knuth(strm, is_hex, offset));
}
inline ZDD ZDD::import_knuth(std::istream& strm, bool is_hex, int offset) {
    return ZDD_ID(zdd_import_knuth(strm, is_hex, offset));
}

inline void ZDD::export_binary(FILE* strm) const {
    zdd_export_binary(strm, root);
}

inline void ZDD::export_binary(std::ostream& strm) const {
    zdd_export_binary(strm, root);
}

inline ZDD ZDD::import_binary(FILE* strm, bool ignore_type) {
    return ZDD_ID(zdd_import_binary(strm, ignore_type));
}

inline ZDD ZDD::import_binary(std::istream& strm, bool ignore_type) {
    return ZDD_ID(zdd_import_binary(strm, ignore_type));
}

inline void ZDD::export_binary_multi(FILE* strm, const std::vector<ZDD>& zdds) {
    std::vector<bddp> roots;
    roots.reserve(zdds.size());
    for (size_t i = 0; i < zdds.size(); i++) roots.push_back(zdds[i].get_id());
    zdd_export_binary_multi(strm, roots.data(), roots.size());
}

inline void ZDD::export_binary_multi(std::ostream& strm, const std::vector<ZDD>& zdds) {
    std::vector<bddp> roots;
    roots.reserve(zdds.size());
    for (size_t i = 0; i < zdds.size(); i++) roots.push_back(zdds[i].get_id());
    zdd_export_binary_multi(strm, roots.data(), roots.size());
}

inline std::vector<ZDD> ZDD::import_binary_multi(FILE* strm, bool ignore_type) {
    std::vector<bddp> raw = zdd_import_binary_multi(strm, ignore_type);
    std::vector<ZDD> result;
    result.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); i++) result.push_back(ZDD_ID(raw[i]));
    return result;
}

inline std::vector<ZDD> ZDD::import_binary_multi(std::istream& strm, bool ignore_type) {
    std::vector<bddp> raw = zdd_import_binary_multi(strm, ignore_type);
    std::vector<ZDD> result;
    result.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); i++) result.push_back(ZDD_ID(raw[i]));
    return result;
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

inline void ZDD::save_graphviz(FILE* strm, DrawMode mode) const {
    zdd_save_graphviz(strm, root, mode);
}

inline void ZDD::save_graphviz(std::ostream& strm, DrawMode mode) const {
    zdd_save_graphviz(strm, root, mode);
}

inline void ZDD::save_svg(const char* filename, const SvgParams& params) const {
    zdd_save_svg(filename, root, params);
}
inline void ZDD::save_svg(const char* filename) const {
    zdd_save_svg(filename, root);
}
inline void ZDD::save_svg(std::ostream& strm, const SvgParams& params) const {
    zdd_save_svg(strm, root, params);
}
inline void ZDD::save_svg(std::ostream& strm) const {
    zdd_save_svg(strm, root);
}
inline std::string ZDD::save_svg(const SvgParams& params) const {
    return zdd_save_svg(root, params);
}
inline std::string ZDD::save_svg() const {
    return zdd_save_svg(root);
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

// ---------------------------------------------------------------
// Hypergeometric sampling: draw k from population N where K are
// "successes", using sequential Bernoulli trials.
// Returns the number of successes drawn.
// ---------------------------------------------------------------
namespace detail {

template<typename RNG>
bigint::BigInt hypergeometric_sample(
    const bigint::BigInt& k,      // number of draws
    const bigint::BigInt& K,      // successes in population
    const bigint::BigInt& N,      // population size
    RNG& rng)
{
    const bigint::BigInt zero(0);
    const bigint::BigInt one(1);

    // Edge cases
    if (k.is_zero() || K.is_zero()) return zero;
    if (K == N) return k;

    // Sequential Bernoulli: for each of k draws, decide success/fail
    bigint::BigInt successes(0);
    bigint::BigInt remaining_K = K;
    bigint::BigInt remaining_N = N;

    // We iterate min(k, N) times at most.
    // At each step: P(success) = remaining_K / remaining_N
    // Draw uniform in [0, remaining_N), success if < remaining_K
    bigint::BigInt draws_left = k;
    while (draws_left > zero && remaining_K > zero) {
        // If remaining draws >= remaining population, take all remaining successes
        if (draws_left >= remaining_N) {
            successes = successes + remaining_K;
            break;
        }
        // If remaining successes == remaining population, every draw is success
        if (remaining_K == remaining_N) {
            successes = successes + draws_left;
            break;
        }

        bigint::BigInt r = bigint::uniform_random(remaining_N, rng);
        if (r < remaining_K) {
            successes = successes + one;
            remaining_K = remaining_K - one;
        }
        remaining_N = remaining_N - one;
        draws_left = draws_left - one;
    }
    return successes;
}

template<typename RNG>
bddp sample_k_rec(
    bddp f,
    const bigint::BigInt& k,
    RNG& rng,
    CountMemoMap& memo)
{
    // Base cases
    if (k.is_zero() || f == bddempty) return bddempty;
    if (f == bddsingle) return bddsingle;  // k >= 1, only ∅ in family

    BDD_RecurGuard guard;

    bigint::BigInt total = bddexactcount(f, memo);
    if (k >= total) return f;

    // Decompose with ZDD complement edge handling
    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddvar var = node_var(f_raw);
    bddp raw_lo = node_lo(f_raw);
    bddp f_hi = node_hi(f_raw);
    bddp f_lo = comp ? bddnot(raw_lo) : raw_lo;

    bigint::BigInt count_hi = bddexactcount(f_hi, memo);
    bigint::BigInt count_lo = total - count_hi;

    // Hypergeometric: from k draws in population of size total,
    // count_hi are "successes" (hi-branch sets)
    bigint::BigInt k_hi = hypergeometric_sample(k, count_hi, total, rng);
    bigint::BigInt k_lo = k - k_hi;

    bddp result_lo = sample_k_rec(f_lo, k_lo, rng, memo);
    bddp result_hi = sample_k_rec(f_hi, k_hi, rng, memo);

    return ZDD::getnode_raw(var, result_lo, result_hi);
}

template<typename RNG>
bddp random_subset_rec(bddp f, double p, RNG& rng,
                        std::uniform_real_distribution<double>& dist)
{
    if (f == bddempty) return bddempty;
    if (f == bddsingle) return (dist(rng) < p) ? bddsingle : bddempty;

    BDD_RecurGuard guard;

    bool comp = (f & BDD_COMP_FLAG) != 0;
    bddp f_raw = f & ~BDD_COMP_FLAG;

    bddvar var = node_var(f_raw);
    bddp raw_lo = node_lo(f_raw);
    bddp f_hi = node_hi(f_raw);
    bddp f_lo = comp ? bddnot(raw_lo) : raw_lo;

    bddp result_lo = random_subset_rec(f_lo, p, rng, dist);
    bddp result_hi = random_subset_rec(f_hi, p, rng, dist);

    return ZDD::getnode_raw(var, result_lo, result_hi);
}

} // namespace detail

template<typename RNG>
ZDD ZDD::sample_k(int64_t k, RNG& rng, ZddCountMemo& memo) {
    if (root == bddnull) {
        throw std::invalid_argument("sample_k: null ZDD");
    }
    if (k < 0) {
        throw std::invalid_argument("sample_k: k must be >= 0");
    }
    if (memo.f() != root) {
        throw std::invalid_argument(
            "sample_k: memo was created for a different ZDD");
    }
    if (!memo.stored()) {
        exact_count(memo);
    }

    if (k == 0 || root == bddempty) return ZDD(0);  // empty family

    bigint::BigInt bk(k);
    bigint::BigInt total = bddexactcount(root, memo.map());
    if (bk >= total) return *this;

    bddp result = bdd_gc_guard([&]() -> bddp {
        // Handle ∅ membership: if ∅ ∈ F, decide whether to include it
        bool has_empty = bddhasempty(root);
        if (has_empty) {
            // P(include ∅) = k / total
            bigint::BigInt r = bigint::uniform_random(total, rng);
            if (r < bk) {
                // Include ∅, sample k-1 from f without ∅
                bddp f_no_empty = bddnot(root);  // toggle ∅
                bddp sampled = detail::sample_k_rec(
                    f_no_empty, bk - bigint::BigInt(1), rng, memo.map());
                return bddnot(sampled);  // add ∅ back
            } else {
                // Exclude ∅, sample k from f without ∅
                bddp f_no_empty = bddnot(root);
                bddp sampled = detail::sample_k_rec(
                    f_no_empty, bk, rng, memo.map());
                return sampled;
            }
        }

        return detail::sample_k_rec(root, bk, rng, memo.map());
    });
    return ZDD_ID(result);
}

template<typename RNG>
ZDD ZDD::random_subset(double p, RNG& rng) {
    if (root == bddnull) {
        throw std::invalid_argument("random_subset: null ZDD");
    }
    if (!(p >= 0.0 && p <= 1.0)) {  // catches NaN too
        throw std::invalid_argument("random_subset: p must be in [0.0, 1.0]");
    }
    if (root == bddempty || p == 0.0) return ZDD(0);
    if (p == 1.0) return *this;

    std::uniform_real_distribution<double> dist(0.0, 1.0);

    bddp result = bdd_gc_guard([&]() -> bddp {
        // Handle ∅ membership: if ∅ ∈ F, decide whether to include it
        bool has_empty = bddhasempty(root);
        if (has_empty) {
            bool include_empty = (dist(rng) < p);
            bddp f_no_empty = bddnot(root);
            bddp sampled = detail::random_subset_rec(
                f_no_empty, p, rng, dist);
            if (include_empty) {
                return bddnot(sampled);  // add ∅ back
            }
            return sampled;
        }

        return detail::random_subset_rec(root, p, rng, dist);
    });
    return ZDD_ID(result);
}

template<typename RNG>
std::vector<bddvar> ZDD::weighted_sample(
    const std::vector<double>& weights, WeightMode mode,
    RNG& rng, WeightedSampleMemo& memo)
{
    return weighted_sample_impl(
        weights, mode,
        [&rng](double upper) -> double {
            std::uniform_real_distribution<double> dist(0.0, upper);
            return dist(rng);
        }, memo);
}

template<typename RNG>
std::vector<bddvar> ZDD::boltzmann_sample(
    const std::vector<double>& weights, double beta,
    RNG& rng, WeightedSampleMemo& memo)
{
    if (memo.mode() != WeightMode::Product) {
        throw std::invalid_argument(
            "boltzmann_sample: memo must use Product mode");
    }
    // On first call, validate weights/beta by computing transformed weights.
    // On subsequent calls (memo already stored), skip redundant exp() calls
    // and use the memo's stored weights directly.
    if (memo.stored()) {
        return weighted_sample_impl(
            memo.weights(), WeightMode::Product,
            [&rng](double upper) -> double {
                std::uniform_real_distribution<double> dist(0.0, upper);
                return dist(rng);
            }, memo);
    }
    std::vector<double> tw = boltzmann_weights(weights, beta);
    return weighted_sample_impl(
        tw, WeightMode::Product,
        [&rng](double upper) -> double {
            std::uniform_real_distribution<double> dist(0.0, upper);
            return dist(rng);
        }, memo);
}

template<typename RNG>
ZDD ZDD::random_family(bddvar n, RNG& rng) {
    // Ensure variables exist
    while (bdd_varcount < n) {
        bddnewvar();
    }
    // Sort variables 1..n by level descending for correct ZDD node ordering
    std::vector<bddvar> sorted_vars(n);
    for (bddvar i = 0; i < n; ++i) sorted_vars[i] = i + 1;
    std::sort(sorted_vars.begin(), sorted_vars.end(),
              [](bddvar a, bddvar b) { return var2level[a] > var2level[b]; });
    std::uniform_int_distribution<int> coin(0, 1);
    // Recursive lambda: build random family over sorted variables
    std::function<bddp(size_t)> build = [&](size_t idx) -> bddp {
        if (idx >= sorted_vars.size()) {
            return coin(rng) ? bddsingle : bddempty;
        }
        bddp lo = build(idx + 1);
        bddp hi = build(idx + 1);
        return ZDD::getnode_raw(sorted_vars[idx], lo, hi);
    };
    return ZDD_ID(build(0));
}

#include "unreduced_dd.h"
#include "qdd.h"

// --- QDD binary format inline definitions ---

inline void QDD::export_binary(FILE* strm) const {
    qdd_export_binary(strm, root);
}
inline void QDD::export_binary(std::ostream& strm) const {
    qdd_export_binary(strm, root);
}
inline QDD QDD::import_binary(FILE* strm, bool ignore_type) {
    return QDD_ID(qdd_import_binary(strm, ignore_type));
}
inline QDD QDD::import_binary(std::istream& strm, bool ignore_type) {
    return QDD_ID(qdd_import_binary(strm, ignore_type));
}

inline void QDD::export_binary_multi(FILE* strm, const std::vector<QDD>& qdds) {
    std::vector<bddp> roots;
    roots.reserve(qdds.size());
    for (size_t i = 0; i < qdds.size(); i++) roots.push_back(qdds[i].get_id());
    qdd_export_binary_multi(strm, roots.data(), roots.size());
}

inline void QDD::export_binary_multi(std::ostream& strm, const std::vector<QDD>& qdds) {
    std::vector<bddp> roots;
    roots.reserve(qdds.size());
    for (size_t i = 0; i < qdds.size(); i++) roots.push_back(qdds[i].get_id());
    qdd_export_binary_multi(strm, roots.data(), roots.size());
}

inline std::vector<QDD> QDD::import_binary_multi(FILE* strm, bool ignore_type) {
    std::vector<bddp> raw = qdd_import_binary_multi(strm, ignore_type);
    std::vector<QDD> result;
    result.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); i++) result.push_back(QDD_ID(raw[i]));
    return result;
}

inline std::vector<QDD> QDD::import_binary_multi(std::istream& strm, bool ignore_type) {
    std::vector<bddp> raw = qdd_import_binary_multi(strm, ignore_type);
    std::vector<QDD> result;
    result.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); i++) result.push_back(QDD_ID(raw[i]));
    return result;
}

// --- UnreducedDD binary format inline definitions ---

inline void UnreducedDD::export_binary(FILE* strm) const {
    unreduced_export_binary(strm, root);
}
inline void UnreducedDD::export_binary(std::ostream& strm) const {
    unreduced_export_binary(strm, root);
}
inline UnreducedDD UnreducedDD::import_binary(FILE* strm) {
    UnreducedDD u(0);
    u.root = unreduced_import_binary(strm);
    return u;
}
inline UnreducedDD UnreducedDD::import_binary(std::istream& strm) {
    UnreducedDD u(0);
    u.root = unreduced_import_binary(strm);
    return u;
}

#include "zdd_weight_iter.h"
#include "zdd_rank_iter.h"
#include "zdd_random_iter.h"
#include "mtbdd.h"
#include "mvdd.h"

#endif
