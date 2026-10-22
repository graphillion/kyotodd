#include "rotpidd.h"
#include "bdd_internal.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace kyotodd {


/* --- Global variables --- */
int RotPiDD_TopVar = 0;
int RotPiDD_VarTableSize = 16;
int RotPiDD_LevOfX[RotPiDD_MaxVar + 1];
int* RotPiDD_XOfLev = 0;

/* --- RotPiDD_NewVar --- */
int RotPiDD_NewVar()
{
    if (RotPiDD_TopVar >= RotPiDD_MaxVar) {
        throw std::overflow_error("RotPiDD_NewVar: too many variables");
    }

    /* First call: allocate RotPiDD_XOfLev */
    if (RotPiDD_TopVar == 0) {
        RotPiDD_XOfLev = new int[RotPiDD_VarTableSize];
        std::memset(RotPiDD_XOfLev, 0, sizeof(int) * RotPiDD_VarTableSize);
        RotPiDD_LevOfX[0] = 0;
        RotPiDD_LevOfX[1] = 0;
    }

    /* Create RotPiDD_TopVar new BDD variables for rotations (n, n-1), ..., (n, 1) */
    for (int i = 0; i < RotPiDD_TopVar; i++) {
        BDD_NewVar();
    }

    RotPiDD_TopVar++;

    /* Record top BDD level for this RotPiDD variable */
    int toplev = static_cast<int>(bddlevofvar(bddvarused()));
    if (RotPiDD_TopVar > 1) {
        RotPiDD_LevOfX[RotPiDD_TopVar] = toplev;
    }

    /* Expand RotPiDD_XOfLev if necessary */
    if (toplev >= RotPiDD_VarTableSize) {
        int newsize = RotPiDD_VarTableSize;
        while (newsize <= toplev) newsize *= 4;
        int* newtable = new int[newsize];
        std::memcpy(newtable, RotPiDD_XOfLev, sizeof(int) * RotPiDD_VarTableSize);
        std::memset(newtable + RotPiDD_VarTableSize, 0, sizeof(int) * (newsize - RotPiDD_VarTableSize));
        delete[] RotPiDD_XOfLev;
        RotPiDD_XOfLev = newtable;
        RotPiDD_VarTableSize = newsize;
    }

    /* Set RotPiDD_XOfLev for newly added BDD levels */
    if (RotPiDD_TopVar > 1) {
        for (int lev = toplev; lev > toplev - (RotPiDD_TopVar - 1); lev--) {
            RotPiDD_XOfLev[lev] = RotPiDD_TopVar;
        }
    }

    return RotPiDD_TopVar;
}

int RotPiDD_VarUsed()
{
    return RotPiDD_TopVar;
}

/* ================================================================ */
/*  left_rot                                                         */
/* ================================================================ */

RotPiDD RotPiDD::left_rot(int u, int v) const
{
    return left_rot(u, v, BddExecMode::Auto);
}

RotPiDD RotPiDD::left_rot(int u, int v, BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (u < 1 || u > RotPiDD_VarUsed() || v < 1 || v > RotPiDD_VarUsed()) {
        throw std::invalid_argument("RotPiDD::left_rot: variable out of range");
    }
    if (select_iter_1op(zdd_.GetID(), mode)) return left_rot_iter(*this, u, v);
    return left_rot_rec(*this, u, v);
}

RotPiDD RotPiDD::left_rot_rec(const RotPiDD& p, int u, int v)
{
    BDD_RecurGuard guard;
    if (u == v) return p;
    if (u < v) std::swap(u, v);
    /* Now u > v */

    int x = p.top_x();
    if (x < u) {
        /* All variables are below u; just toggle (u, v) */
        bddvar bv = bddvaroflev(static_cast<bddvar>(RotPiDD_Lev_XY(u, v)));
        return RotPiDD(p.zdd_.Change(bv));
    }

    /* Cache lookup */
    bddp fx = p.zdd_.GetID();
    bddp gx = static_cast<bddp>(u * (RotPiDD_MaxVar + 1) + v);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_LEFTROT, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = p.zdd_.Top();
    int y = p.top_y();
    RotPiDD p0(p.zdd_.OffSet(top));
    RotPiDD p1(p.zdd_.OnSet0(top));

    /* Recursive computation based on relationship between [v..u] and [y..x] */
    RotPiDD np0 = left_rot_rec(p0, u, v);
    RotPiDD np1;
    int ny;

    if (u < y) {
        /* [v..u] is entirely below [y..x], commutative */
        np1 = left_rot_rec(p1, u, v);
        ny = y;
    } else if (u == y) {
        /* LeftRot(u, v) o LeftRot(x, u) = LeftRot(x, v) */
        np1 = p1;
        ny = v;
    } else if (v <= y) {
        /* Overlapping: LeftRot(u, v) o LeftRot(x, y) = LeftRot(x, y+1) o LeftRot(u-1, v) */
        np1 = left_rot_rec(p1, u - 1, v);
        ny = y + 1;
    } else {
        /* [v..u] inside [y..x]: LeftRot(u, v) o LeftRot(x, y) = LeftRot(x, y) o LeftRot(u-1, v-1) */
        np1 = left_rot_rec(p1, u - 1, v - 1);
        ny = y;
    }

    RotPiDD r = np0 + left_rot_rec(np1, x, ny);

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_LEFTROT, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  swap                                                             */
/* ================================================================ */

RotPiDD RotPiDD::swap(int a, int b) const
{
    return swap(a, b, BddExecMode::Auto);
}

RotPiDD RotPiDD::swap(int a, int b, BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (a < 1 || a > RotPiDD_VarUsed() || b < 1 || b > RotPiDD_VarUsed()) {
        throw std::invalid_argument("RotPiDD::swap: variable out of range");
    }
    if (a == b) return *this;
    if (a > b) return swap(b, a, mode);
    /* Now a < b */

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    bddp gx = static_cast<bddp>(a * (RotPiDD_MaxVar + 1) + b);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_SWAP, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Build swap permutation from left rotations */
    RotPiDD swap_perm = RotPiDD(1).left_rot(a, b, mode);
    for (int i = b - 1; i >= a + 1; i--) {
        swap_perm = RotPiDD(1).left_rot(i - 1, i, mode).multiply(swap_perm, mode);
    }

    RotPiDD r = swap_perm.multiply(*this, mode);

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_SWAP, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  reverse                                                          */
/* ================================================================ */

RotPiDD RotPiDD::reverse(int l, int r) const
{
    return reverse(l, r, BddExecMode::Auto);
}

RotPiDD RotPiDD::reverse(int l, int r, BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (l < 1 || l > RotPiDD_VarUsed() || r < 1 || r > RotPiDD_VarUsed()) {
        throw std::invalid_argument("RotPiDD::reverse: variable out of range");
    }
    if (l == r) return *this;
    if (l > r) return reverse(r, l, mode);
    /* Now l < r */

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    bddp gx = static_cast<bddp>(l * (RotPiDD_MaxVar + 1) + r);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_REVERSE, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Build reverse permutation from left rotations */
    RotPiDD reverse_perm = RotPiDD(1);
    for (int i = r; i >= l + 1; i--) {
        reverse_perm = RotPiDD(1).left_rot(l, i, mode).multiply(reverse_perm, mode);
    }

    RotPiDD res = reverse_perm.multiply(*this, mode);

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_REVERSE, fx, gx, res.zdd_.GetID());
    return res;
}

/* ================================================================ */
/*  cofact                                                           */
/* ================================================================ */

RotPiDD RotPiDD::cofact(int u, int v) const
{
    return cofact(u, v, BddExecMode::Auto);
}

RotPiDD RotPiDD::cofact(int u, int v, BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (u < 1 || u > RotPiDD_VarUsed() || v < 1 || v > RotPiDD_VarUsed()) {
        throw std::invalid_argument("RotPiDD::cofact: variable out of range");
    }
    if (select_iter_1op(zdd_.GetID(), mode)) return cofact_iter(*this, u, v);
    return cofact_rec(*this, u, v);
}

RotPiDD RotPiDD::cofact_rec(const RotPiDD& p, int u, int v)
{
    BDD_RecurGuard guard;
    int x = p.top_x();
    int y = p.top_y();

    /* Terminal / early-exit cases */
    if (x < u || x < v) {
        return (u == v) ? p : RotPiDD(0);
    }
    if (x == u && y > v) {
        return RotPiDD(0);
    }

    /* Shannon decomposition */
    bddvar top = p.zdd_.Top();
    RotPiDD p0(p.zdd_.OffSet(top));
    RotPiDD p1(p.zdd_.OnSet0(top));

    if (x == u) {
        if (y == v) return p1;
        return cofact_rec(p0, u, v);
    }
    if (y == v) {
        return cofact_rec(p0, u, v);
    }

    /* Cache lookup */
    bddp fx = p.zdd_.GetID();
    bddp gx = static_cast<bddp>(u * (RotPiDD_MaxVar + 1) + v);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_COFACT, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Recursive computation */
    RotPiDD r = cofact_rec(p0, u, v);
    if (u >= v) {
        r += left_rot_rec(cofact_rec(p1, u, v), x, RotPiDD_Y_YUV(y, u, v));
    } else {
        r += left_rot_rec(cofact_rec(p1, u, RotPiDD_U_XYU(x, y, v)), x, RotPiDD_Y_YUV(y, v, u));
    }

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_COFACT, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  operator* (composition)                                          */
/* ================================================================ */

RotPiDD operator*(const RotPiDD& p, const RotPiDD& q)
{
    return p.multiply(q);
}

RotPiDD RotPiDD::multiply(const RotPiDD& q) const
{
    return multiply(q, BddExecMode::Auto);
}

RotPiDD RotPiDD::multiply(const RotPiDD& q, BddExecMode mode) const
{
    if (zdd_.GetID() == bddempty || q.zdd_.GetID() == bddempty)
        return RotPiDD(0);
    if (zdd_.GetID() == bddsingle) return q;
    if (q.zdd_.GetID() == bddsingle) return *this;
    if (zdd_.GetID() == bddnull || q.zdd_.GetID() == bddnull)
        return RotPiDD(-1);
    if (select_iter_2op(zdd_.GetID(), q.zdd_.GetID(), mode)) {
        return multiply_iter(*this, q);
    }
    return multiply_rec(*this, q);
}

RotPiDD RotPiDD::multiply_rec(const RotPiDD& p, const RotPiDD& q)
{
    BDD_RecurGuard guard;
    if (p.zdd_.GetID() == bddempty || q.zdd_.GetID() == bddempty)
        return RotPiDD(0);
    if (p.zdd_.GetID() == bddsingle) return q;
    if (q.zdd_.GetID() == bddsingle) return p;
    if (p.zdd_.GetID() == bddnull || q.zdd_.GetID() == bddnull)
        return RotPiDD(-1);

    /* Cache lookup */
    bddp fx = p.zdd_.GetID();
    bddp gx = q.zdd_.GetID();
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_MULT, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Decompose q */
    int qx = q.top_x();
    int qy = q.top_y();
    bddvar top = q.zdd_.Top();
    RotPiDD q0(q.zdd_.OffSet(top));
    RotPiDD q1(q.zdd_.OnSet0(top));

    /* Recursive computation */
    RotPiDD r = multiply_rec(p, q0) + left_rot_rec(multiply_rec(p, q1), qx, qy);

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_MULT, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  odd / even                                                       */
/* ================================================================ */

RotPiDD RotPiDD::odd() const
{
    return odd(BddExecMode::Auto);
}

RotPiDD RotPiDD::odd(BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (select_iter_1op(zdd_.GetID(), mode)) return odd_iter(*this);
    return odd_rec(*this);
}

RotPiDD RotPiDD::odd_rec(const RotPiDD& p)
{
    BDD_RecurGuard guard;
    if (p.top_x() == 0) return RotPiDD(0);  /* identity permutation is even */

    /* Cache lookup */
    bddp fx = p.zdd_.GetID();
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_ODD, fx, 0);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = p.zdd_.Top();
    int x = p.top_x();
    int y = p.top_y();
    RotPiDD p0(p.zdd_.OffSet(top));
    RotPiDD p1(p.zdd_.OnSet0(top));

    /* LeftRot(x, y) is (x - y) transpositions, so parity = (x - y) mod 2 */
    RotPiDD r;
    if ((x - y) % 2 != 0) {
        /* Odd number of transpositions: parity flips */
        r = odd_rec(p0) + left_rot_rec(p1 - odd_rec(p1), x, y);
    } else {
        /* Even number of transpositions: parity preserved */
        r = odd_rec(p0) + left_rot_rec(odd_rec(p1), x, y);
    }

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_ODD, fx, 0, r.zdd_.GetID());
    return r;
}

RotPiDD RotPiDD::even() const
{
    return even(BddExecMode::Auto);
}

RotPiDD RotPiDD::even(BddExecMode mode) const
{
    return *this - this->odd(mode);
}

/* ================================================================ */
/*  rot_bound                                                        */
/* ================================================================ */

RotPiDD RotPiDD::rot_bound(int n) const
{
    return RotPiDD(zdd_.PermitSym(n));
}

/* ================================================================ */
/*  order                                                            */
/* ================================================================ */

RotPiDD RotPiDD::order(int a, int b) const
{
    return order(a, b, BddExecMode::Auto);
}

RotPiDD RotPiDD::order(int a, int b, BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (select_iter_1op(zdd_.GetID(), mode)) return order_iter(*this, a, b);
    return order_rec(*this, a, b);
}

RotPiDD RotPiDD::order_rec(const RotPiDD& p, int a, int b)
{
    BDD_RecurGuard guard;
    if (p.zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (p.zdd_.GetID() == bddempty) return RotPiDD(0);
    if (p.zdd_.GetID() == bddsingle) {
        return (a < b) ? RotPiDD(1) : RotPiDD(0);
    }

    int x = p.top_x();
    if (x < a && b < a) return RotPiDD(0);
    if (x < b && a < b) return p;

    /* Cache lookup */
    bddp fx = p.zdd_.GetID();
    bddp gx = static_cast<bddp>(a * (RotPiDD_MaxVar + 1) + b);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_ORDER, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = p.zdd_.Top();
    int y = p.top_y();
    RotPiDD p0(p.zdd_.OffSet(top));
    RotPiDD p1(p.zdd_.OnSet0(top));

    RotPiDD r = order_rec(p0, a, b);

    if (y == b) {
        r += left_rot_rec(p1, x, y);
    } else if (y != a) {
        int na = (y < a) ? a - 1 : a;
        int nb = (y < b) ? b - 1 : b;
        r += left_rot_rec(order_rec(p1, na, nb), x, y);
    }
    /* if y == a, nothing is added */

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_ORDER, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  inverse                                                          */
/* ================================================================ */

RotPiDD RotPiDD::inverse() const
{
    return inverse(BddExecMode::Auto);
}

RotPiDD RotPiDD::inverse(BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull || zdd_.GetID() == bddempty || zdd_.GetID() == bddsingle)
        return *this;
    if (select_iter_1op(zdd_.GetID(), mode)) return inverse_iter(*this);
    return inverse_rec(*this);
}

RotPiDD RotPiDD::inverse_rec(const RotPiDD& p)
{
    BDD_RecurGuard guard;
    if (p.zdd_.GetID() == bddnull || p.zdd_.GetID() == bddempty || p.zdd_.GetID() == bddsingle)
        return p;

    /* Cache lookup */
    bddp fx = p.zdd_.GetID();
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_INVERSE, fx, 0);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = p.zdd_.Top();
    int x = p.top_x();
    int y = p.top_y();
    RotPiDD p0(p.zdd_.OffSet(top));
    RotPiDD p1(p.zdd_.OnSet0(top));

    /* Build inverse of LeftRot(x, y): right rotation = LeftRot(y,y+1) o ... o LeftRot(x-1,x) */
    RotPiDD invrot(1);
    for (int i = y; i <= x - 1; i++) {
        invrot = left_rot_rec(invrot, i, i + 1);
    }

    RotPiDD r = inverse_rec(p0) + multiply_rec(invrot, inverse_rec(p1));

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_INVERSE, fx, 0, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  insert                                                           */
/* ================================================================ */

RotPiDD RotPiDD::insert(int p, int v) const
{
    return insert(p, v, BddExecMode::Auto);
}

RotPiDD RotPiDD::insert(int p, int v, BddExecMode mode) const
{
    if (zdd_.GetID() == bddempty || zdd_.GetID() == bddnull) return *this;
    if (select_iter_1op(zdd_.GetID(), mode)) return insert_iter(*this, p, v);
    return insert_rec(*this, p, v);
}

RotPiDD RotPiDD::insert_rec(const RotPiDD& f, int p, int v)
{
    BDD_RecurGuard guard;
    if (f.zdd_.GetID() == bddempty || f.zdd_.GetID() == bddnull) return f;

    /* Cache lookup */
    bddp fx = f.zdd_.GetID();
    bddp gx = static_cast<bddp>(p * (RotPiDD_MaxVar + 1) + v);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_INSERT, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    int x = f.top_x();
    int y = f.top_y();

    if (f.zdd_.GetID() == bddsingle || x + 1 < v || x < p) {
        RotPiDD r = left_rot_rec(f, p, v);
        bddwcache(BDD_OP_ROTPIDD_INSERT, fx, gx, r.zdd_.GetID());
        return r;
    }

    /* Shannon decomposition */
    bddvar top = f.zdd_.Top();
    RotPiDD p0(f.zdd_.OffSet(top));
    RotPiDD p1(f.zdd_.OnSet0(top));

    int nx = x + (p <= x ? 1 : 0);
    int ny = y + (v <= y ? 1 : 0);
    int np = p;
    int nv = v - (y < v ? 1 : 0);

    RotPiDD r = insert_rec(p0, p, v) + left_rot_rec(insert_rec(p1, np, nv), nx, ny);

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_INSERT, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  remove_max                                                       */
/* ================================================================ */

RotPiDD RotPiDD::remove_max(int k) const
{
    return remove_max(k, BddExecMode::Auto);
}

RotPiDD RotPiDD::remove_max(int k, BddExecMode mode) const
{
    if (k <= 0) return *this;
    if (zdd_.GetID() == bddempty || zdd_.GetID() == bddnull || zdd_.GetID() == bddsingle)
        return *this;
    if (select_iter_1op(zdd_.GetID(), mode)) return remove_max_iter(*this, k);
    return remove_max_rec(*this, k);
}

RotPiDD RotPiDD::remove_max_rec(const RotPiDD& p, int k)
{
    BDD_RecurGuard guard;
    if (k <= 0) return p;
    if (p.zdd_.GetID() == bddempty || p.zdd_.GetID() == bddnull || p.zdd_.GetID() == bddsingle)
        return p;

    int x = p.top_x();
    if (x < k) return p;

    /* Cache lookup */
    bddp fx = p.zdd_.GetID();
    bddp gx = static_cast<bddp>(k);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_REMOVEMAX, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = p.zdd_.Top();
    int y = p.top_y();
    RotPiDD p0(p.zdd_.OffSet(top));
    RotPiDD p1(p.zdd_.OnSet0(top));

    RotPiDD r = remove_max_rec(p0, k) + left_rot_rec(remove_max_rec(p1, k - 1), x - 1, y);

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_REMOVEMAX, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  normalizeRotPiDD                                                 */
/* ================================================================ */

RotPiDD RotPiDD::normalizeRotPiDD(int k) const
{
    return normalizeRotPiDD(k, BddExecMode::Auto);
}

RotPiDD RotPiDD::normalizeRotPiDD(int k, BddExecMode mode) const
{
    if (select_iter_1op(zdd_.GetID(), mode)) return normalize_iter(*this, k);
    return normalize_rec(*this, k);
}

RotPiDD RotPiDD::normalize_rec(const RotPiDD& p, int k)
{
    BDD_RecurGuard guard;
    int x = p.top_x();
    if (x <= k) return p;

    /* Cache lookup */
    bddp fx = p.zdd_.GetID();
    bddp gx = static_cast<bddp>(k);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_NORMALIZE, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = p.zdd_.Top();
    RotPiDD p0(p.zdd_.OffSet(top));
    RotPiDD p1(p.zdd_.OnSet0(top));

    RotPiDD r = normalize_rec(p0, k) + normalize_rec(p1, k);

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_NORMALIZE, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  Conversion: normalizePerm                                        */
/* ================================================================ */

void RotPiDD::normalizePerm(std::vector<int>& v)
{
    std::vector<int> pos(v);
    std::sort(pos.begin(), pos.end());
    for (size_t i = 0; i < v.size(); i++) {
        v[i] = static_cast<int>(std::upper_bound(pos.begin(), pos.end(), v[i]) - pos.begin());
    }
}

/* ================================================================ */
/*  Conversion: from_vector                                          */
/* ================================================================ */

RotPiDD RotPiDD::from_vector(std::vector<int> v)
{
    return from_vector(v, BddExecMode::Auto);
}

RotPiDD RotPiDD::from_vector(std::vector<int> v, BddExecMode mode)
{
    normalizePerm(v);
    int n = static_cast<int>(v.size());

    /* Build identity mapping */
    std::vector<int> id(n);
    for (int i = 0; i < n; i++) id[i] = i + 1;

    /* Decompose permutation into left rotations */
    std::vector< std::pair<int, int> > rots;
    for (int i = n - 1; i >= 0; i--) {
        int j = 0;
        for (; j < i; j++) {
            if (id[j] == v[i]) break;
        }
        if (j < i) {
            rots.push_back(std::make_pair(j + 1, i + 1));
            /* Apply LeftRot(i+1, j+1) to id: shift id[j..i-1] left by one */
            for (int k = j; k < i; k++) {
                std::swap(id[k], id[k + 1]);
            }
        }
    }

    /* Build RotPiDD from rotations (apply in reverse order) */
    RotPiDD res(1);
    for (int i = static_cast<int>(rots.size()) - 1; i >= 0; i--) {
        res = res.left_rot(rots[i].first, rots[i].second, mode);
    }
    return res;
}

/* ================================================================ */
/*  Conversion: to_vector_of_perms                                   */
/* ================================================================ */

void RotPiDD::to_perms_rec(const RotPiDD& p, int dim, int* varmap,
                           std::vector< std::vector<int> >& result)
{
    BDD_RecurGuard guard;
    if (p == RotPiDD(0)) return;
    if (p == RotPiDD(1)) {
        std::vector<int> perm(dim);
        for (int i = 0; i < dim; i++) perm[i] = varmap[i];
        result.push_back(perm);
        return;
    }

    int x = p.top_x();
    int y = p.top_y();

    /* p1 = part with LeftRot(x, y); p0 = part without */
    RotPiDD p1 = cofact_rec(p, x, y);
    RotPiDD p0 = p - left_rot_rec(p1, x, y);

    /* Enumerate p0 (VarMap unchanged) */
    to_perms_rec(p0, dim, varmap, result);

    /* Apply LeftRot(x, y) to VarMap: rotate VarMap[y-1..x-1] left */
    std::rotate(varmap + y - 1, varmap + y, varmap + x);

    /* Enumerate p1 */
    to_perms_rec(p1, dim, varmap, result);

    /* Restore VarMap: rotate back (right rotation) */
    std::rotate(varmap + y - 1, varmap + x - 1, varmap + x);
}

std::vector< std::vector<int> > RotPiDD::to_vector_of_perms() const
{
    return to_vector_of_perms(BddExecMode::Auto);
}

std::vector< std::vector<int> > RotPiDD::to_vector_of_perms(BddExecMode mode) const
{
    std::vector< std::vector<int> > result;
    if (*this == RotPiDD(0)) return result;

    int dim = RotPiDD_TopVar;
    std::vector<int> varmap(dim);
    for (int i = 0; i < dim; i++) varmap[i] = i + 1;

    if (select_iter_1op(zdd_.GetID(), mode)) {
        to_perms_iter(*this, dim, varmap.data(), result);
    } else {
        to_perms_rec(*this, dim, varmap.data(), result);
    }
    return result;
}

/* ================================================================ */
/*  Conversion: from_permutation / to_permutations                   */
/* ================================================================ */

RotPiDD RotPiDD::from_permutation(const Permutation& p)
{
    return from_permutation(p, BddExecMode::Auto);
}

RotPiDD RotPiDD::from_permutation(const Permutation& p, BddExecMode mode)
{
    return from_vector(p.to_vector(), mode);
}

std::vector<Permutation> RotPiDD::to_permutations() const
{
    return to_permutations(BddExecMode::Auto);
}

std::vector<Permutation> RotPiDD::to_permutations(BddExecMode mode) const
{
    std::vector< std::vector<int> > vecs = to_vector_of_perms(mode);
    std::vector<Permutation> result;
    result.reserve(vecs.size());
    for (size_t i = 0; i < vecs.size(); i++) {
        result.push_back(Permutation(vecs[i]));
    }
    return result;
}

/* ================================================================ */
/*  extract_one                                                      */
/* ================================================================ */

RotPiDD RotPiDD::extract_one()
{
    return extract_one(BddExecMode::Auto);
}

RotPiDD RotPiDD::extract_one(BddExecMode mode) const
{
    if (zdd_.GetID() == bddsingle || zdd_.GetID() == bddempty || zdd_.GetID() == bddnull)
        return *this;
    if (select_iter_1op(zdd_.GetID(), mode)) return extract_one_iter(*this);
    return extract_one_rec(*this);
}

RotPiDD RotPiDD::extract_one_rec(const RotPiDD& p)
{
    BDD_RecurGuard guard;
    if (p.zdd_.GetID() == bddsingle || p.zdd_.GetID() == bddempty || p.zdd_.GetID() == bddnull)
        return p;

    bddvar top = p.zdd_.Top();
    RotPiDD p1(p.zdd_.OnSet0(top));
    return left_rot_rec(extract_one_rec(p1), p.top_x(), p.top_y());
}

/* ================================================================ */
/*  print                                                            */
/* ================================================================ */

void RotPiDD::print() const
{
    zdd_.Print();
}

/* ================================================================ */
/*  Enum — permutation vector notation                               */
/* ================================================================ */

void RotPiDD::enumerate_print(int dim, const int* varmap, bool& flag)
{
    /* Print one permutation */
    if (flag) std::cout << " + ";
    flag = true;

    std::cout << "[";
    for (int i = 0; i < dim; i++) {
        if (i > 0) std::cout << " ";
        std::cout << varmap[i];
    }
    std::cout << "]";
}

void RotPiDD::enumerate_rec(const RotPiDD& p, int dim, int* varmap, bool& flag)
{
    BDD_RecurGuard guard;
    if (p == RotPiDD(0)) return;
    if (p == RotPiDD(1)) {
        enumerate_print(dim, varmap, flag);
        return;
    }

    int x = p.top_x();
    int y = p.top_y();

    /* p1 = part with LeftRot(x, y); p0 = part without */
    RotPiDD p1 = cofact_rec(p, x, y);
    RotPiDD p0 = p - left_rot_rec(p1, x, y);

    /* Enumerate p0 first */
    enumerate_rec(p0, dim, varmap, flag);

    /* Apply LeftRot(x, y) to VarMap: rotate VarMap[y-1..x-1] left */
    std::rotate(varmap + y - 1, varmap + y, varmap + x);

    enumerate_rec(p1, dim, varmap, flag);

    /* Restore VarMap */
    std::rotate(varmap + y - 1, varmap + x - 1, varmap + x);
}

void RotPiDD::enumerate() const
{
    enumerate(BddExecMode::Auto);
}

void RotPiDD::enumerate(BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) { std::cout << "(undefined)"; return; }
    if (*this == RotPiDD(0)) { std::cout << "0"; return; }
    if (*this == RotPiDD(1)) { std::cout << "1"; return; }

    int dim = RotPiDD_TopVar;
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
/*  Enum2 — rotation notation                                        */
/* ================================================================ */

void RotPiDD::enumerate2_print(const std::vector<int>& levs, bool& flag)
{
    /* Print one permutation as rotations */
    if (flag) std::cout << " + ";
    flag = true;

    if (levs.empty()) {
        std::cout << "1";
    } else {
        for (int i = static_cast<int>(levs.size()) - 1; i >= 0; i--) {
            int lev = levs[static_cast<size_t>(i)];
            int tx = RotPiDD_X_Lev(lev);
            int ty = RotPiDD_Y_Lev(lev);
            std::cout << "(" << tx << ":" << ty << ")";
        }
    }
}

void RotPiDD::enumerate2_rec(const RotPiDD& p, std::vector<int>& levs, bool& flag)
{
    BDD_RecurGuard guard;
    if (p == RotPiDD(-1) || p == RotPiDD(0)) return;
    if (p == RotPiDD(1)) {
        enumerate2_print(levs, flag);
        return;
    }

    int x = p.top_x();
    int y = p.top_y();
    int lev = p.top_lev();

    /* p1 = part with LeftRot(x, y); p0 = part without */
    RotPiDD p1 = cofact_rec(p, x, y);
    RotPiDD p0 = p - left_rot_rec(p1, x, y);

    /* Enumerate p0 first */
    enumerate2_rec(p0, levs, flag);

    /* Record this level and enumerate p1 */
    levs.push_back(lev);

    enumerate2_rec(p1, levs, flag);

    levs.pop_back();
}

void RotPiDD::enumerate2() const
{
    enumerate2(BddExecMode::Auto);
}

void RotPiDD::enumerate2(BddExecMode mode) const
{
    if (zdd_.GetID() == bddnull) { std::cout << "(undefined)" << std::endl; return; }
    if (*this == RotPiDD(0)) { std::cout << "0" << std::endl; return; }
    if (*this == RotPiDD(1)) { std::cout << "1" << std::endl; return; }

    std::vector<int> levs;
    levs.reserve(static_cast<size_t>(bddvarused()) + 1);
    bool flag = false;

    if (select_iter_1op(zdd_.GetID(), mode)) {
        enumerate2_iter(*this, levs, flag);
    } else {
        enumerate2_rec(*this, levs, flag);
    }
    std::cout << std::endl;
}

/* ================================================================ */
/*  contradictionMaximization                                        */
/* ================================================================ */

long long int RotPiDD::contradictionMaximization(
    unsigned long long int used_set,
    std::vector<int>& unused_list,
    int n,
    std::unordered_map< std::pair<bddp, unsigned long long int>, long long int, hash_func >& hash,
    const std::vector< std::vector<int> >& w) const
{
    return contradictionMaximization(used_set, unused_list, n, hash, w,
                                     BddExecMode::Auto);
}

long long int RotPiDD::contradictionMaximization(
    unsigned long long int used_set,
    std::vector<int>& unused_list,
    int n,
    contmax_hash& hash,
    const std::vector< std::vector<int> >& w,
    BddExecMode mode) const
{
    if (n > 63)
        throw std::invalid_argument(
            "contradictionMaximization: n exceeds 63 (bitmask limit)");
    if (n > static_cast<int>(unused_list.size()))
        throw std::invalid_argument(
            "contradictionMaximization: n exceeds unused_list size");
    if (select_iter_1op(zdd_.GetID(), mode)) {
        return contmax_iter(*this, used_set, unused_list, n, hash, w);
    }
    return contmax_rec(*this, used_set, unused_list, n, hash, w);
}

long long int RotPiDD::contmax_rec(
    const RotPiDD& p, unsigned long long int used_set,
    std::vector<int>& unused_list, int n, contmax_hash& hash,
    const std::vector< std::vector<int> >& w)
{
    BDD_RecurGuard guard;
    if (n > 63)
        throw std::invalid_argument(
            "contradictionMaximization: n exceeds 63 (bitmask limit)");
    if (n > static_cast<int>(unused_list.size()))
        throw std::invalid_argument(
            "contradictionMaximization: n exceeds unused_list size");
    if (p.zdd_.GetID() == bddempty || p.zdd_.GetID() == bddnull)
        return -1000000000LL;

    /* Memoization check */
    auto key = std::make_pair(p.zdd_.GetID(), used_set);
    auto it = hash.find(key);
    if (it != hash.end()) return it->second;

    if (p.zdd_.GetID() == bddsingle) {
        hash[key] = 0;
        return 0;
    }

    int x = p.top_x();
    int y = p.top_y();
    bddvar top = p.zdd_.Top();

    /* Mark elements from x to n-1 as used */
    unsigned long long int saved_used = used_set;
    for (int i = x; i < n; i++) {
        used_set |= (1ULL << unused_list[i]);
    }

    /* 0-branch (no rotation) */
    RotPiDD rp0(p.zdd_.OffSet(top));
    long long int left_val = contmax_rec(rp0, used_set, unused_list, x, hash, w);

    /* 1-branch (with rotation) */
    RotPiDD rp1(p.zdd_.OnSet0(top));

    /* Apply LeftRot(x, y) to unused_list */
    for (int i = y - 1; i < x - 1; i++) {
        std::swap(unused_list[i], unused_list[i + 1]);
    }

    int removed_element = unused_list[x - 1];
    used_set |= (1ULL << removed_element);

    long long int right_val = 0;
    for (int i = y - 1; i < x - 1; i++) {
        right_val += w[unused_list[x - 1]][unused_list[i]];
    }
    right_val += contmax_rec(rp1, used_set, unused_list, x - 1, hash, w);

    /* Restore unused_list */
    for (int i = x - 1; i > y - 1; i--) {
        std::swap(unused_list[i], unused_list[i - 1]);
    }

    long long int result = std::max(left_val, right_val);
    hash[std::make_pair(p.zdd_.GetID(), saved_used)] = result;
    return result;
}

std::map<bddvar, std::string> RotPiDD::svg_var_name_map() {
    std::map<bddvar, std::string> m;
    if (RotPiDD_TopVar < 2 || !RotPiDD_XOfLev) return m;
    // Iterate over each RotPiDD element x (2..RotPiDD_TopVar).
    // Element x uses levels RotPiDD_LevOfX[x] down to RotPiDD_LevOfX[x]-(x-2).
    // Each level corresponds to rotation (x, y) where y = RotPiDD_LevOfX[x] - lev + 1.
    for (int x = 2; x <= RotPiDD_TopVar; ++x) {
        int base_lev = RotPiDD_LevOfX[x];
        for (int y = 1; y < x; ++y) {
            int lev = base_lev - y + 1;
            bddvar v = bddvaroflev(static_cast<bddvar>(lev));
            m[v] = "(" + std::to_string(x) + "," + std::to_string(y) + ")";
        }
    }
    return m;
}

} // namespace kyotodd
