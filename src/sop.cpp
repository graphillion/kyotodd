#include "sop.h"
#include "bdd_internal.h"
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <string>

/* ================================================================ */
/*  Helper: validate even VarID                                      */
/* ================================================================ */

static void check_even(int v, const char* func)
{
    if (v & 1) {
        throw std::invalid_argument(
            std::string(func) + ": variable must be even, got " +
            std::to_string(v));
    }
}

static void check_even_shift(int n, const char* func)
{
    if (n & 1) {
        throw std::invalid_argument(
            std::string(func) + ": shift amount must be even, got " +
            std::to_string(n));
    }
}

/* ================================================================ */
/*  SOP_NewVar / SOP_NewVarOfLev                                     */
/* ================================================================ */

int SOP_NewVar()
{
    // Ensure the first of the pair gets an odd VarID.
    // VarIDs are sequential (1, 2, 3, ...), so if the current count
    // is odd, the next VarID would be even — skip one to fix parity.
    if (bddvarused() % 2 != 0) {
        BDD_NewVar();  // padding to align parity
    }
    BDD_NewVar();  // odd VarID (negative literal)
    bddvar v = BDD_NewVar();  // even VarID (positive literal)
    return static_cast<int>(v);
}

int SOP_NewVarOfLev(int lev)
{
    check_even_shift(lev, "SOP_NewVarOfLev");
    // Ensure the first of the pair gets an odd VarID.
    if (bddvarused() % 2 != 0) {
        BDD_NewVar();  // padding to align parity
    }
    BDD_NewVarOfLev(static_cast<bddvar>(lev - 1));  // odd level
    bddvar v = BDD_NewVarOfLev(static_cast<bddvar>(lev));  // even level
    return static_cast<int>(v);
}

/* ================================================================ */
/*  SOP constructors / destructor / assignment                       */
/* ================================================================ */

SOP::SOP() : _zbdd() {}

SOP::SOP(int val) : _zbdd(val) {}

SOP::SOP(const SOP& f) : _zbdd(f._zbdd) {}

SOP::SOP(const ZDD& zbdd) : _zbdd(zbdd) {}

SOP::~SOP() {}

SOP& SOP::operator=(const SOP& f)
{
    _zbdd = f._zbdd;
    return *this;
}

/* ================================================================ */
/*  Factor operations                                                */
/* ================================================================ */

SOP SOP::Factor1(int v) const
{
    check_even(v, "SOP::Factor1");
    return SOP(_zbdd.OnSet0(static_cast<bddvar>(v)));
}

SOP SOP::Factor0(int v) const
{
    check_even(v, "SOP::Factor0");
    return SOP(_zbdd.OnSet0(static_cast<bddvar>(v - 1)));
}

SOP SOP::FactorD(int v) const
{
    check_even(v, "SOP::FactorD");
    return SOP(_zbdd.Offset(static_cast<bddvar>(v))
                   .Offset(static_cast<bddvar>(v - 1)));
}

/* ================================================================ */
/*  And operations                                                   */
/* ================================================================ */

SOP SOP::And1(int v) const
{
    check_even(v, "SOP::And1");
    bddvar vv = static_cast<bddvar>(v);
    bddvar vm = static_cast<bddvar>(v - 1);
    // 1. Remove cubes containing negative literal (v-1)
    ZDD z = _zbdd.Offset(vm);
    // 2. Remove variable v from all cubes: OnSet0(v) + Offset(v)
    z = z.OnSet0(vv) + z.Offset(vv);
    // 3. Add variable v to all cubes
    z = z.Change(vv);
    return SOP(z);
}

SOP SOP::And0(int v) const
{
    check_even(v, "SOP::And0");
    bddvar vv = static_cast<bddvar>(v);
    bddvar vm = static_cast<bddvar>(v - 1);
    // 1. Remove cubes containing positive literal (v)
    ZDD z = _zbdd.Offset(vv);
    // 2. Remove variable v-1 from all cubes: OnSet0(v-1) + Offset(v-1)
    z = z.OnSet0(vm) + z.Offset(vm);
    // 3. Add variable v-1 to all cubes
    z = z.Change(vm);
    return SOP(z);
}

/* ================================================================ */
/*  Shift operators                                                  */
/* ================================================================ */

SOP SOP::operator<<(int n) const
{
    check_even_shift(n, "SOP::operator<<");
    return SOP(_zbdd << n);
}

SOP SOP::operator>>(int n) const
{
    check_even_shift(n, "SOP::operator>>");
    return SOP(_zbdd >> n);
}

SOP& SOP::operator<<=(int n)
{
    *this = *this << n;
    return *this;
}

SOP& SOP::operator>>=(int n)
{
    *this = *this >> n;
    return *this;
}

/* ================================================================ */
/*  Compound assignment operators                                    */
/* ================================================================ */

SOP& SOP::operator&=(const SOP& f) { _zbdd = _zbdd & f._zbdd; return *this; }
SOP& SOP::operator+=(const SOP& f) { _zbdd = _zbdd + f._zbdd; return *this; }
SOP& SOP::operator-=(const SOP& f) { _zbdd = _zbdd - f._zbdd; return *this; }
SOP& SOP::operator*=(const SOP& f) { *this = *this * f; return *this; }
SOP& SOP::operator/=(const SOP& f) { *this = *this / f; return *this; }
SOP& SOP::operator%=(const SOP& f) { *this = *this - (*this / f) * f; return *this; }

/* ================================================================ */
/*  Binary operators                                                 */
/* ================================================================ */

SOP operator&(const SOP& f, const SOP& g) { return SOP(f._zbdd & g._zbdd); }
SOP operator+(const SOP& f, const SOP& g) { return SOP(f._zbdd + g._zbdd); }
SOP operator-(const SOP& f, const SOP& g) { return SOP(f._zbdd - g._zbdd); }

int operator==(const SOP& f, const SOP& g) { return (f._zbdd == g._zbdd) ? 1 : 0; }
int operator!=(const SOP& f, const SOP& g) { return !(f == g); }

/* ================================================================ */
/*  Query methods                                                    */
/* ================================================================ */

int SOP::Top() const
{
    int t = static_cast<int>(_zbdd.Top());
    return (t + 1) & ~1;
}

uint64_t SOP::Size() const { return _zbdd.Size(); }

uint64_t SOP::Cube() const { return _zbdd.Card(); }

uint64_t SOP::Lit() const { return _zbdd.Lit(); }

ZDD SOP::GetZBDD() const { return _zbdd; }

/* ================================================================ */
/*  SOP algebraic multiplication                                     */
/* ================================================================ */

SOP operator*(const SOP& pc, const SOP& qc)
{
    bddp fp = pc._zbdd.GetID();
    bddp gp = qc._zbdd.GetID();

    /* Terminal cases */
    if (fp == bddnull || gp == bddnull) return SOP(-1);
    if (fp == bddempty || gp == bddempty) return SOP(0);
    if (fp == bddsingle) return qc;
    if (gp == bddsingle) return pc;

    /* Cache lookup */
    ZDD cached = BDD_CacheZDD(BDD_OP_SOP_MULT, fp, gp);
    if (cached != ZDD::Null) return SOP(cached);

    /* Ensure p has higher top level than q (swap if necessary) */
    SOP p = pc, q = qc;
    int ptop_var = p.Top();
    int qtop_var = q.Top();
    if (ptop_var != 0 && qtop_var != 0) {
        bddvar plev = bddlevofvar(static_cast<bddvar>(ptop_var));
        bddvar qlev = bddlevofvar(static_cast<bddvar>(qtop_var));
        if (plev < qlev) {
            std::swap(p, q);
            std::swap(fp, gp);
        }
    }

    int top = p.Top();
    SOP p1 = p.Factor1(top);
    SOP p0 = p.Factor0(top);
    SOP pD = p.FactorD(top);

    SOP r;
    /* Check if q depends on top */
    int qtop = q.Top();
    bool q_has_top = false;
    if (qtop != 0) {
        bddvar qlev = bddlevofvar(static_cast<bddvar>(qtop));
        bddvar toplev = bddlevofvar(static_cast<bddvar>(top));
        q_has_top = (qlev >= toplev);
        /* More precise: check if q actually uses variable top */
        if (q_has_top) {
            /* q's top level >= top's level. Check if they match. */
            q_has_top = (qtop == top);
        }
    }

    if (!q_has_top) {
        /* top is not in q */
        r = (p1 * q).And1(top) + (p0 * q).And0(top) + (pD * q);
    } else {
        /* top is in both p and q */
        SOP q1 = q.Factor1(top);
        SOP q0 = q.Factor0(top);
        SOP qD = q.FactorD(top);

        SOP pos = ((p1 * q1) + (p1 * qD) + (pD * q1)).And1(top);
        SOP neg = ((p0 * q0) + (p0 * qD) + (pD * q0)).And0(top);
        SOP dc  = pD * qD;
        r = pos + neg + dc;
    }

    /* Cache store */
    bddwcache(BDD_OP_SOP_MULT, fp, gp, r._zbdd.GetID());
    return r;
}

/* ================================================================ */
/*  SOP algebraic division                                           */
/* ================================================================ */

SOP operator/(const SOP& fc, const SOP& pc)
{
    bddp fp = fc._zbdd.GetID();
    bddp gp = pc._zbdd.GetID();

    /* Terminal cases */
    if (fp == bddnull || gp == bddnull) return SOP(-1);
    if (gp == bddsingle) return fc;
    if (fp == gp) return SOP(1);
    if (gp == bddempty) {
        throw std::invalid_argument("SOP: division by zero");
    }

    /* If f's top level < p's top level, result is 0 */
    int ftop = fc.Top();
    int ptop = pc.Top();
    if (ftop == 0) return SOP(0);
    if (ptop != 0) {
        bddvar flev = bddlevofvar(static_cast<bddvar>(ftop));
        bddvar plev = bddlevofvar(static_cast<bddvar>(ptop));
        if (flev < plev) return SOP(0);
    }

    /* Cache lookup */
    ZDD cached = BDD_CacheZDD(BDD_OP_SOP_DIV, fp, gp);
    if (cached != ZDD::Null) return SOP(cached);

    /* Decompose p on its top variable */
    int top = ptop;
    SOP p1 = pc.Factor1(top);
    SOP p0 = pc.Factor0(top);
    SOP pD = pc.FactorD(top);

    SOP q(-1);  // uninitialized

    if (p1._zbdd.GetID() != bddempty) {
        q = fc.Factor1(top) / p1;
    }
    if (q._zbdd.GetID() != bddempty &&
        p0._zbdd.GetID() != bddempty) {
        SOP q2 = fc.Factor0(top) / p0;
        if (q._zbdd.GetID() == bddnull) {
            q = q2;
        } else {
            q &= q2;
        }
    }
    if (q._zbdd.GetID() != bddempty &&
        pD._zbdd.GetID() != bddempty) {
        SOP q3 = fc.FactorD(top) / pD;
        if (q._zbdd.GetID() == bddnull) {
            q = q3;
        } else {
            q &= q3;
        }
    }

    /* If q is still uninitialized (all factors were zero), result is 0 */
    if (q._zbdd.GetID() == bddnull) q = SOP(0);

    /* Cache store */
    bddwcache(BDD_OP_SOP_DIV, fp, gp, q._zbdd.GetID());
    return q;
}

SOP operator%(const SOP& f, const SOP& p)
{
    return f - (f / p) * p;
}

/* ================================================================ */
/*  GetBDD                                                           */
/* ================================================================ */

BDD SOP::GetBDD() const
{
    bddp fp = _zbdd.GetID();

    /* Terminal cases */
    if (fp == bddnull) return BDD(-1);
    if (fp == bddempty) return BDD(0);
    if (fp == bddsingle) return BDD(1);

    /* Cache lookup */
    BDD cached = BDD_CacheBDD(BDD_OP_SOP_BDD, fp, 0);
    if (cached != BDD::Null) return cached;

    int top = Top();
    BDD x = BDDvar(static_cast<bddvar>(top));
    BDD f = (x & Factor1(top).GetBDD()) |
            (~x & Factor0(top).GetBDD()) |
            FactorD(top).GetBDD();

    /* Cache store */
    bddwcache(BDD_OP_SOP_BDD, fp, 0, f.GetID());
    return f;
}

/* ================================================================ */
/*  Predicates                                                       */
/* ================================================================ */

int SOP::IsPolyCube() const
{
    int top = Top();
    if (top == 0) return 0;

    SOP f1 = Factor1(top);
    SOP f0 = Factor0(top);
    SOP fD = FactorD(top);

    if (f1._zbdd.GetID() != bddempty) {
        if (f0._zbdd.GetID() != bddempty ||
            fD._zbdd.GetID() != bddempty) return 1;
        return f1.IsPolyCube();
    }
    /* f1 is zero */
    if (fD._zbdd.GetID() != bddempty) return 1;
    return f0.IsPolyCube();
}

int SOP::IsPolyLit() const
{
    int top = Top();
    if (top == 0) return 0;

    SOP f1 = Factor1(top);
    SOP f0 = Factor0(top);
    SOP fD = FactorD(top);

    if (fD._zbdd.GetID() != bddempty) return 1;
    if (f0._zbdd.GetID() == bddsingle &&
        f1._zbdd.GetID() == bddempty) return 0;  // ~x only
    if (f1._zbdd.GetID() == bddsingle &&
        f0._zbdd.GetID() == bddempty) return 0;  // x only
    return 1;
}

/* ================================================================ */
/*  Support                                                          */
/* ================================================================ */

SOP SOP::Support() const
{
    bddp fp = _zbdd.GetID();
    if (fp == bddnull) return SOP(-1);
    if (fp == bddempty || fp == bddsingle) return SOP(0);

    /* Get ZBDD support (single set of all appearing ZBDD variables) */
    ZDD sup = _zbdd.Support();

    /* Walk through the support set, collecting unique even variable IDs.
     * For each ZBDD variable t, round to even: (t + 1) & ~1.
     * Build a single cube with positive literals for all logic variables. */
    SOP result(1);
    int last_even = -1;

    ZDD iter = sup;
    while (iter.GetID() != bddsingle && iter.GetID() != bddempty) {
        bddvar t = iter.Top();
        int even_v = (static_cast<int>(t) + 1) & ~1;
        if (even_v != last_even) {
            result = result.And1(even_v);
            last_even = even_v;
        }
        /* Advance: OnSet0(top) on a single set removes top and gives the rest */
        iter = iter.OnSet0(t);
    }

    return result;
}

/* ================================================================ */
/*  Divisor                                                          */
/* ================================================================ */

SOP SOP::Divisor() const
{
    bddp fp = _zbdd.GetID();
    if (fp == bddnull) return SOP(-1);
    if (fp == bddempty) return SOP(0);
    if (!IsPolyCube()) return SOP(1);

    SOP f = *this;
    SOP sup = Support();

    /* Walk through support variables */
    int t = sup.Top();
    while (t > 0) {
        SOP f0 = f.Factor0(t);
        if (f0.IsPolyCube()) {
            f = f0;
        } else {
            SOP f1 = f.Factor1(t);
            if (f1.IsPolyCube()) {
                f = f1;
            }
        }
        /* Get next support variable */
        sup = sup.FactorD(t);
        t = sup.Top();
    }

    return f;
}

/* ================================================================ */
/*  Implicants                                                       */
/* ================================================================ */

SOP SOP::Implicants(BDD f) const
{
    bddp sp = _zbdd.GetID();
    bddp fp = f.GetID();

    /* Terminal cases */
    if (sp == bddempty) return SOP(0);
    if (fp == bddempty) return SOP(0);       // f = 0 → no implicants
    if (fp == bddsingle) return *this;       // f = 1 → all cubes are implicants
    if (sp == bddsingle && fp != bddsingle) return SOP(0);  // tautology cube, f ≠ 1

    /* Cache lookup */
    ZDD cached = BDD_CacheZDD(BDD_OP_SOP_IMPL, sp, fp);
    if (cached != ZDD::Null) return SOP(cached);

    /* Choose top variable */
    int stop = Top();
    int ftop_raw = static_cast<int>(f.Top());
    int ftop = (ftop_raw + 1) & ~1;  // round to even SOP variable

    int top;
    if (stop == 0) {
        top = ftop;
    } else if (ftop == 0) {
        top = stop;
    } else {
        bddvar slev = bddlevofvar(static_cast<bddvar>(stop));
        bddvar flev = bddlevofvar(static_cast<bddvar>(ftop));
        top = (slev >= flev) ? stop : ftop;
    }

    BDD f0 = f.At0(static_cast<bddvar>(top));
    BDD f1 = f.At1(static_cast<bddvar>(top));

    SOP r0 = Factor0(top).Implicants(f0).And0(top);
    SOP r1 = Factor1(top).Implicants(f1).And1(top);

    SOP pD = FactorD(top);
    SOP rD;
    if (pD._zbdd.GetID() != bddempty) {
        rD = pD.Implicants(f0 & f1);
    }

    SOP r = r0 + r1 + rD;

    /* Cache store */
    bddwcache(BDD_OP_SOP_IMPL, sp, fp, r._zbdd.GetID());
    return r;
}

/* ================================================================ */
/*  Swap                                                             */
/* ================================================================ */

SOP SOP::Swap(int v1, int v2) const
{
    check_even(v1, "SOP::Swap");
    check_even(v2, "SOP::Swap");
    ZDD z = _zbdd.Swap(static_cast<bddvar>(v1), static_cast<bddvar>(v2));
    z = z.Swap(static_cast<bddvar>(v1 - 1), static_cast<bddvar>(v2 - 1));
    return SOP(z);
}

/* ================================================================ */
/*  InvISOP                                                          */
/* ================================================================ */

SOP SOP::InvISOP() const
{
    return SOP_ISOP(~GetBDD());
}

/* ================================================================ */
/*  Print                                                            */
/* ================================================================ */

void SOP::Print() const
{
    bddp fp = _zbdd.GetID();
    int top = 0;
    bddvar lev = 0;
    if (fp != bddnull && fp != bddempty && fp != bddsingle) {
        top = Top();
        lev = bddlevofvar(static_cast<bddvar>(top));
    }
    printf("[ %llu Var:%d(%u) Size:%llu ]\n",
        static_cast<unsigned long long>(fp),
        top, static_cast<unsigned>(lev),
        static_cast<unsigned long long>(Size()));
}

/* ================================================================ */
/*  PrintPla (delegates to SOPV)                                     */
/* ================================================================ */

void SOP::PrintPla() const
{
    SOPV(*this).PrintPla();
}

/* ================================================================ */
/*  ISOP (Irredundant Sum-of-Products)                               */
/* ================================================================ */

/* Internal: BCpair holds (BDD, SOP) pair for ISOP recursion */
struct BCpair {
    BDD _f;
    SOP _cs;
};

static BCpair ISOP_rec(BDD s, BDD r);

SOP SOP_ISOP(BDD f)
{
    BCpair result = ISOP_rec(f, ~f);
    return result._cs;
}

SOP SOP_ISOP(BDD on, BDD dc)
{
    BCpair result = ISOP_rec(on | dc, ~on | dc);
    return result._cs;
}

static BCpair ISOP_rec(BDD s, BDD r)
{
    bddp sp = s.GetID();
    bddp rp = r.GetID();

    /* Terminal cases */
    if (sp == bddnull || rp == bddnull) {
        return {BDD(-1), SOP(-1)};
    }
    if (rp == bddsingle) {
        /* r = 1 means everything is forbidden → empty cover */
        return {BDD(0), SOP(0)};
    }
    if (sp == bddsingle) {
        /* s = 1 means must cover everything → tautology */
        return {BDD(1), SOP(1)};
    }

    /* Cache lookup: both caches must hit */
    bddp cached_bdd = bddrcache(BDD_OP_ISOP1, sp, rp);
    if (cached_bdd != bddnull) {
        bddp cached_sop = bddrcache(BDD_OP_ISOP2, sp, rp);
        if (cached_sop != bddnull) {
            return {BDD_ID(cached_bdd), SOP(ZDD_ID(cached_sop))};
        }
    }

    /* Choose top variable (highest level between s and r) */
    bddvar stop = s.Top();
    bddvar rtop = r.Top();
    bddvar top;
    if (stop == 0) {
        top = rtop;
    } else if (rtop == 0) {
        top = stop;
    } else {
        bddvar slev = bddlevofvar(stop);
        bddvar rlev = bddlevofvar(rtop);
        top = (slev >= rlev) ? stop : rtop;
    }

    /* Round to even SOP variable */
    int sop_top = (static_cast<int>(top) + 1) & ~1;

    BDD s0 = s.At0(static_cast<bddvar>(sop_top));
    BDD s1 = s.At1(static_cast<bddvar>(sop_top));
    BDD r0 = r.At0(static_cast<bddvar>(sop_top));
    BDD r1 = r.At1(static_cast<bddvar>(sop_top));

    /* 1. Positive literal side: ISOP(s1, r1 | s0) */
    BCpair pos = ISOP_rec(s1, r1 | s0);
    SOP c1 = pos._cs.And1(sop_top);
    BDD f1 = pos._f;

    /* 2. Negative literal side: ISOP(s0, r0 | s1) */
    BCpair neg = ISOP_rec(s0, r0 | s1);
    SOP c0 = neg._cs.And0(sop_top);
    BDD f0 = neg._f;

    /* 3. Don't-care side: ISOP(s0 & s1, ~(s0 & s1) | ((r0 | f0) & (r1 | f1))) */
    BDD sD = s0 & s1;
    BCpair dc = ISOP_rec(sD, ~sD | ((r0 | f0) & (r1 | f1)));
    SOP cD = dc._cs;
    BDD fD = dc._f;

    /* Combine */
    BDD x = BDDvar(static_cast<bddvar>(sop_top));
    BDD result_f = (~x & f0) | (x & f1) | fD;
    SOP result_cs = c1 + c0 + cD;

    /* Cache store (skip if result is -1) */
    bddp rfp = result_f.GetID();
    bddp rcp = result_cs.GetZBDD().GetID();
    if (rfp != bddnull) {
        bddwcache(BDD_OP_ISOP1, sp, rp, rfp);
    }
    if (rcp != bddnull) {
        bddwcache(BDD_OP_ISOP2, sp, rp, rcp);
    }

    return {result_f, result_cs};
}

/* ================================================================ */
/*  SOPV constructors / destructor / assignment                      */
/* ================================================================ */

SOPV::SOPV() : _v() {}

SOPV::SOPV(const SOPV& v) : _v(v._v) {}

SOPV::SOPV(const ZBDDV& zbddv) : _v(zbddv) {}

SOPV::SOPV(const SOP& f, int loc) : _v(f.GetZBDD(), loc) {}

SOPV::~SOPV() {}

SOPV& SOPV::operator=(const SOPV& v)
{
    _v = v._v;
    return *this;
}

/* ================================================================ */
/*  SOPV compound assignment                                         */
/* ================================================================ */

SOPV& SOPV::operator&=(const SOPV& v) { _v = _v & v._v; return *this; }
SOPV& SOPV::operator+=(const SOPV& v) { _v = _v + v._v; return *this; }
SOPV& SOPV::operator-=(const SOPV& v) { _v = _v - v._v; return *this; }

/* ================================================================ */
/*  SOPV shift operators                                             */
/* ================================================================ */

SOPV SOPV::operator<<(int n) const { return SOPV(_v << (n << 1)); }
SOPV SOPV::operator>>(int n) const { return SOPV(_v >> (n << 1)); }
SOPV& SOPV::operator<<=(int n) { *this = *this << n; return *this; }
SOPV& SOPV::operator>>=(int n) { *this = *this >> n; return *this; }

/* ================================================================ */
/*  SOPV Factor / And operations                                     */
/* ================================================================ */

SOPV SOPV::Factor1(int v) const
{
    check_even(v, "SOPV::Factor1");
    return SOPV(_v.OnSet0(v));
}

SOPV SOPV::Factor0(int v) const
{
    check_even(v, "SOPV::Factor0");
    return SOPV(_v.OnSet0(v - 1));
}

SOPV SOPV::FactorD(int v) const
{
    check_even(v, "SOPV::FactorD");
    return SOPV(_v.OffSet(v).OffSet(v - 1));
}

SOPV SOPV::And1(int v) const
{
    check_even(v, "SOPV::And1");
    ZBDDV z = _v.OffSet(v - 1);
    z = z.OnSet0(v) + z.OffSet(v);
    z = z.Change(v);
    return SOPV(z);
}

SOPV SOPV::And0(int v) const
{
    check_even(v, "SOPV::And0");
    ZBDDV z = _v.OffSet(v);
    z = z.OnSet0(v - 1) + z.OffSet(v - 1);
    z = z.Change(v - 1);
    return SOPV(z);
}

/* ================================================================ */
/*  SOPV query methods                                               */
/* ================================================================ */

int SOPV::Top() const
{
    int t = _v.Top();
    return (t + 1) & ~1;
}

uint64_t SOPV::Size() const { return _v.Size(); }

uint64_t SOPV::Cube() const
{
    int last = Last();
    uint64_t total = 0;
    for (int i = 0; i <= last; i++) {
        SOP s = GetSOP(i);
        if (s.GetZBDD().GetID() == bddnull) return 0;
        total += s.Cube();
    }
    return total;
}

uint64_t SOPV::Lit() const
{
    int last = Last();
    uint64_t total = 0;
    for (int i = 0; i <= last; i++) {
        SOP s = GetSOP(i);
        if (s.GetZBDD().GetID() == bddnull) return 0;
        total += s.Lit();
    }
    return total;
}

int SOPV::Last() const { return _v.Last(); }

SOP SOPV::GetSOP(int index) const { return SOP(_v.GetZBDD(index)); }

ZBDDV SOPV::GetZBDDV() const { return _v; }

SOPV SOPV::Mask(int start, int length) const { return SOPV(_v.Mask(start, length)); }

/* ================================================================ */
/*  SOPV Swap                                                        */
/* ================================================================ */

SOPV SOPV::Swap(int v1, int v2) const
{
    check_even(v1, "SOPV::Swap");
    check_even(v2, "SOPV::Swap");
    ZBDDV z = _v.Swap(v1, v2);
    z = z.Swap(v1 - 1, v2 - 1);
    return SOPV(z);
}

/* ================================================================ */
/*  SOPV binary operators                                            */
/* ================================================================ */

SOPV operator&(const SOPV& f, const SOPV& g) { return SOPV(f._v & g._v); }
SOPV operator+(const SOPV& f, const SOPV& g) { return SOPV(f._v + g._v); }
SOPV operator-(const SOPV& f, const SOPV& g) { return SOPV(f._v - g._v); }
int operator==(const SOPV& v1, const SOPV& v2) { return (v1._v == v2._v) ? 1 : 0; }
int operator!=(const SOPV& v1, const SOPV& v2) { return !(v1 == v2); }

/* ================================================================ */
/*  SOPV Print                                                       */
/* ================================================================ */

void SOPV::Print() const
{
    int last = Last();
    for (int i = 0; i <= last; i++) {
        printf("f%d: ", i);
        GetSOP(i).Print();
    }
    printf("Size= %llu\n", static_cast<unsigned long long>(Size()));
    printf("Cube= %llu\n", static_cast<unsigned long long>(Cube()));
    printf("Lit=  %llu\n", static_cast<unsigned long long>(Lit()));
}

/* ================================================================ */
/*  SOPV PrintPla                                                    */
/* ================================================================ */

static void PrintPla_rec(const SOPV& sv, int lev, int min_lev,
                         int nin, std::vector<char>& buf, int nout)
{
    if (lev < min_lev) {
        /* At terminal: print this cube */
        /* Input section */
        for (int i = nin - 1; i >= 0; i--) {
            putchar(buf[i]);
        }
        putchar(' ');
        /* Output section */
        for (int i = 0; i < nout; i++) {
            SOP s = sv.GetSOP(i);
            bddp sp = s.GetZBDD().GetID();
            if (sp == bddsingle) {
                putchar('1');
            } else {
                putchar('0');
            }
        }
        putchar('\n');
        return;
    }

    /* Convert level to even VarID */
    bddvar var = bddvaroflev(static_cast<bddvar>(lev));
    int sop_var = (static_cast<int>(var) + 1) & ~1;  // round to even
    int var_idx = (lev - min_lev) / 2;

    SOPV f1 = sv.Factor1(sop_var);
    SOPV f0 = sv.Factor0(sop_var);
    SOPV fD = sv.FactorD(sop_var);

    /* Positive literal */
    if (f1.GetZBDDV().GetMetaZBDD().GetID() != bddempty) {
        buf[var_idx] = '1';
        PrintPla_rec(f1, lev - 2, min_lev, nin, buf, nout);
    }
    /* Negative literal */
    if (f0.GetZBDDV().GetMetaZBDD().GetID() != bddempty) {
        buf[var_idx] = '0';
        PrintPla_rec(f0, lev - 2, min_lev, nin, buf, nout);
    }
    /* Don't care */
    if (fD.GetZBDDV().GetMetaZBDD().GetID() != bddempty) {
        buf[var_idx] = '-';
        PrintPla_rec(fD, lev - 2, min_lev, nin, buf, nout);
    }
}

int SOPV::PrintPla() const
{
    bddp vp = _v.GetMetaZBDD().GetID();
    if (vp == bddnull) return 1;

    int top_lev = 0;
    int top_var = Top();
    if (top_var != 0) {
        top_lev = static_cast<int>(bddlevofvar(static_cast<bddvar>(top_var)));
    }

    /* SOP variable levels start above BDDV system variables */
    int min_lev = BDDV_Active ? (BDDV_SysVarTop + 1) : 1;
    /* Round min_lev up to even (SOP variables use even levels) */
    min_lev = (min_lev + 1) & ~1;

    int nin = (top_lev >= min_lev) ? (top_lev - min_lev) / 2 + 1 : 0;
    int nout = Last() + 1;

    printf(".i %d\n", nin);
    printf(".o %d\n", nout);
    printf(".p %llu\n", static_cast<unsigned long long>(Cube()));

    if (top_lev >= min_lev) {
        std::vector<char> buf(nin, '-');
        PrintPla_rec(*this, top_lev, min_lev, nin, buf, nout);
    } else {
        /* All constant: just output the values */
        for (int i = 0; i < nout; i++) {
            SOP s = GetSOP(i);
            bddp sp = s.GetZBDD().GetID();
            if (sp == bddsingle) {
                putchar('1');
            } else {
                putchar('0');
            }
        }
        putchar('\n');
    }

    printf(".e\n");
    return 0;
}

/* ================================================================ */
/*  SOPV_ISOP functions                                              */
/* ================================================================ */

SOPV SOPV_ISOP(BDDV v)
{
    return SOPV_ISOP(v, BDDV(BDD(0), v.Len()));
}

SOPV SOPV_ISOP(BDDV on, BDDV dc)
{
    int len = on.Len();
    if (len != dc.Len()) {
        throw std::invalid_argument("SOPV_ISOP: on and dc must have same length");
    }

    SOPV csv;
    for (int i = 0; i < len; i++) {
        SOP cs = SOP_ISOP(on.GetBDD(i), dc.GetBDD(i));
        csv += SOPV(cs, i);
    }
    return csv;
}

SOPV SOPV_ISOP2(BDDV v)
{
    return SOPV_ISOP2(v, BDDV(BDD(0), v.Len()));
}

SOPV SOPV_ISOP2(BDDV on, BDDV dc)
{
    int len = on.Len();
    if (len != dc.Len()) {
        throw std::invalid_argument("SOPV_ISOP2: on and dc must have same length");
    }

    SOPV csv;
    SOPV phase;

    for (int i = 0; i < len; i++) {
        SOP cs1 = SOP_ISOP(on.GetBDD(i), dc.GetBDD(i));
        SOP cs2 = SOP_ISOP(~on.GetBDD(i), dc.GetBDD(i));

        if (cs1.GetZBDD().GetID() == bddnull ||
            cs2.GetZBDD().GetID() == bddnull) {
            return SOPV(SOP(-1));
        }

        SOPV csv1 = csv + SOPV(cs1, i);
        SOPV csv2 = csv + SOPV(cs2, i);

        if (csv1.Lit() <= csv2.Lit()) {
            csv = csv1;
            /* phase[i + len] = 0 (SOP(0)), nothing to add */
        } else {
            csv = csv2;
            phase += SOPV(SOP(1), i + len);
        }
    }

    return csv + phase;
}
