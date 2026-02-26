#include <gtest/gtest.h>
#include "bdd.h"

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

// --- BDD/ZDD constructors ---

TEST_F(BDDTest, BDDConstructor) {
    BDD f(0);
    BDD t(1);
    BDD n(-1);
    EXPECT_EQ(f.root, bddfalse);
    EXPECT_EQ(t.root, bddtrue);
    EXPECT_EQ(n.root, bddnull);
}

TEST_F(BDDTest, ZDDConstructor) {
    ZDD e(0);
    ZDD s(1);
    ZDD n(-1);
    EXPECT_EQ(e.root, bddempty);
    EXPECT_EQ(s.root, bddsingle);
    EXPECT_EQ(n.root, bddnull);
}

// --- Static const objects ---

TEST_F(BDDTest, BDDStaticConsts) {
    EXPECT_EQ(BDD::False.root, bddfalse);
    EXPECT_EQ(BDD::True.root, bddtrue);
    EXPECT_EQ(BDD::Null.root, bddnull);
}

TEST_F(BDDTest, ZDDStaticConsts) {
    EXPECT_EQ(ZDD::Empty.root, bddempty);
    EXPECT_EQ(ZDD::Single.root, bddsingle);
    EXPECT_EQ(ZDD::Null.root, bddnull);
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
    EXPECT_EQ(b.root, bddprime(v));
}

// --- BDD_ID ---

TEST_F(BDDTest, BDD_ID) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    BDD b = BDD_ID(p);
    EXPECT_EQ(b.root, p);
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
    // getnode(v, bddtrue, bddtrue) should return bddtrue (reduction)
    bddp result = getnode(v, bddtrue, bddtrue);
    EXPECT_EQ(result, bddtrue);

    // getnode(v, bddfalse, bddfalse) should return bddfalse
    result = getnode(v, bddfalse, bddfalse);
    EXPECT_EQ(result, bddfalse);
}

// --- BDD operator& and operator&= ---

TEST_F(BDDTest, OperatorAnd) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD c = a & b;
    EXPECT_EQ(c.root, bddand(a.root, b.root));
}

TEST_F(BDDTest, OperatorAndAssign) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    bddp expected = bddand(a.root, b.root);
    a &= b;
    EXPECT_EQ(a.root, expected);
}

TEST_F(BDDTest, OperatorAndWithFalse) {
    bddvar v = BDD_NewVar();
    BDD a = BDDvar(v);
    BDD f = BDD::False;
    BDD result = a & f;
    EXPECT_EQ(result.root, bddfalse);
}

TEST_F(BDDTest, OperatorAndWithTrue) {
    bddvar v = BDD_NewVar();
    BDD a = BDDvar(v);
    BDD t = BDD::True;
    BDD result = a & t;
    EXPECT_EQ(result.root, a.root);
}
