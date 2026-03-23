#include <gtest/gtest.h>
#include "ctoi.h"
#include <climits>
#include <cstring>

class CtoITest : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        if (!BDDV_Active) {
            BDDV_Init();
        }
        while (BDDV_UserTopLev() < 5) {
            BDDV_NewVar();
        }
    }
};

/* ================================================================ */
/*  Constructors                                                     */
/* ================================================================ */

TEST_F(CtoITest, DefaultConstructor) {
    CtoI c;
    EXPECT_EQ(c, CtoI(0));
    EXPECT_EQ(c.GetZBDD(), ZDD(0));
}

TEST_F(CtoITest, IntConstructorZero) {
    CtoI c(0);
    EXPECT_EQ(c.GetZBDD(), ZDD(0));
}

TEST_F(CtoITest, IntConstructorOne) {
    CtoI c(1);
    EXPECT_EQ(c.GetZBDD(), ZDD(1));
}

TEST_F(CtoITest, IntConstructorTwo) {
    CtoI c(2);
    EXPECT_EQ(c.GetInt(), 2);
}

TEST_F(CtoITest, IntConstructorThree) {
    CtoI c(3);
    EXPECT_EQ(c.GetInt(), 3);
}

TEST_F(CtoITest, IntConstructorLarger) {
    CtoI c(42);
    EXPECT_EQ(c.GetInt(), 42);
}

TEST_F(CtoITest, IntConstructorNegative) {
    CtoI c(-1);
    EXPECT_EQ(c.GetInt(), -1);
}

TEST_F(CtoITest, IntConstructorNegativeLarger) {
    CtoI c(-5);
    EXPECT_EQ(c.GetInt(), -5);
}

TEST_F(CtoITest, IntConstructorINTMIN) {
    int val = INT_MIN;
    EXPECT_THROW({ CtoI tmp(val); (void)tmp; }, std::overflow_error);
}

TEST_F(CtoITest, CopyConstructor) {
    CtoI a(7);
    CtoI b(a);
    EXPECT_EQ(a, b);
    EXPECT_EQ(b.GetInt(), 7);
}

TEST_F(CtoITest, ZBDDConstructor) {
    CtoI c(ZDD(1));
    EXPECT_EQ(c.GetZBDD(), ZDD(1));
}

/* ================================================================ */
/*  Assignment                                                       */
/* ================================================================ */

TEST_F(CtoITest, Assignment) {
    CtoI a(5);
    CtoI b;
    b = a;
    EXPECT_EQ(b.GetInt(), 5);
}

/* ================================================================ */
/*  Addition                                                         */
/* ================================================================ */

TEST_F(CtoITest, AddZeroZero) {
    EXPECT_EQ(CtoI(0) + CtoI(0), CtoI(0));
}

TEST_F(CtoITest, AddOneOne) {
    CtoI r = CtoI(1) + CtoI(1);
    EXPECT_EQ(r.GetInt(), 2);
}

TEST_F(CtoITest, AddOnePlusTwo) {
    CtoI r = CtoI(1) + CtoI(2);
    EXPECT_EQ(r.GetInt(), 3);
}

TEST_F(CtoITest, AddTwoPlusTwo) {
    CtoI r = CtoI(2) + CtoI(2);
    EXPECT_EQ(r.GetInt(), 4);
}

TEST_F(CtoITest, AddLarger) {
    CtoI r = CtoI(10) + CtoI(20);
    EXPECT_EQ(r.GetInt(), 30);
}

TEST_F(CtoITest, AddNegative) {
    CtoI r = CtoI(5) + CtoI(-3);
    EXPECT_EQ(r.GetInt(), 2);
}

/* ================================================================ */
/*  Subtraction                                                      */
/* ================================================================ */

TEST_F(CtoITest, SubBasic) {
    CtoI r = CtoI(5) - CtoI(3);
    EXPECT_EQ(r.GetInt(), 2);
}

TEST_F(CtoITest, SubToZero) {
    CtoI r = CtoI(5) - CtoI(5);
    EXPECT_EQ(r.GetInt(), 0);
}

TEST_F(CtoITest, SubToNegative) {
    CtoI r = CtoI(3) - CtoI(5);
    EXPECT_EQ(r.GetInt(), -2);
}

/* ================================================================ */
/*  Unary minus                                                      */
/* ================================================================ */

TEST_F(CtoITest, UnaryMinus) {
    CtoI a(7);
    CtoI b = -a;
    EXPECT_EQ(b.GetInt(), -7);
}

TEST_F(CtoITest, UnaryMinusZero) {
    CtoI a(0);
    CtoI b = -a;
    EXPECT_EQ(b, CtoI(0));
}

/* ================================================================ */
/*  Multiplication                                                   */
/* ================================================================ */

TEST_F(CtoITest, MultByZero) {
    CtoI r = CtoI(5) * CtoI(0);
    EXPECT_EQ(r, CtoI(0));
}

TEST_F(CtoITest, MultByOne) {
    CtoI r = CtoI(5) * CtoI(1);
    EXPECT_EQ(r.GetInt(), 5);
}

TEST_F(CtoITest, MultBasic) {
    CtoI r = CtoI(3) * CtoI(4);
    EXPECT_EQ(r.GetInt(), 12);
}

TEST_F(CtoITest, MultNeg) {
    CtoI r = CtoI(3) * CtoI(-2);
    EXPECT_EQ(r.GetInt(), -6);
}

/* ================================================================ */
/*  Division                                                         */
/* ================================================================ */

TEST_F(CtoITest, DivBasic) {
    CtoI r = CtoI(10) / CtoI(2);
    EXPECT_EQ(r.GetInt(), 5);
}

TEST_F(CtoITest, DivByOne) {
    CtoI r = CtoI(7) / CtoI(1);
    EXPECT_EQ(r.GetInt(), 7);
}

TEST_F(CtoITest, DivByZeroThrows) {
    EXPECT_THROW(CtoI(5) / CtoI(0), std::invalid_argument);
}

/* ================================================================ */
/*  Modulo                                                           */
/* ================================================================ */

TEST_F(CtoITest, ModBasic) {
    CtoI r = CtoI(7) % CtoI(3);
    EXPECT_EQ(r.GetInt(), 1);
}

/* ================================================================ */
/*  Equality                                                         */
/* ================================================================ */

TEST_F(CtoITest, EqualityTrue) {
    EXPECT_EQ(CtoI(5) == CtoI(5), 1);
}

TEST_F(CtoITest, EqualityFalse) {
    EXPECT_EQ(CtoI(5) == CtoI(3), 0);
}

TEST_F(CtoITest, InequalityTrue) {
    EXPECT_EQ(CtoI(5) != CtoI(3), 1);
}

/* ================================================================ */
/*  Compound assignment                                              */
/* ================================================================ */

TEST_F(CtoITest, PlusEquals) {
    CtoI a(3);
    a += CtoI(4);
    EXPECT_EQ(a.GetInt(), 7);
}

TEST_F(CtoITest, MinusEquals) {
    CtoI a(10);
    a -= CtoI(3);
    EXPECT_EQ(a.GetInt(), 7);
}

TEST_F(CtoITest, TimesEquals) {
    CtoI a(3);
    a *= CtoI(5);
    EXPECT_EQ(a.GetInt(), 15);
}

/* ================================================================ */
/*  Access / predicates                                              */
/* ================================================================ */

TEST_F(CtoITest, IsBoolConstant) {
    EXPECT_EQ(CtoI(0).IsBool(), 1);
    EXPECT_EQ(CtoI(1).IsBool(), 1);
}

TEST_F(CtoITest, IsBoolNonConstant) {
    /* CtoI(2) has system variables → not boolean */
    EXPECT_EQ(CtoI(2).IsBool(), 0);
}

TEST_F(CtoITest, IsConstTrue) {
    EXPECT_EQ(CtoI(5).IsConst(), 1);
}

TEST_F(CtoITest, TopDigitZero) {
    EXPECT_EQ(CtoI(0).TopDigit(), 0);
    EXPECT_EQ(CtoI(1).TopDigit(), 0);
}

TEST_F(CtoITest, TopDigitNonZero) {
    /* CtoI(2) should have a non-zero top digit */
    EXPECT_GT(CtoI(2).TopDigit(), 0);
}

TEST_F(CtoITest, Size) {
    EXPECT_GE(CtoI(0).Size(), 0u);
    EXPECT_GE(CtoI(5).Size(), 0u);
}

/* ================================================================ */
/*  Factor decomposition                                             */
/* ================================================================ */

TEST_F(CtoITest, Factor0Factor1System) {
    /* For a constant CtoI, Factor0/Factor1 by system variable */
    CtoI c(3);
    int sv = BDDV_SysVarTop;  /* system var 20 (sign bit) */
    CtoI f0 = c.Factor0(sv);
    CtoI f1 = c.Factor1(sv);
    /* f0 and f1 are the parts without/with the sign bit */
    EXPECT_NE(f0, CtoI_Null());
    EXPECT_NE(f1, CtoI_Null());
}

/* ================================================================ */
/*  Set operations                                                   */
/* ================================================================ */

TEST_F(CtoITest, UnionDisjoint) {
    /* Union of disjoint CtoI values */
    CtoI a(1);  /* {∅} */
    CtoI b(ZDD(0));  /* empty */
    EXPECT_EQ(CtoI_Union(a, b), a);
}

TEST_F(CtoITest, IntsecSelf) {
    CtoI a(1);
    EXPECT_EQ(CtoI_Intsec(a, a), a);
}

TEST_F(CtoITest, DiffSelf) {
    CtoI a(1);
    EXPECT_EQ(CtoI_Diff(a, a), CtoI(0));
}

/* ================================================================ */
/*  Null                                                             */
/* ================================================================ */

TEST_F(CtoITest, NullValue) {
    CtoI n = CtoI_Null();
    EXPECT_EQ(n.GetZBDD(), ZDD(-1));
}

/* ================================================================ */
/*  Comparison functions                                             */
/* ================================================================ */

TEST_F(CtoITest, GT_ConstantPositive) {
    CtoI a(5);
    CtoI b(3);
    CtoI r = CtoI_GT(a, b);
    /* For constant comparison, r should be non-zero (a > b is true) */
    EXPECT_NE(r, CtoI(0));
}

TEST_F(CtoITest, GT_ConstantFalse) {
    CtoI a(3);
    CtoI b(5);
    CtoI r = CtoI_GT(a, b);
    EXPECT_EQ(r, CtoI(0));
}

TEST_F(CtoITest, GE_Equal) {
    CtoI a(5);
    CtoI b(5);
    CtoI r = CtoI_GE(a, b);
    /* a >= b is true when equal */
    EXPECT_NE(r, CtoI(0));
}

TEST_F(CtoITest, LT_Basic) {
    CtoI r = CtoI_LT(CtoI(3), CtoI(5));
    EXPECT_NE(r, CtoI(0));
}

TEST_F(CtoITest, EQ_Basic) {
    CtoI r = CtoI_EQ(CtoI(5), CtoI(5));
    EXPECT_NE(r, CtoI(0));
}

TEST_F(CtoITest, NE_Basic) {
    CtoI r = CtoI_NE(CtoI(5), CtoI(3));
    EXPECT_NE(r, CtoI(0));
}

/* ================================================================ */
/*  Constant comparison methods                                      */
/* ================================================================ */

TEST_F(CtoITest, EQ_Const_Basic) {
    CtoI c(5);
    CtoI r = c.EQ_Const(CtoI(5));
    EXPECT_NE(r, CtoI(0));
}

TEST_F(CtoITest, NE_Const_Basic) {
    CtoI c(5);
    CtoI r = c.NE_Const(CtoI(3));
    EXPECT_NE(r, CtoI(0));
}

/* ================================================================ */
/*  Filter operations                                                */
/* ================================================================ */

TEST_F(CtoITest, FilterThenAll) {
    CtoI c(5);
    CtoI cond(1);
    EXPECT_EQ(c.FilterThen(cond).GetInt(), 5);
}

TEST_F(CtoITest, FilterThenNone) {
    CtoI c(5);
    CtoI cond(0);
    EXPECT_EQ(c.FilterThen(cond), CtoI(0));
}

TEST_F(CtoITest, FilterElseAll) {
    CtoI c(5);
    CtoI cond(0);
    /* No filter → keep everything */
    EXPECT_EQ(c.FilterElse(cond).GetInt(), 5);
}

TEST_F(CtoITest, FilterElseNone) {
    CtoI c(5);
    CtoI cond(1);
    /* Everything matches → nothing left */
    EXPECT_EQ(c.FilterElse(cond), CtoI(0));
}

/* ================================================================ */
/*  MaxVal / MinVal                                                  */
/* ================================================================ */

TEST_F(CtoITest, MaxValConstant) {
    CtoI c(7);
    CtoI mx = c.MaxVal();
    EXPECT_EQ(mx.GetInt(), 7);
}

TEST_F(CtoITest, MinValConstant) {
    CtoI c(7);
    CtoI mn = c.MinVal();
    EXPECT_EQ(mn.GetInt(), 7);
}

TEST_F(CtoITest, MinValNegative) {
    CtoI c(-3);
    EXPECT_EQ(c.MinVal().GetInt(), -3);
}

/* ================================================================ */
/*  Abs / Sign                                                       */
/* ================================================================ */

TEST_F(CtoITest, AbsPositive) {
    CtoI c(5);
    EXPECT_EQ(c.Abs().GetInt(), 5);
}

TEST_F(CtoITest, AbsNegative) {
    CtoI c(-5);
    EXPECT_EQ(c.Abs().GetInt(), 5);
}

TEST_F(CtoITest, SignPositive) {
    CtoI c(5);
    EXPECT_EQ(c.Sign().GetInt(), 1);
}

TEST_F(CtoITest, SignNegative) {
    CtoI c(-5);
    EXPECT_EQ(c.Sign().GetInt(), -1);
}

TEST_F(CtoITest, SignZero) {
    CtoI c(0);
    EXPECT_EQ(c.Sign(), CtoI(0));
}

/* ================================================================ */
/*  CtoI_ITE / Max / Min                                             */
/* ================================================================ */

TEST_F(CtoITest, ITE_Basic) {
    CtoI cond(1);  /* non-zero → then branch */
    CtoI r = CtoI_ITE(cond, CtoI(10), CtoI(20));
    EXPECT_EQ(r.GetInt(), 10);
}

TEST_F(CtoITest, ITE_Else) {
    CtoI cond(0);  /* zero → else branch */
    CtoI r = CtoI_ITE(cond, CtoI(10), CtoI(20));
    EXPECT_EQ(r.GetInt(), 20);
}

TEST_F(CtoITest, MaxFunction) {
    CtoI r = CtoI_Max(CtoI(3), CtoI(7));
    EXPECT_EQ(r.GetInt(), 7);
}

TEST_F(CtoITest, MinFunction) {
    CtoI r = CtoI_Min(CtoI(3), CtoI(7));
    EXPECT_EQ(r.GetInt(), 3);
}

/* ================================================================ */
/*  Aggregation                                                      */
/* ================================================================ */

TEST_F(CtoITest, TotalValConstant) {
    CtoI c(5);
    EXPECT_EQ(c.TotalVal().GetInt(), 5);
}

TEST_F(CtoITest, CountTermsConstant) {
    CtoI c(5);
    /* Constant function: 1 combination (empty set) has value 5 */
    EXPECT_EQ(c.CountTerms().GetInt(), 1);
}

TEST_F(CtoITest, CountTermsZero) {
    CtoI c(0);
    /* No combinations */
    EXPECT_EQ(c.CountTerms().GetInt(), 0);
}

/* ================================================================ */
/*  ConstTerm / Support                                              */
/* ================================================================ */

TEST_F(CtoITest, ConstTermOfConstant) {
    CtoI c(5);
    EXPECT_EQ(c.ConstTerm().GetInt(), 5);
}

TEST_F(CtoITest, SupportOfConstant) {
    CtoI c(5);
    /* Constant: support is {∅} (empty set present) or just the value */
    /* Support on boolean converts via NonZero first */
    CtoI s = c.Support();
    EXPECT_NE(s, CtoI_Null());
}

/* ================================================================ */
/*  GetInt                                                           */
/* ================================================================ */

TEST_F(CtoITest, GetIntVarious) {
    for (int i = -10; i <= 10; i++) {
        EXPECT_EQ(CtoI(i).GetInt(), i) << "Failed for i=" << i;
    }
}

TEST_F(CtoITest, GetIntLarger) {
    EXPECT_EQ(CtoI(100).GetInt(), 100);
    EXPECT_EQ(CtoI(255).GetInt(), 255);
    EXPECT_EQ(CtoI(1000).GetInt(), 1000);
}

/* ================================================================ */
/*  StrNum10 / StrNum16                                              */
/* ================================================================ */

TEST_F(CtoITest, StrNum10Basic) {
    char buf[64];
    CtoI c(42);
    int err = c.StrNum10(buf);
    EXPECT_EQ(err, 0);
    EXPECT_STREQ(buf, "42");
}

TEST_F(CtoITest, StrNum10Zero) {
    char buf[64];
    CtoI c(0);
    /* For CtoI(0), it's empty function, StrNum10 should work */
    /* IsConst is true, TopDigit is 0, GetInt returns 0 */
    int err = c.StrNum10(buf);
    EXPECT_EQ(err, 0);
    EXPECT_STREQ(buf, "0");
}

TEST_F(CtoITest, StrNum16Basic) {
    char buf[64];
    CtoI c(255);
    int err = c.StrNum16(buf);
    EXPECT_EQ(err, 0);
    EXPECT_STREQ(buf, "ff");
}

/* ================================================================ */
/*  PutForm                                                          */
/* ================================================================ */

TEST_F(CtoITest, PutFormConstant) {
    CtoI c(5);
    testing::internal::CaptureStdout();
    int err = c.PutForm();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(err, 0);
    EXPECT_FALSE(output.empty());
}

TEST_F(CtoITest, PutFormZero) {
    CtoI c(0);
    testing::internal::CaptureStdout();
    int err = c.PutForm();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_EQ(err, 0);
    EXPECT_NE(output.find("0"), std::string::npos);
}

/* ================================================================ */
/*  CtoI_atoi                                                        */
/* ================================================================ */

TEST_F(CtoITest, AtoiDecimal) {
    CtoI c = CtoI_atoi("42");
    EXPECT_EQ(c.GetInt(), 42);
}

TEST_F(CtoITest, AtoiHex) {
    CtoI c = CtoI_atoi("0xff");
    EXPECT_EQ(c.GetInt(), 255);
}

TEST_F(CtoITest, AtoiBinary) {
    CtoI c = CtoI_atoi("0b1010");
    EXPECT_EQ(c.GetInt(), 10);
}

TEST_F(CtoITest, AtoiNegative) {
    CtoI c = CtoI_atoi("-5");
    EXPECT_EQ(c.GetInt(), -5);
}

TEST_F(CtoITest, AtoiEmpty) {
    CtoI c = CtoI_atoi("");
    EXPECT_EQ(c, CtoI(0));
}

/* ================================================================ */
/*  Digit / ShiftDigit                                               */
/* ================================================================ */

TEST_F(CtoITest, ShiftDigitUp) {
    /* ShiftDigit(1) uses TimesSysVar(20) which multiplies by -2 */
    CtoI c(1);
    CtoI shifted = c.ShiftDigit(1);
    EXPECT_EQ(shifted.GetInt(), -2);
}

TEST_F(CtoITest, ShiftDigitUp2) {
    /* ShiftDigit(2) uses TimesSysVar(19) which multiplies by 4 */
    CtoI c(1);
    CtoI shifted = c.ShiftDigit(2);
    EXPECT_EQ(shifted.GetInt(), 4);
}

TEST_F(CtoITest, ShiftDigitDown) {
    CtoI c(4);
    CtoI shifted = c.ShiftDigit(-2);
    EXPECT_EQ(shifted.GetInt(), 1);
}

TEST_F(CtoITest, ShiftDigitAdditionConsistency) {
    /* Addition uses ShiftDigit internally: 1+1 = 2 */
    CtoI r = CtoI(1) + CtoI(1);
    EXPECT_EQ(r.GetInt(), 2);
}

TEST_F(CtoITest, DigiBasic) {
    /* CtoI(1) is boolean, Digit(0) should return it */
    CtoI c(1);
    CtoI d = c.Digit(0);
    EXPECT_EQ(d, CtoI(1));
}

/* ================================================================ */
/*  TimesSysVar / DivBySysVar                                        */
/* ================================================================ */

TEST_F(CtoITest, TimesSysVarSign) {
    CtoI c(1);
    CtoI r = c.TimesSysVar(BDDV_SysVarTop);
    /* TimesSysVar(20) multiplies by -2 */
    EXPECT_EQ(r.GetInt(), -2);
}

TEST_F(CtoITest, TimesSysVar19) {
    CtoI c(1);
    CtoI r = c.TimesSysVar(BDDV_SysVarTop - 1);
    /* TimesSysVar(19) multiplies by 4 */
    EXPECT_EQ(r.GetInt(), 4);
}

TEST_F(CtoITest, DivBySysVarSign) {
    CtoI c(1);
    CtoI times = c.TimesSysVar(BDDV_SysVarTop);
    CtoI back = times.DivBySysVar(BDDV_SysVarTop);
    EXPECT_EQ(back.GetInt(), 1);
}

/* ================================================================ */
/*  NonZero                                                          */
/* ================================================================ */

TEST_F(CtoITest, NonZeroOfZero) {
    EXPECT_EQ(CtoI(0).NonZero(), CtoI(0));
}

TEST_F(CtoITest, NonZeroOfOne) {
    CtoI nz = CtoI(1).NonZero();
    EXPECT_EQ(nz, CtoI(1));
}

TEST_F(CtoITest, NonZeroOfConstant) {
    CtoI nz = CtoI(5).NonZero();
    /* Should return the boolean part indicating non-zero */
    EXPECT_NE(nz, CtoI(0));
}

/* ================================================================ */
/*  Meet                                                             */
/* ================================================================ */

TEST_F(CtoITest, MeetBothOne) {
    CtoI r = CtoI_Meet(CtoI(1), CtoI(1));
    EXPECT_EQ(r.GetInt(), 1);
}

TEST_F(CtoITest, MeetWithZero) {
    CtoI r = CtoI_Meet(CtoI(5), CtoI(0));
    EXPECT_EQ(r, CtoI(0));
}

/* ================================================================ */
/*  With item variables (non-constant CtoI)                          */
/* ================================================================ */

TEST_F(CtoITest, ItemVariableBasic) {
    /* Use actual user/item variables (levels above BDDV_SysVarTop) */
    bddvar user_var = BDD_VarOfLev(BDDV_SysVarTop + 1);

    /* Create a CtoI: {x} → 1, using ZBDD singleton */
    ZDD single = ZDD::singleton(user_var);
    CtoI c(single);

    EXPECT_EQ(c.IsBool(), 1);  /* boolean: only items, values 0/1 */
    EXPECT_EQ(c.IsConst(), 0); /* not constant: depends on item variable */
    EXPECT_EQ(c.TopItem(), static_cast<int>(user_var));
}

TEST_F(CtoITest, ItemVariableAddition) {
    bddvar x = BDD_VarOfLev(BDDV_SysVarTop + 1);
    bddvar y = BDD_VarOfLev(BDDV_SysVarTop + 2);

    /* CtoI where {x} has value 1 */
    CtoI cx(ZDD::singleton(x));
    /* CtoI where {y} has value 1 */
    CtoI cy(ZDD::singleton(y));

    /* Union: {x}→1, {y}→1 */
    CtoI combined = CtoI_Union(cx, cy);
    EXPECT_NE(combined, CtoI(0));

    /* Count terms should be 2 (two combinations with non-zero value) */
    EXPECT_EQ(combined.CountTerms().GetInt(), 2);
}

TEST_F(CtoITest, FilterThenWithItems) {
    bddvar x = BDD_VarOfLev(BDDV_SysVarTop + 1);
    bddvar y = BDD_VarOfLev(BDDV_SysVarTop + 2);

    /* Build: {x}→1, {y}→1, {x,y}→1 */
    ZDD z = ZDD::singleton(x) + ZDD::singleton(y)
            + ZDD::single_set({x, y});
    CtoI c(z);

    /* Filter: keep only combinations containing x */
    CtoI filter(ZDD::singleton(x) + ZDD::single_set({x, y}));
    CtoI result = c.FilterThen(filter);
    EXPECT_NE(result, CtoI(0));
}

TEST_F(CtoITest, TotalValWithItems) {
    bddvar x = BDD_VarOfLev(BDDV_SysVarTop + 1);
    bddvar y = BDD_VarOfLev(BDDV_SysVarTop + 2);

    /* {x}→1, {y}→1 → total = 2 */
    CtoI c = CtoI_Union(CtoI(ZDD::singleton(x)), CtoI(ZDD::singleton(y)));
    EXPECT_EQ(c.TotalVal().GetInt(), 2);
}

/* ================================================================ */
/*  FreqPat (basic smoke tests)                                      */
/* ================================================================ */

TEST_F(CtoITest, FreqPatAConstant) {
    CtoI c(5);
    CtoI r = c.FreqPatA(3);
    EXPECT_NE(r, CtoI(0));  /* 5 >= 3, should find something */
}

TEST_F(CtoITest, FreqPatAConstantBelow) {
    CtoI c(2);
    CtoI r = c.FreqPatA(5);
    EXPECT_EQ(r, CtoI(0));  /* 2 < 5, should find nothing */
}

TEST_F(CtoITest, FreqPatAVConstant) {
    CtoI c(5);
    CtoI r = c.FreqPatAV(3);
    EXPECT_EQ(r.GetInt(), 5);  /* Returns value itself */
}

TEST_F(CtoITest, FreqPatMConstant) {
    CtoI c(5);
    CtoI r = c.FreqPatM(3);
    EXPECT_NE(r, CtoI(0));
}

TEST_F(CtoITest, FreqPatCConstant) {
    CtoI c(5);
    CtoI r = c.FreqPatC(3);
    EXPECT_NE(r, CtoI(0));
}

/* ================================================================ */
/*  ReduceItems (basic)                                              */
/* ================================================================ */

TEST_F(CtoITest, ReduceItemsSelf) {
    CtoI c(5);
    CtoI r = c.ReduceItems(CtoI(1));
    EXPECT_NE(r, CtoI_Null());
}

/* ================================================================ */
/*  Arithmetic consistency checks                                    */
/* ================================================================ */

TEST_F(CtoITest, ArithmeticConsistency) {
    /* (a + b) - b == a */
    for (int a = -5; a <= 5; a++) {
        for (int b = -5; b <= 5; b++) {
            CtoI ca(a);
            CtoI cb(b);
            CtoI sum = ca + cb;
            CtoI diff = sum - cb;
            EXPECT_EQ(diff.GetInt(), a)
                << "Failed for a=" << a << ", b=" << b;
        }
    }
}

TEST_F(CtoITest, MultiplicationConsistency) {
    for (int a = -3; a <= 3; a++) {
        for (int b = -3; b <= 3; b++) {
            CtoI ca(a);
            CtoI cb(b);
            CtoI prod = ca * cb;
            EXPECT_EQ(prod.GetInt(), a * b)
                << "Failed for a=" << a << ", b=" << b;
        }
    }
}

/* ================================================================ */
/*  MaxVal / MinVal with null                                        */
/* ================================================================ */

TEST_F(CtoITest, MaxValNull) {
    CtoI n = CtoI_Null();
    CtoI mx = n.MaxVal();
    EXPECT_EQ(mx.GetZBDD().GetID(), bddnull);
}

TEST_F(CtoITest, MinValNull) {
    CtoI n = CtoI_Null();
    CtoI mn = n.MinVal();
    EXPECT_EQ(mn.GetZBDD().GetID(), bddnull);
}
