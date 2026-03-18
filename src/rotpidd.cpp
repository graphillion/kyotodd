#include "rotpidd.h"
#include "bdd_internal.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <algorithm>
#include <stdexcept>

/* --- Global variables --- */
int RotPiDD_TopVar = 0;
int RotPiDD_VarTableSize = 16;
int RotPiDD_LevOfX[RotPiDD_MaxVar + 1];
int* RotPiDD_XOfLev = 0;

/* --- RotPiDD_NewVar --- */
int RotPiDD_NewVar()
{
    if (RotPiDD_TopVar >= RotPiDD_MaxVar) {
        std::cerr << "RotPiDD_NewVar: too many variables" << std::endl;
        std::exit(1);
    }

    /* First call: allocate RotPiDD_XOfLev */
    if (RotPiDD_TopVar == 0) {
        RotPiDD_XOfLev = new int[RotPiDD_VarTableSize];
        RotPiDD_XOfLev[0] = 0;
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
/*  LeftRot                                                          */
/* ================================================================ */

RotPiDD RotPiDD::LeftRot(int u, int v) const
{
    if (zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (u < 1 || u > RotPiDD_VarUsed() || v < 1 || v > RotPiDD_VarUsed()) {
        std::cerr << "RotPiDD::LeftRot: variable out of range" << std::endl;
        std::exit(1);
    }
    if (u == v) return *this;
    if (u < v) return LeftRot(v, u);
    /* Now u > v */

    int x = TopX();
    if (x < u) {
        /* All variables are below u; just toggle (u, v) */
        bddvar bv = bddvaroflev(static_cast<bddvar>(RotPiDD_Lev_XY(u, v)));
        return RotPiDD(zdd_.Change(bv));
    }

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    bddp gx = static_cast<bddp>(u * (RotPiDD_MaxVar + 1) + v);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_LEFTROT, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = zdd_.Top();
    int y = TopY();
    RotPiDD p0(zdd_.Offset(top));
    RotPiDD p1(zdd_.OnSet0(top));

    /* Recursive computation based on relationship between [v..u] and [y..x] */
    RotPiDD np0 = p0.LeftRot(u, v);
    RotPiDD np1;
    int ny;

    if (u < y) {
        /* [v..u] is entirely below [y..x], commutative */
        np1 = p1.LeftRot(u, v);
        ny = y;
    } else if (u == y) {
        /* LeftRot(u, v) o LeftRot(x, u) = LeftRot(x, v) */
        np1 = p1;
        ny = v;
    } else if (v <= y) {
        /* Overlapping: LeftRot(u, v) o LeftRot(x, y) = LeftRot(x, y+1) o LeftRot(u-1, v) */
        np1 = p1.LeftRot(u - 1, v);
        ny = y + 1;
    } else {
        /* [v..u] inside [y..x]: LeftRot(u, v) o LeftRot(x, y) = LeftRot(x, y) o LeftRot(u-1, v-1) */
        np1 = p1.LeftRot(u - 1, v - 1);
        ny = y;
    }

    RotPiDD r = np0 + np1.LeftRot(x, ny);

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_LEFTROT, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  Swap                                                             */
/* ================================================================ */

RotPiDD RotPiDD::Swap(int a, int b) const
{
    if (zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (a < 1 || a > RotPiDD_VarUsed() || b < 1 || b > RotPiDD_VarUsed()) {
        std::cerr << "RotPiDD::Swap: variable out of range" << std::endl;
        std::exit(1);
    }
    if (a == b) return *this;
    if (a > b) return Swap(b, a);
    /* Now a < b */

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    bddp gx = static_cast<bddp>(a * (RotPiDD_MaxVar + 1) + b);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_SWAP, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Build swap permutation from left rotations */
    RotPiDD swap_perm = RotPiDD(1).LeftRot(a, b);
    for (int i = b - 1; i >= a + 1; i--) {
        swap_perm = RotPiDD(1).LeftRot(i - 1, i) * swap_perm;
    }

    RotPiDD r = swap_perm * *this;

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_SWAP, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  Reverse                                                          */
/* ================================================================ */

RotPiDD RotPiDD::Reverse(int l, int r) const
{
    if (zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (l < 1 || l > RotPiDD_VarUsed() || r < 1 || r > RotPiDD_VarUsed()) {
        std::cerr << "RotPiDD::Reverse: variable out of range" << std::endl;
        std::exit(1);
    }
    if (l == r) return *this;
    if (l > r) return Reverse(r, l);
    /* Now l < r */

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    bddp gx = static_cast<bddp>(l * (RotPiDD_MaxVar + 1) + r);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_REVERSE, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Build reverse permutation from left rotations */
    RotPiDD reverse_perm = RotPiDD(1);
    for (int i = r; i >= l + 1; i--) {
        reverse_perm = RotPiDD(1).LeftRot(l, i) * reverse_perm;
    }

    RotPiDD res = reverse_perm * *this;

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_REVERSE, fx, gx, res.zdd_.GetID());
    return res;
}

/* ================================================================ */
/*  Cofact                                                           */
/* ================================================================ */

RotPiDD RotPiDD::Cofact(int u, int v) const
{
    if (zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (u < 1 || u > RotPiDD_VarUsed() || v < 1 || v > RotPiDD_VarUsed()) {
        std::cerr << "RotPiDD::Cofact: variable out of range" << std::endl;
        std::exit(1);
    }

    int x = TopX();
    int y = TopY();

    /* Terminal / early-exit cases */
    if (x < u || x < v) {
        return (u == v) ? *this : RotPiDD(0);
    }
    if (x == u && y > v) {
        return RotPiDD(0);
    }

    /* Shannon decomposition */
    bddvar top = zdd_.Top();
    RotPiDD p0(zdd_.Offset(top));
    RotPiDD p1(zdd_.OnSet0(top));

    if (x == u) {
        if (y == v) return p1;
        return p0.Cofact(u, v);
    }
    if (y == v) {
        return p0.Cofact(u, v);
    }

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    bddp gx = static_cast<bddp>(u * (RotPiDD_MaxVar + 1) + v);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_COFACT, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Recursive computation */
    RotPiDD r = p0.Cofact(u, v);
    if (u >= v) {
        r += p1.Cofact(u, v).LeftRot(x, RotPiDD_Y_YUV(y, u, v));
    } else {
        r += p1.Cofact(u, RotPiDD_U_XYU(x, y, v)).LeftRot(x, RotPiDD_Y_YUV(y, v, u));
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
    int qx = q.TopX();
    int qy = q.TopY();
    bddvar top = q.zdd_.Top();
    RotPiDD q0(q.zdd_.Offset(top));
    RotPiDD q1(q.zdd_.OnSet0(top));

    /* Recursive computation */
    RotPiDD r = (p * q0) + (p * q1).LeftRot(qx, qy);

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_MULT, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  Odd / Even                                                       */
/* ================================================================ */

RotPiDD RotPiDD::Odd() const
{
    if (zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (TopX() == 0) return RotPiDD(0);  /* identity permutation is even */

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_ODD, fx, 0);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = zdd_.Top();
    int x = TopX();
    int y = TopY();
    RotPiDD p0(zdd_.Offset(top));
    RotPiDD p1(zdd_.OnSet0(top));

    /* LeftRot(x, y) is (x - y) transpositions, so parity = (x - y) mod 2 */
    RotPiDD r = p0.Odd() + p1.Even().LeftRot(x, y);

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_ODD, fx, 0, r.zdd_.GetID());
    return r;
}

RotPiDD RotPiDD::Even() const
{
    return *this - this->Odd();
}

/* ================================================================ */
/*  RotBound                                                         */
/* ================================================================ */

RotPiDD RotPiDD::RotBound(int n) const
{
    return RotPiDD(zdd_.PermitSym(n));
}

/* ================================================================ */
/*  Order                                                            */
/* ================================================================ */

RotPiDD RotPiDD::Order(int a, int b) const
{
    if (zdd_.GetID() == bddnull) return RotPiDD(-1);
    if (zdd_.GetID() == bddempty) return RotPiDD(0);
    if (zdd_.GetID() == bddsingle) {
        return (a < b) ? RotPiDD(1) : RotPiDD(0);
    }

    int x = TopX();
    if (x < a && b < a) return RotPiDD(0);
    if (x < b && a < b) return *this;

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    bddp gx = static_cast<bddp>(a * (RotPiDD_MaxVar + 1) + b);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_ORDER, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = zdd_.Top();
    int y = TopY();
    RotPiDD p0(zdd_.Offset(top));
    RotPiDD p1(zdd_.OnSet0(top));

    RotPiDD r = p0.Order(a, b);

    if (y == b) {
        r += p1.LeftRot(x, y);
    } else if (y != a) {
        int na = (y < a) ? a - 1 : a;
        int nb = (y < b) ? b - 1 : b;
        r += p1.Order(na, nb).LeftRot(x, y);
    }
    /* if y == a, nothing is added */

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_ORDER, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  Inverse                                                          */
/* ================================================================ */

RotPiDD RotPiDD::Inverse() const
{
    if (zdd_.GetID() == bddnull || zdd_.GetID() == bddempty || zdd_.GetID() == bddsingle)
        return *this;

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_INVERSE, fx, 0);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = zdd_.Top();
    int x = TopX();
    int y = TopY();
    RotPiDD p0(zdd_.Offset(top));
    RotPiDD p1(zdd_.OnSet0(top));

    /* Build inverse of LeftRot(x, y): right rotation = LeftRot(y,y+1) o ... o LeftRot(x-1,x) */
    RotPiDD invrot(1);
    for (int i = y; i <= x - 1; i++) {
        invrot = invrot.LeftRot(i, i + 1);
    }

    RotPiDD r = p0.Inverse() + invrot * p1.Inverse();

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_INVERSE, fx, 0, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  Insert                                                           */
/* ================================================================ */

RotPiDD RotPiDD::Insert(int p, int v) const
{
    if (zdd_.GetID() == bddempty || zdd_.GetID() == bddnull) return *this;

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    bddp gx = static_cast<bddp>(p * (RotPiDD_MaxVar + 1) + v);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_INSERT, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    int x = TopX();
    int y = TopY();

    if (zdd_.GetID() == bddsingle || x + 1 < v || x < p) {
        RotPiDD r = this->LeftRot(p, v);
        bddwcache(BDD_OP_ROTPIDD_INSERT, fx, gx, r.zdd_.GetID());
        return r;
    }

    /* Shannon decomposition */
    bddvar top = zdd_.Top();
    RotPiDD p0(zdd_.Offset(top));
    RotPiDD p1(zdd_.OnSet0(top));

    int nx = x + (p <= x ? 1 : 0);
    int ny = y + (v <= y ? 1 : 0);
    int np = p;
    int nv = v - (y < v ? 1 : 0);

    RotPiDD r = p0.Insert(p, v) + p1.Insert(np, nv).LeftRot(nx, ny);

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_INSERT, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  RemoveMax                                                        */
/* ================================================================ */

RotPiDD RotPiDD::RemoveMax(int k) const
{
    if (zdd_.GetID() == bddempty || zdd_.GetID() == bddnull || zdd_.GetID() == bddsingle)
        return *this;

    int x = TopX();
    if (x < k) return *this;

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    bddp gx = static_cast<bddp>(k);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_REMOVEMAX, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = zdd_.Top();
    int y = TopY();
    RotPiDD p0(zdd_.Offset(top));
    RotPiDD p1(zdd_.OnSet0(top));

    RotPiDD r = p0.RemoveMax(k) + p1.RemoveMax(k - 1).LeftRot(x - 1, y);

    /* Cache store */
    bddwcache(BDD_OP_ROTPIDD_REMOVEMAX, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  normalizeRotPiDD                                                 */
/* ================================================================ */

RotPiDD RotPiDD::normalizeRotPiDD(int k) const
{
    int x = TopX();
    if (x <= k) return *this;

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    bddp gx = static_cast<bddp>(k);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_ROTPIDD_NORMALIZE, fx, gx);
    if (cache_result != ZDD::Null) return RotPiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = zdd_.Top();
    RotPiDD p0(zdd_.Offset(top));
    RotPiDD p1(zdd_.OnSet0(top));

    RotPiDD r = p0.normalizeRotPiDD(k) + p1.normalizeRotPiDD(k);

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
/*  Conversion: VECtoRotPiDD                                         */
/* ================================================================ */

RotPiDD RotPiDD::VECtoRotPiDD(std::vector<int> v)
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
        res = res.LeftRot(rots[i].first, rots[i].second);
    }
    return res;
}

/* ================================================================ */
/*  Conversion: RotPiDDToVectorOfPerms                               */
/* ================================================================ */

static int* RotPiDD_ToVec_VarMap;

static void RotPiDD_ToVecOfPerms(RotPiDD p, int dim,
                                  std::vector< std::vector<int> >& result)
{
    if (p == RotPiDD(0)) return;
    if (p == RotPiDD(1)) {
        std::vector<int> perm(dim);
        for (int i = 0; i < dim; i++) perm[i] = RotPiDD_ToVec_VarMap[i];
        result.push_back(perm);
        return;
    }

    int x = p.TopX();
    int y = p.TopY();

    /* p1 = part with LeftRot(x, y); p0 = part without */
    RotPiDD p1 = p.Cofact(x, y);
    RotPiDD p0 = p - p1.LeftRot(x, y);

    /* Enumerate p0 (VarMap unchanged) */
    RotPiDD_ToVecOfPerms(p0, dim, result);

    /* Apply LeftRot(x, y) to VarMap: rotate VarMap[y-1..x-1] left */
    std::rotate(RotPiDD_ToVec_VarMap + y - 1,
                RotPiDD_ToVec_VarMap + y,
                RotPiDD_ToVec_VarMap + x);

    /* Enumerate p1 */
    RotPiDD_ToVecOfPerms(p1, dim, result);

    /* Restore VarMap: rotate back (right rotation) */
    std::rotate(RotPiDD_ToVec_VarMap + y - 1,
                RotPiDD_ToVec_VarMap + x - 1,
                RotPiDD_ToVec_VarMap + x);
}

std::vector< std::vector<int> > RotPiDD::RotPiDDToVectorOfPerms() const
{
    std::vector< std::vector<int> > result;
    if (*this == RotPiDD(0)) return result;

    int dim = RotPiDD_TopVar;
    RotPiDD_ToVec_VarMap = new int[dim];
    for (int i = 0; i < dim; i++) RotPiDD_ToVec_VarMap[i] = i + 1;

    RotPiDD_ToVecOfPerms(*this, dim, result);

    delete[] RotPiDD_ToVec_VarMap;
    return result;
}

/* ================================================================ */
/*  Extract_One                                                      */
/* ================================================================ */

RotPiDD RotPiDD::Extract_One()
{
    if (zdd_.GetID() == bddsingle || zdd_.GetID() == bddempty || zdd_.GetID() == bddnull)
        return *this;

    bddvar top = zdd_.Top();
    RotPiDD p1(zdd_.OnSet0(top));
    return RotPiDD(p1).Extract_One().LeftRot(TopX(), TopY());
}

/* ================================================================ */
/*  Print                                                            */
/* ================================================================ */

void RotPiDD::Print() const
{
    zdd_.Print();
}

/* ================================================================ */
/*  Enum — permutation vector notation                               */
/* ================================================================ */

static int* RotPiDD_Enum_VarMap;
static int  RotPiDD_Enum_Flag;

static void RotPiDD_EnumRec(RotPiDD p, int dim)
{
    if (p == RotPiDD(0)) return;
    if (p == RotPiDD(1)) {
        /* Print one permutation */
        if (RotPiDD_Enum_Flag) std::cout << " + ";
        RotPiDD_Enum_Flag = 1;

        std::cout << "[";
        for (int i = 0; i < dim; i++) {
            if (i > 0) std::cout << " ";
            std::cout << RotPiDD_Enum_VarMap[i];
        }
        std::cout << "]";
        return;
    }

    int x = p.TopX();
    int y = p.TopY();

    /* p1 = part with LeftRot(x, y); p0 = part without */
    RotPiDD p1 = p.Cofact(x, y);
    RotPiDD p0 = p - p1.LeftRot(x, y);

    /* Enumerate p0 first */
    RotPiDD_EnumRec(p0, dim);

    /* Apply LeftRot(x, y) to VarMap: rotate VarMap[y-1..x-1] left */
    std::rotate(RotPiDD_Enum_VarMap + y - 1,
                RotPiDD_Enum_VarMap + y,
                RotPiDD_Enum_VarMap + x);

    RotPiDD_EnumRec(p1, dim);

    /* Restore VarMap */
    std::rotate(RotPiDD_Enum_VarMap + y - 1,
                RotPiDD_Enum_VarMap + x - 1,
                RotPiDD_Enum_VarMap + x);
}

void RotPiDD::Enum() const
{
    if (zdd_.GetID() == bddnull) { std::cout << "(undefined)"; return; }
    if (*this == RotPiDD(0)) { std::cout << "0"; return; }
    if (*this == RotPiDD(1)) { std::cout << "1"; return; }

    int dim = RotPiDD_TopVar;
    RotPiDD_Enum_VarMap = new int[dim];
    for (int i = 0; i < dim; i++) RotPiDD_Enum_VarMap[i] = i + 1;
    RotPiDD_Enum_Flag = 0;

    RotPiDD_EnumRec(*this, dim);

    delete[] RotPiDD_Enum_VarMap;
}

/* ================================================================ */
/*  Enum2 — rotation notation                                        */
/* ================================================================ */

static int* RotPiDD_Enum2_VarMap;
static int  RotPiDD_Enum2_Depth;
static int  RotPiDD_Enum2_Flag;

static void RotPiDD_Enum2Rec(RotPiDD p)
{
    if (p == RotPiDD(-1) || p == RotPiDD(0)) return;
    if (p == RotPiDD(1)) {
        /* Print one permutation as rotations */
        if (RotPiDD_Enum2_Flag) std::cout << " + ";
        RotPiDD_Enum2_Flag = 1;

        if (RotPiDD_Enum2_Depth == 0) {
            std::cout << "1";
        } else {
            for (int i = RotPiDD_Enum2_Depth - 1; i >= 0; i--) {
                int lev = RotPiDD_Enum2_VarMap[i];
                int tx = RotPiDD_X_Lev(lev);
                int ty = RotPiDD_Y_Lev(lev);
                std::cout << "(" << tx << ":" << ty << ")";
            }
        }
        return;
    }

    int x = p.TopX();
    int y = p.TopY();
    int lev = p.TopLev();

    /* p1 = part with LeftRot(x, y); p0 = part without */
    RotPiDD p1 = p.Cofact(x, y);
    RotPiDD p0 = p - p1.LeftRot(x, y);

    /* Enumerate p0 first */
    RotPiDD_Enum2Rec(p0);

    /* Record this level and enumerate p1 */
    RotPiDD_Enum2_VarMap[RotPiDD_Enum2_Depth] = lev;
    RotPiDD_Enum2_Depth++;

    RotPiDD_Enum2Rec(p1);

    RotPiDD_Enum2_Depth--;
}

void RotPiDD::Enum2() const
{
    if (zdd_.GetID() == bddnull) { std::cout << "(undefined)" << std::endl; return; }
    if (*this == RotPiDD(0)) { std::cout << "0" << std::endl; return; }
    if (*this == RotPiDD(1)) { std::cout << "1" << std::endl; return; }

    int maxdepth = static_cast<int>(bddvarused());
    RotPiDD_Enum2_VarMap = new int[maxdepth + 1];
    RotPiDD_Enum2_Depth = 0;
    RotPiDD_Enum2_Flag = 0;

    RotPiDD_Enum2Rec(*this);

    delete[] RotPiDD_Enum2_VarMap;
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
    if (zdd_.GetID() == bddempty || zdd_.GetID() == bddnull)
        return -1000000000LL;

    /* Memoization check */
    auto key = std::make_pair(zdd_.GetID(), used_set);
    auto it = hash.find(key);
    if (it != hash.end()) return it->second;

    if (zdd_.GetID() == bddsingle) {
        hash[key] = 0;
        return 0;
    }

    int x = TopX();
    int y = TopY();
    bddvar top = zdd_.Top();

    /* Mark elements from x to n-1 as used */
    unsigned long long int saved_used = used_set;
    for (int i = x; i < n; i++) {
        used_set |= (1ULL << unused_list[i]);
    }

    /* 0-branch (no rotation) */
    RotPiDD rp0(zdd_.Offset(top));
    long long int left_val = rp0.contradictionMaximization(used_set, unused_list, x, hash, w);

    /* 1-branch (with rotation) */
    RotPiDD rp1(zdd_.OnSet0(top));

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
    right_val += rp1.contradictionMaximization(used_set, unused_list, x - 1, hash, w);

    /* Restore unused_list */
    for (int i = x - 1; i > y - 1; i--) {
        std::swap(unused_list[i], unused_list[i - 1]);
    }

    long long int result = std::max(left_val, right_val);
    hash[std::make_pair(zdd_.GetID(), saved_used)] = result;
    return result;
}
