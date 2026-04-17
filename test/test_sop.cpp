#include <gtest/gtest.h>
#include "sop.h"
#include <sstream>

using namespace kyotodd;

/* ================================================================ */
/*  Test fixture                                                     */
/* ================================================================ */

class SOPTest : public ::testing::Test {
protected:
    static bool initialized_;

    static void SetUpTestSuite() {
        if (!initialized_) {
            BDDV_Init();
            initialized_ = true;
        }
    }
};

bool SOPTest::initialized_ = false;

/* ================================================================ */
/*  SOP_NewVar / SOP_NewVarOfLev                                     */
/* ================================================================ */

TEST_F(SOPTest, NewVar) {
    int v = SOP_NewVar();
    EXPECT_EQ(v & 1, 0);  // must be even
    EXPECT_GT(v, 0);
}

TEST_F(SOPTest, NewVarAfterOddNonSOPVar) {
    // Creating an odd number of non-SOP variables before SOP_NewVar
    // used to break parity. Now SOP_NewVar pads if needed.
    BDDV_NewVar();  // one extra variable (makes var count odd)
    int v = SOP_NewVar();
    EXPECT_EQ(v & 1, 0);  // must still be even
    EXPECT_GT(v, 0);
    // Must be usable with SOP operations
    EXPECT_NO_THROW(SOP(1).And1(v));
}

TEST_F(SOPTest, NewVarOddLevThrows) {
    EXPECT_THROW(SOP_NewVarOfLev(3), std::invalid_argument);
}

/* ================================================================ */
/*  Constructors                                                     */
/* ================================================================ */

TEST_F(SOPTest, DefaultConstructor) {
    SOP s;
    EXPECT_EQ(s.GetZBDD().GetID(), bddempty);
}

TEST_F(SOPTest, IntConstructorZero) {
    SOP s(0);
    EXPECT_EQ(s.GetZBDD().GetID(), bddempty);
}

TEST_F(SOPTest, IntConstructorOne) {
    SOP s(1);
    EXPECT_EQ(s.GetZBDD().GetID(), bddsingle);
}

TEST_F(SOPTest, CopyConstructor) {
    SOP a(1);
    SOP b(a);
    EXPECT_EQ(a == b, 1);
}

TEST_F(SOPTest, ZBDDConstructor) {
    ZDD z(1);
    SOP s(z);
    EXPECT_EQ(s.GetZBDD().GetID(), bddsingle);
}

TEST_F(SOPTest, Assignment) {
    SOP a(1);
    SOP b;
    b = a;
    EXPECT_EQ(a == b, 1);
}

/* ================================================================ */
/*  Factor operations                                                */
/* ================================================================ */

TEST_F(SOPTest, FactorOddVarThrows) {
    SOP s(1);
    EXPECT_THROW(s.Factor1(3), std::invalid_argument);
    EXPECT_THROW(s.Factor0(3), std::invalid_argument);
    EXPECT_THROW(s.FactorD(3), std::invalid_argument);
}

TEST_F(SOPTest, FactorDecomposition) {
    // Create SOP: x_v (positive literal for variable v)
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);  // single cube with positive literal x_v

    // Factor1(v) should give tautology (cube without x_v)
    SOP f1 = x.Factor1(v);
    EXPECT_EQ(f1 == SOP(1), 1);

    // Factor0(v) should be empty (no cubes with ~x_v)
    SOP f0 = x.Factor0(v);
    EXPECT_EQ(f0 == SOP(0), 1);

    // FactorD(v) should be empty (no cubes without any literal of v)
    SOP fD = x.FactorD(v);
    EXPECT_EQ(fD == SOP(0), 1);
}

TEST_F(SOPTest, FactorDecompositionNegLit) {
    // Create SOP: ~x_v (negative literal for variable v)
    int v = SOP_NewVar();
    SOP nx = SOP(1).And0(v);  // single cube with negative literal

    SOP f1 = nx.Factor1(v);
    EXPECT_EQ(f1 == SOP(0), 1);

    SOP f0 = nx.Factor0(v);
    EXPECT_EQ(f0 == SOP(1), 1);

    SOP fD = nx.FactorD(v);
    EXPECT_EQ(fD == SOP(0), 1);
}

TEST_F(SOPTest, FactorDecompositionDontCare) {
    // Tautology cube: has no literal for any variable
    int v = SOP_NewVar();
    SOP taut(1);

    SOP f1 = taut.Factor1(v);
    EXPECT_EQ(f1 == SOP(0), 1);

    SOP f0 = taut.Factor0(v);
    EXPECT_EQ(f0 == SOP(0), 1);

    SOP fD = taut.FactorD(v);
    EXPECT_EQ(fD == SOP(1), 1);
}

/* ================================================================ */
/*  And operations                                                   */
/* ================================================================ */

TEST_F(SOPTest, And1OddVarThrows) {
    SOP s(1);
    EXPECT_THROW(s.And1(3), std::invalid_argument);
    EXPECT_THROW(s.And0(3), std::invalid_argument);
}

TEST_F(SOPTest, And1CreatesPositiveLiteral) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);

    // Decomposing back should give Factor1 = 1, Factor0 = 0
    EXPECT_EQ(x.Factor1(v) == SOP(1), 1);
    EXPECT_EQ(x.Factor0(v) == SOP(0), 1);
    EXPECT_EQ(x.FactorD(v) == SOP(0), 1);
}

TEST_F(SOPTest, And0CreatesNegativeLiteral) {
    int v = SOP_NewVar();
    SOP nx = SOP(1).And0(v);

    EXPECT_EQ(nx.Factor1(v) == SOP(0), 1);
    EXPECT_EQ(nx.Factor0(v) == SOP(1), 1);
    EXPECT_EQ(nx.FactorD(v) == SOP(0), 1);
}

TEST_F(SOPTest, And1CancelsNegative) {
    // AND x_v with ~x_v should give empty
    int v = SOP_NewVar();
    SOP nx = SOP(1).And0(v);
    SOP result = nx.And1(v);
    EXPECT_EQ(result == SOP(0), 1);
}

/* ================================================================ */
/*  Shift operators                                                  */
/* ================================================================ */

TEST_F(SOPTest, ShiftOddThrows) {
    SOP s(1);
    EXPECT_THROW(s << 1, std::invalid_argument);
    EXPECT_THROW(s >> 1, std::invalid_argument);
}

TEST_F(SOPTest, ShiftZero) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    EXPECT_EQ((x << 0) == x, 1);
    EXPECT_EQ((x >> 0) == x, 1);
}

TEST_F(SOPTest, LeftShift) {
    int v1 = SOP_NewVar();
    int v2 = SOP_NewVar();
    SOP x1 = SOP(1).And1(v1);
    SOP x2 = SOP(1).And1(v2);

    // Shift x1 up by (v2 - v1) levels should give x2
    int shift = static_cast<int>(bddlevofvar(static_cast<bddvar>(v2)))
              - static_cast<int>(bddlevofvar(static_cast<bddvar>(v1)));
    SOP shifted = x1 << shift;
    EXPECT_EQ(shifted == x2, 1);
}

TEST_F(SOPTest, ShiftCompoundAssign) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOP orig = x;
    x <<= 2;
    x >>= 2;
    EXPECT_EQ(x == orig, 1);
}

/* ================================================================ */
/*  Set operations (union, intersect, diff)                          */
/* ================================================================ */

TEST_F(SOPTest, Union) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOP nx = SOP(1).And0(v);
    SOP sum = x + nx;

    // sum should have 2 cubes
    EXPECT_EQ(sum.Cube(), 2u);
}

TEST_F(SOPTest, Intersect) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOP nx = SOP(1).And0(v);
    SOP sum = x + nx;

    EXPECT_EQ((sum & x) == x, 1);
    EXPECT_EQ((x & nx) == SOP(0), 1);
}

TEST_F(SOPTest, Subtract) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOP nx = SOP(1).And0(v);
    SOP sum = x + nx;

    EXPECT_EQ((sum - x) == nx, 1);
}

TEST_F(SOPTest, CompoundAssign) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOP nx = SOP(1).And0(v);
    SOP a = x;
    a += nx;
    EXPECT_EQ(a.Cube(), 2u);

    SOP b = a;
    b &= x;
    EXPECT_EQ(b == x, 1);

    SOP c = a;
    c -= x;
    EXPECT_EQ(c == nx, 1);
}

/* ================================================================ */
/*  Equality                                                         */
/* ================================================================ */

TEST_F(SOPTest, Equality) {
    SOP a(1);
    SOP b(1);
    SOP c(0);
    EXPECT_EQ(a == b, 1);
    EXPECT_EQ(a != c, 1);
    EXPECT_EQ(a == c, 0);
}

/* ================================================================ */
/*  Query methods                                                    */
/* ================================================================ */

TEST_F(SOPTest, TopReturnsEven) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    int top = x.Top();
    EXPECT_EQ(top, v);
    EXPECT_EQ(top & 1, 0);  // must be even
}

TEST_F(SOPTest, TopOfEmpty) {
    SOP s(0);
    EXPECT_EQ(s.Top(), 0);
}

TEST_F(SOPTest, SizeNonzero) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    EXPECT_GT(x.Size(), 0u);
}

TEST_F(SOPTest, CubeCount) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOP nx = SOP(1).And0(v);
    EXPECT_EQ(x.Cube(), 1u);
    EXPECT_EQ((x + nx).Cube(), 2u);
    EXPECT_EQ(SOP(0).Cube(), 0u);
    EXPECT_EQ(SOP(1).Cube(), 1u);
}

TEST_F(SOPTest, LitCount) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    EXPECT_EQ(x.Lit(), 1u);
    EXPECT_EQ(SOP(1).Lit(), 0u);  // tautology has 0 literals
}

TEST_F(SOPTest, GetZBDD) {
    SOP s(1);
    ZDD z = s.GetZBDD();
    EXPECT_EQ(z.GetID(), bddsingle);
}

/* ================================================================ */
/*  Algebraic multiplication                                         */
/* ================================================================ */

TEST_F(SOPTest, MultByZero) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    EXPECT_EQ((x * SOP(0)) == SOP(0), 1);
    EXPECT_EQ((SOP(0) * x) == SOP(0), 1);
}

TEST_F(SOPTest, MultByOne) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    EXPECT_EQ((x * SOP(1)) == x, 1);
    EXPECT_EQ((SOP(1) * x) == x, 1);
}

TEST_F(SOPTest, MultSingleLiterals) {
    // x_a * x_b = single cube with both literals
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);
    SOP prod = xa * xb;

    EXPECT_EQ(prod.Cube(), 1u);
    EXPECT_EQ(prod.Lit(), 2u);

    // Factor decomposition
    EXPECT_EQ(prod.Factor1(a) == xb, 1);
    EXPECT_EQ(prod.Factor1(b) == xa, 1);
}

TEST_F(SOPTest, MultDistributive) {
    // (x_a + x_b) * x_c = x_a*x_c + x_b*x_c
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    int c = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);
    SOP xc = SOP(1).And1(c);

    SOP lhs = (xa + xb) * xc;
    SOP rhs = (xa * xc) + (xb * xc);
    EXPECT_EQ(lhs == rhs, 1);
}

TEST_F(SOPTest, MultCompoundAssign) {
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);
    SOP prod = xa;
    prod *= xb;
    EXPECT_EQ(prod == (xa * xb), 1);
}

/* ================================================================ */
/*  Algebraic division                                               */
/* ================================================================ */

TEST_F(SOPTest, DivByOne) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    EXPECT_EQ((x / SOP(1)) == x, 1);
}

TEST_F(SOPTest, DivSelf) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    EXPECT_EQ((x / x) == SOP(1), 1);
}

TEST_F(SOPTest, DivByZeroThrows) {
    SOP s(1);
    EXPECT_THROW(s / SOP(0), std::invalid_argument);
}

TEST_F(SOPTest, DivBasic) {
    // (x_a * x_b + x_a * x_c) / x_a = x_b + x_c
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    int c = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);
    SOP xc = SOP(1).And1(c);

    SOP f = xa * xb + xa * xc;
    SOP q = f / xa;
    EXPECT_EQ(q == (xb + xc), 1);
}

TEST_F(SOPTest, DivRemainder) {
    // f = x_a*x_b + x_c
    // f / x_a = x_b
    // f % x_a = x_c
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    int c = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);
    SOP xc = SOP(1).And1(c);

    SOP f = xa * xb + xc;
    SOP q = f / xa;
    EXPECT_EQ(q == xb, 1);

    SOP r = f % xa;
    EXPECT_EQ(r == xc, 1);
}

TEST_F(SOPTest, DivModCompoundAssign) {
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);
    SOP f = xa * xb;

    SOP q = f;
    q /= xa;
    EXPECT_EQ(q == xb, 1);

    SOP r = f;
    r %= xa;
    EXPECT_EQ(r == SOP(0), 1);
}

/* ================================================================ */
/*  GetBDD                                                           */
/* ================================================================ */

TEST_F(SOPTest, GetBDDConstants) {
    EXPECT_EQ(SOP(0).GetBDD() == BDD(0), 1);
    EXPECT_EQ(SOP(1).GetBDD() == BDD(1), 1);
}

TEST_F(SOPTest, GetBDDPositiveLiteral) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    BDD bdd_x = x.GetBDD();
    BDD expected = BDD::prime(static_cast<bddvar>(v));
    EXPECT_EQ(bdd_x == expected, 1);
}

TEST_F(SOPTest, GetBDDNegativeLiteral) {
    int v = SOP_NewVar();
    SOP nx = SOP(1).And0(v);
    BDD bdd_nx = nx.GetBDD();
    BDD expected = BDD::prime_not(static_cast<bddvar>(v));
    EXPECT_EQ(bdd_nx == expected, 1);
}

TEST_F(SOPTest, GetBDDMultipleCubes) {
    // x_a + x_b should give BDD for (x_a OR x_b)
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);
    SOP sum = xa + xb;

    BDD result = sum.GetBDD();
    BDD expected = BDD::prime(static_cast<bddvar>(a)) |
                   BDD::prime(static_cast<bddvar>(b));
    EXPECT_EQ(result == expected, 1);
}

/* ================================================================ */
/*  IsPolyCube / IsPolyLit                                           */
/* ================================================================ */

TEST_F(SOPTest, IsPolyCube) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOP nx = SOP(1).And0(v);

    EXPECT_EQ(SOP(0).IsPolyCube(), 0);
    EXPECT_EQ(SOP(1).IsPolyCube(), 0);
    EXPECT_EQ(x.IsPolyCube(), 0);
    EXPECT_EQ((x + nx).IsPolyCube(), 1);
}

TEST_F(SOPTest, IsPolyLit) {
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP na = SOP(1).And0(a);
    SOP xb = SOP(1).And1(b);

    EXPECT_EQ(SOP(0).IsPolyLit(), 0);
    EXPECT_EQ(xa.IsPolyLit(), 0);   // single positive literal
    EXPECT_EQ(na.IsPolyLit(), 0);   // single negative literal
    EXPECT_EQ((xa * xb).IsPolyLit(), 1);  // product of two literals
    EXPECT_EQ((xa + xb).IsPolyLit(), 1);  // sum of two literals
}

/* ================================================================ */
/*  Support                                                          */
/* ================================================================ */

TEST_F(SOPTest, SupportConstants) {
    EXPECT_EQ(SOP(0).Support() == SOP(0), 1);
    EXPECT_EQ(SOP(1).Support() == SOP(0), 1);
}

TEST_F(SOPTest, SupportSingleVar) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOP sup = x.Support();

    // Support should be a single cube with positive literal for v
    EXPECT_EQ(sup.Cube(), 1u);
    EXPECT_EQ(sup.Top(), v);
}

TEST_F(SOPTest, SupportNegLit) {
    // ~x should also report variable x in support
    int v = SOP_NewVar();
    SOP nx = SOP(1).And0(v);
    SOP sup = nx.Support();

    EXPECT_EQ(sup.Top(), v);
}

/* ================================================================ */
/*  Divisor                                                          */
/* ================================================================ */

TEST_F(SOPTest, DivisorSingleCube) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    EXPECT_EQ(x.Divisor() == SOP(1), 1);
}

TEST_F(SOPTest, DivisorMultiCube) {
    // f = x_a*x_b + x_a*x_c → Divisor should find a non-trivial factor
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    int c = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);
    SOP xc = SOP(1).And1(c);
    SOP f = xa * xb + xa * xc;

    SOP d = f.Divisor();
    // Divisor is a non-trivial factor
    EXPECT_GE(d.Cube(), 1u);
}

/* ================================================================ */
/*  Implicants                                                       */
/* ================================================================ */

TEST_F(SOPTest, ImplicantsAllTrue) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOP result = x.Implicants(BDD(1));
    EXPECT_EQ(result == x, 1);
}

TEST_F(SOPTest, ImplicantsAllFalse) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOP result = x.Implicants(BDD(0));
    EXPECT_EQ(result == SOP(0), 1);
}

TEST_F(SOPTest, ImplicantsBasic) {
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);

    // SOP = x_a + x_b, f = BDD(x_a)
    // Only x_a is an implicant of x_a
    SOP sum = xa + xb;
    BDD f = BDD::prime(static_cast<bddvar>(a));
    SOP impl = sum.Implicants(f);
    EXPECT_EQ(impl == xa, 1);
}

/* ================================================================ */
/*  Swap                                                             */
/* ================================================================ */

TEST_F(SOPTest, SwapOddVarThrows) {
    SOP s(1);
    EXPECT_THROW(s.Swap(1, 2), std::invalid_argument);
}

TEST_F(SOPTest, SwapBasic) {
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);

    SOP swapped = xa.Swap(a, b);
    EXPECT_EQ(swapped == xb, 1);
}

/* ================================================================ */
/*  ISOP                                                             */
/* ================================================================ */

TEST_F(SOPTest, ISOPConstants) {
    EXPECT_EQ(SOP_ISOP(BDD(0)) == SOP(0), 1);
    EXPECT_EQ(SOP_ISOP(BDD(1)) == SOP(1), 1);
}

TEST_F(SOPTest, ISOPPositiveLiteral) {
    int v = SOP_NewVar();
    BDD bv = BDD::prime(static_cast<bddvar>(v));
    SOP isop = SOP_ISOP(bv);

    // Should produce single cube with positive literal
    EXPECT_EQ(isop.Cube(), 1u);
    EXPECT_EQ(isop.GetBDD() == bv, 1);
}

TEST_F(SOPTest, ISOPOr) {
    // BDD for x_a | x_b
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    BDD ba = BDD::prime(static_cast<bddvar>(a));
    BDD bb = BDD::prime(static_cast<bddvar>(b));
    BDD f = ba | bb;

    SOP isop = SOP_ISOP(f);
    // ISOP should be irredundant and cover f
    EXPECT_EQ(isop.GetBDD() == f, 1);
    EXPECT_LE(isop.Cube(), 2u);
}

TEST_F(SOPTest, ISOPWithDontCare) {
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    BDD ba = BDD::prime(static_cast<bddvar>(a));
    BDD bb = BDD::prime(static_cast<bddvar>(b));

    // on = a & b, dc = a & ~b (can optionally cover a&~b)
    BDD on = ba & bb;
    BDD dc = ba & ~bb;

    SOP isop = SOP_ISOP(on, dc);
    // Result should cover on but not contradict (on | dc) boundaries
    BDD isop_bdd = isop.GetBDD();
    // isop_bdd should imply (on | dc) and include on
    EXPECT_EQ((on & ~isop_bdd) == BDD(0), 1);  // on ⊆ isop
    EXPECT_EQ((isop_bdd & ~(on | dc)) == BDD(0), 1);  // isop ⊆ on ∪ dc
}

TEST_F(SOPTest, InvISOP) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOP inv = x.InvISOP();

    // InvISOP gives ISOP of negation
    BDD expected = ~x.GetBDD();
    EXPECT_EQ(inv.GetBDD() == expected, 1);
}

/* ================================================================ */
/*  Print (just ensure no crash)                                     */
/* ================================================================ */

TEST_F(SOPTest, PrintNoCrash) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    testing::internal::CaptureStdout();
    x.Print();
    fflush(stdout);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(output.empty());
}

TEST_F(SOPTest, PrintPlaNoCrash) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    testing::internal::CaptureStdout();
    x.PrintPla();
    fflush(stdout);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(output.empty());
}

/* ================================================================ */
/*  SOPV tests                                                       */
/* ================================================================ */

TEST_F(SOPTest, SOPVDefaultConstructor) {
    SOPV sv;
    EXPECT_EQ(sv.Last(), 0);
}

TEST_F(SOPTest, SOPVFromSOP) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOPV sv(x, 0);

    EXPECT_EQ(sv.GetSOP(0) == x, 1);
}

TEST_F(SOPTest, SOPVCopy) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOPV sv(x, 0);
    SOPV sv2(sv);
    EXPECT_EQ(sv == sv2, 1);
}

TEST_F(SOPTest, SOPVAssignment) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOPV sv(x, 0);
    SOPV sv2;
    sv2 = sv;
    EXPECT_EQ(sv == sv2, 1);
}

TEST_F(SOPTest, SOPVMultipleElements) {
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);

    SOPV sv;
    sv += SOPV(xa, 0);
    sv += SOPV(xb, 1);

    EXPECT_EQ(sv.GetSOP(0) == xa, 1);
    EXPECT_EQ(sv.GetSOP(1) == xb, 1);
    EXPECT_EQ(sv.Last(), 1);
}

TEST_F(SOPTest, SOPVSetOperations) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOPV a(x, 0);
    SOPV b(x, 0);
    SOPV c(SOP(1).And0(v), 0);

    EXPECT_EQ((a & b) == a, 1);
    EXPECT_EQ(a == b, 1);
    EXPECT_EQ(a != c, 1);
}

TEST_F(SOPTest, SOPVFactor) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOPV sv(x, 0);

    SOPV f1 = sv.Factor1(v);
    EXPECT_EQ(f1.GetSOP(0) == SOP(1), 1);

    SOPV f0 = sv.Factor0(v);
    EXPECT_EQ(f0.GetSOP(0) == SOP(0), 1);

    SOPV fD = sv.FactorD(v);
    EXPECT_EQ(fD.GetSOP(0) == SOP(0), 1);
}

TEST_F(SOPTest, SOPVAnd) {
    int v = SOP_NewVar();
    SOPV sv(SOP(1), 0);

    SOPV a1 = sv.And1(v);
    EXPECT_EQ(a1.GetSOP(0).Factor1(v) == SOP(1), 1);

    SOPV a0 = sv.And0(v);
    EXPECT_EQ(a0.GetSOP(0).Factor0(v) == SOP(1), 1);
}

TEST_F(SOPTest, SOPVTop) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOPV sv(x, 0);
    EXPECT_EQ(sv.Top(), v);
    EXPECT_EQ(sv.Top() & 1, 0);
}

TEST_F(SOPTest, SOPVCubeAndLit) {
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);

    SOPV sv;
    sv += SOPV(xa, 0);
    sv += SOPV(xb, 1);

    // Total cubes = 1 + 1 = 2 (sum across outputs)
    EXPECT_EQ(sv.Cube(), 2u);
    EXPECT_EQ(sv.Lit(), 2u);
}

TEST_F(SOPTest, SOPVCubeAndLitSharedCube) {
    int a = SOP_NewVar();
    SOP xa = SOP(1).And1(a);

    // Same cube in both outputs
    SOPV sv;
    sv += SOPV(xa, 0);
    sv += SOPV(xa, 1);

    // Total cubes = 1 + 1 = 2 (sum, not union)
    EXPECT_EQ(sv.Cube(), 2u);
    // Total lits = 1 + 1 = 2
    EXPECT_EQ(sv.Lit(), 2u);
}

TEST_F(SOPTest, SOPVShift) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOPV sv(x, 0);

    SOPV shifted = sv << 1;
    SOPV back = shifted >> 1;
    EXPECT_EQ(back.GetSOP(0) == x, 1);
}

TEST_F(SOPTest, SOPVSwap) {
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOPV sv(xa, 0);

    SOPV swapped = sv.Swap(a, b);
    EXPECT_EQ(swapped.GetSOP(0) == SOP(1).And1(b), 1);
}

TEST_F(SOPTest, SOPVSwapOddThrows) {
    SOPV sv;
    EXPECT_THROW(sv.Swap(1, 2), std::invalid_argument);
}

TEST_F(SOPTest, SOPVMask) {
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);

    SOPV sv;
    sv += SOPV(xa, 0);
    sv += SOPV(xb, 1);

    SOPV m = sv.Mask(0);
    EXPECT_EQ(m.GetSOP(0) == xa, 1);
}

TEST_F(SOPTest, SOPVPrintNoCrash) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOPV sv(x, 0);

    testing::internal::CaptureStdout();
    sv.Print();
    fflush(stdout);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(output.empty());
}

TEST_F(SOPTest, SOPVPrintPlaNoCrash) {
    int v = SOP_NewVar();
    SOP x = SOP(1).And1(v);
    SOPV sv(x, 0);

    testing::internal::CaptureStdout();
    sv.PrintPla();
    fflush(stdout);
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(output.empty());
}

TEST_F(SOPTest, SOPVPrintPlaOutputUsesZeroNotTilde) {
    int va = SOP_NewVar();
    int vb = SOP_NewVar();
    SOP xa = SOP(1).And1(va);
    SOP xb = SOP(1).And1(vb);
    // Output 0 has cube xa, output 1 has cube xb
    // For cube xa: output is "10". For cube xb: output is "01".
    SOPV sv;
    sv += SOPV(xa, 0);
    sv += SOPV(xb, 1);

    testing::internal::CaptureStdout();
    sv.PrintPla();
    fflush(stdout);
    std::string output = testing::internal::GetCapturedStdout();

    // PLA output must use '0' for false outputs, not '~'
    EXPECT_EQ(output.find('~'), std::string::npos);
    // Should contain "10" and "01" in the output columns
    EXPECT_NE(output.find("10"), std::string::npos);
    EXPECT_NE(output.find("01"), std::string::npos);
}

/* ================================================================ */
/*  SOPV_ISOP                                                        */
/* ================================================================ */

TEST_F(SOPTest, SOPV_ISOP_Basic) {
    int v = SOP_NewVar();
    BDD bv = BDD::prime(static_cast<bddvar>(v));
    BDDV bddv(bv);

    SOPV result = SOPV_ISOP(bddv);
    SOP s = result.GetSOP(0);
    EXPECT_EQ(s.GetBDD() == bv, 1);
}

TEST_F(SOPTest, SOPV_ISOP_MultiOutput) {
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    BDD ba = BDD::prime(static_cast<bddvar>(a));
    BDD bb = BDD::prime(static_cast<bddvar>(b));

    BDDV on(ba, 1);
    on = on || BDDV(bb);

    SOPV result = SOPV_ISOP(on);
    EXPECT_EQ(result.GetSOP(0).GetBDD() == ba, 1);
    EXPECT_EQ(result.GetSOP(1).GetBDD() == bb, 1);
}

/* ================================================================ */
/*  SOPV_ISOP2                                                       */
/* ================================================================ */

TEST_F(SOPTest, SOPV_ISOP2_Basic) {
    int v = SOP_NewVar();
    BDD bv = BDD::prime(static_cast<bddvar>(v));
    BDDV bddv(bv);

    SOPV result = SOPV_ISOP2(bddv);
    // First element is the cover, element at index Len should be phase
    SOP s = result.GetSOP(0);
    BDD sop_bdd = s.GetBDD();
    // Should cover bv (possibly via negation)
    EXPECT_TRUE(sop_bdd == bv || sop_bdd == ~bv);
}

/* ================================================================ */
/*  Mult with same variable (positive and negative interactions)     */
/* ================================================================ */

TEST_F(SOPTest, MultPositiveNegativeCancel) {
    // x_a * ~x_a should give 0 (positive AND negative cancel)
    int a = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP na = SOP(1).And0(a);
    SOP prod = xa * na;
    EXPECT_EQ(prod == SOP(0), 1);
}

TEST_F(SOPTest, MultPositiveDC) {
    // x_a * 1 (tautology) = x_a
    int a = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP prod = xa * SOP(1);
    EXPECT_EQ(prod == xa, 1);
}

TEST_F(SOPTest, MultTwoCubesWithSharedVar) {
    // (x_a * x_b) * (x_a * x_c) = x_a * x_b * x_c
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    int c = SOP_NewVar();
    SOP xa = SOP(1).And1(a);
    SOP xb = SOP(1).And1(b);
    SOP xc = SOP(1).And1(c);

    SOP lhs = (xa * xb) * (xa * xc);
    SOP rhs = xa * xb * xc;
    EXPECT_EQ(lhs == rhs, 1);
}

/* ================================================================ */
/*  GetBDD roundtrip via ISOP                                        */
/* ================================================================ */

TEST_F(SOPTest, GetBDD_ISOP_Roundtrip) {
    // Build SOP from BDD via ISOP, convert back to BDD
    int a = SOP_NewVar();
    int b = SOP_NewVar();
    BDD ba = BDD::prime(static_cast<bddvar>(a));
    BDD bb = BDD::prime(static_cast<bddvar>(b));
    BDD f = (ba & bb) | (~ba & ~bb);  // XNOR

    SOP isop = SOP_ISOP(f);
    BDD roundtrip = isop.GetBDD();
    EXPECT_EQ(roundtrip == f, 1);
}
