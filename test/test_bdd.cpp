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

TEST_F(BDDTest, BddLevOfVarInvalidRange) {
    BDD_NewVar();
    EXPECT_THROW(bddlevofvar(0), std::invalid_argument);
    EXPECT_THROW(bddlevofvar(2), std::invalid_argument);
}

TEST_F(BDDTest, BddVarOfLevInvalidRange) {
    BDD_NewVar();
    EXPECT_THROW(bddvaroflev(0), std::invalid_argument);
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
    bddp ab = bddand(p1, p2);  // 3 nodes
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

// --- Cross-operation identities ---

TEST_F(BDDTest, DeMorganIdentities) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp a = bddprime(v1);
    bddp b = bddprime(v2);
    // ~(a & b) = ~a | ~b
    EXPECT_EQ(bddnot(bddand(a, b)), bddor(bddnot(a), bddnot(b)));
    // ~(a | b) = ~a & ~b
    EXPECT_EQ(bddnot(bddor(a, b)), bddand(bddnot(a), bddnot(b)));
    // a | b = ~(~a & ~b)
    EXPECT_EQ(bddor(a, b), bddnot(bddand(bddnot(a), bddnot(b))));
}

TEST_F(BDDTest, XorOrAndRelation) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp a = bddprime(v1);
    bddp b = bddprime(v2);
    // a ^ b = (a | b) & ~(a & b)
    EXPECT_EQ(bddxor(a, b), bddand(bddor(a, b), bddnot(bddand(a, b))));
    // a ^ b = (a & ~b) | (~a & b)
    EXPECT_EQ(bddxor(a, b), bddor(bddand(a, bddnot(b)), bddand(bddnot(a), b)));
}
