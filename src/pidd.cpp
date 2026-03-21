#include "pidd.h"
#include "bdd_internal.h"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <vector>

/* --- Global variables --- */
int PiDD_TopVar = 0;
int PiDD_VarTableSize = 16;
int PiDD_LevOfX[PiDD_MaxVar + 1];
int* PiDD_XOfLev = 0;

/* --- PiDD_NewVar --- */
int PiDD_NewVar()
{
    if (PiDD_TopVar >= PiDD_MaxVar) {
        std::cerr << "PiDD_NewVar: too many variables" << std::endl;
        std::exit(1);
    }

    /* First call: allocate PiDD_XOfLev */
    if (PiDD_TopVar == 0) {
        PiDD_XOfLev = new int[PiDD_VarTableSize];
        PiDD_XOfLev[0] = 0;
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

PiDD PiDD::Swap(int u, int v) const
{
    if (zdd_.GetID() == bddnull) return PiDD(-1);
    if (u < 1 || u > PiDD_VarUsed() || v < 1 || v > PiDD_VarUsed()) {
        std::cerr << "PiDD::Swap: variable out of range" << std::endl;
        std::exit(1);
    }
    if (u == v) return *this;
    if (u < v) return Swap(v, u);
    /* Now u > v */

    int x = TopX();
    if (x < u) {
        /* No transposition involves u; just toggle (u, v) */
        bddvar bv = bddvaroflev(static_cast<bddvar>(PiDD_Lev_XY(u, v)));
        return PiDD(zdd_.Change(bv));
    }

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    bddp gx = static_cast<bddp>(u * (PiDD_MaxVar + 1) + v);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_SWAP, fx, gx);
    if (cache_result != ZDD::Null) return PiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = zdd_.Top();
    int y = TopY();
    PiDD p0(zdd_.Offset(top));
    PiDD p1(zdd_.OnSet0(top));

    /* Recursive computation */
    PiDD r = p0.Swap(u, v) + p1.Swap(PiDD_U_XYU(x, y, u), v).Swap(x, PiDD_Y_YUV(y, u, v));

    /* Cache store */
    bddwcache(BDD_OP_PIDD_SWAP, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  Cofact                                                          */
/* ================================================================ */

PiDD PiDD::Cofact(int u, int v) const
{
    if (zdd_.GetID() == bddnull) return PiDD(-1);
    if (u < 1 || u > PiDD_VarUsed() || v < 1 || v > PiDD_VarUsed()) {
        std::cerr << "PiDD::Cofact: variable out of range" << std::endl;
        std::exit(1);
    }

    int x = TopX();
    int y = TopY();

    /* Terminal / early-exit cases */
    if (x < u && x < v) {
        return (u == v) ? *this : PiDD(0);
    }
    if (x == u && y > v) {
        return PiDD(0);
    }

    /* Shannon decomposition */
    bddvar top = zdd_.Top();
    PiDD p0(zdd_.Offset(top));
    PiDD p1(zdd_.OnSet0(top));

    if (x == u) {
        if (y == v) return p1;
        return p0.Cofact(u, v);
    }
    if (y == v) {
        return p0.Cofact(u, v);
    }

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    bddp gx = static_cast<bddp>(u * (PiDD_MaxVar + 1) + v);
    ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_COFACT, fx, gx);
    if (cache_result != ZDD::Null) return PiDD(cache_result);

    /* Recursive computation */
    PiDD r = p0.Cofact(u, v);
    if (u >= v) {
        r += p1.Cofact(u, v).Swap(x, PiDD_Y_YUV(y, u, v));
    } else {
        r += p1.Cofact(u, PiDD_U_XYU(x, y, v)).Swap(x, PiDD_Y_YUV(y, v, u));
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
    int qx = q.TopX();
    int qy = q.TopY();
    bddvar top = q.zdd_.Top();
    PiDD q0(q.zdd_.Offset(top));
    PiDD q1(q.zdd_.OnSet0(top));

    /* Recursive computation */
    PiDD r = (p * q0) + (p * q1).Swap(qx, qy);

    /* Cache store */
    bddwcache(BDD_OP_PIDD_MULT, fx, gx, r.zdd_.GetID());
    return r;
}

/* ================================================================ */
/*  operator/ (division)                                            */
/* ================================================================ */

PiDD operator/(const PiDD& f, const PiDD& p)
{
    if (f.zdd_.GetID() == bddnull || p.zdd_.GetID() == bddnull)
        return PiDD(-1);
    if (p.zdd_.GetID() == bddsingle) return f;
    if (p.zdd_.GetID() == bddempty) {
        std::cerr << "PiDD: division by zero" << std::endl;
        std::exit(1);
    }
    if (f.TopX() < p.TopX()) return PiDD(0);

    /* Cache lookup */
    bddp fx = f.zdd_.GetID();
    bddp gx = p.zdd_.GetID();
    ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_DIV, fx, gx);
    if (cache_result != ZDD::Null) return PiDD(cache_result);

    /* Decompose p */
    int px = p.TopX();
    int py = p.TopY();
    bddvar top = p.zdd_.Top();
    PiDD p1(p.zdd_.OnSet0(top));

    /* q = (f.Cofact(px, py) / p1).Cofact(py, py) */
    PiDD q = (f.Cofact(px, py) / p1).Cofact(py, py);

    if (q.zdd_.GetID() != bddempty) {
        PiDD p0(p.zdd_.Offset(top));
        if (p0.zdd_.GetID() != bddempty) {
            q = q & (f / p0);
        }
    }

    /* Cache store */
    bddwcache(BDD_OP_PIDD_DIV, fx, gx, q.zdd_.GetID());
    return q;
}

/* ================================================================ */
/*  Odd / Even                                                      */
/* ================================================================ */

PiDD PiDD::Odd() const
{
    if (zdd_.GetID() == bddnull) return PiDD(-1);
    if (TopX() == 0) return PiDD(0);  /* identity permutation is even */

    /* Cache lookup */
    bddp fx = zdd_.GetID();
    ZDD cache_result = BDD_CacheZDD(BDD_OP_PIDD_ODD, fx, 0);
    if (cache_result != ZDD::Null) return PiDD(cache_result);

    /* Shannon decomposition */
    bddvar top = zdd_.Top();
    int x = TopX();
    int y = TopY();
    PiDD p0(zdd_.Offset(top));
    PiDD p1(zdd_.OnSet0(top));

    /* Odd of p0; Even of p1 swapped back (parity flips) */
    PiDD r = p0.Odd() + p1.Even().Swap(x, y);

    /* Cache store */
    bddwcache(BDD_OP_PIDD_ODD, fx, 0, r.zdd_.GetID());
    return r;
}

PiDD PiDD::Even() const
{
    return *this - this->Odd();
}

/* ================================================================ */
/*  SwapBound                                                       */
/* ================================================================ */

PiDD PiDD::SwapBound(int n) const
{
    return PiDD(zdd_.PermitSym(n));
}

/* ================================================================ */
/*  Print                                                           */
/* ================================================================ */

void PiDD::Print() const
{
    zdd_.Print();
}

/* ================================================================ */
/*  Enum — permutation vector notation                              */
/* ================================================================ */

static int* PiDD_Enum_VarMap;
static int  PiDD_Enum_Flag;

static void PiDD_EnumRec(PiDD p, int dim)
{
    if (p == PiDD(0)) return;
    if (p == PiDD(1)) {
        /* Print one permutation */
        if (PiDD_Enum_Flag) std::cout << " + ";
        PiDD_Enum_Flag = 1;

        /* Find last non-identity position */
        int d0 = 0;
        for (int i = 0; i < dim; i++) {
            if (PiDD_Enum_VarMap[i] != i + 1) d0 = i + 1;
        }
        if (d0 == 0) {
            std::cout << "1";
        } else {
            std::cout << "[";
            for (int i = 0; i < d0; i++) {
                if (i > 0) std::cout << " ";
                if (PiDD_Enum_VarMap[i] == i + 1)
                    std::cout << ".";
                else
                    std::cout << PiDD_Enum_VarMap[i];
            }
            std::cout << "]";
        }
        return;
    }

    int x = p.TopX();
    int y = p.TopY();

    /* p1 = part containing (x,y); p0 = part not containing (x,y) */
    PiDD p1 = p.Cofact(x, y);
    PiDD p0 = p - p1.Swap(x, y);

    /* Enumerate p0 first */
    PiDD_EnumRec(p0, dim);

    /* Apply transposition (x, y) to VarMap and enumerate p1 */
    int tmp = PiDD_Enum_VarMap[x - 1];
    PiDD_Enum_VarMap[x - 1] = PiDD_Enum_VarMap[y - 1];
    PiDD_Enum_VarMap[y - 1] = tmp;

    PiDD_EnumRec(p1, dim);

    /* Restore VarMap */
    tmp = PiDD_Enum_VarMap[x - 1];
    PiDD_Enum_VarMap[x - 1] = PiDD_Enum_VarMap[y - 1];
    PiDD_Enum_VarMap[y - 1] = tmp;
}

void PiDD::Enum() const
{
    if (zdd_.GetID() == bddnull) { std::cout << "(undefined)"; return; }
    if (*this == PiDD(0)) { std::cout << "0"; return; }
    if (*this == PiDD(1)) { std::cout << "1"; return; }

    int dim = TopX();
    std::vector<int> varmap(dim);
    for (int i = 0; i < dim; i++) varmap[i] = i + 1;
    PiDD_Enum_VarMap = varmap.data();
    PiDD_Enum_Flag = 0;

    PiDD_EnumRec(*this, dim);

    PiDD_Enum_VarMap = nullptr;
}

/* ================================================================ */
/*  Enum2 — transposition notation                                  */
/* ================================================================ */

static int* PiDD_Enum2_VarMap;
static int  PiDD_Enum2_Depth;
static int  PiDD_Enum2_Flag;

static void PiDD_Enum2Rec(PiDD p)
{
    if (p == PiDD(0)) return;
    if (p == PiDD(1)) {
        /* Print one permutation as transpositions */
        if (PiDD_Enum2_Flag) std::cout << " + ";
        PiDD_Enum2_Flag = 1;

        if (PiDD_Enum2_Depth == 0) {
            std::cout << "1";
        } else {
            for (int i = 0; i < PiDD_Enum2_Depth; i++) {
                int lev = PiDD_Enum2_VarMap[i];
                int tx = PiDD_X_Lev(lev);
                int ty = PiDD_Y_Lev(lev);
                std::cout << "(" << tx << ":" << ty << ")";
            }
        }
        return;
    }

    int x = p.TopX();
    int y = p.TopY();
    int lev = p.TopLev();

    /* p1 = part containing (x,y); p0 = part not containing (x,y) */
    PiDD p1 = p.Cofact(x, y);
    PiDD p0 = p - p1.Swap(x, y);

    /* Enumerate p0 first */
    PiDD_Enum2Rec(p0);

    /* Record this level and enumerate p1 */
    PiDD_Enum2_VarMap[PiDD_Enum2_Depth] = lev;
    PiDD_Enum2_Depth++;

    PiDD_Enum2Rec(p1);

    PiDD_Enum2_Depth--;
}

void PiDD::Enum2() const
{
    if (zdd_.GetID() == bddnull) { std::cout << "(undefined)"; return; }
    if (*this == PiDD(0)) { std::cout << "0"; return; }
    if (*this == PiDD(1)) { std::cout << "1"; return; }

    int maxdepth = static_cast<int>(bddvarused());
    std::vector<int> varmap(maxdepth + 1);
    PiDD_Enum2_VarMap = varmap.data();
    PiDD_Enum2_Depth = 0;
    PiDD_Enum2_Flag = 0;

    PiDD_Enum2Rec(*this);

    PiDD_Enum2_VarMap = nullptr;
}
