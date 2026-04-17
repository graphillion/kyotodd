#include <gtest/gtest.h>
#include "btoi.h"
#include <cstring>
#include <climits>

using namespace kyotodd;

class BtoITest : public ::testing::Test {
protected:
    static bool initialized_;
    static void SetUpTestSuite() {
        if (!initialized_) {
            BDDV_Init();
            initialized_ = true;
        }
    }
};
bool BtoITest::initialized_ = false;

/* ================================================================ */
/*  Constructors                                                     */
/* ================================================================ */

TEST_F(BtoITest, DefaultConstructor) {
    BtoI a;
    EXPECT_EQ(a.GetInt(), 0);
    EXPECT_EQ(a.Len(), 1);
}

TEST_F(BtoITest, IntConstructorPositive) {
    EXPECT_EQ(BtoI(0).GetInt(), 0);
    EXPECT_EQ(BtoI(1).GetInt(), 1);
    EXPECT_EQ(BtoI(5).GetInt(), 5);
    EXPECT_EQ(BtoI(42).GetInt(), 42);
    EXPECT_EQ(BtoI(127).GetInt(), 127);
    EXPECT_EQ(BtoI(255).GetInt(), 255);
    EXPECT_EQ(BtoI(1000).GetInt(), 1000);
}

TEST_F(BtoITest, IntConstructorNegative) {
    EXPECT_EQ(BtoI(-1).GetInt(), -1);
    EXPECT_EQ(BtoI(-5).GetInt(), -5);
    EXPECT_EQ(BtoI(-128).GetInt(), -128);
    EXPECT_EQ(BtoI(-1000).GetInt(), -1000);
}

TEST_F(BtoITest, CopyConstructor) {
    BtoI a(42);
    BtoI b(a);
    EXPECT_EQ(a == b, 1);
    EXPECT_EQ(b.GetInt(), 42);
}

TEST_F(BtoITest, BDDConstructorZero) {
    BtoI a(BDD(0));
    EXPECT_EQ(a.GetInt(), 0);
}

TEST_F(BtoITest, BDDConstructorOne) {
    BtoI a(BDD(1));
    EXPECT_EQ(a.GetInt(), 1);
}

TEST_F(BtoITest, BDDVConstructor) {
    BtoI a(5);
    BtoI b(a.GetMetaBDDV());
    EXPECT_EQ(a == b, 1);
}

TEST_F(BtoITest, Assignment) {
    BtoI a(42);
    BtoI b;
    b = a;
    EXPECT_EQ(b.GetInt(), 42);
}

/* ================================================================ */
/*  Addition                                                         */
/* ================================================================ */

TEST_F(BtoITest, AddZero) {
    EXPECT_EQ((BtoI(5) + BtoI(0)).GetInt(), 5);
    EXPECT_EQ((BtoI(0) + BtoI(5)).GetInt(), 5);
}

TEST_F(BtoITest, AddPositive) {
    EXPECT_EQ((BtoI(3) + BtoI(4)).GetInt(), 7);
    EXPECT_EQ((BtoI(100) + BtoI(200)).GetInt(), 300);
}

TEST_F(BtoITest, AddNegative) {
    EXPECT_EQ((BtoI(-3) + BtoI(-4)).GetInt(), -7);
    EXPECT_EQ((BtoI(5) + BtoI(-3)).GetInt(), 2);
    EXPECT_EQ((BtoI(-5) + BtoI(3)).GetInt(), -2);
}

TEST_F(BtoITest, AddCompound) {
    BtoI a(10);
    a += BtoI(5);
    EXPECT_EQ(a.GetInt(), 15);
}

/* ================================================================ */
/*  Subtraction                                                      */
/* ================================================================ */

TEST_F(BtoITest, SubZero) {
    EXPECT_EQ((BtoI(5) - BtoI(0)).GetInt(), 5);
}

TEST_F(BtoITest, SubSelf) {
    EXPECT_EQ((BtoI(42) - BtoI(42)).GetInt(), 0);
}

TEST_F(BtoITest, SubPositive) {
    EXPECT_EQ((BtoI(7) - BtoI(3)).GetInt(), 4);
    EXPECT_EQ((BtoI(3) - BtoI(7)).GetInt(), -4);
}

TEST_F(BtoITest, SubNegative) {
    EXPECT_EQ((BtoI(-3) - BtoI(-7)).GetInt(), 4);
}

TEST_F(BtoITest, SubCompound) {
    BtoI a(10);
    a -= BtoI(3);
    EXPECT_EQ(a.GetInt(), 7);
}

/* ================================================================ */
/*  Unary operators                                                  */
/* ================================================================ */

TEST_F(BtoITest, UnaryNegate) {
    EXPECT_EQ((-BtoI(5)).GetInt(), -5);
    EXPECT_EQ((-BtoI(-5)).GetInt(), 5);
    EXPECT_EQ((-BtoI(0)).GetInt(), 0);
}

TEST_F(BtoITest, UnaryBitwiseNot) {
    // ~x = -(x+1) in two's complement
    EXPECT_EQ((~BtoI(0)).GetInt(), -1);
    EXPECT_EQ((~BtoI(5)).GetInt(), -6);
    EXPECT_EQ((~BtoI(-1)).GetInt(), 0);
}

TEST_F(BtoITest, UnaryLogicalNot) {
    EXPECT_EQ((!BtoI(0)).GetInt(), 1);
    EXPECT_EQ((!BtoI(5)).GetInt(), 0);
    EXPECT_EQ((!BtoI(-1)).GetInt(), 0);
}

/* ================================================================ */
/*  Multiplication                                                   */
/* ================================================================ */

TEST_F(BtoITest, MultByZero) {
    EXPECT_EQ((BtoI(5) * BtoI(0)).GetInt(), 0);
    EXPECT_EQ((BtoI(0) * BtoI(5)).GetInt(), 0);
}

TEST_F(BtoITest, MultByOne) {
    EXPECT_EQ((BtoI(5) * BtoI(1)).GetInt(), 5);
    EXPECT_EQ((BtoI(1) * BtoI(5)).GetInt(), 5);
}

TEST_F(BtoITest, MultPositive) {
    EXPECT_EQ((BtoI(3) * BtoI(4)).GetInt(), 12);
    EXPECT_EQ((BtoI(7) * BtoI(7)).GetInt(), 49);
}

TEST_F(BtoITest, MultNegative) {
    EXPECT_EQ((BtoI(-3) * BtoI(4)).GetInt(), -12);
    EXPECT_EQ((BtoI(3) * BtoI(-4)).GetInt(), -12);
    EXPECT_EQ((BtoI(-3) * BtoI(-4)).GetInt(), 12);
}

TEST_F(BtoITest, MultCompound) {
    BtoI a(6);
    a *= BtoI(7);
    EXPECT_EQ(a.GetInt(), 42);
}

/* ================================================================ */
/*  Division                                                         */
/* ================================================================ */

TEST_F(BtoITest, DivByOne) {
    EXPECT_EQ((BtoI(42) / BtoI(1)).GetInt(), 42);
}

TEST_F(BtoITest, DivSelf) {
    EXPECT_EQ((BtoI(42) / BtoI(42)).GetInt(), 1);
}

TEST_F(BtoITest, DivPositive) {
    EXPECT_EQ((BtoI(12) / BtoI(3)).GetInt(), 4);
    EXPECT_EQ((BtoI(13) / BtoI(3)).GetInt(), 4);
}

TEST_F(BtoITest, DivNegative) {
    EXPECT_EQ((BtoI(-12) / BtoI(3)).GetInt(), -4);
    EXPECT_EQ((BtoI(12) / BtoI(-3)).GetInt(), -4);
    EXPECT_EQ((BtoI(-12) / BtoI(-3)).GetInt(), 4);
}

TEST_F(BtoITest, DivByZeroThrows) {
    EXPECT_THROW(BtoI(5) / BtoI(0), std::invalid_argument);
}

/* ================================================================ */
/*  Remainder                                                        */
/* ================================================================ */

TEST_F(BtoITest, Remainder) {
    EXPECT_EQ((BtoI(13) % BtoI(3)).GetInt(), 1);
    EXPECT_EQ((BtoI(12) % BtoI(3)).GetInt(), 0);
}

/* ================================================================ */
/*  Bitwise operations                                               */
/* ================================================================ */

TEST_F(BtoITest, BitwiseAnd) {
    EXPECT_EQ((BtoI(0b1100) & BtoI(0b1010)).GetInt(), 0b1000);
}

TEST_F(BtoITest, BitwiseOr) {
    EXPECT_EQ((BtoI(0b1100) | BtoI(0b1010)).GetInt(), 0b1110);
}

TEST_F(BtoITest, BitwiseXor) {
    EXPECT_EQ((BtoI(0b1100) ^ BtoI(0b1010)).GetInt(), 0b0110);
}

TEST_F(BtoITest, BitwiseCompound) {
    BtoI a(0b1100);
    a &= BtoI(0b1010);
    EXPECT_EQ(a.GetInt(), 0b1000);

    BtoI b(0b1100);
    b |= BtoI(0b0011);
    EXPECT_EQ(b.GetInt(), 0b1111);

    BtoI c(0b1100);
    c ^= BtoI(0b1010);
    EXPECT_EQ(c.GetInt(), 0b0110);
}

/* ================================================================ */
/*  Shift operators                                                  */
/* ================================================================ */

TEST_F(BtoITest, LeftShiftConst) {
    EXPECT_EQ((BtoI(1) << BtoI(3)).GetInt(), 8);
    EXPECT_EQ((BtoI(5) << BtoI(2)).GetInt(), 20);
}

TEST_F(BtoITest, RightShiftConst) {
    EXPECT_EQ((BtoI(8) >> BtoI(3)).GetInt(), 1);
    EXPECT_EQ((BtoI(20) >> BtoI(2)).GetInt(), 5);
}

TEST_F(BtoITest, ArithRightShiftNeg) {
    // Arithmetic right shift preserves sign
    EXPECT_EQ((BtoI(-8) >> BtoI(1)).GetInt(), -4);
}

TEST_F(BtoITest, ShiftCompound) {
    BtoI a(1);
    a <<= BtoI(4);
    EXPECT_EQ(a.GetInt(), 16);
    a >>= BtoI(2);
    EXPECT_EQ(a.GetInt(), 4);
}

/* ================================================================ */
/*  Equality / comparison                                            */
/* ================================================================ */

TEST_F(BtoITest, Equality) {
    EXPECT_EQ(BtoI(5) == BtoI(5), 1);
    EXPECT_EQ(BtoI(5) != BtoI(3), 1);
    EXPECT_EQ(BtoI(5) == BtoI(3), 0);
}

TEST_F(BtoITest, EqualityNonCanonical) {
    // BtoI(0) uses a length-1 BDDV, while BDDV(BDD(0), 2)
    // creates a length-2 BDDV. Both represent integer 0.
    BtoI a(0);
    BtoI b(BDDV(BDD(0), 2));
    EXPECT_EQ(a == b, 1);
    EXPECT_EQ(a != b, 0);
}

TEST_F(BtoITest, BtoI_EQ_Constants) {
    EXPECT_EQ(BtoI_EQ(BtoI(5), BtoI(5)).GetInt(), 1);
    EXPECT_EQ(BtoI_EQ(BtoI(5), BtoI(3)).GetInt(), 0);
}

TEST_F(BtoITest, BtoI_GT) {
    EXPECT_EQ(BtoI_GT(BtoI(5), BtoI(3)).GetInt(), 1);
    EXPECT_EQ(BtoI_GT(BtoI(3), BtoI(5)).GetInt(), 0);
    EXPECT_EQ(BtoI_GT(BtoI(5), BtoI(5)).GetInt(), 0);
}

TEST_F(BtoITest, BtoI_LT) {
    EXPECT_EQ(BtoI_LT(BtoI(3), BtoI(5)).GetInt(), 1);
    EXPECT_EQ(BtoI_LT(BtoI(5), BtoI(3)).GetInt(), 0);
}

TEST_F(BtoITest, BtoI_GE) {
    EXPECT_EQ(BtoI_GE(BtoI(5), BtoI(3)).GetInt(), 1);
    EXPECT_EQ(BtoI_GE(BtoI(5), BtoI(5)).GetInt(), 1);
    EXPECT_EQ(BtoI_GE(BtoI(3), BtoI(5)).GetInt(), 0);
}

TEST_F(BtoITest, BtoI_LE) {
    EXPECT_EQ(BtoI_LE(BtoI(3), BtoI(5)).GetInt(), 1);
    EXPECT_EQ(BtoI_LE(BtoI(5), BtoI(5)).GetInt(), 1);
    EXPECT_EQ(BtoI_LE(BtoI(5), BtoI(3)).GetInt(), 0);
}

TEST_F(BtoITest, BtoI_NE) {
    EXPECT_EQ(BtoI_NE(BtoI(5), BtoI(3)).GetInt(), 1);
    EXPECT_EQ(BtoI_NE(BtoI(5), BtoI(5)).GetInt(), 0);
}

/* ================================================================ */
/*  BtoI_ITE                                                         */
/* ================================================================ */

TEST_F(BtoITest, ITE_BDDCondTrue) {
    BtoI result = BtoI_ITE(BDD(1), BtoI(10), BtoI(20));
    EXPECT_EQ(result.GetInt(), 10);
}

TEST_F(BtoITest, ITE_BDDCondFalse) {
    BtoI result = BtoI_ITE(BDD(0), BtoI(10), BtoI(20));
    EXPECT_EQ(result.GetInt(), 20);
}

TEST_F(BtoITest, ITE_BtoICondNonZero) {
    BtoI result = BtoI_ITE(BtoI(1), BtoI(10), BtoI(20));
    EXPECT_EQ(result.GetInt(), 10);
}

TEST_F(BtoITest, ITE_BtoICondZero) {
    BtoI result = BtoI_ITE(BtoI(0), BtoI(10), BtoI(20));
    EXPECT_EQ(result.GetInt(), 20);
}

/* ================================================================ */
/*  Access functions                                                 */
/* ================================================================ */

TEST_F(BtoITest, GetSignBDD) {
    EXPECT_EQ(BtoI(5).GetSignBDD().GetID(), bddempty);   // non-negative
    EXPECT_EQ(BtoI(-5).GetSignBDD().GetID(), bddsingle);  // negative (BDD(1))
    EXPECT_EQ(BtoI(0).GetSignBDD().GetID(), bddempty);    // zero
}

TEST_F(BtoITest, Len) {
    EXPECT_EQ(BtoI(0).Len(), 1);
    EXPECT_GE(BtoI(5).Len(), 2);
    EXPECT_GE(BtoI(-1).Len(), 1);
}

TEST_F(BtoITest, GetBDDSignExtension) {
    BtoI a(5);  // positive: sign bit is 0
    // Accessing beyond Len() should return sign bit (0)
    EXPECT_EQ(a.GetBDD(100).GetID(), bddempty);
}

TEST_F(BtoITest, GetMetaBDDV) {
    BtoI a(42);
    BDDV v = a.GetMetaBDDV();
    EXPECT_EQ(v.Len(), a.Len());
}

TEST_F(BtoITest, Top) {
    EXPECT_EQ(BtoI(5).Top(), 0);  // constant: no variable dependency
}

TEST_F(BtoITest, Size) {
    EXPECT_GE(BtoI(5).Size(), 0u);
}

/* ================================================================ */
/*  Cofactor                                                         */
/* ================================================================ */

TEST_F(BtoITest, At0At1Constants) {
    // For a constant BtoI, cofactoring doesn't change value
    // Need a user variable first
    int v = static_cast<int>(BDDV_NewVar());
    BtoI a(5);
    EXPECT_EQ(a.At0(v).GetInt(), 5);
    EXPECT_EQ(a.At1(v).GetInt(), 5);
}

TEST_F(BtoITest, At0At1Variable) {
    int v = static_cast<int>(BDDV_NewVar());
    BDD x = BDD::prime(static_cast<bddvar>(v));
    // BtoI from BDD variable: 0 when x=0, 1 when x=1
    BtoI a(x);
    EXPECT_EQ(a.At0(v).GetInt(), 0);
    EXPECT_EQ(a.At1(v).GetInt(), 1);
}

/* ================================================================ */
/*  UpperBound / LowerBound                                          */
/* ================================================================ */

TEST_F(BtoITest, BoundsConstant) {
    BtoI a(42);
    EXPECT_EQ(a.UpperBound().GetInt(), 42);
    EXPECT_EQ(a.LowerBound().GetInt(), 42);
}

TEST_F(BtoITest, BoundsVariable) {
    int v = static_cast<int>(BDDV_NewVar());
    BDD x = BDD::prime(static_cast<bddvar>(v));
    BtoI a(x);  // 0 or 1
    EXPECT_EQ(a.UpperBound().GetInt(), 1);
    EXPECT_EQ(a.LowerBound().GetInt(), 0);
}

/* ================================================================ */
/*  Conversion: StrNum10, StrNum16                                   */
/* ================================================================ */

TEST_F(BtoITest, StrNum10Positive) {
    char buf[64];
    BtoI(42).StrNum10(buf);
    EXPECT_STREQ(buf, "42");
}

TEST_F(BtoITest, StrNum10Zero) {
    char buf[64];
    BtoI(0).StrNum10(buf);
    EXPECT_STREQ(buf, "0");
}

TEST_F(BtoITest, StrNum10Negative) {
    char buf[64];
    BtoI(-42).StrNum10(buf);
    EXPECT_STREQ(buf, "-42");
}

TEST_F(BtoITest, StrNum16Positive) {
    char buf[64];
    BtoI(255).StrNum16(buf);
    EXPECT_STREQ(buf, "ff");
}

TEST_F(BtoITest, StrNum16Zero) {
    char buf[64];
    BtoI(0).StrNum16(buf);
    EXPECT_STREQ(buf, "0");
}

/* ================================================================ */
/*  BtoI_atoi                                                        */
/* ================================================================ */

TEST_F(BtoITest, AtoiDecimal) {
    EXPECT_EQ(BtoI_atoi("42").GetInt(), 42);
    EXPECT_EQ(BtoI_atoi("-42").GetInt(), -42);
    EXPECT_EQ(BtoI_atoi("0").GetInt(), 0);
}

TEST_F(BtoITest, AtoiHex) {
    EXPECT_EQ(BtoI_atoi("0xff").GetInt(), 255);
    EXPECT_EQ(BtoI_atoi("0x1A").GetInt(), 26);
}

TEST_F(BtoITest, AtoiBinary) {
    EXPECT_EQ(BtoI_atoi("0b1010").GetInt(), 10);
    EXPECT_EQ(BtoI_atoi("0b1111").GetInt(), 15);
}

/* ================================================================ */
/*  BDD-dependent BtoI                                               */
/* ================================================================ */

TEST_F(BtoITest, BDDDependentAddition) {
    int v = static_cast<int>(BDDV_NewVar());
    BDD x = BDD::prime(static_cast<bddvar>(v));
    BtoI a(x);      // 0 or 1 depending on x
    BtoI b(3);
    BtoI sum = a + b;  // 3 or 4

    EXPECT_EQ(sum.At0(v).GetInt(), 3);
    EXPECT_EQ(sum.At1(v).GetInt(), 4);
}

TEST_F(BtoITest, BDDDependentITE) {
    int v = static_cast<int>(BDDV_NewVar());
    BDD x = BDD::prime(static_cast<bddvar>(v));
    BtoI result = BtoI_ITE(x, BtoI(10), BtoI(20));

    EXPECT_EQ(result.At0(v).GetInt(), 20);
    EXPECT_EQ(result.At1(v).GetInt(), 10);
}

TEST_F(BtoITest, BDDDependentComparison) {
    int v = static_cast<int>(BDDV_NewVar());
    BDD x = BDD::prime(static_cast<bddvar>(v));
    BtoI a(x);  // 0 or 1
    BtoI gt = BtoI_GT(a, BtoI(0));

    // When x=0: a=0, not > 0 → 0
    EXPECT_EQ(gt.At0(v).GetInt(), 0);
    // When x=1: a=1, > 0 → 1
    EXPECT_EQ(gt.At1(v).GetInt(), 1);
}

/* ================================================================ */
/*  Print (no crash)                                                 */
/* ================================================================ */

TEST_F(BtoITest, PrintNoCrash) {
    testing::internal::CaptureStdout();
    BtoI(42).Print();
    fflush(stdout);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(output.empty());
}

/* ================================================================ */
/*  Spread                                                           */
/* ================================================================ */

TEST_F(BtoITest, SpreadConstant) {
    // Spread on a constant doesn't change value
    BtoI a(5);
    BtoI s = a.Spread(1);
    EXPECT_EQ(s.GetInt(), 5);
}

/* ================================================================ */
/*  Overflow                                                         */
/* ================================================================ */

TEST_F(BtoITest, OverflowBDD) {
    BtoI a(BDD(-1));
    EXPECT_EQ(a.GetMetaBDDV().GetBDD(0).GetID(), bddnull);
}

TEST_F(BtoITest, IntMinThrows) {
    int val = INT_MIN;
    EXPECT_THROW({ BtoI tmp(val); (void)tmp; }, std::overflow_error);
}

/* ================================================================ */
/*  Constrained UpperBound / LowerBound                              */
/* ================================================================ */

TEST_F(BtoITest, BoundsConstrainedSingleValue) {
    int v = static_cast<int>(BDDV_NewVar());
    BDD x = BDD::prime(static_cast<bddvar>(v));
    BtoI a = BtoI_ITE(x, BtoI(10), BtoI(-3));

    EXPECT_EQ(a.UpperBound().GetInt(), 10);
    EXPECT_EQ(a.LowerBound().GetInt(), -3);

    /* Under x, only value is 10 */
    EXPECT_EQ(a.UpperBound(x).GetInt(), 10);
    EXPECT_EQ(a.LowerBound(x).GetInt(), 10);

    /* Under ~x, only value is -3 */
    EXPECT_EQ(a.UpperBound(~x).GetInt(), -3);
    EXPECT_EQ(a.LowerBound(~x).GetInt(), -3);
}

TEST_F(BtoITest, BoundsConstrainedAllNegative) {
    int v = static_cast<int>(BDDV_NewVar());
    BDD x = BDD::prime(static_cast<bddvar>(v));
    BtoI a = BtoI_ITE(x, BtoI(-3), BtoI(-7));

    EXPECT_EQ(a.UpperBound().GetInt(), -3);
    EXPECT_EQ(a.LowerBound().GetInt(), -7);

    EXPECT_EQ(a.UpperBound(x).GetInt(), -3);
    EXPECT_EQ(a.LowerBound(x).GetInt(), -3);
    EXPECT_EQ(a.UpperBound(~x).GetInt(), -7);
    EXPECT_EQ(a.LowerBound(~x).GetInt(), -7);
}

/* ================================================================ */
/*  GetInt / StrNum10 / StrNum16 non-constant rejection              */
/* ================================================================ */

TEST_F(BtoITest, GetIntNonConstantThrows) {
    int v = static_cast<int>(BDDV_NewVar());
    BDD x = BDD::prime(static_cast<bddvar>(v));
    BtoI bi(x);
    EXPECT_THROW(bi.GetInt(), std::invalid_argument);
}

TEST_F(BtoITest, StrNum10NonConstantThrows) {
    int v = static_cast<int>(BDDV_NewVar());
    BDD x = BDD::prime(static_cast<bddvar>(v));
    BtoI bi(x);
    char buf[256];
    EXPECT_THROW(bi.StrNum10(buf), std::invalid_argument);
}

TEST_F(BtoITest, StrNum16NonConstantThrows) {
    int v = static_cast<int>(BDDV_NewVar());
    BDD x = BDD::prime(static_cast<bddvar>(v));
    BtoI bi(x);
    char buf[256];
    EXPECT_THROW(bi.StrNum16(buf), std::invalid_argument);
}

/* ================================================================ */
/*  BtoI_atoi invalid string rejection                               */
/* ================================================================ */

TEST_F(BtoITest, AtoiInvalidBinaryThrows) {
    EXPECT_THROW(BtoI_atoi("0b102"), std::invalid_argument);
}

TEST_F(BtoITest, AtoiInvalidDecimalThrows) {
    EXPECT_THROW(BtoI_atoi("12x34"), std::invalid_argument);
}

TEST_F(BtoITest, AtoiInvalidHexThrows) {
    EXPECT_THROW(BtoI_atoi("0x1g"), std::invalid_argument);
}
