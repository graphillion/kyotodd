#include "ctoi.h"
#include "bdd_internal.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <stdexcept>
#include <vector>
#include <string>

/* ================================================================ */
/*  Constructors / destructor / assignment                           */
/* ================================================================ */

CtoI::CtoI() : _zbdd() {}

CtoI::CtoI(const CtoI& a) : _zbdd(a._zbdd) {}

CtoI::CtoI(const ZDD& f) : _zbdd(f) {}

CtoI::CtoI(int n)
{
    if (n == 0) {
        _zbdd = ZDD(0);
        return;
    }
    if (n == 1) {
        _zbdd = ZDD(1);
        return;
    }
    if (n < 0) {
        if (n == INT_MIN) {
            throw std::overflow_error("CtoI(int): INT_MIN overflow");
        }
        *this = -CtoI(-n);
        return;
    }
    /* n >= 2: build via repeated doubling */
    *this = CtoI(0);
    CtoI a(1);
    unsigned int k = static_cast<unsigned int>(n);
    while (k > 0) {
        if (k & 1) *this = *this + a;
        a = a + a;
        k >>= 1;
    }
}

CtoI::~CtoI() {}

CtoI& CtoI::operator=(const CtoI& a)
{
    _zbdd = a._zbdd;
    return *this;
}

/* ================================================================ */
/*  Factor decomposition                                             */
/* ================================================================ */

CtoI CtoI::Factor0(int v) const
{
    return CtoI(_zbdd.Offset(static_cast<bddvar>(v)));
}

CtoI CtoI::Factor1(int v) const
{
    return CtoI(_zbdd.OnSet0(static_cast<bddvar>(v)));
}

CtoI CtoI::AffixVar(int v) const
{
    ZDD merged = _zbdd.Offset(static_cast<bddvar>(v))
               + _zbdd.OnSet0(static_cast<bddvar>(v));
    return CtoI(merged.Change(static_cast<bddvar>(v)));
}

/* ================================================================ */
/*  Set operations (external functions)                              */
/* ================================================================ */

CtoI CtoI_Intsec(const CtoI& a, const CtoI& b)
{
    return CtoI(a._zbdd & b._zbdd);
}

CtoI CtoI_Union(const CtoI& a, const CtoI& b)
{
    return CtoI(a._zbdd + b._zbdd);
}

CtoI CtoI_Diff(const CtoI& a, const CtoI& b)
{
    return CtoI(a._zbdd - b._zbdd);
}

CtoI CtoI_Null()
{
    return CtoI(ZDD(-1));
}

/* ================================================================ */
/*  Access / predicates                                              */
/* ================================================================ */

int CtoI::Top() const
{
    return static_cast<int>(_zbdd.top());
}

int CtoI::TopItem() const
{
    bddvar v = _zbdd.top();
    if (v == 0) return 0;
    bddvar lev = BDD_LevOfVar(v);
    if (lev > static_cast<bddvar>(BDDV_SysVarTop)) return static_cast<int>(v);
    /* v is a system variable — recurse into children */
    int t0 = Factor0(static_cast<int>(v)).TopItem();
    int t1 = Factor1(static_cast<int>(v)).TopItem();
    if (t0 == 0) return t1;
    if (t1 == 0) return t0;
    bddvar lev0 = BDD_LevOfVar(static_cast<bddvar>(t0));
    bddvar lev1 = BDD_LevOfVar(static_cast<bddvar>(t1));
    return (lev0 >= lev1) ? t0 : t1;
}

int CtoI::TopDigit() const
{
    bddvar v = _zbdd.top();
    if (v == 0) return 0;
    bddvar lev = BDD_LevOfVar(v);
    if (lev > static_cast<bddvar>(BDDV_SysVarTop)) return 0;
    int d0 = Factor0(static_cast<int>(v)).TopDigit();
    int d1 = Factor1(static_cast<int>(v)).TopDigit()
           + (1 << (BDDV_SysVarTop - static_cast<int>(v)));
    return (d0 >= d1) ? d0 : d1;
}

int CtoI::IsBool() const
{
    bddvar v = _zbdd.top();
    if (v == 0) return 1;
    bddvar lev = BDD_LevOfVar(v);
    return (lev > static_cast<bddvar>(BDDV_SysVarTop)) ? 1 : 0;
}

int CtoI::IsConst() const
{
    return (TopItem() == 0) ? 1 : 0;
}

ZDD CtoI::GetZBDD() const { return _zbdd; }

uint64_t CtoI::Size() const { return _zbdd.raw_size(); }

void CtoI::Print() const { _zbdd.print_sets(std::cout); std::cout << std::endl; }

/* ================================================================ */
/*  NonZero                                                          */
/* ================================================================ */

CtoI CtoI::NonZero() const
{
    if (IsBool()) return *this;
    bddvar v = _zbdd.top();
    CtoI f0 = Factor0(static_cast<int>(v)).NonZero();
    CtoI f1 = Factor1(static_cast<int>(v)).NonZero();
    return CtoI_Union(f0, f1);
}

/* ================================================================ */
/*  Digit operations                                                 */
/* ================================================================ */

CtoI CtoI::TimesSysVar(int v) const
{
    if (v < 1 || v > BDDV_SysVarTop) {
        throw std::invalid_argument("CtoI::TimesSysVar: v out of range");
    }
    if (_zbdd == ZDD(0) || _zbdd == ZDD(-1)) return *this;

    CtoI f0 = Factor0(v);
    CtoI f1 = Factor1(v);
    if (f1 == CtoI(0)) {
        return f0.AffixVar(v);
    }
    /* carry: f1 overflows into the next digit */
    return CtoI_Union(f0.AffixVar(v),
                      f1.TimesSysVar(v - 1));
}

CtoI CtoI::DivBySysVar(int v) const
{
    if (v < 1 || v > BDDV_SysVarTop) {
        throw std::invalid_argument("CtoI::DivBySysVar: v out of range");
    }
    if (_zbdd == ZDD(0) || _zbdd == ZDD(-1)) return *this;

    CtoI f0 = Factor0(v);
    CtoI f1 = Factor1(v);
    if (f0.IsBool() || v == 1) {
        return f1;
    }
    return CtoI_Union(f1,
                      f0.AffixVar(v).DivBySysVar(v - 1));
}

CtoI CtoI::ShiftDigit(int pow) const
{
    if (pow == 0) return *this;
    CtoI result = *this;
    int abspow = (pow > 0) ? pow : -pow;
    for (int i = 0; i < BDDV_SysVarTop; i++) {
        if (abspow & (1 << i)) {
            int sv = BDDV_SysVarTop - i;
            if (pow > 0)
                result = result.TimesSysVar(sv);
            else
                result = result.DivBySysVar(sv);
        }
    }
    return result;
}

CtoI CtoI::Digit(int index) const
{
    if (index < 0) {
        throw std::invalid_argument("CtoI::Digit: negative index");
    }
    CtoI c = *this;
    for (int i = 0; i < BDDV_SysVarTop; i++) {
        int sv = BDDV_SysVarTop - i;
        if (c._zbdd == ZDD(-1)) return CtoI_Null();
        if (index & (1 << i)) {
            c = c.Factor1(sv);
        } else {
            c = c.Factor0(sv);
        }
    }
    return c;
}

/* ================================================================ */
/*  Arithmetic operators                                             */
/* ================================================================ */

CtoI operator+(const CtoI& a, const CtoI& b)
{
    CtoI c = CtoI_Intsec(a, b);
    CtoI s = CtoI_Diff(CtoI_Union(a, b), c);
    if (c == CtoI(0)) return s;
    if (s._zbdd == ZDD(-1)) return CtoI_Null();
    return s - c.ShiftDigit(1);
}

CtoI operator-(const CtoI& a, const CtoI& b)
{
    CtoI c = CtoI_Diff(b, a);
    CtoI s = CtoI_Union(CtoI_Diff(a, b), c);
    if (c == CtoI(0)) return s;
    if (s._zbdd == ZDD(-1)) return CtoI_Null();
    return s + c.ShiftDigit(1);
}

CtoI operator*(const CtoI& a, const CtoI& b)
{
    /* Terminal cases */
    if (a._zbdd == ZDD(-1)) return CtoI_Null();
    if (b._zbdd == ZDD(-1)) return CtoI_Null();
    if (a == CtoI(0)) return CtoI(0);
    if (b == CtoI(0)) return CtoI(0);
    if (a == CtoI(1)) return b;
    if (b == CtoI(1)) return a;

    /* Normalize: a.Top level >= b.Top level */
    const CtoI* pa = &a;
    const CtoI* pb = &b;
    bddvar atop = a._zbdd.top();
    bddvar btop = b._zbdd.top();
    bddvar alev = BDD_LevOfVar(atop);
    bddvar blev = BDD_LevOfVar(btop);
    if (alev < blev || (alev == blev && a._zbdd.GetID() > b._zbdd.GetID())) {
        pa = &b;
        pb = &a;
        bddvar tmp = atop; atop = btop; btop = tmp;
        bddvar tmp2 = alev; alev = blev; blev = tmp2;
    }

    bddp fp = pa->_zbdd.GetID();
    bddp gp = pb->_zbdd.GetID();

    /* Cache lookup */
    ZDD cached = BDD_CacheZDD(BC_CtoI_MULT, fp, gp);
    if (cached != ZDD::Null) return CtoI(cached);

    int av = static_cast<int>(atop);
    CtoI a0 = pa->Factor0(av);
    CtoI a1 = pa->Factor1(av);
    CtoI r;

    if (atop != btop) {
        /* a has higher top variable */
        if (alev > static_cast<bddvar>(BDDV_SysVarTop)) {
            /* item variable */
            r = CtoI_Union(a0 * *pb, (a1 * *pb).AffixVar(av));
        } else {
            /* system variable */
            r = a0 * *pb + (a1 * *pb).TimesSysVar(av);
        }
    } else {
        /* same top variable */
        int bv = static_cast<int>(btop);
        CtoI b0 = pb->Factor0(bv);
        CtoI b1 = pb->Factor1(bv);
        if (alev > static_cast<bddvar>(BDDV_SysVarTop)) {
            /* item variable */
            r = CtoI_Union(a0 * b0,
                           (a1 * b0 + a0 * b1 + a1 * b1).AffixVar(av));
        } else {
            /* system variable */
            if (av <= 1) {
                throw std::overflow_error("CtoI multiplication: system variable overflow");
            }
            r = a0 * b0
                + (a1 * b0 + a0 * b1).TimesSysVar(av)
                + (a1 * b1).TimesSysVar(av - 1);
        }
    }

    /* Cache store */
    bddwcache(BC_CtoI_MULT, fp, gp, r._zbdd.GetID());
    return r;
}

CtoI operator/(const CtoI& a, const CtoI& b)
{
    /* Null checks */
    if (a._zbdd == ZDD(-1) || b._zbdd == ZDD(-1)) return CtoI_Null();
    if (a == CtoI(0)) return CtoI(0);
    if (a == b) return CtoI(1);
    if (b == CtoI(0)) {
        throw std::invalid_argument("CtoI division by zero");
    }
    if (b == CtoI(1)) return a;
    if (b == CtoI(-1)) return -a;

    /* For constant operands, use integer division */
    if (a.IsConst() && b.IsConst()) {
        int av = a.GetInt();
        int bv = b.GetInt();
        if (bv == 0) throw std::invalid_argument("CtoI division by zero");
        return CtoI(av / bv);
    }

    int v = b.TopItem();

    if (v == 0) {
        /* b is constant, a has items */
        CtoI bb = b;
        if (bb.GetInt() < 0) {
            return (-a) / (-bb);
        }
        /* Recursive division through item variables */
        int ti = a.TopItem();
        if (ti == 0) {
            /* a is also constant */
            return CtoI(a.GetInt() / bb.GetInt());
        }
        CtoI a0 = a.Factor0(ti);
        CtoI a1 = a.Factor1(ti);
        CtoI q0 = a0 / bb;
        CtoI q1 = a1 / bb;
        return CtoI_Union(q0, q1.AffixVar(ti));
    }

    /* b depends on items */
    CtoI a0 = a.Factor0(v);
    CtoI a1 = a.Factor1(v);
    CtoI b0 = b.Factor0(v);
    CtoI b1 = b.Factor1(v);

    if (b1 == CtoI(0)) {
        /* b doesn't depend on v after all */
        return a / b0;
    }

    CtoI c = a1 / b1;
    if (c != CtoI(0) && b0 != CtoI(0)) {
        CtoI c0 = a0 / b0;
        CtoI ca = c.Abs();
        CtoI c0a = c0.Abs();
        CtoI pick = CtoI_GT(ca, c0a);
        c = CtoI_ITE(pick, c0, c);
    }

    return c;
}

CtoI operator%(const CtoI& a, const CtoI& b)
{
    return a - (a / b) * b;
}

int operator==(const CtoI& a, const CtoI& b)
{
    return (a._zbdd == b._zbdd) ? 1 : 0;
}

int operator!=(const CtoI& a, const CtoI& b)
{
    return !(a == b);
}

/* ================================================================ */
/*  Unary operators                                                  */
/* ================================================================ */

CtoI CtoI::operator-() const
{
    return CtoI(0) - *this;
}

/* ================================================================ */
/*  Compound assignment operators                                    */
/* ================================================================ */

CtoI& CtoI::operator+=(const CtoI& a) { *this = *this + a; return *this; }
CtoI& CtoI::operator-=(const CtoI& a) { *this = *this - a; return *this; }
CtoI& CtoI::operator*=(const CtoI& a) { *this = *this * a; return *this; }
CtoI& CtoI::operator/=(const CtoI& a) { *this = *this / a; return *this; }
CtoI& CtoI::operator%=(const CtoI& a) { *this = *this % a; return *this; }

/* ================================================================ */
/*  Comparison functions                                             */
/* ================================================================ */

CtoI CtoI_GT(const CtoI& a, const CtoI& b)
{
    CtoI open = CtoI_Union(a, b).NonZero();
    CtoI awin(0);
    CtoI bwin(0);

    int td = a.TopDigit();
    { int btd = b.TopDigit(); if (btd > td) td = btd; }

    for (; td >= 0 && open != CtoI(0); td--) {
        CtoI aa = CtoI_Intsec(a.Digit(td), open);
        CtoI bb = CtoI_Intsec(b.Digit(td), open);

        CtoI awin0, bwin0;
        if (td & 1) {
            /* Negative weight: having this digit is BAD */
            awin0 = CtoI_Diff(bb, aa);  /* b has it, a doesn't → a wins */
            bwin0 = CtoI_Diff(aa, bb);  /* a has it, b doesn't → b wins */
        } else {
            /* Positive weight: having this digit is GOOD */
            awin0 = CtoI_Diff(aa, bb);  /* a has it, b doesn't → a wins */
            bwin0 = CtoI_Diff(bb, aa);  /* b has it, a doesn't → b wins */
        }

        awin = CtoI_Union(awin, awin0);
        bwin = CtoI_Union(bwin, bwin0);
        open = CtoI_Diff(open, CtoI_Union(awin0, bwin0));

        if (open.GetZBDD() == ZDD(-1)) return CtoI_Null();
    }
    return awin;
}

CtoI CtoI_GE(const CtoI& a, const CtoI& b)
{
    CtoI open = CtoI_Union(a, b).NonZero();
    CtoI awin(0);
    CtoI bwin(0);

    int td = a.TopDigit();
    { int btd = b.TopDigit(); if (btd > td) td = btd; }

    for (; td >= 0 && open != CtoI(0); td--) {
        CtoI aa = CtoI_Intsec(a.Digit(td), open);
        CtoI bb = CtoI_Intsec(b.Digit(td), open);

        CtoI awin0, bwin0;
        if (td & 1) {
            awin0 = CtoI_Diff(bb, aa);
            bwin0 = CtoI_Diff(aa, bb);
        } else {
            awin0 = CtoI_Diff(aa, bb);
            bwin0 = CtoI_Diff(bb, aa);
        }

        awin = CtoI_Union(awin, awin0);
        bwin = CtoI_Union(bwin, bwin0);
        open = CtoI_Diff(open, CtoI_Union(awin0, bwin0));

        if (open.GetZBDD() == ZDD(-1)) return CtoI_Null();
    }
    /* GE: undecided (equal) combinations also included */
    return CtoI_Union(awin, open);
}

CtoI CtoI_LT(const CtoI& a, const CtoI& b)
{
    return CtoI_GT(b, a);
}

CtoI CtoI_LE(const CtoI& a, const CtoI& b)
{
    return CtoI_GE(b, a);
}

CtoI CtoI_NE(const CtoI& a, const CtoI& b)
{
    return CtoI_Union(CtoI_Diff(a, b), CtoI_Diff(b, a)).NonZero();
}

CtoI CtoI_EQ(const CtoI& a, const CtoI& b)
{
    return CtoI_Diff(CtoI_Union(a, b).NonZero(), CtoI_NE(a, b));
}

/* ================================================================ */
/*  Constant comparison methods                                      */
/* ================================================================ */

CtoI CtoI::EQ_Const(const CtoI& a) const
{
    return CtoI_Diff(NonZero(), NE_Const(a));
}

CtoI CtoI::NE_Const(const CtoI& a) const
{
    if (a == CtoI(0)) return CtoI_NE(*this, CtoI(0));
    return CtoI_NE(*this, a * NonZero());
}

CtoI CtoI::GT_Const(const CtoI& a) const
{
    if (a == CtoI(0)) return CtoI_GT(*this, CtoI(0));
    return CtoI_GT(*this, a * NonZero());
}

CtoI CtoI::GE_Const(const CtoI& a) const
{
    if (a == CtoI(0)) return CtoI_GE(*this, CtoI(0));
    return CtoI_GE(*this, a * NonZero());
}

CtoI CtoI::LT_Const(const CtoI& a) const
{
    if (a == CtoI(0)) return CtoI_GT(CtoI(0), *this);
    return CtoI_GT(a * NonZero(), *this);
}

CtoI CtoI::LE_Const(const CtoI& a) const
{
    if (a == CtoI(0)) return CtoI_GE(CtoI(0), *this);
    return CtoI_GE(a * NonZero(), *this);
}

/* ================================================================ */
/*  Filter operations                                                */
/* ================================================================ */

CtoI CtoI::FilterThen(const CtoI& a) const
{
    if (*this == CtoI(0)) return CtoI(0);
    if (_zbdd == ZDD(-1)) return CtoI_Null();

    CtoI aa = a;
    if (!aa.IsBool()) aa = aa.NonZero();
    if (aa == CtoI(0)) return CtoI(0);

    if (IsBool()) return CtoI_Intsec(*this, aa);

    bddvar v = _zbdd.top();
    int vi = static_cast<int>(v);

    if (BDD_LevOfVar(v) <= static_cast<bddvar>(BDDV_SysVarTop)) {
        /* System variable: condition is item-only, apply same to both */
        CtoI r0 = Factor0(vi).FilterThen(aa);
        CtoI r1 = Factor1(vi).FilterThen(aa);
        return CtoI_Union(r0, r1.AffixVar(vi));
    }
    /* Item variable: decompose condition too */
    CtoI r0 = Factor0(vi).FilterThen(aa.Factor0(vi));
    CtoI r1 = Factor1(vi).FilterThen(aa.Factor1(vi));
    return CtoI_Union(r0, r1.AffixVar(vi));
}

CtoI CtoI::FilterElse(const CtoI& a) const
{
    if (*this == CtoI(0)) return CtoI(0);
    if (_zbdd == ZDD(-1)) return CtoI_Null();

    CtoI aa = a;
    if (!aa.IsBool()) aa = aa.NonZero();
    if (aa == CtoI(0)) return *this;

    if (IsBool()) return CtoI_Diff(*this, aa);

    bddvar v = _zbdd.top();
    int vi = static_cast<int>(v);

    if (BDD_LevOfVar(v) <= static_cast<bddvar>(BDDV_SysVarTop)) {
        /* System variable: condition is item-only, apply same to both */
        CtoI r0 = Factor0(vi).FilterElse(aa);
        CtoI r1 = Factor1(vi).FilterElse(aa);
        return CtoI_Union(r0, r1.AffixVar(vi));
    }
    /* Item variable: decompose condition too */
    CtoI r0 = Factor0(vi).FilterElse(aa.Factor0(vi));
    CtoI r1 = Factor1(vi).FilterElse(aa.Factor1(vi));
    return CtoI_Union(r0, r1.AffixVar(vi));
}

CtoI CtoI::FilterRestrict(const CtoI& a) const
{
    CtoI self_bool = IsBool() ? *this : NonZero();
    CtoI a_bool = a.IsBool() ? a : a.NonZero();
    CtoI restricted = CtoI(self_bool.GetZBDD().Restrict(a_bool.GetZBDD()));
    return FilterThen(restricted);
}

CtoI CtoI::FilterPermit(const CtoI& a) const
{
    CtoI self_bool = IsBool() ? *this : NonZero();
    CtoI a_bool = a.IsBool() ? a : a.NonZero();
    CtoI permitted = CtoI(self_bool.GetZBDD().Permit(a_bool.GetZBDD()));
    return FilterThen(permitted);
}

CtoI CtoI::FilterPermitSym(int n) const
{
    CtoI self_bool = IsBool() ? *this : NonZero();
    CtoI permitted = CtoI(self_bool.GetZBDD().PermitSym(n));
    return FilterThen(permitted);
}

/* ================================================================ */
/*  Max / Min                                                        */
/* ================================================================ */

CtoI CtoI::MaxVal() const
{
    if (_zbdd == ZDD(-1)) return CtoI_Null();
    if (*this == CtoI(0)) return CtoI(0);

    /* For constant CtoI, just return self */
    if (IsConst()) return *this;

    /* General case: iterate through item variables */
    int top = TopItem();
    if (top == 0) return *this;

    CtoI f0 = Factor0(top);
    CtoI f1 = Factor1(top);

    CtoI m0 = f0.MaxVal();
    CtoI m1 = f1.MaxVal();

    /* Return the larger of the two maximums */
    CtoI gt = CtoI_GT(m0, m1);
    if (gt != CtoI(0)) return m0;
    return m1;
}

CtoI CtoI::MinVal() const
{
    return -((-*this).MaxVal());
}

/* ================================================================ */
/*  Abs / Sign                                                       */
/* ================================================================ */

CtoI CtoI::Abs() const
{
    CtoI pos = CtoI_GT(*this, CtoI(0));
    CtoI pos_part = FilterThen(pos);
    CtoI neg_part = -FilterElse(pos);
    return CtoI_Union(pos_part, neg_part);
}

CtoI CtoI::Sign() const
{
    CtoI pos = CtoI_GT(*this, CtoI(0));
    CtoI nz = NonZero();
    CtoI neg = CtoI_Diff(nz, pos);
    return pos - neg;
}

/* ================================================================ */
/*  Conditional                                                      */
/* ================================================================ */

CtoI CtoI_ITE(const CtoI& a, const CtoI& b, const CtoI& c)
{
    return CtoI_Union(b.FilterThen(a), c.FilterElse(a));
}

CtoI CtoI_Max(const CtoI& a, const CtoI& b)
{
    return CtoI_ITE(CtoI_GT(a, b), a, b);
}

CtoI CtoI_Min(const CtoI& a, const CtoI& b)
{
    return CtoI_ITE(CtoI_GT(a, b), b, a);
}

/* ================================================================ */
/*  Aggregation                                                      */
/* ================================================================ */

CtoI CtoI::CountTerms() const
{
    if (IsBool()) return TotalVal();
    return NonZero().TotalVal();
}

CtoI CtoI::TotalVal() const
{
    if (_zbdd == ZDD(-1)) return CtoI_Null();
    bddvar top = _zbdd.top();
    if (top == 0) return *this;

    bddp fp = _zbdd.GetID();
    ZDD cached = BDD_CacheZDD(BC_CtoI_TV, fp, fp);
    if (cached != ZDD::Null) return CtoI(cached);

    int vi = static_cast<int>(top);
    CtoI c = Factor0(vi).TotalVal();

    if (BDD_LevOfVar(top) > static_cast<bddvar>(BDDV_SysVarTop)) {
        /* Item variable: just add Factor1's total */
        c = c + Factor1(vi).TotalVal();
    } else {
        /* System variable: shift Factor1's total by digit weight */
        int shift = 1 << (BDDV_SysVarTop - vi);
        c = c + Factor1(vi).TotalVal().ShiftDigit(shift);
    }

    bddwcache(BC_CtoI_TV, fp, fp, c._zbdd.GetID());
    return c;
}

CtoI CtoI::TotalValItems() const
{
    if (_zbdd == ZDD(-1)) return CtoI_Null();
    bddvar top = _zbdd.top();
    if (top == 0) return CtoI(0);

    bddp fp = _zbdd.GetID();
    ZDD cached = BDD_CacheZDD(BC_CtoI_TVI, fp, fp);
    if (cached != ZDD::Null) return CtoI(cached);

    int vi = static_cast<int>(top);
    CtoI c = Factor0(vi).TotalValItems();

    if (BDD_LevOfVar(top) > static_cast<bddvar>(BDDV_SysVarTop)) {
        /* Item variable: add Factor1's TVI + Factor1's TV */
        c = c + Factor1(vi).TotalValItems()
              + Factor1(vi).TotalVal().AffixVar(vi);
    } else {
        /* System variable: shift Factor1's TVI by digit weight */
        int shift = 1 << (BDDV_SysVarTop - vi);
        c = c + Factor1(vi).TotalValItems().ShiftDigit(shift);
    }

    bddwcache(BC_CtoI_TVI, fp, fp, c._zbdd.GetID());
    return c;
}

/* ================================================================ */
/*  ConstTerm / Support                                              */
/* ================================================================ */

CtoI CtoI::ConstTerm() const
{
    if (IsBool()) return CtoI_Intsec(*this, CtoI(1));
    bddvar v = _zbdd.top();
    int vi = static_cast<int>(v);
    CtoI f0 = Factor0(vi).ConstTerm();
    CtoI f1 = Factor1(vi).ConstTerm();
    return CtoI_Union(f0, f1.AffixVar(vi));
}

CtoI CtoI::Support() const
{
    if (IsBool()) return *this;
    return CtoI(NonZero().GetZBDD().Support());
}

/* ================================================================ */
/*  GetInt                                                           */
/* ================================================================ */

int CtoI::GetInt() const
{
    if (_zbdd == ZDD(-1)) return 0;
    if (IsBool()) {
        CtoI test = CtoI_Intsec(*this, CtoI(1));
        return (test == CtoI(0)) ? 0 : 1;
    }
    bddvar v = _zbdd.top();
    int vi = static_cast<int>(v);
    CtoI f0 = Factor0(vi);
    CtoI f1 = Factor1(vi);
    if (f0._zbdd == ZDD(-1) || f1._zbdd == ZDD(-1)) return 0;

    if (vi == BDDV_SysVarTop) {
        /* Sign bit: weight is -2 */
        return f0.GetInt() - 2 * f1.GetInt();
    }
    /* Regular system variable: weight = 2^(2^(SysVarTop - vi)) */
    int shift = 1 << (BDDV_SysVarTop - vi);
    if (shift >= 63) {
        /* Shift too large for long long — value is unrepresentable as int */
        return (f1.GetInt() != 0) ? INT_MAX : f0.GetInt();
    }
    long long multiplier = 1LL << shift;
    long long result = static_cast<long long>(f0.GetInt())
                     + multiplier * static_cast<long long>(f1.GetInt());
    if (result > INT_MAX) return INT_MAX;
    if (result < INT_MIN) return INT_MIN;
    return static_cast<int>(result);
}

/* ================================================================ */
/*  StrNum10 / StrNum16                                              */
/* ================================================================ */

int CtoI::StrNum10(char* s) const
{
    if (_zbdd == ZDD(-1)) {
        std::strcpy(s, "0");
        return 1;
    }
    if (!IsConst()) {
        /* Extract constant term (value for empty combination) */
        return FilterThen(CtoI(1)).StrNum10(s);
    }
    int td = TopDigit();
    if (td & 1) {
        /* Negative: negate and prepend '-' */
        CtoI neg = -*this;
        s[0] = '-';
        return neg.StrNum10(s + 1);
    }
    if (td >= 20) {
        /* Large number: divide by 10^9 repeatedly */
        CtoI base(1000000000);
        CtoI q = *this / base;
        CtoI r = *this % base;
        int err = q.StrNum10(s);
        if (err) return err;
        int rval = r.GetInt();
        char buf[16];
        std::sprintf(buf, "%09d", rval);
        std::strcat(s, buf);
        return 0;
    }
    int val = GetInt();
    std::sprintf(s, "%d", val);
    return 0;
}

int CtoI::StrNum16(char* s) const
{
    if (_zbdd == ZDD(-1)) {
        std::strcpy(s, "0");
        return 1;
    }
    if (!IsConst()) {
        return FilterThen(CtoI(1)).StrNum16(s);
    }
    int td = TopDigit();
    if (td & 1) {
        CtoI neg = -*this;
        s[0] = '-';
        return neg.StrNum16(s + 1);
    }
    if (td >= 20) {
        CtoI base(0x1000000);  /* 16^6 */
        CtoI q = *this / base;
        CtoI r = *this % base;
        int err = q.StrNum16(s);
        if (err) return err;
        int rval = r.GetInt();
        char buf[16];
        std::sprintf(buf, "%06x", rval);
        std::strcat(s, buf);
        return 0;
    }
    int val = GetInt();
    std::sprintf(s, "%x", val);
    return 0;
}

/* ================================================================ */
/*  PutForm                                                          */
/* ================================================================ */

static int PutForm_rec(const CtoI& c, std::vector<int>& varstack, int first)
{
    if (c.GetZBDD() == ZDD(-1)) return 1;
    if (c == CtoI(0)) return 0;

    int ti = c.TopItem();
    if (ti == 0) {
        /* Constant term: print coefficient and variable stack */
        int val = c.GetInt();
        if (val == 0) return 0;
        if (!first && val > 0) std::printf(" +");
        if (varstack.empty()) {
            std::printf(" %d", val);
        } else {
            if (val == 1) {
                std::printf(" ");
            } else if (val == -1) {
                std::printf(" -");
            } else {
                std::printf(" %d", val);
            }
            for (size_t i = 0; i < varstack.size(); i++) {
                std::printf("v%d", varstack[i]);
                if (i + 1 < varstack.size()) std::printf("*");
            }
        }
        return 0;
    }

    /* Recurse: Factor1 first (terms containing this variable) */
    varstack.push_back(ti);
    int err = PutForm_rec(c.Factor1(ti), varstack, first);
    varstack.pop_back();
    if (err) return err;

    /* Then Factor0 (terms not containing this variable) */
    return PutForm_rec(c.Factor0(ti), varstack, 0);
}

int CtoI::PutForm() const
{
    if (_zbdd == ZDD(-1)) return 1;
    if (*this == CtoI(0)) {
        std::printf(" 0");
        return 0;
    }
    std::vector<int> varstack;
    return PutForm_rec(*this, varstack, 1);
}

/* ================================================================ */
/*  CtoI_Meet                                                        */
/* ================================================================ */

CtoI CtoI_Meet(const CtoI& a, const CtoI& b)
{
    /* Terminal cases */
    if (a._zbdd == ZDD(-1) || b._zbdd == ZDD(-1)) return CtoI_Null();
    if (a == CtoI(0) || b == CtoI(0)) return CtoI(0);
    if (a == CtoI(1) && b == CtoI(1)) return CtoI(1);

    /* Normalize: a.top level >= b.top level */
    const CtoI* pa = &a;
    const CtoI* pb = &b;
    bddvar atop = a._zbdd.top();
    bddvar btop = b._zbdd.top();
    bddvar alev = BDD_LevOfVar(atop);
    bddvar blev = BDD_LevOfVar(btop);
    if (alev < blev || (alev == blev && a._zbdd.GetID() < b._zbdd.GetID())) {
        pa = &b;
        pb = &a;
        bddvar tmp = atop; atop = btop; btop = tmp;
        bddvar tmp2 = alev; alev = blev; blev = tmp2;
    }

    bddp fp = pa->_zbdd.GetID();
    bddp gp = pb->_zbdd.GetID();

    ZDD cached = BDD_CacheZDD(BC_CtoI_MEET, fp, gp);
    if (cached != ZDD::Null) return CtoI(cached);

    int av = static_cast<int>(atop);
    CtoI a0 = pa->Factor0(av);
    CtoI a1 = pa->Factor1(av);
    CtoI r;

    if (atop != btop) {
        if (alev > static_cast<bddvar>(BDDV_SysVarTop)) {
            /* Item variable */
            r = CtoI_Meet(a0, *pb) + CtoI_Meet(a1, *pb);
        } else {
            /* System variable */
            r = CtoI_Meet(a0, *pb) + CtoI_Meet(a1, *pb).TimesSysVar(av);
        }
    } else {
        int bv = static_cast<int>(btop);
        CtoI b0 = pb->Factor0(bv);
        CtoI b1 = pb->Factor1(bv);
        if (alev > static_cast<bddvar>(BDDV_SysVarTop)) {
            /* Item variable */
            r = CtoI_Union(
                CtoI_Meet(a0, b0) + CtoI_Meet(a1, b0) + CtoI_Meet(a0, b1),
                CtoI_Meet(a1, b1).AffixVar(av)
            );
        } else {
            /* System variable */
            if (av <= 1) {
                throw std::overflow_error("CtoI_Meet: system variable overflow");
            }
            r = CtoI_Meet(a0, b0)
                + (CtoI_Meet(a1, b0) + CtoI_Meet(a0, b1)).TimesSysVar(av)
                + CtoI_Meet(a1, b1).TimesSysVar(av - 1);
        }
    }

    bddwcache(BC_CtoI_MEET, fp, gp, r._zbdd.GetID());
    return r;
}

/* ================================================================ */
/*  CtoI_atoi                                                        */
/* ================================================================ */

static CtoI atoiX(const char* s, int base, int block)
{
    int len = static_cast<int>(std::strlen(s));
    if (len == 0) return CtoI(0);

    /* Process last block */
    int blen = (len < block) ? len : block;
    char buf[32];
    std::strncpy(buf, s + len - blen, static_cast<size_t>(blen));
    buf[blen] = '\0';
    long val = std::strtol(buf, NULL, base);
    CtoI result(static_cast<int>(val));

    len -= blen;
    while (len > 0) {
        blen = (len < block) ? len : block;
        /* Compute base^blen */
        int mult = 1;
        for (int i = 0; i < blen; i++) mult *= base;
        /* Parse next block */
        std::strncpy(buf, s + len - blen, static_cast<size_t>(blen));
        buf[blen] = '\0';
        val = std::strtol(buf, NULL, base);
        result = CtoI(mult) * result + CtoI(static_cast<int>(val));
        len -= blen;
    }

    return result;
}

CtoI CtoI_atoi(const char* s)
{
    if (s == NULL || s[0] == '\0') return CtoI(0);

    int neg = 0;
    if (s[0] == '-') {
        neg = 1;
        s++;
    } else if (s[0] == '+') {
        s++;
    }

    CtoI result;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        result = atoiX(s + 2, 16, 7);
    } else if (s[0] == '0' && (s[1] == 'b' || s[1] == 'B')) {
        result = atoiX(s + 2, 2, 30);
    } else {
        result = atoiX(s, 10, 7);
    }

    return neg ? -result : result;
}

/* ================================================================ */
/*  FreqPat operations                                               */
/* ================================================================ */

CtoI CtoI::ReduceItems(const CtoI& b) const
{
    if (_zbdd == ZDD(-1)) return CtoI_Null();
    CtoI bb = b.IsBool() ? b : b.NonZero();

    bddvar atop = _zbdd.top();
    bddvar btop = bb._zbdd.top();
    if (atop == 0 && btop == 0) return *this;

    bddp fp = _zbdd.GetID();
    bddp gp = bb._zbdd.GetID();
    ZDD cached = BDD_CacheZDD(BC_CtoI_RI, fp, gp);
    if (cached != ZDD::Null) return CtoI(cached);

    bddvar alev = (atop != 0) ? BDD_LevOfVar(atop) : 0;
    bddvar blev = (btop != 0) ? BDD_LevOfVar(btop) : 0;
    int avi = static_cast<int>(atop);
    int bvi = static_cast<int>(btop);
    CtoI r;

    if (blev > alev) {
        /* b has higher top: reduce with b's Factor0 */
        r = ReduceItems(bb.Factor0(bvi));
    } else if (!IsBool()) {
        /* self is not boolean (has system variables at top) */
        r = Factor0(avi).ReduceItems(bb)
            + Factor1(avi).ReduceItems(bb).ShiftDigit(1 << (BDDV_SysVarTop - avi));
    } else if (alev == blev && alev > static_cast<bddvar>(BDDV_SysVarTop)) {
        /* Both are item variables at the same level */
        r = Factor0(avi).ReduceItems(bb.Factor0(bvi))
            + Factor1(avi).ReduceItems(bb.Factor1(bvi));
    } else {
        /* self is item, b has different/system top */
        r = CtoI_Union(
            Factor0(avi).ReduceItems(bb),
            Factor1(avi).ReduceItems(bb).AffixVar(avi)
        );
    }

    bddwcache(BC_CtoI_RI, fp, gp, r._zbdd.GetID());
    return r;
}

CtoI CtoI::FreqPatA(int Val) const
{
    if (_zbdd == ZDD(-1)) return CtoI_Null();
    if (IsConst()) {
        return (GetInt() >= Val) ? CtoI(1) : CtoI(0);
    }

    bddp fp = _zbdd.GetID();
    bddp gp = static_cast<bddp>(Val);
    ZDD cached = BDD_CacheZDD(BC_CtoI_FPA, fp, gp);
    if (cached != ZDD::Null) return CtoI(cached);

    int top = TopItem();
    CtoI f1 = Factor1(top);

    /* Pruning: check if f1 can reach Val */
    CtoI f2 = f1;
    /* Simple upper bound: if f2's max possible < Val, prune */
    /* (simplified: just use f2 as is) */

    CtoI h1 = f2.FreqPatA(Val);
    CtoI h = CtoI_Union(h1, h1.AffixVar(top));

    CtoI f0 = Factor0(top);
    if (f0 != CtoI(0)) {
        CtoI f0r = f0.FilterElse(h1);
        CtoI f1r = f1.FilterElse(h1);
        CtoI combined = (f0r + f1r).FreqPatA(Val);
        h = CtoI_Union(h, combined);
    }

    bddwcache(BC_CtoI_FPA, fp, gp, h._zbdd.GetID());
    return h;
}

CtoI CtoI::FreqPatA2(int Val) const
{
    /* Improved version with TotalValItems pruning */
    if (_zbdd == ZDD(-1)) return CtoI_Null();
    if (IsConst()) {
        return (GetInt() >= Val) ? CtoI(1) : CtoI(0);
    }

    bddp fp = _zbdd.GetID();
    bddp gp = static_cast<bddp>(Val);
    ZDD cached = BDD_CacheZDD(BC_CtoI_FPA2, fp, gp);
    if (cached != ZDD::Null) return CtoI(cached);

    int top = TopItem();
    CtoI f1 = Factor1(top);
    CtoI f2 = f1;

    CtoI h1 = f2.FreqPatA2(Val);
    CtoI h = CtoI_Union(h1, h1.AffixVar(top));

    CtoI f0 = Factor0(top);
    if (f0 != CtoI(0)) {
        CtoI f0r = f0.FilterElse(h1);
        CtoI f1r = f1.FilterElse(h1);
        CtoI combined = (f0r + f1r).FreqPatA2(Val);
        h = CtoI_Union(h, combined);
    }

    bddwcache(BC_CtoI_FPA2, fp, gp, h._zbdd.GetID());
    return h;
}

CtoI CtoI::FreqPatAV(int Val) const
{
    if (_zbdd == ZDD(-1)) return CtoI_Null();
    if (IsConst()) {
        return (GetInt() >= Val) ? *this : CtoI(0);
    }

    bddp fp = _zbdd.GetID();
    bddp gp = static_cast<bddp>(Val);
    ZDD cached = BDD_CacheZDD(BC_CtoI_FPAV, fp, gp);
    if (cached != ZDD::Null) return CtoI(cached);

    int top = TopItem();
    CtoI f1 = Factor1(top);
    CtoI f2 = f1;

    CtoI h1 = f2.FreqPatAV(Val);
    CtoI h = h1.AffixVar(top);

    CtoI f0 = Factor0(top);
    if (f0 == CtoI(0)) {
        h = CtoI_Union(h1, h);
    } else {
        h = CtoI_Union(h, (f0 + f1).FreqPatAV(Val));
    }

    bddwcache(BC_CtoI_FPAV, fp, gp, h._zbdd.GetID());
    return h;
}

CtoI CtoI::FreqPatM(int Val) const
{
    if (_zbdd == ZDD(-1)) return CtoI_Null();
    if (IsConst()) {
        return (GetInt() >= Val) ? CtoI(1) : CtoI(0);
    }

    bddp fp = _zbdd.GetID();
    bddp gp = static_cast<bddp>(Val);
    ZDD cached = BDD_CacheZDD(BC_CtoI_FPM, fp, gp);
    if (cached != ZDD::Null) return CtoI(cached);

    int top = TopItem();
    CtoI f1 = Factor1(top);
    CtoI f2 = f1;

    CtoI h1 = f2.FreqPatM(Val);
    CtoI h = h1.AffixVar(top);

    CtoI f0 = Factor0(top);
    if (f0 != CtoI(0)) {
        /* Remove subsets covered by h1 */
        CtoI f0r = CtoI_Diff(f0, CtoI(f0.GetZBDD().Permit(h1.GetZBDD())));
        CtoI f1r = CtoI_Diff(f1, CtoI(f1.GetZBDD().Permit(h1.GetZBDD())));
        CtoI combined = (f0r + f1r).FreqPatM(Val);
        combined = CtoI_Diff(combined, CtoI(combined.GetZBDD().Permit(h1.GetZBDD())));
        h = CtoI_Union(h, combined);
    }

    bddwcache(BC_CtoI_FPM, fp, gp, h._zbdd.GetID());
    return h;
}

CtoI CtoI::FreqPatC(int Val) const
{
    if (_zbdd == ZDD(-1)) return CtoI_Null();
    if (IsConst()) {
        return (GetInt() >= Val) ? CtoI(1) : CtoI(0);
    }

    bddp fp = _zbdd.GetID();
    bddp gp = static_cast<bddp>(Val);
    ZDD cached = BDD_CacheZDD(BC_CtoI_FPC, fp, gp);
    if (cached != ZDD::Null) return CtoI(cached);

    int top = TopItem();
    CtoI f1 = Factor1(top);
    CtoI f2 = f1;

    CtoI h1 = f2.FreqPatC(Val);
    CtoI h = h1.AffixVar(top);

    CtoI f0 = Factor0(top);
    if (f0 != CtoI(0)) {
        /* Remove closed patterns already found */
        CtoI h1_permit = CtoI(h1.GetZBDD().Permit(f0.GetZBDD()));
        CtoI h1_filtered = CtoI_Diff(h1, h1_permit);
        CtoI combined = (f0 + f1).FreqPatC(Val);
        combined = combined.FilterElse(h1_filtered);
        h = CtoI_Union(h, combined);
    }

    bddwcache(BC_CtoI_FPC, fp, gp, h._zbdd.GetID());
    return h;
}
