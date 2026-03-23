#include "bddv.h"
#include <gtest/gtest.h>
#include <sstream>
#include <cstdio>
#include <cstring>

class BDDVTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDDV_Init(256);
    }
};

// ============================================================
// Initialization
// ============================================================

TEST_F(BDDVTest, InitSetsActiveFlag) {
    EXPECT_EQ(BDDV_Active, 1);
}

TEST_F(BDDVTest, SystemVariablesCreated) {
    // After BDDV_Init, BDDV_SysVarTop variables should exist
    EXPECT_GE(static_cast<int>(BDD_VarUsed()), BDDV_SysVarTop);
}

TEST_F(BDDVTest, UserTopLevInitiallyZero) {
    EXPECT_EQ(BDDV_UserTopLev(), 0);
}

TEST_F(BDDVTest, NewVarCreatesUserVariable) {
    int v = BDDV_NewVar();
    EXPECT_GT(v, BDDV_SysVarTop);
    EXPECT_EQ(BDDV_UserTopLev(), 1);
}

TEST_F(BDDVTest, MultipleNewVars) {
    BDDV_NewVar();
    BDDV_NewVar();
    BDDV_NewVar();
    EXPECT_EQ(BDDV_UserTopLev(), 3);
}

TEST_F(BDDVTest, SystemVarsAtLowestLevels) {
    // With standard layout, system vars 1..20 are at levels 1..20
    // User vars are above them at levels 21+
    BDDV_NewVar();
    BDDV_NewVar();
    // System var 1 should be at a level <= BDDV_SysVarTop
    bddvar lev1 = BDD_LevOfVar(1);
    EXPECT_LE(static_cast<int>(lev1), BDDV_SysVarTop);
    // User vars should be at levels > BDDV_SysVarTop
    EXPECT_EQ(BDDV_UserTopLev(), 2);
}

// ============================================================
// Constructors
// ============================================================

TEST_F(BDDVTest, DefaultConstructor) {
    BDDV v;
    EXPECT_EQ(v.Len(), 0);
}

TEST_F(BDDVTest, ConstructFromBDD) {
    BDDV v(BDD(1));
    EXPECT_EQ(v.Len(), 1);
    EXPECT_EQ(v.GetBDD(0).GetID(), BDD(1).GetID());
}

TEST_F(BDDVTest, ConstructFromBDDFalse) {
    BDDV v(BDD(0));
    EXPECT_EQ(v.Len(), 1);
    EXPECT_EQ(v.GetBDD(0).GetID(), BDD(0).GetID());
}

TEST_F(BDDVTest, ConstructFromBDDWithLen) {
    BDDV v(BDD(1), 4);
    EXPECT_EQ(v.Len(), 4);
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(v.GetBDD(i).GetID(), BDD(1).GetID());
    }
}

TEST_F(BDDVTest, ConstructFromBDDLenZero) {
    BDDV v(BDD(1), 0);
    EXPECT_EQ(v.Len(), 0);
}

TEST_F(BDDVTest, ConstructFromBDDNegativeLenThrows) {
    EXPECT_THROW(BDDV(BDD(1), -1), std::invalid_argument);
}

TEST_F(BDDVTest, ConstructFromBDDExcessiveLenThrows) {
    EXPECT_THROW(BDDV(BDD(1), BDDV_MaxLen + 1), std::invalid_argument);
}

TEST_F(BDDVTest, ConstructFromBDDNull) {
    BDDV v(BDD(-1), 5);
    EXPECT_EQ(v.Len(), 1); // null forces len=1
}

TEST_F(BDDVTest, ConstructFromBDDWithLenRejectsSystemVar) {
    // System variable 1 should be rejected (same as the length-1 constructor)
    BDD sys = BDDvar(1);
    EXPECT_THROW(BDDV(sys, 2), std::invalid_argument);
    EXPECT_THROW(BDDV(sys, 4), std::invalid_argument);
    // Null BDD is still allowed (error value)
    EXPECT_NO_THROW(BDDV(BDD(-1), 2));
}

TEST_F(BDDVTest, CopyConstructor) {
    BDDV v1(BDD(1), 3);
    BDDV v2(v1);
    EXPECT_EQ(v2.Len(), 3);
    EXPECT_EQ(v1 == v2, 1);
}

TEST_F(BDDVTest, Assignment) {
    BDDV v1(BDD(1), 3);
    BDDV v2;
    v2 = v1;
    EXPECT_EQ(v2.Len(), 3);
    EXPECT_EQ(v1 == v2, 1);
}

// ============================================================
// GetLev (tested indirectly through constructors)
// ============================================================

TEST_F(BDDVTest, GetLevValues) {
    // len=1 → lev=0, len=2 → lev=1, len=3 → lev=2, len=4 → lev=2,
    // len=5 → lev=3
    BDDV v1(BDD(1), 1);
    BDDV v2(BDD(1), 2);
    BDDV v3(BDD(1), 3);
    BDDV v4(BDD(1), 4);
    BDDV v5(BDD(1), 5);

    // We can verify indirectly via Former/Latter lengths
    EXPECT_EQ(v2.Former().Len(), 1);
    EXPECT_EQ(v2.Latter().Len(), 1);
    EXPECT_EQ(v3.Former().Len(), 2);
    EXPECT_EQ(v3.Latter().Len(), 1);
    EXPECT_EQ(v4.Former().Len(), 2);
    EXPECT_EQ(v4.Latter().Len(), 2);
    EXPECT_EQ(v5.Former().Len(), 4);
    EXPECT_EQ(v5.Latter().Len(), 1);
}

// ============================================================
// GetBDD / GetMetaBDD / Uniform / Len
// ============================================================

TEST_F(BDDVTest, GetBDDOutOfRangeThrows) {
    BDDV v(BDD(1), 3);
    EXPECT_THROW(v.GetBDD(-1), std::invalid_argument);
    EXPECT_THROW(v.GetBDD(3), std::invalid_argument);
}

TEST_F(BDDVTest, GetMetaBDD) {
    BDD f(1);
    BDDV v(f);
    EXPECT_EQ(v.GetMetaBDD().GetID(), f.GetID());
}

TEST_F(BDDVTest, UniformAllSame) {
    BDDV v(BDD(1), 5);
    EXPECT_EQ(v.Uniform(), 1);
}

TEST_F(BDDVTest, UniformDifferent) {
    int va = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV v = BDDV(x) || BDDV(~x);
    EXPECT_EQ(v.Uniform(), 0);
}

TEST_F(BDDVTest, Len) {
    EXPECT_EQ(BDDV().Len(), 0);
    EXPECT_EQ(BDDV(BDD(0)).Len(), 1);
    EXPECT_EQ(BDDV(BDD(0), 7).Len(), 7);
}

// ============================================================
// Former / Latter
// ============================================================

TEST_F(BDDVTest, FormerOfLen1) {
    BDDV v(BDD(1));
    EXPECT_EQ(v.Former().Len(), 0);
}

TEST_F(BDDVTest, FormerOfLen0) {
    BDDV v;
    // _len == 0 → _len <= 1 → return empty
    EXPECT_EQ(v.Former().Len(), 0);
}

TEST_F(BDDVTest, LatterOfLen0) {
    BDDV v;
    EXPECT_EQ(v.Latter().Len(), 0);
}

TEST_F(BDDVTest, LatterOfLen1) {
    BDDV v(BDD(1));
    EXPECT_EQ(v.Latter().Len(), 1);
    EXPECT_EQ(v.Latter().GetBDD(0).GetID(), BDD(1).GetID());
}

TEST_F(BDDVTest, FormerLatterRoundtrip) {
    int va = BDDV_NewVar();
    int vb = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDD y = BDDvar(static_cast<bddvar>(vb));
    BDDV v = BDDV(x) || BDDV(y) || BDDV(x & y);

    BDDV former = v.Former();
    BDDV latter = v.Latter();
    EXPECT_EQ(former.Len(), 2);
    EXPECT_EQ(latter.Len(), 1);

    // Reconstruct
    BDDV reconstructed = former || latter;
    EXPECT_EQ(reconstructed == v, 1);
}

// ============================================================
// Logic operators
// ============================================================

TEST_F(BDDVTest, AndOperator) {
    int va = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV a = BDDV(x) || BDDV(BDD(1));
    BDDV b = BDDV(BDD(1)) || BDDV(~x);

    BDDV result = a & b;
    EXPECT_EQ(result.Len(), 2);
    EXPECT_EQ(result.GetBDD(0).GetID(), x.GetID());
    EXPECT_EQ(result.GetBDD(1).GetID(), (~x).GetID());
}

TEST_F(BDDVTest, OrOperator) {
    int va = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV a = BDDV(x) || BDDV(BDD(0));
    BDDV b = BDDV(BDD(0)) || BDDV(~x);

    BDDV result = a | b;
    EXPECT_EQ(result.Len(), 2);
    EXPECT_EQ(result.GetBDD(0).GetID(), x.GetID());
    EXPECT_EQ(result.GetBDD(1).GetID(), (~x).GetID());
}

TEST_F(BDDVTest, XorOperator) {
    BDDV a = BDDV(BDD(1)) || BDDV(BDD(0));
    BDDV b = BDDV(BDD(1)) || BDDV(BDD(1));

    BDDV result = a ^ b;
    EXPECT_EQ(result.Len(), 2);
    EXPECT_EQ(result.GetBDD(0).GetID(), BDD(0).GetID());
    EXPECT_EQ(result.GetBDD(1).GetID(), BDD(1).GetID());
}

TEST_F(BDDVTest, NotOperator) {
    BDDV a = BDDV(BDD(1)) || BDDV(BDD(0));
    BDDV result = ~a;
    EXPECT_EQ(result.Len(), 2);
    EXPECT_EQ(result.GetBDD(0).GetID(), BDD(0).GetID());
    EXPECT_EQ(result.GetBDD(1).GetID(), BDD(1).GetID());
}

TEST_F(BDDVTest, LengthMismatchThrows) {
    BDDV a(BDD(1), 2);
    BDDV b(BDD(1), 3);
    EXPECT_THROW(a & b, std::invalid_argument);
    EXPECT_THROW(a | b, std::invalid_argument);
    EXPECT_THROW(a ^ b, std::invalid_argument);
}

TEST_F(BDDVTest, InPlaceOperators) {
    BDDV a = BDDV(BDD(1)) || BDDV(BDD(0));
    BDDV b = BDDV(BDD(0)) || BDDV(BDD(1));
    BDDV c = a;

    a &= b;
    EXPECT_EQ(a.GetBDD(0).GetID(), BDD(0).GetID());
    EXPECT_EQ(a.GetBDD(1).GetID(), BDD(0).GetID());

    a = c;
    a |= b;
    EXPECT_EQ(a.GetBDD(0).GetID(), BDD(1).GetID());
    EXPECT_EQ(a.GetBDD(1).GetID(), BDD(1).GetID());

    a = c;
    a ^= b;
    EXPECT_EQ(a.GetBDD(0).GetID(), BDD(1).GetID());
    EXPECT_EQ(a.GetBDD(1).GetID(), BDD(1).GetID());
}

// ============================================================
// Comparison operators
// ============================================================

TEST_F(BDDVTest, EqualityOperator) {
    BDDV a(BDD(1), 3);
    BDDV b(BDD(1), 3);
    BDDV c(BDD(0), 3);
    BDDV d(BDD(1), 2);

    EXPECT_EQ(a == b, 1);
    EXPECT_EQ(a == c, 0);
    EXPECT_EQ(a == d, 0);
    EXPECT_EQ(a != c, 1);
    EXPECT_EQ(a != b, 0);
}

// ============================================================
// Concatenation
// ============================================================

TEST_F(BDDVTest, ConcatEmptyLeft) {
    BDDV a;
    BDDV b(BDD(1), 3);
    BDDV result = a || b;
    EXPECT_EQ(result == b, 1);
}

TEST_F(BDDVTest, ConcatEmptyRight) {
    BDDV a(BDD(1), 3);
    BDDV b;
    BDDV result = a || b;
    EXPECT_EQ(result == a, 1);
}

TEST_F(BDDVTest, ConcatTwoElements) {
    BDDV a(BDD(0));
    BDDV b(BDD(1));
    BDDV result = a || b;
    EXPECT_EQ(result.Len(), 2);
    EXPECT_EQ(result.GetBDD(0).GetID(), BDD(0).GetID());
    EXPECT_EQ(result.GetBDD(1).GetID(), BDD(1).GetID());
}

TEST_F(BDDVTest, ConcatMultiple) {
    int va = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV v = BDDV(BDD(0)) || BDDV(BDD(1)) || BDDV(x) || BDDV(~x);
    EXPECT_EQ(v.Len(), 4);
    EXPECT_EQ(v.GetBDD(0).GetID(), BDD(0).GetID());
    EXPECT_EQ(v.GetBDD(1).GetID(), BDD(1).GetID());
    EXPECT_EQ(v.GetBDD(2).GetID(), x.GetID());
    EXPECT_EQ(v.GetBDD(3).GetID(), (~x).GetID());
}

TEST_F(BDDVTest, ConcatNonPowerOfTwo) {
    BDDV v = BDDV(BDD(0)) || BDDV(BDD(1)) || BDDV(BDD(0));
    EXPECT_EQ(v.Len(), 3);
    EXPECT_EQ(v.GetBDD(0).GetID(), BDD(0).GetID());
    EXPECT_EQ(v.GetBDD(1).GetID(), BDD(1).GetID());
    EXPECT_EQ(v.GetBDD(2).GetID(), BDD(0).GetID());
}

// ============================================================
// Shift operators
// ============================================================

TEST_F(BDDVTest, LeftShiftUniform) {
    int va = BDDV_NewVar();
    // Need a second user variable so shift destination level exists
    BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV v(x, 2);
    BDDV shifted = v << 1;
    EXPECT_EQ(shifted.Len(), 2);
    // After shifting, all elements should be the same (still uniform)
    EXPECT_EQ(shifted.Uniform(), 1);
    // The shifted BDD should be different from the original
    BDD shifted_bdd = shifted.GetBDD(0);
    EXPECT_NE(shifted_bdd.GetID(), x.GetID());
}

TEST_F(BDDVTest, RightShiftUniform) {
    int va = BDDV_NewVar();
    int vb = BDDV_NewVar();
    // Use the variable at the higher level so right-shift has room
    BDD y = BDDvar(static_cast<bddvar>(vb));
    BDDV v(y, 2);
    BDDV shifted = v >> 1;
    EXPECT_EQ(shifted.Len(), 2);
    EXPECT_EQ(shifted.Uniform(), 1);
    // Shifted result should equal x (the variable one level below)
    BDD x = BDDvar(static_cast<bddvar>(va));
    EXPECT_EQ(shifted.GetBDD(0).GetID(), x.GetID());
}

TEST_F(BDDVTest, ShiftInPlace) {
    int va = BDDV_NewVar();
    BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV v(x, 2);
    BDDV original = v;
    v <<= 1;
    EXPECT_EQ(v.Len(), 2);
    EXPECT_NE(v.GetBDD(0).GetID(), original.GetBDD(0).GetID());
}

// ============================================================
// At0, At1
// ============================================================

TEST_F(BDDVTest, At0At1) {
    int va = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV v = BDDV(x) || BDDV(~x);

    BDDV at0 = v.At0(va);
    EXPECT_EQ(at0.Len(), 2);
    EXPECT_EQ(at0.GetBDD(0).GetID(), BDD(0).GetID()); // x|x=0 = 0
    EXPECT_EQ(at0.GetBDD(1).GetID(), BDD(1).GetID()); // ~x|x=0 = 1

    BDDV at1 = v.At1(va);
    EXPECT_EQ(at1.Len(), 2);
    EXPECT_EQ(at1.GetBDD(0).GetID(), BDD(1).GetID()); // x|x=1 = 1
    EXPECT_EQ(at1.GetBDD(1).GetID(), BDD(0).GetID()); // ~x|x=1 = 0
}

// ============================================================
// Cofact
// ============================================================

TEST_F(BDDVTest, CofactBasic) {
    int va = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV f = BDDV(x);
    BDDV g = BDDV(BDD(1));
    BDDV result = f.Cofact(g);
    EXPECT_EQ(result.Len(), 1);
    // Cofactor of x with respect to 1 should be x
    EXPECT_EQ(result.GetBDD(0).GetID(), x.GetID());
}

TEST_F(BDDVTest, CofactLengthMismatchThrows) {
    BDDV a(BDD(1), 2);
    BDDV b(BDD(1), 3);
    EXPECT_THROW(a.Cofact(b), std::invalid_argument);
}

// ============================================================
// Swap
// ============================================================

TEST_F(BDDVTest, SwapVariables) {
    int va = BDDV_NewVar();
    int vb = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDD y = BDDvar(static_cast<bddvar>(vb));

    BDDV v = BDDV(x) || BDDV(y);
    BDDV swapped = v.Swap(va, vb);

    EXPECT_EQ(swapped.GetBDD(0).GetID(), y.GetID());
    EXPECT_EQ(swapped.GetBDD(1).GetID(), x.GetID());
}

// ============================================================
// Top
// ============================================================

TEST_F(BDDVTest, TopConstant) {
    BDDV v(BDD(0), 3);
    EXPECT_EQ(v.Top(), 0);
}

TEST_F(BDDVTest, TopWithVariable) {
    int va = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV v(x, 2);
    EXPECT_EQ(v.Top(), va);
}

TEST_F(BDDVTest, TopMixed) {
    int va = BDDV_NewVar();
    int vb = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDD y = BDDvar(static_cast<bddvar>(vb));

    BDDV v = BDDV(x) || BDDV(y);
    int top = v.Top();
    // Should be the variable with the higher level
    bddvar lev_a = BDD_LevOfVar(static_cast<bddvar>(va));
    bddvar lev_b = BDD_LevOfVar(static_cast<bddvar>(vb));
    if (lev_a >= lev_b) {
        EXPECT_EQ(top, va);
    } else {
        EXPECT_EQ(top, vb);
    }
}

// ============================================================
// Size
// ============================================================

TEST_F(BDDVTest, SizeConstant) {
    BDDV v(BDD(0), 3);
    EXPECT_EQ(v.Size(), 0u);
}

TEST_F(BDDVTest, SizeWithVariable) {
    int va = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV v(x, 2);
    // x is a single node; uniform means all share it
    EXPECT_EQ(v.Size(), 1u);
}

TEST_F(BDDVTest, SizeEmpty) {
    BDDV v;
    EXPECT_EQ(v.Size(), 0u);
}

// ============================================================
// Part
// ============================================================

TEST_F(BDDVTest, PartFullRange) {
    BDDV v(BDD(1), 4);
    BDDV p = v.Part(0, 4);
    EXPECT_EQ(p == v, 1);
}

TEST_F(BDDVTest, PartSubRange) {
    int va = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV v = BDDV(BDD(0)) || BDDV(BDD(1)) || BDDV(x) || BDDV(~x);

    BDDV p = v.Part(1, 2);
    EXPECT_EQ(p.Len(), 2);
    EXPECT_EQ(p.GetBDD(0).GetID(), BDD(1).GetID());
    EXPECT_EQ(p.GetBDD(1).GetID(), x.GetID());
}

TEST_F(BDDVTest, PartSingleElement) {
    BDDV v = BDDV(BDD(0)) || BDDV(BDD(1));
    BDDV p = v.Part(1, 1);
    EXPECT_EQ(p.Len(), 1);
    EXPECT_EQ(p.GetBDD(0).GetID(), BDD(1).GetID());
}

TEST_F(BDDVTest, PartEmpty) {
    BDDV v(BDD(1), 3);
    BDDV p = v.Part(0, 0);
    EXPECT_EQ(p.Len(), 0);
}

TEST_F(BDDVTest, PartOutOfRangeThrows) {
    BDDV v(BDD(1), 3);
    EXPECT_THROW(v.Part(-1, 1), std::invalid_argument);
    EXPECT_THROW(v.Part(0, 4), std::invalid_argument);
    EXPECT_THROW(v.Part(2, 2), std::invalid_argument);
}

// ============================================================
// Spread
// ============================================================

TEST_F(BDDVTest, SpreadUniform) {
    int va = BDDV_NewVar();
    BDDV_NewVar();
    BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV v(x, 2);
    BDDV result = v.Spread(1);
    EXPECT_EQ(result.Len(), 2);
    EXPECT_EQ(result.Uniform(), 1);
}

// ============================================================
// BDDV_Imply
// ============================================================

TEST_F(BDDVTest, ImplyTrue) {
    int va = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV a = BDDV(x) || BDDV(BDD(0));
    BDDV b = BDDV(BDD(1)) || BDDV(BDD(1));
    EXPECT_EQ(BDDV_Imply(a, b), 1);
}

TEST_F(BDDVTest, ImplyFalse) {
    BDDV a = BDDV(BDD(1)) || BDDV(BDD(1));
    BDDV b = BDDV(BDD(0)) || BDDV(BDD(1));
    EXPECT_EQ(BDDV_Imply(a, b), 0);
}

TEST_F(BDDVTest, ImplyLengthMismatch) {
    BDDV a(BDD(1), 2);
    BDDV b(BDD(1), 3);
    EXPECT_EQ(BDDV_Imply(a, b), 0);
}

// ============================================================
// BDDV_Mask1 / BDDV_Mask2
// ============================================================

TEST_F(BDDVTest, Mask1) {
    BDDV m = BDDV_Mask1(2, 5);
    EXPECT_EQ(m.Len(), 5);
    for (int i = 0; i < 5; i++) {
        if (i == 2) {
            EXPECT_EQ(m.GetBDD(i).GetID(), BDD(1).GetID());
        } else {
            EXPECT_EQ(m.GetBDD(i).GetID(), BDD(0).GetID());
        }
    }
}

TEST_F(BDDVTest, Mask1Index0) {
    BDDV m = BDDV_Mask1(0, 3);
    EXPECT_EQ(m.GetBDD(0).GetID(), BDD(1).GetID());
    EXPECT_EQ(m.GetBDD(1).GetID(), BDD(0).GetID());
    EXPECT_EQ(m.GetBDD(2).GetID(), BDD(0).GetID());
}

TEST_F(BDDVTest, Mask1InvalidThrows) {
    EXPECT_THROW(BDDV_Mask1(-1, 5), std::invalid_argument);
    EXPECT_THROW(BDDV_Mask1(5, 5), std::invalid_argument);
    EXPECT_THROW(BDDV_Mask1(0, -1), std::invalid_argument);
}

TEST_F(BDDVTest, Mask2) {
    BDDV m = BDDV_Mask2(2, 5);
    EXPECT_EQ(m.Len(), 5);
    EXPECT_EQ(m.GetBDD(0).GetID(), BDD(0).GetID());
    EXPECT_EQ(m.GetBDD(1).GetID(), BDD(0).GetID());
    EXPECT_EQ(m.GetBDD(2).GetID(), BDD(1).GetID());
    EXPECT_EQ(m.GetBDD(3).GetID(), BDD(1).GetID());
    EXPECT_EQ(m.GetBDD(4).GetID(), BDD(1).GetID());
}

TEST_F(BDDVTest, Mask2AllTrue) {
    BDDV m = BDDV_Mask2(0, 3);
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(m.GetBDD(i).GetID(), BDD(1).GetID());
    }
}

TEST_F(BDDVTest, Mask2AllFalse) {
    BDDV m = BDDV_Mask2(3, 3);
    for (int i = 0; i < 3; i++) {
        EXPECT_EQ(m.GetBDD(i).GetID(), BDD(0).GetID());
    }
}

// ============================================================
// Export / Print (smoke tests)
// ============================================================

TEST_F(BDDVTest, ExportDoesNotCrash) {
    int va = BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV v = BDDV(x) || BDDV(~x);

    FILE* devnull = fopen("/dev/null", "w");
    ASSERT_NE(devnull, nullptr);
    v.Export(devnull);
    fclose(devnull);
}

TEST_F(BDDVTest, PrintDoesNotCrash) {
    BDDV v(BDD(1), 2);
    // Redirect stdout
    testing::internal::CaptureStdout();
    v.Print();
    std::string output = testing::internal::GetCapturedStdout();
    EXPECT_FALSE(output.empty());
}

// ============================================================
// BDDV_Import
// ============================================================

TEST_F(BDDVTest, ImportBasic) {
    // Create a simple import file:
    // _i 1 _o 2 _n 1
    // 2 1 F T
    // 2 3
    // (node 2: var at level 1, lo=F, hi=T → variable)
    // output 0 = node 2, output 1 = node 3 (negated node 2)
    const char* data = "_i 1 _o 2 _n 1\n2 1 F T\n2 3\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    // Ensure at least 1 user variable exists
    if (BDDV_UserTopLev() < 1) {
        BDDV_NewVar();
    }

    BDDV result = BDDV_Import(f);
    fclose(f);

    EXPECT_EQ(result.Len(), 2);
}

TEST_F(BDDVTest, ImportInvalidReturnsNull) {
    const char* data = "garbage data\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    BDDV result = BDDV_Import(f);
    fclose(f);

    // Should return a null BDDV (length 1 with null BDD)
    EXPECT_EQ(result.Len(), 1);
    EXPECT_EQ(result.GetMetaBDD().GetID(), bddnull);
}

TEST_F(BDDVTest, ImportTrailingJunkInNodeToken) {
    // "2junk" should be rejected (not silently treated as node 2)
    const char* data = "_i 1 _o 1 _n 1\n2 1 F T\n2junk\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    if (BDDV_UserTopLev() < 1) {
        BDDV_NewVar();
    }

    BDDV result = BDDV_Import(f);
    fclose(f);

    EXPECT_EQ(result.Len(), 1);
    EXPECT_EQ(result.GetMetaBDD().GetID(), bddnull);
}

TEST_F(BDDVTest, ImportTrailingJunkInChildToken) {
    // "2junk" as a child token should be rejected
    const char* data = "_i 2 _o 1 _n 2\n2 1 F T\n4 2 F 2junk\n4\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    while (BDDV_UserTopLev() < 2) {
        BDDV_NewVar();
    }

    BDDV result = BDDV_Import(f);
    fclose(f);

    EXPECT_EQ(result.Len(), 1);
    EXPECT_EQ(result.GetMetaBDD().GetID(), bddnull);
}

// ============================================================
// BDDV_ImportPla
// ============================================================

TEST_F(BDDVTest, ImportPlaBasic) {
    // Simple PLA: 2 inputs, 1 output
    // .i 2
    // .o 1
    // 1- 1
    // -1 1
    // .e
    const char* data = ".i 2\n.o 1\n1- 1\n-1 1\n.e\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    BDDV result = BDDV_ImportPla(f);
    fclose(f);

    // Result should be onset || dcset, so length = 2
    EXPECT_EQ(result.Len(), 2);
    // onset[0] should be x1 | x2
    BDD onset = result.GetBDD(0);
    EXPECT_NE(onset.GetID(), BDD(0).GetID());
}

// ============================================================
// Larger integration tests
// ============================================================

TEST_F(BDDVTest, ConcatAndGetBDDLarger) {
    // Build an 8-element BDDV with different constants
    BDDV v;
    for (int i = 0; i < 8; i++) {
        v = v || BDDV(BDD(i % 2));
    }
    EXPECT_EQ(v.Len(), 8);
    for (int i = 0; i < 8; i++) {
        bddp expected = (i % 2) ? bddtrue : bddfalse;
        EXPECT_EQ(v.GetBDD(i).GetID(), expected);
    }
}

TEST_F(BDDVTest, PartAcrossBoundary) {
    // 5-element BDDV: [0, 1, 0, 1, 0]
    BDDV v;
    for (int i = 0; i < 5; i++) {
        v = v || BDDV(BDD(i % 2));
    }
    // Part(1, 3) → [1, 0, 1]
    BDDV p = v.Part(1, 3);
    EXPECT_EQ(p.Len(), 3);
    EXPECT_EQ(p.GetBDD(0).GetID(), bddtrue);
    EXPECT_EQ(p.GetBDD(1).GetID(), bddfalse);
    EXPECT_EQ(p.GetBDD(2).GetID(), bddtrue);
}

TEST_F(BDDVTest, UniformAfterConcat) {
    // All elements are the same → should be uniform
    BDDV v = BDDV(BDD(1)) || BDDV(BDD(1)) || BDDV(BDD(1)) || BDDV(BDD(1));
    EXPECT_EQ(v.Len(), 4);
    EXPECT_EQ(v.Uniform(), 1);
}

TEST_F(BDDVTest, NonUniformShift) {
    int va = BDDV_NewVar();
    BDDV_NewVar();
    BDD x = BDDvar(static_cast<bddvar>(va));
    BDDV v = BDDV(x) || BDDV(BDD(0));
    BDDV shifted = v << 1;
    EXPECT_EQ(shifted.Len(), 2);
    // Element 1 should still be 0 after shift
    EXPECT_EQ(shifted.GetBDD(1).GetID(), BDD(0).GetID());
    // Element 0 should be shifted
    EXPECT_NE(shifted.GetBDD(0).GetID(), x.GetID());
}

// ============================================================
// BDDV_ImportPla sopf=1
// ============================================================

TEST_F(BDDVTest, ImportPlaSopfBasic) {
    // 1-input 1-output PLA with sopf=1
    const char* data = ".i 1\n.o 1\n1 1\n.e\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    BDDV result = BDDV_ImportPla(f, 1);
    fclose(f);

    EXPECT_EQ(result.Len(), 2);  // onset || dcset
    BDD onset = result.GetBDD(0);
    EXPECT_NE(onset.GetID(), BDD(0).GetID());
}

TEST_F(BDDVTest, ImportPlaSopfMultiInput) {
    // 2-input, 1-output: onset = x0 & x1
    const char* data = ".i 2\n.o 1\n11 1\n.e\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    BDDV result = BDDV_ImportPla(f, 1);
    fclose(f);

    EXPECT_EQ(result.Len(), 2);
    BDD onset = result.GetBDD(0);
    EXPECT_NE(onset.GetID(), BDD(0).GetID());

    // onset should have two variables (at even user levels)
    bddvar v0 = bddvaroflev(
        static_cast<bddvar>(BDDV_SysVarTop + 2));
    bddvar v1 = bddvaroflev(
        static_cast<bddvar>(BDDV_SysVarTop + 4));
    EXPECT_EQ(onset.At1(v0).At1(v1).GetID(), bddsingle);
    EXPECT_EQ(onset.At0(v0).GetID(), bddempty);
}

// ============================================================
// BDDV_ImportPla malformed product term
// ============================================================

TEST_F(BDDVTest, ImportPlaMalformedThrows) {
    // Input character '2' is invalid
    const char* data = ".i 1\n.o 1\n2 1\n.e\n";
    FILE* f = fmemopen(const_cast<char*>(data), strlen(data), "r");
    ASSERT_NE(f, nullptr);

    EXPECT_THROW(BDDV_ImportPla(f), std::invalid_argument);
    fclose(f);
}
