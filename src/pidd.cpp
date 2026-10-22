#include "pidd.h"
#include "bdd_internal.h"
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace kyotodd {


/* --- Global variables --- */
int PiDD_TopVar = 0;
int PiDD_VarTableSize = 16;
int PiDD_LevOfX[PiDD_MaxVar + 1];
int* PiDD_XOfLev = 0;

/* --- PiDD_NewVar --- */
int PiDD_NewVar()
{
    if (PiDD_TopVar >= PiDD_MaxVar) {
        throw std::overflow_error("PiDD_NewVar: too many variables");
    }

    /* First call: allocate PiDD_XOfLev */
    if (PiDD_TopVar == 0) {
        PiDD_XOfLev = new int[PiDD_VarTableSize];
        std::memset(PiDD_XOfLev, 0, sizeof(int) * PiDD_VarTableSize);
        PiDD_LevOfX[0] = 0;
        PiDD_LevOfX[1] = 0;
    }

    /* Create PiDD_TopVar new BDD variables for transpositions (n, n-1), ..., (n, 1) */
    for (int i = 0; i < PiDD_TopVar; i++) {
        BDD_NewVar();
    }

    PiDD_TopVar++;

    /* Record top BDD level for this PiDD variable */
    int toplev = static_cast<int>(bddlevofvar(bddvarused()));
    if (PiDD_TopVar > 1) {
        PiDD_LevOfX[PiDD_TopVar] = toplev;
    }

    /* Expand PiDD_XOfLev if necessary */
    if (toplev >= PiDD_VarTableSize) {
        int newsize = PiDD_VarTableSize;
        while (newsize <= toplev) newsize *= 4;
        int* newtable = new int[newsize];
        std::memcpy(newtable, PiDD_XOfLev, sizeof(int) * PiDD_VarTableSize);
        std::memset(newtable + PiDD_VarTableSize, 0, sizeof(int) * (newsize - PiDD_VarTableSize));
        delete[] PiDD_XOfLev;
        PiDD_XOfLev = newtable;
        PiDD_VarTableSize = newsize;
    }

    /* Set PiDD_XOfLev for newly added BDD levels */
    if (PiDD_TopVar > 1) {
        for (int lev = toplev; lev > toplev - (PiDD_TopVar - 1); lev--) {
            PiDD_XOfLev[lev] = PiDD_TopVar;
        }
    }

    return PiDD_TopVar;
}

int PiDD_VarUsed()
{
    return PiDD_TopVar;
}

/* ================================================================ */
/*  Swap                                                            */
/* ================================================================ */

PiDD PiDD::swap(int u, int v) const
{
    return swap(u, v, BddExecMode::Auto);
}

PiDD PiDD::swap(int u, int v, BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) return PiDD(-1);
    if (u < 1 || u > PiDD_VarUsed() || v < 1 || v > PiDD_VarUsed()) {
        throw std::invalid_argument("PiDD::swap: variable out of range");
    }
    if (select_iter_1op(zdd_.GetID(), mode)) return swap_iter(*this, u, v);
    return swap_rec(*this, u, v);
}

PiDD PiDD::swap_rec(const PiDD& p, int u, int v)
{
    BDD_RecurGuard guard;
    if (u == v) return p;
    if (u < v) std::swap(u, v);
    /* Now u > v */

    int x = p.top_x();
    if (x < u) {
        /* No transposition involves u; just toggle (u, v) */
        bddvar bv = bddvaroflev(static_cast<bddvar>(PiDD_Lev_XY(u, v)));
        return PiDD(p.zdd_.Change(bv));
    }

    /* Cache lookup */
    bddp fx = p.zdd_.GetID();
    bddp gx = static_cast<bddp>(u * (PiDD_MaxVar + 1) + v);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_SWAP, fx, gx);
    if (cache_result != ZDD::Null) return PiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = p.zdd_.Top();
    int y = p.top_y();
    PiDD p0(p.zdd_.OffSet(top));
    PiDD p1(p.zdd_.OnSet0(top));

    /* Recursive computation */
    PiDD r = swap_rec(p0, u, v)
           + swap_rec(swap_rec(p1, PiDD_U_XYU(x, y, u), v), x, PiDD_Y_YUV(y, u, v));

    /* Cache store */
    bddwcache(BDD_OP_PIDD_SWAP, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  Cofact                                                          */
/* ================================================================ */

PiDD PiDD::cofact(int u, int v) const
{
    return cofact(u, v, BddExecMode::Auto);
}

PiDD PiDD::cofact(int u, int v, BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) return PiDD(-1);
    if (u < 1 || u > PiDD_VarUsed() || v < 1 || v > PiDD_VarUsed()) {
        throw std::invalid_argument("PiDD::cofact: variable out of range");
    }
    if (select_iter_1op(zdd_.GetID(), mode)) return cofact_iter(*this, u, v);
    return cofact_rec(*this, u, v);
}

PiDD PiDD::cofact_rec(const PiDD& p, int u, int v)
{
    BDD_RecurGuard guard;
    int x = p.top_x();
    int y = p.top_y();

    /* Terminal / early-exit cases */
    if (x < u && x < v) {
        return (u == v) ? p : PiDD(0);
    }
    if (x == u && y > v) {
        return PiDD(0);
    }

    /* Shannon decomposition */
    bddvar top = p.zdd_.Top();
    PiDD p0(p.zdd_.OffSet(top));
    PiDD p1(p.zdd_.OnSet0(top));

    if (x == u) {
        if (y == v) return p1;
        return cofact_rec(p0, u, v);
    }
    if (y == v) {
        return cofact_rec(p0, u, v);
    }

    /* Cache lookup */
    bddp fx = p.zdd_.GetID();
    bddp gx = static_cast<bddp>(u * (PiDD_MaxVar + 1) + v);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_COFACT, fx, gx);
    if (cache_result != ZDD::Null) return PiDD(cache_result);

    /* Recursive computation */
    PiDD r = cofact_rec(p0, u, v);
    if (u >= v) {
        r += swap_rec(cofact_rec(p1, u, v), x, PiDD_Y_YUV(y, u, v));
    } else {
        r += swap_rec(cofact_rec(p1, u, PiDD_U_XYU(x, y, v)), x, PiDD_Y_YUV(y, v, u));
    }

    /* Cache store */
    bddwcache(BDD_OP_PIDD_COFACT, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  operator* (composition)                                         */
/* ================================================================ */

PiDD operator*(const PiDD& p, const PiDD& q)
{
    return p.multiply(q);
}

PiDD PiDD::multiply(const PiDD& q) const
{
    return multiply(q, BddExecMode::Auto);
}

PiDD PiDD::multiply(const PiDD& q, BddExecMode mode) const
{
    if (zdd_.GetID() == bddempty || q.zdd_.GetID() == bddempty)
        return PiDD(0);
    if (zdd_.GetID() == bddsingle) return q;
    if (q.zdd_.GetID() == bddsingle) return *this;
    if (zdd_.GetID() == bddnull || q.zdd_.GetID() == bddnull)
        return PiDD(-1);
    if (select_iter_2op(zdd_.GetID(), q.zdd_.GetID(), mode)) {
        return multiply_iter(*this, q);
    }
    return multiply_rec(*this, q);
}

PiDD PiDD::multiply_rec(const PiDD& p, const PiDD& q)
{
    BDD_RecurGuard guard;
    if (p.zdd_.GetID() == bddempty || q.zdd_.GetID() == bddempty)
        return PiDD(0);
    if (p.zdd_.GetID() == bddsingle) return q;
    if (q.zdd_.GetID() == bddsingle) return p;
    if (p.zdd_.GetID() == bddnull || q.zdd_.GetID() == bddnull)
        return PiDD(-1);

    /* Cache lookup */
    bddp fx = p.zdd_.GetID();
    bddp gx = q.zdd_.GetID();
    ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_MULT, fx, gx);
    if (cache_result != ZDD::Null) return PiDD(cache_result);

    /* Decompose q */
    int qx = q.top_x();
    int qy = q.top_y();
    bddvar top = q.zdd_.Top();
    PiDD q0(q.zdd_.OffSet(top));
    PiDD q1(q.zdd_.OnSet0(top));

    /* Recursive computation */
    PiDD r = multiply_rec(p, q0) + swap_rec(multiply_rec(p, q1), qx, qy);

    /* Cache store */
    bddwcache(BDD_OP_PIDD_MULT, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  operator/ (division)                                            */
/* ================================================================ */

PiDD operator/(const PiDD& f, const PiDD& p)
{
    return f.divide(p);
}

PiDD PiDD::divide(const PiDD& p) const
{
    return divide(p, BddExecMode::Auto);
}

PiDD PiDD::divide(const PiDD& p, BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull || p.zdd_.GetID() == bddnull)
        return PiDD(-1);
    if (p.zdd_.GetID() == bddsingle) return *this;
    if (p.zdd_.GetID() == bddempty) {
        throw std::invalid_argument("PiDD: division by zero");
    }
    if (top_x() < p.top_x()) return PiDD(0);
    if (select_iter_2op(zdd_.GetID(), p.zdd_.GetID(), mode)) {
        return divide_iter(*this, p);
    }
    return divide_rec(*this, p);
}

PiDD PiDD::divide_rec(const PiDD& f, const PiDD& p)
{
    BDD_RecurGuard guard;
    if (f.zdd_.GetID() == bddnull || p.zdd_.GetID() == bddnull)
        return PiDD(-1);
    if (p.zdd_.GetID() == bddsingle) return f;
    if (p.zdd_.GetID() == bddempty) {
        throw std::invalid_argument("PiDD: division by zero");
    }
    if (f.top_x() < p.top_x()) return PiDD(0);

    /* Cache lookup */
    bddp fx = f.zdd_.GetID();
    bddp gx = p.zdd_.GetID();
    ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_DIV, fx, gx);
    if (cache_result != ZDD::Null) return PiDD(cache_result);

    /* Decompose p */
    int px = p.top_x();
    int py = p.top_y();
    bddvar top = p.zdd_.Top();
    PiDD p1(p.zdd_.OnSet0(top));

    /* q = (f.cofact(px, py) / p1).cofact(py, py) */
    PiDD q = cofact_rec(divide_rec(cofact_rec(f, px, py), p1), py, py);

    if (q.zdd_.GetID() != bddempty) {
        PiDD p0(p.zdd_.OffSet(top));
        if (p0.zdd_.GetID() != bddempty) {
            q = q & divide_rec(f, p0);
        }
    }

    /* Cache store */
    bddwcache(BDD_OP_PIDD_DIV, fx, gx, q.zdd_.GetID());
    return q;
}

/* ================================================================ */
/*  Odd / Even                                                      */
/* ================================================================ */

PiDD PiDD::odd() const
{
    return odd(BddExecMode::Auto);
}

PiDD PiDD::odd(BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) return PiDD(-1);
    if (select_iter_1op(zdd_.GetID(), mode)) return odd_iter(*this);
    return odd_rec(*this);
}

PiDD PiDD::odd_rec(const PiDD& p)
{
    BDD_RecurGuard guard;
    if (p.top_x() == 0) return PiDD(0);  /* identity permutation is even */

    /* Cache lookup */
    bddp fx = p.zdd_.GetID();
    ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_ODD, fx, 0);
    if (cache_result != ZDD::Null) return PiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = p.zdd_.Top();
    int x = p.top_x();
    int y = p.top_y();
    PiDD p0(p.zdd_.OffSet(top));
    PiDD p1(p.zdd_.OnSet0(top));

    /* Odd of p0; Even of p1 swapped back (parity flips) */
    PiDD r = odd_rec(p0) + swap_rec(p1 - odd_rec(p1), x, y);

    /* Cache store */
    bddwcache(BDD_OP_PIDD_ODD, fx, 0, r.zdd_.GetID());
    return r;
}

PiDD PiDD::even() const
{
    return even(BddExecMode::Auto);
}

PiDD PiDD::even(BddExecMode mode) const
{
    return *this - this->odd(mode);
}

/* ================================================================ */
/*  SwapBound                                                       */
/* ================================================================ */

PiDD PiDD::swap_bound(int n) const
{
    return PiDD(zdd_.PermitSym(n));
}

/* ================================================================ */
/*  Conversion: from_permutation / to_permutations                  */
/* ================================================================ */

PiDD PiDD::from_permutation(const Permutation& p)
{
    return from_permutation(p, BddExecMode::Auto);
}

PiDD PiDD::from_permutation(const Permutation& p, BddExecMode mode)
{
    const int n = p.size();
    std::vector<int> cur(p.to_vector());

    /* Decompose into transpositions: repeatedly take the largest position
       x with cur(x) != x and peel off tau(x, cur(x)). Composing cur with
       that transposition on the value side fixes x, so
       p = tau(x1, y1) o tau(x2, y2) o ... with x1 > x2 > ... */
    std::vector< std::pair<int, int> > swaps;
    for (int x = n; x >= 2; x--) {
        int y = cur[x - 1];
        if (y == x) continue;
        swaps.push_back(std::make_pair(x, y));
        /* Swap the values x and y (positions > x are already fixed) */
        for (int i = 0; i < x; i++) {
            if (cur[i] == x) cur[i] = y;
            else if (cur[i] == y) cur[i] = x;
        }
    }

    /* swap(x, y) multiplies the transposition on the left (apply the
       permutation first, then tau), so apply the list in reverse order. */
    PiDD res(1);
    for (int i = static_cast<int>(swaps.size()) - 1; i >= 0; i--) {
        res = res.swap(swaps[i].first, swaps[i].second, mode);
    }
    return res;
}

std::vector<Permutation> PiDD::to_permutations() const
{
    return to_permutations(BddExecMode::Auto);
}

std::vector<Permutation> PiDD::to_permutations(BddExecMode mode) const
{
    std::vector<Permutation> result;
    if (zdd_.GetID() == bddnull) return result;
    if (*this == PiDD(0)) return result;

    int dim = PiDD_VarUsed();
    std::vector<int> varmap(dim);
    for (int i = 0; i < dim; i++) varmap[i] = i + 1;

    if (select_iter_1op(zdd_.GetID(), mode)) {
        to_perms_iter(*this, dim, varmap.data(), result);
    } else {
        to_perms_rec(*this, dim, varmap.data(), result);
    }
    return result;
}

void PiDD::to_perms_rec(const PiDD& p, int dim, int* varmap,
                        std::vector<Permutation>& result)
{
    BDD_RecurGuard guard;
    if (p == PiDD(0)) return;
    if (p == PiDD(1)) {
        result.push_back(Permutation(std::vector<int>(varmap, varmap + dim)));
        return;
    }

    int x = p.top_x();
    int y = p.top_y();

    /* p1 = part containing (x,y); p0 = part not containing (x,y) */
    PiDD p1 = cofact_rec(p, x, y);
    PiDD p0 = p - swap_rec(p1, x, y);

    /* Collect p0 first (VarMap unchanged) */
    to_perms_rec(p0, dim, varmap, result);

    /* Apply transposition (x, y) to VarMap and collect p1 */
    std::swap(varmap[x - 1], varmap[y - 1]);

    to_perms_rec(p1, dim, varmap, result);

    /* Restore VarMap */
    std::swap(varmap[x - 1], varmap[y - 1]);
}

/* ================================================================ */
/*  Print                                                           */
/* ================================================================ */

void PiDD::print() const
{
    zdd_.Print();
}

/* ================================================================ */
/*  Enum — permutation vector notation                              */
/* ================================================================ */

void PiDD::enumerate_print(int dim, const int* varmap, bool& flag)
{
    /* Print one permutation */
    if (flag) std::cout << " + ";
    flag = true;

    /* Find last non-identity position */
    int d0 = 0;
    for (int i = 0; i < dim; i++) {
        if (varmap[i] != i + 1) d0 = i + 1;
    }
    if (d0 == 0) {
        std::cout << "1";
    } else {
        std::cout << "[";
        for (int i = 0; i < d0; i++) {
            if (i > 0) std::cout << " ";
            if (varmap[i] == i + 1)
                std::cout << ".";
            else
                std::cout << varmap[i];
        }
        std::cout << "]";
    }
}

void PiDD::enumerate_rec(const PiDD& p, int dim, int* varmap, bool& flag)
{
    BDD_RecurGuard guard;
    if (p == PiDD(0)) return;
    if (p == PiDD(1)) {
        enumerate_print(dim, varmap, flag);
        return;
    }

    int x = p.top_x();
    int y = p.top_y();

    /* p1 = part containing (x,y); p0 = part not containing (x,y) */
    PiDD p1 = cofact_rec(p, x, y);
    PiDD p0 = p - swap_rec(p1, x, y);

    /* Enumerate p0 first */
    enumerate_rec(p0, dim, varmap, flag);

    /* Apply transposition (x, y) to VarMap and enumerate p1 */
    std::swap(varmap[x - 1], varmap[y - 1]);

    enumerate_rec(p1, dim, varmap, flag);

    /* Restore VarMap */
    std::swap(varmap[x - 1], varmap[y - 1]);
}

void PiDD::enumerate() const
{
    enumerate(BddExecMode::Auto);
}

void PiDD::enumerate(BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) { std::cout << "(undefined)"; return; }
    if (*this == PiDD(0)) { std::cout << "0"; return; }
    if (*this == PiDD(1)) { std::cout << "1"; return; }

    int dim = top_x();
    std::vector<int> varmap(dim);
    for (int i = 0; i < dim; i++) varmap[i] = i + 1;
    bool flag = false;

    if (select_iter_1op(zdd_.GetID(), mode)) {
        enumerate_iter(*this, dim, varmap.data(), flag);
    } else {
        enumerate_rec(*this, dim, varmap.data(), flag);
    }
}

/* ================================================================ */
/*  Enum2 — transposition notation                                  */
/* ================================================================ */

void PiDD::enumerate2_print(const std::vector<int>& levs, bool& flag)
{
    /* Print one permutation as transpositions */
    if (flag) std::cout << " + ";
    flag = true;

    if (levs.empty()) {
        std::cout << "1";
    } else {
        for (size_t i = 0; i < levs.size(); i++) {
            int lev = levs[i];
            int tx = PiDD_X_Lev(lev);
            int ty = PiDD_Y_Lev(lev);
            std::cout << "(" << tx << ":" << ty << ")";
        }
    }
}

void PiDD::enumerate2_rec(const PiDD& p, std::vector<int>& levs, bool& flag)
{
    BDD_RecurGuard guard;
    if (p == PiDD(0)) return;
    if (p == PiDD(1)) {
        enumerate2_print(levs, flag);
        return;
    }

    int x = p.top_x();
    int y = p.top_y();
    int lev = p.top_lev();

    /* p1 = part containing (x,y); p0 = part not containing (x,y) */
    PiDD p1 = cofact_rec(p, x, y);
    PiDD p0 = p - swap_rec(p1, x, y);

    /* Enumerate p0 first */
    enumerate2_rec(p0, levs, flag);

    /* Record this level and enumerate p1 */
    levs.push_back(lev);

    enumerate2_rec(p1, levs, flag);

    levs.pop_back();
}

void PiDD::enumerate2() const
{
    enumerate2(BddExecMode::Auto);
}

void PiDD::enumerate2(BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) { std::cout << "(undefined)"; return; }
    if (*this == PiDD(0)) { std::cout << "0"; return; }
    if (*this == PiDD(1)) { std::cout << "1"; return; }

    std::vector<int> levs;
    levs.reserve(static_cast<size_t>(bddvarused()) + 1);
    bool flag = false;

    if (select_iter_1op(zdd_.GetID(), mode)) {
        enumerate2_iter(*this, levs, flag);
    } else {
        enumerate2_rec(*this, levs, flag);
    }
}

std::map<bddvar, std::string> PiDD::svg_var_name_map() {
    std::map<bddvar, std::string> m;
    if (PiDD_TopVar < 2 || !PiDD_XOfLev) return m;
    // Iterate over each PiDD element x (2..PiDD_TopVar).
    // Element x uses levels PiDD_LevOfX[x] down to PiDD_LevOfX[x]-(x-2).
    // Each level corresponds to transposition (x, y) where y = PiDD_LevOfX[x] - lev + 1.
    for (int x = 2; x <= PiDD_TopVar; ++x) {
        int base_lev = PiDD_LevOfX[x];
        for (int y = 1; y < x; ++y) {
            int lev = base_lev - y + 1;
            bddvar v = bddvaroflev(static_cast<bddvar>(lev));
            m[v] = "(" + std::to_string(x) + "," + std::to_string(y) + ")";
        }
    }
    return m;
}

} // namespace kyotodd
