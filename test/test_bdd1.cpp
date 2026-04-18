#include <gtest/gtest.h>
#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <algorithm>
#include <cstring>
#include <random>
#include <sstream>
#include <unordered_set>
#include <climits>
#include <limits>

using namespace kyotodd;

class BDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

// --- Terminal constants ---

TEST_F(BDDTest, TerminalConstants) {
    EXPECT_NE(bddfalse, bddtrue);
    EXPECT_EQ(bddfalse, bddempty);
    EXPECT_EQ(bddtrue, bddsingle);
    EXPECT_NE(bddfalse, bddnull);
    EXPECT_NE(bddtrue, bddnull);
}

TEST_F(BDDTest, BddVarMax) {
    // 31-bit maximum
    EXPECT_EQ(bddvarmax, (UINT32_C(1) << (sizeof(uint64_t) * 8 - BDD_NODE_VAR_SHIFT)) - 1);
    EXPECT_EQ(bddvarmax, UINT32_C(0x7FFFFFFF));
}

TEST_F(BDDTest, BddValMax) {
    // 47-bit maximum
    EXPECT_EQ(bddvalmax, BDD_CONST_FLAG - 1);
    EXPECT_EQ(bddvalmax, UINT64_C(0x7FFFFFFFFFFF));
}

TEST_F(BDDTest, BddConst) {
    EXPECT_EQ(bddconst(0), bddfalse);
    EXPECT_EQ(bddconst(1), bddtrue);
    EXPECT_THROW(bddconst(2), std::invalid_argument);
    EXPECT_THROW(bddconst(42), std::invalid_argument);
    EXPECT_THROW(bddconst(bddvalmax), std::invalid_argument);
}

TEST_F(BDDTest, RecurLimitAndCount) {
    EXPECT_EQ(BDD_RecurLimit, 8192);
    EXPECT_EQ(BDD_RecurCount, 0);
    // Normal operation should leave count at 0 after completion
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp a = bddprime(v1);
    bddp b = bddprime(v2);
    bddp c = bddand(a, b);
    EXPECT_EQ(BDD_RecurCount, 0);
    (void)c;
}

// --- BDD/ZDD constructors ---

TEST_F(BDDTest, BDDDefaultConstructor) {
    BDD f;
    EXPECT_EQ(f.GetID(), bddfalse);
}

TEST_F(BDDTest, BDDConstructor) {
    BDD f(0);
    BDD t(1);
    BDD n(-1);
    EXPECT_EQ(f.GetID(), bddfalse);
    EXPECT_EQ(t.GetID(), bddtrue);
    EXPECT_EQ(n.GetID(), bddnull);
}

TEST_F(BDDTest, ZDDConstructor) {
    ZDD e(0);
    ZDD s(1);
    ZDD n(-1);
    EXPECT_EQ(e.GetID(), bddempty);
    EXPECT_EQ(s.GetID(), bddsingle);
    EXPECT_EQ(n.GetID(), bddnull);
}

// --- Static const objects ---

TEST_F(BDDTest, BDDStaticConsts) {
    EXPECT_EQ(BDD::False.GetID(), bddfalse);
    EXPECT_EQ(BDD::True.GetID(), bddtrue);
    EXPECT_EQ(BDD::Null.GetID(), bddnull);
}

TEST_F(BDDTest, ZDDStaticConsts) {
    EXPECT_EQ(ZDD::Empty.GetID(), bddempty);
    EXPECT_EQ(ZDD::Single.GetID(), bddsingle);
    EXPECT_EQ(ZDD::Null.GetID(), bddnull);
}

// --- BDD_NewVar ---

TEST_F(BDDTest, NewVar) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    EXPECT_EQ(v1, 1u);
    EXPECT_EQ(v2, 2u);
    EXPECT_EQ(v3, 3u);
}

TEST_F(BDDTest, NewVarMultiple) {
    std::vector<bddvar> vars = bddnewvar(5);
    EXPECT_EQ(vars.size(), 5u);
    EXPECT_EQ(vars[0], 1u);
    EXPECT_EQ(vars[1], 2u);
    EXPECT_EQ(vars[2], 3u);
    EXPECT_EQ(vars[3], 4u);
    EXPECT_EQ(vars[4], 5u);
    EXPECT_EQ(bddvarused(), 5u);
}

TEST_F(BDDTest, NewVarMultipleZero) {
    std::vector<bddvar> vars = bddnewvar(0);
    EXPECT_EQ(vars.size(), 0u);
    EXPECT_EQ(bddvarused(), 0u);
}

// --- bddprime ---

TEST_F(BDDTest, BddPrime) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_NE(p, bddfalse);
    EXPECT_NE(p, bddtrue);
    EXPECT_NE(p, bddnull);
    // p should be an even number (regular node)
    EXPECT_EQ(p & BDD_COMP_FLAG, 0u);
    EXPECT_EQ(p & BDD_CONST_FLAG, 0u);
}

TEST_F(BDDTest, BddPrimeSameVar) {
    bddvar v = BDD_NewVar();
    bddp p1 = bddprime(v);
    bddp p2 = bddprime(v);
    EXPECT_EQ(p1, p2);  // unique table should return the same node
}

// --- BDDvar ---

TEST_F(BDDTest, BDDvar) {
    bddvar v = BDD_NewVar();
    BDD b = BDDvar(v);
    EXPECT_EQ(b.GetID(), bddprime(v));
}

// --- BDD::prime / BDD::prime_not ---

TEST_F(BDDTest, BDD_Prime) {
    bddvar v = BDD_NewVar();
    BDD b = BDD::prime(v);
    EXPECT_EQ(b.GetID(), bddprime(v));
}

TEST_F(BDDTest, BDD_PrimeNot) {
    bddvar v = BDD_NewVar();
    BDD b = BDD::prime_not(v);
    BDD p = BDD::prime(v);
    EXPECT_EQ(b.GetID(), bddnot(p.GetID()));
    // prime_not is the complement of prime
    EXPECT_EQ(b, ~p);
}

// --- BDD::cube / BDD::clause ---

TEST_F(BDDTest, BDD_Cube_Empty) {
    // Empty cube → true
    BDD f = BDD::cube({});
    EXPECT_EQ(f, BDD::True);
}

TEST_F(BDDTest, BDD_Cube_SinglePositive) {
    BDD f = BDD::cube({1});
    EXPECT_EQ(f, BDD::prime(1));
}

TEST_F(BDDTest, BDD_Cube_SingleNegative) {
    BDD f = BDD::cube({-1});
    EXPECT_EQ(f, BDD::prime_not(1));
}

TEST_F(BDDTest, BDD_Cube_Mixed) {
    // x1 ∧ ¬x2 ∧ x3
    BDD f = BDD::cube({1, -2, 3});
    BDD expected = BDD::prime(1) & BDD::prime_not(2) & BDD::prime(3);
    EXPECT_EQ(f, expected);
}

TEST_F(BDDTest, BDD_Clause_Empty) {
    // Empty clause → false
    BDD f = BDD::clause({});
    EXPECT_EQ(f, BDD::False);
}

TEST_F(BDDTest, BDD_Clause_SinglePositive) {
    BDD f = BDD::clause({1});
    EXPECT_EQ(f, BDD::prime(1));
}

TEST_F(BDDTest, BDD_Clause_SingleNegative) {
    BDD f = BDD::clause({-2});
    EXPECT_EQ(f, BDD::prime_not(2));
}

TEST_F(BDDTest, BDD_Clause_Mixed) {
    // x1 ∨ ¬x2 ∨ x3
    BDD f = BDD::clause({1, -2, 3});
    BDD expected = BDD::prime(1) | BDD::prime_not(2) | BDD::prime(3);
    EXPECT_EQ(f, expected);
}

TEST_F(BDDTest, BDD_Cube_ZeroLiteral) {
    EXPECT_THROW(BDD::cube({0}), std::invalid_argument);
}

TEST_F(BDDTest, BDD_Clause_ZeroLiteral) {
    EXPECT_THROW(BDD::clause({0}), std::invalid_argument);
}

TEST_F(BDDTest, BDD_Cube_INT_MIN) {
    bddnewvar();
    EXPECT_THROW(BDD::cube({INT_MIN}), std::invalid_argument);
}

TEST_F(BDDTest, BDD_Clause_INT_MIN) {
    bddnewvar();
    EXPECT_THROW(BDD::clause({INT_MIN}), std::invalid_argument);
}

// --- BDD_ID ---

TEST_F(BDDTest, BDD_ID) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    BDD b = BDD_ID(p);
    EXPECT_EQ(b.GetID(), p);
}

// --- bddnot ---

TEST_F(BDDTest, BddNot) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    bddp np = bddnot(p);
    EXPECT_NE(p, np);
    EXPECT_EQ(bddnot(np), p);  // double negation
    EXPECT_EQ(np, p ^ BDD_COMP_FLAG);
}

TEST_F(BDDTest, BddNotTerminals) {
    EXPECT_EQ(bddnot(bddfalse), bddtrue);
    EXPECT_EQ(bddnot(bddtrue), bddfalse);
}

// --- bddand: terminal cases ---

TEST_F(BDDTest, BddAndTerminals) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);

    EXPECT_EQ(bddand(bddfalse, bddfalse), bddfalse);
    EXPECT_EQ(bddand(bddfalse, bddtrue), bddfalse);
    EXPECT_EQ(bddand(bddtrue, bddfalse), bddfalse);
    EXPECT_EQ(bddand(bddtrue, bddtrue), bddtrue);

    EXPECT_EQ(bddand(p, bddfalse), bddfalse);
    EXPECT_EQ(bddand(bddfalse, p), bddfalse);
    EXPECT_EQ(bddand(p, bddtrue), p);
    EXPECT_EQ(bddand(bddtrue, p), p);
}

TEST_F(BDDTest, BddAndSelf) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddand(p, p), p);
}

TEST_F(BDDTest, BddAndComplement) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddand(p, bddnot(p)), bddfalse);
}

// --- bddand: two variables ---

TEST_F(BDDTest, BddAndTwoVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp result = bddand(p1, p2);

    // result should not be a terminal
    EXPECT_EQ(result & BDD_CONST_FLAG, 0u);

    // AND of same inputs should give same result (unique table)
    EXPECT_EQ(bddand(p1, p2), result);
}

TEST_F(BDDTest, BddAndCommutativity) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddand(p1, p2), bddand(p2, p1));
}

TEST_F(BDDTest, BddAndAssociativity) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    EXPECT_EQ(bddand(bddand(p1, p2), p3), bddand(p1, bddand(p2, p3)));
}

// --- bddand with complement edges ---

TEST_F(BDDTest, BddAndWithNot) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);

    // x1 AND (NOT x2) should not be trivial
    bddp result = bddand(p1, bddnot(p2));
    EXPECT_NE(result, bddfalse);
    EXPECT_NE(result, bddtrue);

    // (NOT x1) AND (NOT x1) == NOT x1
    EXPECT_EQ(bddand(bddnot(p1), bddnot(p1)), bddnot(p1));
}

// --- getnode reduction rule ---

TEST_F(BDDTest, GetnodeReduction) {
    bddvar v = BDD_NewVar();
    // BDD::getnode(v, bddtrue, bddtrue) should return bddtrue (reduction)
    bddp result = BDD::getnode(v, bddtrue, bddtrue);
    EXPECT_EQ(result, bddtrue);

    // BDD::getnode(v, bddfalse, bddfalse) should return bddfalse
    result = BDD::getnode(v, bddfalse, bddfalse);
    EXPECT_EQ(result, bddfalse);
}

// --- BDD operator& and operator&= ---

TEST_F(BDDTest, OperatorAnd) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD c = a & b;
    EXPECT_EQ(c.GetID(), bddand(a.GetID(), b.GetID()));
}

TEST_F(BDDTest, OperatorAndAssign) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    bddp expected = bddand(a.GetID(), b.GetID());
    a &= b;
    EXPECT_EQ(a.GetID(), expected);
}

TEST_F(BDDTest, OperatorAndWithFalse) {
    bddvar v = BDD_NewVar();
    BDD a = BDDvar(v);
    BDD f = BDD::False;
    BDD result = a & f;
    EXPECT_EQ(result.GetID(), bddfalse);
}

TEST_F(BDDTest, OperatorAndWithTrue) {
    bddvar v = BDD_NewVar();
    BDD a = BDDvar(v);
    BDD t = BDD::True;
    BDD result = a & t;
    EXPECT_EQ(result.GetID(), a.GetID());
}

// --- bddand: parameterized tests for Recursive/Iterative modes ---

class BddAndModeTest : public ::testing::TestWithParam<BddExecMode> {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }

    bddp bddand_mode(bddp f, bddp g) {
        return bddand(f, g, GetParam());
    }
};

INSTANTIATE_TEST_SUITE_P(
    ExecModes,
    BddAndModeTest,
    ::testing::Values(BddExecMode::Recursive, BddExecMode::Iterative, BddExecMode::Auto),
    [](const ::testing::TestParamInfo<BddExecMode>& info) {
        switch (info.param) {
        case BddExecMode::Recursive: return "Recursive";
        case BddExecMode::Iterative: return "Iterative";
        case BddExecMode::Auto: return "Auto";
        }
        return "Unknown";
    }
);

TEST_P(BddAndModeTest, Terminals) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);

    EXPECT_EQ(bddand_mode(bddfalse, bddfalse), bddfalse);
    EXPECT_EQ(bddand_mode(bddfalse, bddtrue), bddfalse);
    EXPECT_EQ(bddand_mode(bddtrue, bddfalse), bddfalse);
    EXPECT_EQ(bddand_mode(bddtrue, bddtrue), bddtrue);

    EXPECT_EQ(bddand_mode(p, bddfalse), bddfalse);
    EXPECT_EQ(bddand_mode(bddfalse, p), bddfalse);
    EXPECT_EQ(bddand_mode(p, bddtrue), p);
    EXPECT_EQ(bddand_mode(bddtrue, p), p);
}

TEST_P(BddAndModeTest, Self) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddand_mode(p, p), p);
}

TEST_P(BddAndModeTest, Complement) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddand_mode(p, bddnot(p)), bddfalse);
}

TEST_P(BddAndModeTest, TwoVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp result = bddand_mode(p1, p2);

    EXPECT_EQ(result & BDD_CONST_FLAG, 0u);
    EXPECT_EQ(bddand_mode(p1, p2), result);
}

TEST_P(BddAndModeTest, Commutativity) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddand_mode(p1, p2), bddand_mode(p2, p1));
}

TEST_P(BddAndModeTest, Associativity) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    EXPECT_EQ(bddand_mode(bddand_mode(p1, p2), p3),
              bddand_mode(p1, bddand_mode(p2, p3)));
}

TEST_P(BddAndModeTest, WithNot) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);

    bddp result = bddand_mode(p1, bddnot(p2));
    EXPECT_NE(result, bddfalse);
    EXPECT_NE(result, bddtrue);

    EXPECT_EQ(bddand_mode(bddnot(p1), bddnot(p1)), bddnot(p1));
}

TEST_P(BddAndModeTest, CrossValidation) {
    // Build a moderately complex BDD and verify both modes produce the same result
    const int n = 10;
    for (int i = 0; i < n; ++i) BDD_NewVar();

    // f = x1 & x2 & ... & x5
    bddp f = bddtrue;
    for (int i = 1; i <= 5; ++i)
        f = bddand_mode(f, bddprime(i));

    // g = x3 & x4 & ... & x8
    bddp g = bddtrue;
    for (int i = 3; i <= 8; ++i)
        g = bddand_mode(g, bddprime(i));

    bddp result = bddand_mode(f, g);

    // Cross-validate against the 2-argument (recursive) version
    bddp f2 = bddtrue;
    for (int i = 1; i <= 5; ++i)
        f2 = bddand(f2, bddprime(i));
    bddp g2 = bddtrue;
    for (int i = 3; i <= 8; ++i)
        g2 = bddand(g2, bddprime(i));
    bddp expected = bddand(f2, g2);

    EXPECT_EQ(result, expected);
}

TEST_P(BddAndModeTest, CacheSharing) {
    // Verify that a result cached by one mode is found by the other
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);

    // Compute with current mode
    bddp r1 = bddand_mode(bddand_mode(p1, p2), p3);

    // Compute with original 2-arg recursive version
    bddp r2 = bddand(bddand(p1, p2), p3);

    EXPECT_EQ(r1, r2);
}

// --- bddxor: parameterized tests for Recursive/Iterative modes ---

class BddXorModeTest : public ::testing::TestWithParam<BddExecMode> {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }

    bddp bddxor_mode(bddp f, bddp g) {
        return bddxor(f, g, GetParam());
    }
};

INSTANTIATE_TEST_SUITE_P(
    ExecModes,
    BddXorModeTest,
    ::testing::Values(BddExecMode::Recursive, BddExecMode::Iterative, BddExecMode::Auto),
    [](const ::testing::TestParamInfo<BddExecMode>& info) {
        switch (info.param) {
        case BddExecMode::Recursive: return "Recursive";
        case BddExecMode::Iterative: return "Iterative";
        case BddExecMode::Auto: return "Auto";
        }
        return "Unknown";
    }
);

TEST_P(BddXorModeTest, Terminals) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);

    EXPECT_EQ(bddxor_mode(bddfalse, bddfalse), bddfalse);
    EXPECT_EQ(bddxor_mode(bddfalse, bddtrue), bddtrue);
    EXPECT_EQ(bddxor_mode(bddtrue, bddfalse), bddtrue);
    EXPECT_EQ(bddxor_mode(bddtrue, bddtrue), bddfalse);

    EXPECT_EQ(bddxor_mode(p, bddfalse), p);
    EXPECT_EQ(bddxor_mode(bddfalse, p), p);
    EXPECT_EQ(bddxor_mode(p, bddtrue), bddnot(p));
    EXPECT_EQ(bddxor_mode(bddtrue, p), bddnot(p));
}

TEST_P(BddXorModeTest, Self) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddxor_mode(p, p), bddfalse);
    EXPECT_EQ(bddxor_mode(p, bddnot(p)), bddtrue);
}

TEST_P(BddXorModeTest, Commutativity) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddxor_mode(p1, p2), bddxor_mode(p2, p1));
}

TEST_P(BddXorModeTest, Associativity) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    EXPECT_EQ(bddxor_mode(bddxor_mode(p1, p2), p3),
              bddxor_mode(p1, bddxor_mode(p2, p3)));
}

TEST_P(BddXorModeTest, ComplementIdentities) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    // XOR(~a, b) = ~XOR(a, b)
    EXPECT_EQ(bddxor_mode(bddnot(p1), p2), bddnot(bddxor_mode(p1, p2)));
    // XOR(a, ~b) = ~XOR(a, b)
    EXPECT_EQ(bddxor_mode(p1, bddnot(p2)), bddnot(bddxor_mode(p1, p2)));
    // XOR(~a, ~b) = XOR(a, b)
    EXPECT_EQ(bddxor_mode(bddnot(p1), bddnot(p2)), bddxor_mode(p1, p2));
}

TEST_P(BddXorModeTest, CrossValidation) {
    // Build a moderately complex BDD and verify modes agree with the 2-arg version
    const int n = 10;
    for (int i = 0; i < n; ++i) BDD_NewVar();

    bddp f = bddfalse;
    for (int i = 1; i <= 5; ++i)
        f = bddxor_mode(f, bddprime(i));

    bddp g = bddfalse;
    for (int i = 3; i <= 8; ++i)
        g = bddxor_mode(g, bddprime(i));

    bddp result = bddxor_mode(f, g);

    bddp f2 = bddfalse;
    for (int i = 1; i <= 5; ++i)
        f2 = bddxor(f2, bddprime(i));
    bddp g2 = bddfalse;
    for (int i = 3; i <= 8; ++i)
        g2 = bddxor(g2, bddprime(i));
    bddp expected = bddxor(f2, g2);

    EXPECT_EQ(result, expected);
}

// --- BDD operator| ---

TEST_F(BDDTest, OperatorOr) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD c = a | b;
    EXPECT_EQ(c.GetID(), bddor(a.GetID(), b.GetID()));
}

TEST_F(BDDTest, OperatorOrAssign) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    bddp expected = bddor(a.GetID(), b.GetID());
    a |= b;
    EXPECT_EQ(a.GetID(), expected);
}

// --- BDD operator^ ---

TEST_F(BDDTest, OperatorXor) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD c = a ^ b;
    EXPECT_EQ(c.GetID(), bddxor(a.GetID(), b.GetID()));
}

TEST_F(BDDTest, OperatorXorAssign) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    bddp expected = bddxor(a.GetID(), b.GetID());
    a ^= b;
    EXPECT_EQ(a.GetID(), expected);
}

// --- BDD operator~ ---

TEST_F(BDDTest, OperatorNot) {
    bddvar v = BDD_NewVar();
    BDD a = BDDvar(v);
    BDD c = ~a;
    EXPECT_EQ(c.GetID(), bddnot(a.GetID()));
}

TEST_F(BDDTest, OperatorDoubleNot) {
    bddvar v = BDD_NewVar();
    BDD a = BDDvar(v);
    BDD c = ~(~a);
    EXPECT_EQ(c.GetID(), a.GetID());
}

// --- BDD operator== / operator!= ---

TEST_F(BDDTest, BDDOperatorEqual) {
    bddvar v1 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v1);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST_F(BDDTest, BDDOperatorNotEqual) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST_F(BDDTest, BDDOperatorEqualConstants) {
    EXPECT_TRUE(BDD::False == BDD::False);
    EXPECT_TRUE(BDD::True == BDD::True);
    EXPECT_FALSE(BDD::False == BDD::True);
    EXPECT_TRUE(BDD::False != BDD::True);
}

// --- BDD operator<< / operator>> ---

TEST_F(BDDTest, OperatorLshift) {
    bddvar v1 = BDD_NewVar();
    (void)BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD c = a << 1;
    EXPECT_EQ(c.GetID(), bddlshiftb(a.GetID(), 1));
}

TEST_F(BDDTest, OperatorLshiftAssign) {
    bddvar v1 = BDD_NewVar();
    (void)BDD_NewVar();
    BDD a = BDDvar(v1);
    bddp expected = bddlshiftb(a.GetID(), 1);
    a <<= 1;
    EXPECT_EQ(a.GetID(), expected);
}

TEST_F(BDDTest, OperatorRshift) {
    (void)BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v2);
    BDD c = a >> 1;
    EXPECT_EQ(c.GetID(), bddrshiftb(a.GetID(), 1));
}

TEST_F(BDDTest, OperatorRshiftAssign) {
    (void)BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v2);
    bddp expected = bddrshiftb(a.GetID(), 1);
    a >>= 1;
    EXPECT_EQ(a.GetID(), expected);
}

// --- Operation cache (bddrcache / bddwcache) ---

TEST_F(BDDTest, CacheMissReturnsNull) {
    bddvar v1 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    // No prior write: should miss
    EXPECT_EQ(bddrcache(BDD_OP_AND, p1, p1), bddnull);
}

TEST_F(BDDTest, CacheWriteAndRead) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp result = bddand(p1, p2);

    // Write to cache and read back
    bddwcache(42, p1, p2, result);
    EXPECT_EQ(bddrcache(42, p1, p2), result);

    // Different op should miss
    EXPECT_EQ(bddrcache(43, p1, p2), bddnull);
}

TEST_F(BDDTest, CacheUsedByBddand) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);

    // Compute a complex expression that reuses subexpressions
    bddp ab = bddand(p1, p2);
    bddp abc = bddand(ab, p3);

    // Recompute: should give same result (cache or unique table)
    EXPECT_EQ(bddand(bddand(p1, p2), p3), abc);
    EXPECT_EQ(bddand(p1, bddand(p2, p3)), abc);  // associativity
}

// --- bddnewvaroflev ---

TEST_F(BDDTest, NewVarOfLevBasic) {
    bddvar v1 = BDD_NewVar();  // var=1, level=1
    bddvar v2 = BDD_NewVar();  // var=2, level=2

    // Insert new var at level 1
    bddvar v3 = bddnewvaroflev(1);
    EXPECT_EQ(v3, 3u);

    // v3 should be at level 1
    EXPECT_EQ(var2level[v3], 1u);
    // v1 was at level 1, now shifted to level 2
    EXPECT_EQ(var2level[v1], 2u);
    // v2 was at level 2, now shifted to level 3
    EXPECT_EQ(var2level[v2], 3u);

    // Reverse mapping
    EXPECT_EQ(level2var[1], v3);
    EXPECT_EQ(level2var[2], v1);
    EXPECT_EQ(level2var[3], v2);
}

TEST_F(BDDTest, NewVarOfLevAtEnd) {
    bddvar v1 = BDD_NewVar();  // var=1, level=1
    bddvar v2 = BDD_NewVar();  // var=2, level=2

    // Insert at level 3 (one past current max)
    bddvar v3 = bddnewvaroflev(3);
    EXPECT_EQ(v3, 3u);

    // No shifting needed
    EXPECT_EQ(var2level[v1], 1u);
    EXPECT_EQ(var2level[v2], 2u);
    EXPECT_EQ(var2level[v3], 3u);

    EXPECT_EQ(level2var[1], v1);
    EXPECT_EQ(level2var[2], v2);
    EXPECT_EQ(level2var[3], v3);
}

TEST_F(BDDTest, NewVarOfLevMiddle) {
    bddvar v1 = BDD_NewVar();  // var=1, level=1
    bddvar v2 = BDD_NewVar();  // var=2, level=2
    bddvar v3 = BDD_NewVar();  // var=3, level=3

    // Insert at level 2
    bddvar v4 = bddnewvaroflev(2);
    EXPECT_EQ(v4, 4u);

    EXPECT_EQ(var2level[v1], 1u);  // unchanged
    EXPECT_EQ(var2level[v4], 2u);  // new var at level 2
    EXPECT_EQ(var2level[v2], 3u);  // shifted from 2 to 3
    EXPECT_EQ(var2level[v3], 4u);  // shifted from 3 to 4

    EXPECT_EQ(level2var[1], v1);
    EXPECT_EQ(level2var[2], v4);
    EXPECT_EQ(level2var[3], v2);
    EXPECT_EQ(level2var[4], v3);
}

TEST_F(BDDTest, NewVarOfLevInvalidRange) {
    BDD_NewVar();  // var=1

    // lev=0 is invalid (terminal level)
    EXPECT_THROW(bddnewvaroflev(0), std::invalid_argument);

    // lev=3 is invalid (current varcount=1, max valid lev=2)
    EXPECT_THROW(bddnewvaroflev(3), std::invalid_argument);
}

TEST_F(BDDTest, NewVarOfLevNoVarsYet) {
    // No vars exist yet, lev=1 is the only valid value
    bddvar v1 = bddnewvaroflev(1);
    EXPECT_EQ(v1, 1u);
    EXPECT_EQ(var2level[v1], 1u);
    EXPECT_EQ(level2var[1], v1);
}

TEST_F(BDDTest, NewVarOfLevInvalidatesCache) {
    bddvar v1 = BDD_NewVar();  // var=1, level=1
    bddvar v2 = BDD_NewVar();  // var=2, level=2
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);

    // Compute and cache: bddand uses level ordering internally
    bddp r1 = bddand(p1, p2);
    // Top variable should be v2 (level 2 > level 1)
    EXPECT_EQ(bddtop(r1), v2);

    // Insert new var at level 1, shifting v1 to level 2, v2 to level 3
    bddvar v3 = bddnewvaroflev(1);
    (void)v3;

    // Recompute: if cache was not cleared, stale result could be returned
    bddp r2 = bddand(p1, p2);
    // After reordering, v2 has level 3 (highest), so it should still be top
    EXPECT_EQ(bddtop(r2), v2);
}

// --- bddlevofvar / bddvaroflev / bddvarused ---

TEST_F(BDDTest, BddVarUsedEmpty) {
    EXPECT_EQ(bddvarused(), 0u);
}

TEST_F(BDDTest, BddVarUsedAfterNewVar) {
    BDD_NewVar();
    EXPECT_EQ(bddvarused(), 1u);
    BDD_NewVar();
    BDD_NewVar();
    EXPECT_EQ(bddvarused(), 3u);
}

TEST_F(BDDTest, BddLevOfVarDefault) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    // Default mapping: var i <-> level i
    EXPECT_EQ(bddlevofvar(v1), 1u);
    EXPECT_EQ(bddlevofvar(v2), 2u);
    EXPECT_EQ(bddlevofvar(v3), 3u);
}

TEST_F(BDDTest, BddVarOfLevDefault) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    EXPECT_EQ(bddvaroflev(1), v1);
    EXPECT_EQ(bddvaroflev(2), v2);
    EXPECT_EQ(bddvaroflev(3), v3);
}

TEST_F(BDDTest, BddLevOfVarAfterNewVarOfLev) {
    bddvar v1 = BDD_NewVar();  // var=1, level=1
    bddvar v2 = BDD_NewVar();  // var=2, level=2
    bddvar v3 = bddnewvaroflev(1);  // var=3, level=1; v1->2, v2->3
    EXPECT_EQ(bddlevofvar(v3), 1u);
    EXPECT_EQ(bddlevofvar(v1), 2u);
    EXPECT_EQ(bddlevofvar(v2), 3u);
    EXPECT_EQ(bddvaroflev(1), v3);
    EXPECT_EQ(bddvaroflev(2), v1);
    EXPECT_EQ(bddvaroflev(3), v2);
}

TEST_F(BDDTest, BddLevOfVarZero) {
    EXPECT_EQ(bddlevofvar(0), 0u);
}

TEST_F(BDDTest, BddLevOfVarInvalidRange) {
    BDD_NewVar();
    EXPECT_THROW(bddlevofvar(2), std::invalid_argument);
}

TEST_F(BDDTest, BddVarOfLevZero) {
    EXPECT_EQ(bddvaroflev(0), 0u);
}

TEST_F(BDDTest, BddVarOfLevInvalidRange) {
    BDD_NewVar();
    EXPECT_THROW(bddvaroflev(2), std::invalid_argument);
}

// --- bddtop ---

TEST_F(BDDTest, BddTopTerminals) {
    EXPECT_EQ(bddtop(bddfalse), 0u);
    EXPECT_EQ(bddtop(bddtrue), 0u);
}

TEST_F(BDDTest, BddTopVariable) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddtop(p1), v1);
    EXPECT_EQ(bddtop(p2), v2);
}

TEST_F(BDDTest, BddTopComplement) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddtop(bddnot(p)), v);
}

TEST_F(BDDTest, BddTopAndResult) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp ab = bddand(p1, p2);
    // v2 has higher level (level 2 > level 1), so it is the top variable
    EXPECT_EQ(bddtop(ab), v2);
}

TEST_F(BDDTest, BddTopNull) {
    EXPECT_THROW(bddtop(bddnull), std::invalid_argument);
}

// --- bddcopy ---

TEST_F(BDDTest, BddCopy) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddcopy(p), p);
    EXPECT_EQ(bddcopy(bddnot(p)), bddnot(p));
    EXPECT_EQ(bddcopy(bddfalse), bddfalse);
    EXPECT_EQ(bddcopy(bddtrue), bddtrue);
}

// --- bddor ---

TEST_F(BDDTest, BddOrTerminals) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddor(bddfalse, bddfalse), bddfalse);
    EXPECT_EQ(bddor(bddfalse, bddtrue), bddtrue);
    EXPECT_EQ(bddor(bddtrue, bddfalse), bddtrue);
    EXPECT_EQ(bddor(bddtrue, bddtrue), bddtrue);
    EXPECT_EQ(bddor(p, bddfalse), p);
    EXPECT_EQ(bddor(p, bddtrue), bddtrue);
}

TEST_F(BDDTest, BddOrSelfAndComplement) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddor(p, p), p);
    EXPECT_EQ(bddor(p, bddnot(p)), bddtrue);
}

TEST_F(BDDTest, BddOrTwoVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp result = bddor(p1, p2);
    // OR(a,b) = ~(~a & ~b) = ~AND(~a,~b)
    EXPECT_EQ(result, bddnot(bddand(bddnot(p1), bddnot(p2))));
    // Commutativity
    EXPECT_EQ(bddor(p1, p2), bddor(p2, p1));
}

// --- bddxor ---

TEST_F(BDDTest, BddXorTerminals) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddxor(bddfalse, bddfalse), bddfalse);
    EXPECT_EQ(bddxor(bddfalse, bddtrue), bddtrue);
    EXPECT_EQ(bddxor(bddtrue, bddfalse), bddtrue);
    EXPECT_EQ(bddxor(bddtrue, bddtrue), bddfalse);
    EXPECT_EQ(bddxor(p, bddfalse), p);
    EXPECT_EQ(bddxor(p, bddtrue), bddnot(p));
}

TEST_F(BDDTest, BddXorSelfAndComplement) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddxor(p, p), bddfalse);
    EXPECT_EQ(bddxor(p, bddnot(p)), bddtrue);
}

TEST_F(BDDTest, BddXorCommutativity) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddxor(p1, p2), bddxor(p2, p1));
}

TEST_F(BDDTest, BddXorAssociativity) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    EXPECT_EQ(bddxor(bddxor(p1, p2), p3), bddxor(p1, bddxor(p2, p3)));
}

TEST_F(BDDTest, BddXorWithNot) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    // XOR(~a, b) = ~XOR(a, b)
    EXPECT_EQ(bddxor(bddnot(p1), p2), bddnot(bddxor(p1, p2)));
    // XOR(a, ~b) = ~XOR(a, b)
    EXPECT_EQ(bddxor(p1, bddnot(p2)), bddnot(bddxor(p1, p2)));
    // XOR(~a, ~b) = XOR(a, b)
    EXPECT_EQ(bddxor(bddnot(p1), bddnot(p2)), bddxor(p1, p2));
}

// --- bddnand ---

TEST_F(BDDTest, BddNand) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddnand(p1, p2), bddnot(bddand(p1, p2)));
    EXPECT_EQ(bddnand(bddfalse, p1), bddtrue);
    EXPECT_EQ(bddnand(bddtrue, p1), bddnot(p1));
}

// --- bddnor ---

TEST_F(BDDTest, BddNor) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddnor(p1, p2), bddnot(bddor(p1, p2)));
    EXPECT_EQ(bddnor(bddtrue, p1), bddfalse);
    EXPECT_EQ(bddnor(bddfalse, bddfalse), bddtrue);
}

// --- bddxnor ---

TEST_F(BDDTest, BddXnor) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddxnor(p1, p2), bddnot(bddxor(p1, p2)));
    EXPECT_EQ(bddxnor(p1, p1), bddtrue);
    EXPECT_EQ(bddxnor(p1, bddnot(p1)), bddfalse);
}

// --- bddsize ---

TEST_F(BDDTest, BddSizeTerminals) {
    EXPECT_EQ(bddsize(bddfalse), 0u);
    EXPECT_EQ(bddsize(bddtrue), 0u);
}

TEST_F(BDDTest, BddSizeSingleVar) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddsize(p), 1u);
    EXPECT_EQ(bddsize(bddnot(p)), 1u);
}

TEST_F(BDDTest, BddSizeAnd) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);  // v2 -> (v1 -> 1, 0), 0
    EXPECT_EQ(bddsize(f), 2u);
}

TEST_F(BDDTest, BddSizeOr) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddor(p1, p2);  // complement of AND(~v1,~v2), shares structure
    EXPECT_EQ(bddsize(f), 2u);
}

TEST_F(BDDTest, BddSizeThreeVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp f = bddand(bddand(p1, p2), p3);
    EXPECT_EQ(bddsize(f), 3u);
}

// --- bddvsize ---

TEST_F(BDDTest, BddVsizeEmpty) {
    bddp arr[1] = {bddfalse};
    EXPECT_EQ(bddvsize(arr, 0), 0u);
}

TEST_F(BDDTest, BddVsizeSingle) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    bddp arr[1] = {p};
    EXPECT_EQ(bddvsize(arr, 1), 1u);
}

TEST_F(BDDTest, BddVsizeSharedNodes) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);   // 2 nodes (v2, v1)
    bddp g = bddor(p1, p2);    // shares the v1 node with f

    // Individually
    EXPECT_EQ(bddsize(f), 2u);
    EXPECT_EQ(bddsize(g), 2u);

    // Together: v1 node is shared, so total is 3 not 4
    bddp arr[2] = {f, g};
    EXPECT_EQ(bddvsize(arr, 2), 3u);
}

TEST_F(BDDTest, BddVsizeVector) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    bddp g = bddor(p1, p2);

    std::vector<bddp> vec = {f, g};
    EXPECT_EQ(bddvsize(vec), 3u);

    // Empty vector
    std::vector<bddp> empty_vec;
    EXPECT_EQ(bddvsize(empty_vec), 0u);
}

// --- bddfree ---

TEST_F(BDDTest, BddFreeNoOp) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    bddfree(p);
    // p should still be valid after bddfree (no-op)
    EXPECT_EQ(bddtop(p), v);
}

// --- bddused ---

TEST_F(BDDTest, BddUsedEmpty) {
    EXPECT_EQ(bddused(), 0u);
}

TEST_F(BDDTest, BddUsedAfterNodes) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);  // 1 node
    EXPECT_EQ(bddused(), 1u);
    bddp p2 = bddprime(v2);  // 2 nodes
    EXPECT_EQ(bddused(), 2u);
    (void)bddand(p1, p2);  // 3 nodes
    EXPECT_EQ(bddused(), 3u);
}

// --- bddat0 / bddat1 ---

TEST_F(BDDTest, BddAt0Terminals) {
    bddvar v = BDD_NewVar();
    EXPECT_EQ(bddat0(bddfalse, v), bddfalse);
    EXPECT_EQ(bddat0(bddtrue, v), bddtrue);
    EXPECT_EQ(bddat1(bddfalse, v), bddfalse);
    EXPECT_EQ(bddat1(bddtrue, v), bddtrue);
}

TEST_F(BDDTest, BddAt0SingleVar) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);  // v ? 1 : 0
    // Substitute 0: take low branch -> bddfalse
    EXPECT_EQ(bddat0(p, v), bddfalse);
    // Substitute 1: take high branch -> bddtrue
    EXPECT_EQ(bddat1(p, v), bddtrue);
}

TEST_F(BDDTest, BddAt0NotVar) {
    bddvar v = BDD_NewVar();
    bddp p = bddnot(bddprime(v));  // ~v
    // ~v at v=0 -> ~(false) = true
    EXPECT_EQ(bddat0(p, v), bddtrue);
    // ~v at v=1 -> ~(true) = false
    EXPECT_EQ(bddat1(p, v), bddfalse);
}

TEST_F(BDDTest, BddAt0VarNotInF) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    // v2 does not appear in p1, so restriction is identity
    EXPECT_EQ(bddat0(p1, v2), p1);
    EXPECT_EQ(bddat1(p1, v2), p1);
}

TEST_F(BDDTest, BddAt0And) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);  // v1 & v2

    // (v1 & v2) at v1=0 -> 0 & v2 = 0
    EXPECT_EQ(bddat0(f, v1), bddfalse);
    // (v1 & v2) at v1=1 -> 1 & v2 = v2
    EXPECT_EQ(bddat1(f, v1), p2);
    // (v1 & v2) at v2=0 -> v1 & 0 = 0
    EXPECT_EQ(bddat0(f, v2), bddfalse);
    // (v1 & v2) at v2=1 -> v1 & 1 = v1
    EXPECT_EQ(bddat1(f, v2), p1);
}

TEST_F(BDDTest, BddAt0Or) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddor(p1, p2);  // v1 | v2

    // (v1 | v2) at v1=0 -> v2
    EXPECT_EQ(bddat0(f, v1), p2);
    // (v1 | v2) at v1=1 -> true
    EXPECT_EQ(bddat1(f, v1), bddtrue);
    // (v1 | v2) at v2=0 -> v1
    EXPECT_EQ(bddat0(f, v2), p1);
    // (v1 | v2) at v2=1 -> true
    EXPECT_EQ(bddat1(f, v2), bddtrue);
}

TEST_F(BDDTest, BddAt0Xor) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddxor(p1, p2);  // v1 ^ v2

    // (v1 ^ v2) at v1=0 -> v2
    EXPECT_EQ(bddat0(f, v1), p2);
    // (v1 ^ v2) at v1=1 -> ~v2
    EXPECT_EQ(bddat1(f, v1), bddnot(p2));
}

TEST_F(BDDTest, BddAt0ThreeVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    // f = (v1 & v2) | v3
    bddp f = bddor(bddand(p1, p2), p3);

    // f at v2=1 -> (v1 & 1) | v3 = v1 | v3
    EXPECT_EQ(bddat1(f, v2), bddor(p1, p3));
    // f at v2=0 -> (v1 & 0) | v3 = v3
    EXPECT_EQ(bddat0(f, v2), p3);
    // f at v3=1 -> true
    EXPECT_EQ(bddat1(f, v3), bddtrue);
    // f at v3=0 -> v1 & v2
    EXPECT_EQ(bddat0(f, v3), bddand(p1, p2));
}

// --- bddite ---

TEST_F(BDDTest, BddIteTerminalF) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // ITE(1, g, h) = g
    EXPECT_EQ(bddite(bddtrue, p, bddfalse), p);
    EXPECT_EQ(bddite(bddtrue, bddfalse, p), bddfalse);
    // ITE(0, g, h) = h
    EXPECT_EQ(bddite(bddfalse, p, bddtrue), bddtrue);
    EXPECT_EQ(bddite(bddfalse, bddfalse, p), p);
}

TEST_F(BDDTest, BddIteIdentity) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // ITE(f, 1, 0) = f
    EXPECT_EQ(bddite(p, bddtrue, bddfalse), p);
    // ITE(f, 0, 1) = ~f
    EXPECT_EQ(bddite(p, bddfalse, bddtrue), bddnot(p));
    // ITE(f, g, g) = g
    EXPECT_EQ(bddite(p, bddtrue, bddtrue), bddtrue);
    EXPECT_EQ(bddite(p, bddfalse, bddfalse), bddfalse);
}

TEST_F(BDDTest, BddIteReducesToAnd) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    // ITE(f, g, 0) = f & g
    EXPECT_EQ(bddite(p1, p2, bddfalse), bddand(p1, p2));
}

TEST_F(BDDTest, BddIteReducesToOr) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    // ITE(f, 1, h) = f | h
    EXPECT_EQ(bddite(p1, bddtrue, p2), bddor(p1, p2));
}

TEST_F(BDDTest, BddIteThreeVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    // ITE(v1, v2, v3) = (v1 & v2) | (~v1 & v3)
    bddp expected = bddor(bddand(p1, p2), bddand(bddnot(p1), p3));
    EXPECT_EQ(bddite(p1, p2, p3), expected);
}

TEST_F(BDDTest, BddIteWithComplement) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    // ITE(~v1, v2, v1) = (~v1 & v2) | (v1 & v1) = (~v1 & v2) | v1 = v1 | v2
    EXPECT_EQ(bddite(bddnot(p1), p2, p1), bddor(p1, p2));
    // ITE(v1, ~v2, v2) = (v1 & ~v2) | (~v1 & v2) = v1 ^ v2
    EXPECT_EQ(bddite(p1, bddnot(p2), p2), bddxor(p1, p2));
}

TEST_F(BDDTest, BddIteMux) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    // ITE as multiplexer: verify consistency with cofactor
    bddp mux = bddite(p1, p2, p3);
    // mux at v1=1 -> v2
    EXPECT_EQ(bddat1(mux, v1), p2);
    // mux at v1=0 -> v3
    EXPECT_EQ(bddat0(mux, v1), p3);
}

TEST_F(BDDTest, BddIteAllComplemented) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    // ITE(~v1, ~v2, ~v3) = (~v1 & ~v2) | (v1 & ~v3)
    bddp expected = bddor(bddand(bddnot(p1), bddnot(p2)),
                          bddand(p1, bddnot(p3)));
    EXPECT_EQ(bddite(bddnot(p1), bddnot(p2), bddnot(p3)), expected);
}

// --- bddsupport ---

TEST_F(BDDTest, BddSupportTerminals) {
    EXPECT_EQ(bddsupport(bddfalse), bddfalse);
    EXPECT_EQ(bddsupport(bddtrue), bddfalse);
}

TEST_F(BDDTest, BddSupportSingleVar) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // support of x is just x itself
    EXPECT_EQ(bddsupport(p), p);
    // support of ~x is also x
    EXPECT_EQ(bddsupport(bddnot(p)), p);
}

TEST_F(BDDTest, BddSupportAnd) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    // support of (x1 & x2) = {x1, x2}, represented as x1 | x2
    bddp sup = bddsupport(f);
    EXPECT_EQ(sup, bddor(p1, p2));
}

TEST_F(BDDTest, BddSupportThreeVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp f = bddor(bddand(p1, p2), p3);
    // support = {v1, v2, v3}
    bddp sup = bddsupport(f);
    EXPECT_EQ(sup, bddor(bddor(p1, p2), p3));
}

TEST_F(BDDTest, BddSupportZeroBranchChain) {
    // Verify the 0-branch chain structure: top -> ... -> lowest -> bddfalse
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddvar v3 = BDD_NewVar();  // level 3
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp f = bddand(bddand(p1, p2), p3);
    bddp sup = bddsupport(f);

    // Top variable is v3 (highest level)
    EXPECT_EQ(bddtop(sup), v3);
    // Follow 0-branch: v3 -> v2
    bddp s1 = bddat0(sup, v3);
    EXPECT_EQ(bddtop(s1), v2);
    // Follow 0-branch: v2 -> v1
    bddp s2 = bddat0(s1, v2);
    EXPECT_EQ(bddtop(s2), v1);
    // Follow 0-branch: v1 -> bddfalse
    bddp s3 = bddat0(s2, v1);
    EXPECT_EQ(s3, bddfalse);

    // All 1-branches lead to bddtrue
    EXPECT_EQ(bddat1(sup, v3), bddtrue);
    EXPECT_EQ(bddat1(s1, v2), bddtrue);
    EXPECT_EQ(bddat1(s2, v1), bddtrue);
}

TEST_F(BDDTest, BddSupportSkippedVar) {
    // f depends on v1 and v3 but not v2
    bddvar v1 = BDD_NewVar();
    (void)BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p3 = bddprime(v3);
    bddp f = bddxor(p1, p3);  // depends on v1, v3 only
    bddp sup = bddsupport(f);
    EXPECT_EQ(sup, bddor(p1, p3));

    // Chain: v3 -> v1 -> bddfalse (v2 is absent)
    EXPECT_EQ(bddtop(sup), v3);
    bddp s1 = bddat0(sup, v3);
    EXPECT_EQ(bddtop(s1), v1);
    EXPECT_EQ(bddat0(s1, v1), bddfalse);
}

// --- bddsupport_vec ---

TEST_F(BDDTest, BddSupportVecTerminals) {
    EXPECT_TRUE(bddsupport_vec(bddfalse).empty());
    EXPECT_TRUE(bddsupport_vec(bddtrue).empty());
}

TEST_F(BDDTest, BddSupportVecSingleVar) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    std::vector<bddvar> sup = bddsupport_vec(p);
    ASSERT_EQ(sup.size(), 1u);
    EXPECT_EQ(sup[0], v);
    // Complement doesn't change support
    EXPECT_EQ(bddsupport_vec(bddnot(p)).size(), 1u);
    EXPECT_EQ(bddsupport_vec(bddnot(p))[0], v);
}

TEST_F(BDDTest, BddSupportVecOrderDescLevel) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddvar v3 = BDD_NewVar();  // level 3
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp f = bddand(bddand(p1, p2), p3);
    std::vector<bddvar> sup = bddsupport_vec(f);
    ASSERT_EQ(sup.size(), 3u);
    // Descending level order: v3 (level 3), v2 (level 2), v1 (level 1)
    EXPECT_EQ(sup[0], v3);
    EXPECT_EQ(sup[1], v2);
    EXPECT_EQ(sup[2], v1);
}

TEST_F(BDDTest, BddSupportVecSkippedVar) {
    bddvar v1 = BDD_NewVar();  // level 1
    BDD_NewVar();               // level 2 (unused)
    bddvar v3 = BDD_NewVar();  // level 3
    bddp p1 = bddprime(v1);
    bddp p3 = bddprime(v3);
    bddp f = bddxor(p1, p3);
    std::vector<bddvar> sup = bddsupport_vec(f);
    ASSERT_EQ(sup.size(), 2u);
    EXPECT_EQ(sup[0], v3);
    EXPECT_EQ(sup[1], v1);
}

// --- bddexist ---

TEST_F(BDDTest, BddExistNoCube) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // No variables to quantify -> return f as-is
    EXPECT_EQ(bddexist(p, bddfalse), p);
    EXPECT_EQ(bddexist(bddfalse, bddfalse), bddfalse);
    EXPECT_EQ(bddexist(bddtrue, bddfalse), bddtrue);
}

TEST_F(BDDTest, BddExistTerminalF) {
    bddvar v = BDD_NewVar();
    bddp cube = bddprime(v);
    EXPECT_EQ(bddexist(bddfalse, cube), bddfalse);
    EXPECT_EQ(bddexist(bddtrue, cube), bddtrue);
}

TEST_F(BDDTest, BddExistSingleVar) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // exist v. v = (v|_{v=0}) | (v|_{v=1}) = 0 | 1 = 1
    EXPECT_EQ(bddexist(p, bddprime(v)), bddtrue);
    // exist v. ~v = (~v|_{v=0}) | (~v|_{v=1}) = 1 | 0 = 1
    EXPECT_EQ(bddexist(bddnot(p), bddprime(v)), bddtrue);
}

TEST_F(BDDTest, BddExistAndOneVar) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);  // v1 & v2
    // exist v1. (v1 & v2) = (0 & v2) | (1 & v2) = 0 | v2 = v2
    EXPECT_EQ(bddexist(f, bddprime(v1)), p2);
    // exist v2. (v1 & v2) = (v1 & 0) | (v1 & 1) = 0 | v1 = v1
    EXPECT_EQ(bddexist(f, bddprime(v2)), p1);
}

TEST_F(BDDTest, BddExistAndAllVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    bddp cube = bddsupport(f);  // {v1, v2}
    // exist v1,v2. (v1 & v2) = true (v1=1,v2=1 makes it true)
    EXPECT_EQ(bddexist(f, cube), bddtrue);
}

TEST_F(BDDTest, BddExistOrOneVar) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddor(p1, p2);  // v1 | v2
    // exist v1. (v1 | v2) = (0 | v2) | (1 | v2) = v2 | 1 = 1
    EXPECT_EQ(bddexist(f, bddprime(v1)), bddtrue);
}

TEST_F(BDDTest, BddExistNotInF) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    // v2 does not appear in p1, so quantifying v2 changes nothing
    EXPECT_EQ(bddexist(p1, bddprime(v2)), p1);
}

TEST_F(BDDTest, BddExistThreeVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    // f = v1 & v2 & v3
    bddp f = bddand(bddand(p1, p2), p3);
    // exist v1. (v1 & v2 & v3) = v2 & v3
    EXPECT_EQ(bddexist(f, bddprime(v1)), bddand(p2, p3));
    // exist v1,v3. (v1 & v2 & v3) = v2
    bddp cube13 = BDD::getnode(v3, bddprime(v1), bddtrue);  // {v1, v3}
    EXPECT_EQ(bddexist(f, cube13), p2);
    // exist v1,v2,v3. (v1 & v2 & v3) = true
    EXPECT_EQ(bddexist(f, bddsupport(f)), bddtrue);
}

TEST_F(BDDTest, BddExistXor) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddxor(p1, p2);  // v1 ^ v2
    // exist v1. (v1 ^ v2) = (0 ^ v2) | (1 ^ v2) = v2 | ~v2 = true
    EXPECT_EQ(bddexist(f, bddprime(v1)), bddtrue);
}

TEST_F(BDDTest, BddExistComplementedCube) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);  // v1 & v2
    bddp cube = bddprime(v1);
    // Complemented cube should give same result as non-complemented
    EXPECT_EQ(bddexist(f, bddnot(cube)), bddexist(f, cube));
}

TEST_F(BDDTest, BddUnivComplementedCube) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddor(p1, p2);  // v1 | v2
    bddp cube = bddprime(v1);
    // Complemented cube should give same result as non-complemented
    EXPECT_EQ(bdduniv(f, bddnot(cube)), bdduniv(f, cube));
}

TEST_F(BDDTest, BddExistComplementedMultiVarCube) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp f = bddand(bddand(p1, p2), p3);
    bddp cube = bddsupport(f);  // {v1, v2, v3}
    EXPECT_EQ(bddexist(f, bddnot(cube)), bddexist(f, cube));
}

TEST_F(BDDTest, BddExistAlwaysFalse) {
    bddvar v1 = BDD_NewVar();
    (void)BDD_NewVar();
    bddp p1 = bddprime(v1);
    // f = v1 & ~v1 = bddfalse; exist over anything is still false
    bddp f = bddand(p1, bddnot(p1));
    EXPECT_EQ(f, bddfalse);
    EXPECT_EQ(bddexist(f, bddprime(v1)), bddfalse);
}

// --- bddexist / bdduniv (vector overload) ---

TEST_F(BDDTest, BddExistVecEmpty) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    std::vector<bddvar> empty;
    EXPECT_EQ(bddexist(p, empty), p);
}

TEST_F(BDDTest, BddExistVecSingle) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    std::vector<bddvar> vars = {v1};
    EXPECT_EQ(bddexist(f, vars), p2);
}

TEST_F(BDDTest, BddExistVecMultiple) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp f = bddand(bddand(p1, p2), p3);
    // exist v1,v3. (v1 & v2 & v3) = v2
    std::vector<bddvar> vars = {v1, v3};
    EXPECT_EQ(bddexist(f, vars), p2);
}

TEST_F(BDDTest, BddExistVecMatchesCube) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp f = bddor(bddand(p1, p2), p3);
    std::vector<bddvar> vars = {v2, v3};
    bddp cube = BDD::getnode(v3, bddprime(v2), bddtrue);
    EXPECT_EQ(bddexist(f, vars), bddexist(f, cube));
}

TEST_F(BDDTest, BddUnivVecEmpty) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    std::vector<bddvar> empty;
    EXPECT_EQ(bdduniv(p, empty), p);
}

TEST_F(BDDTest, BddUnivVecSingle) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddor(p1, p2);
    // forall v1. (v1 | v2) = v2
    std::vector<bddvar> vars = {v1};
    EXPECT_EQ(bdduniv(f, vars), p2);
}

TEST_F(BDDTest, BddUnivVecMultiple) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp f = bddor(bddor(p1, p2), p3);
    // forall v1,v3. (v1 | v2 | v3) = v2
    std::vector<bddvar> vars = {v1, v3};
    EXPECT_EQ(bdduniv(f, vars), p2);
}

TEST_F(BDDTest, BddUnivVecMatchesCube) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp f = bddor(bddand(p1, p2), p3);
    std::vector<bddvar> vars = {v2, v3};
    bddp cube = BDD::getnode(v3, bddprime(v2), bddtrue);
    EXPECT_EQ(bdduniv(f, vars), bdduniv(f, cube));
}

// --- bddexist/bdduniv single variable overload ---

TEST_F(BDDTest, BddExistSingleVarOverload) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    // exist v1. (v1 & v2) = v2
    EXPECT_EQ(bddexistvar(f, v1), p2);
    EXPECT_EQ(bddexistvar(f, v1), bddexist(f, bddprime(v1)));
}

TEST_F(BDDTest, BddUnivSingleVarOverload) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddor(p1, p2);
    // forall v1. (v1 | v2) = v2
    EXPECT_EQ(bddunivvar(f, v1), p2);
    EXPECT_EQ(bddunivvar(f, v1), bdduniv(f, bddprime(v1)));
}

