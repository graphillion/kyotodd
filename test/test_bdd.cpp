#include <gtest/gtest.h>
#include "bdd.h"
#include <sstream>
#include <unordered_set>

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
    bddvar v2 = BDD_NewVar();
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
    bddp cube13 = getnode(v3, bddprime(v1), bddtrue);  // {v1, v3}
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

TEST_F(BDDTest, BddExistAlwaysFalse) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
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
    bddp cube = getnode(v3, bddprime(v2), bddtrue);
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
    bddp cube = getnode(v3, bddprime(v2), bddtrue);
    EXPECT_EQ(bdduniv(f, vars), bdduniv(f, cube));
}

// --- bddcofactor ---

TEST_F(BDDTest, BddCofactorTerminalF) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // f is constant -> return f
    EXPECT_EQ(bddcofactor(bddfalse, p), bddfalse);
    EXPECT_EQ(bddcofactor(bddtrue, p), bddtrue);
}

TEST_F(BDDTest, BddCofactorGFalse) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // g = false -> care region empty -> false
    EXPECT_EQ(bddcofactor(p, bddfalse), bddfalse);
}

TEST_F(BDDTest, BddCofactorGTrue) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // g = true -> no don't care -> return f
    EXPECT_EQ(bddcofactor(p, bddtrue), p);
}

TEST_F(BDDTest, BddCofactorFEqG) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // f = g -> care region always 1 -> true
    EXPECT_EQ(bddcofactor(p, p), bddtrue);
}

TEST_F(BDDTest, BddCofactorFEqNotG) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // f = ~g -> care region always 0 -> false
    EXPECT_EQ(bddcofactor(bddnot(p), p), bddfalse);
    EXPECT_EQ(bddcofactor(p, bddnot(p)), bddfalse);
}

TEST_F(BDDTest, BddCofactorPreservesOnCare) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    // g = p2: care where v2=1. On care region, f = v1 & 1 = v1
    // Cofactor should return something equivalent to v1 on care region
    bddp result = bddcofactor(f, p2);
    // Verify: on care region (v2=1), result must equal f
    EXPECT_EQ(bddand(result, p2), bddand(f, p2));
}

TEST_F(BDDTest, BddCofactorReducesNodes) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);  // 2 nodes
    // g = p2: v2=0 is don't care, so v2 branch can be eliminated
    bddp result = bddcofactor(f, p2);
    EXPECT_LE(bddsize(result), bddsize(f));
}

TEST_F(BDDTest, BddCofactorDontCareOnLow) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    // f = v1 ^ v2, g = v2 (care only when v2=1)
    bddp f = bddxor(p1, p2);
    bddp result = bddcofactor(f, p2);
    // On care region (v2=1): f = v1 ^ 1 = ~v1
    // Result must agree with f where g=1
    EXPECT_EQ(bddand(result, p2), bddand(f, p2));
}

TEST_F(BDDTest, BddCofactorThreeVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    // f = (v1 & v2) | v3, g = v3 (care only when v3=1)
    // On care region (v3=1): f = 1 always, so cofactor = true
    bddp f = bddor(bddand(p1, p2), p3);
    EXPECT_EQ(bddcofactor(f, p3), bddtrue);
}

TEST_F(BDDTest, BddCofactorComplex) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp f = bddor(bddand(p1, p2), bddand(bddnot(p1), p3));
    bddp g = bddor(p2, p3);
    bddp result = bddcofactor(f, g);
    // Correctness: on care region (g=1), result must agree with f
    EXPECT_EQ(bddand(result, g), bddand(f, g));
    // Simplification: result should not be larger than f
    EXPECT_LE(bddsize(result), bddsize(f));
}

// --- bddlshift ---

TEST_F(BDDTest, BddLshiftTerminals) {
    EXPECT_EQ(bddlshift(bddfalse, 1), bddfalse);
    EXPECT_EQ(bddlshift(bddtrue, 1), bddtrue);
    EXPECT_EQ(bddlshift(bddfalse, 5), bddfalse);
}

TEST_F(BDDTest, BddLshiftZero) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddlshift(p, 0), p);
}

TEST_F(BDDTest, BddLshiftSingleVar) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    // level 1 + 1 = level 2 -> v2
    EXPECT_EQ(bddlshift(p1, 1), p2);
}

TEST_F(BDDTest, BddLshiftComplement) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddlshift(bddnot(p1), 1), bddnot(p2));
}

TEST_F(BDDTest, BddLshiftTwoVars) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddvar v3 = BDD_NewVar();  // level 3
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    // v1 & v2, shift 1: level 1->2(v2), level 2->3(v3) => v2 & v3
    EXPECT_EQ(bddlshift(bddand(p1, p2), 1), bddand(p2, p3));
}

TEST_F(BDDTest, BddLshiftAutoNewVar) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddvarused(), 2u);
    // Shift v2 (level 2) by 1 -> level 3, doesn't exist yet
    bddp result = bddlshift(p2, 1);
    EXPECT_EQ(bddvarused(), 3u);
    bddvar v3 = bddvaroflev(3);
    EXPECT_EQ(result, bddprime(v3));
}

TEST_F(BDDTest, BddLshiftAutoNewVarMultiple) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddp p1 = bddprime(v1);
    EXPECT_EQ(bddvarused(), 1u);
    // Shift v1 (level 1) by 3 -> level 4, need vars at levels 2,3,4
    bddp result = bddlshift(p1, 3);
    EXPECT_EQ(bddvarused(), 4u);
    bddvar v4 = bddvaroflev(4);
    EXPECT_EQ(result, bddprime(v4));
}

TEST_F(BDDTest, BddLshiftPreservesOr) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddvar v3 = BDD_NewVar();  // level 3
    bddvar v4 = BDD_NewVar();  // level 4
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp p4 = bddprime(v4);
    // v1 | v2, shift 2: level 1->3(v3), level 2->4(v4) => v3 | v4
    EXPECT_EQ(bddlshift(bddor(p1, p2), 2), bddor(p3, p4));
}

TEST_F(BDDTest, BddLshiftPreservesXor) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddvar v3 = BDD_NewVar();  // level 3
    bddvar v4 = BDD_NewVar();  // level 4
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp p4 = bddprime(v4);
    EXPECT_EQ(bddlshift(bddxor(p1, p2), 2), bddxor(p3, p4));
}

// --- bddrshift ---

TEST_F(BDDTest, BddRshiftTerminals) {
    EXPECT_EQ(bddrshift(bddfalse, 1), bddfalse);
    EXPECT_EQ(bddrshift(bddtrue, 1), bddtrue);
}

TEST_F(BDDTest, BddRshiftZero) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddrshift(p, 0), p);
}

TEST_F(BDDTest, BddRshiftSingleVar) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    // level 2 - 1 = level 1 -> v1
    EXPECT_EQ(bddrshift(p2, 1), p1);
}

TEST_F(BDDTest, BddRshiftComplement) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddrshift(bddnot(p2), 1), bddnot(p1));
}

TEST_F(BDDTest, BddRshiftTwoVars) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddvar v3 = BDD_NewVar();  // level 3
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    // v2 & v3, shift 1: level 2->1(v1), level 3->2(v2) => v1 & v2
    EXPECT_EQ(bddrshift(bddand(p2, p3), 1), bddand(p1, p2));
}

TEST_F(BDDTest, BddRshiftPreservesOr) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddvar v3 = BDD_NewVar();  // level 3
    bddvar v4 = BDD_NewVar();  // level 4
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp p4 = bddprime(v4);
    // v3 | v4, shift 2: level 3->1(v1), level 4->2(v2) => v1 | v2
    EXPECT_EQ(bddrshift(bddor(p3, p4), 2), bddor(p1, p2));
}

TEST_F(BDDTest, BddRshiftPreservesXor) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddvar v3 = BDD_NewVar();  // level 3
    bddvar v4 = BDD_NewVar();  // level 4
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    bddp p4 = bddprime(v4);
    EXPECT_EQ(bddrshift(bddxor(p3, p4), 2), bddxor(p1, p2));
}

TEST_F(BDDTest, BddRshiftInvertsLshift) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddvar v3 = BDD_NewVar();  // level 3
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    // lshift then rshift should be identity
    EXPECT_EQ(bddrshift(bddlshift(f, 1), 1), f);
}

TEST_F(BDDTest, BddRshiftTooLarge) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddp p1 = bddprime(v1);
    // v1 is at level 1, shift 1 would go to level 0 (invalid)
    EXPECT_THROW(bddrshift(p1, 1), std::invalid_argument);
    // v1 at level 1, shift 2 also invalid
    EXPECT_THROW(bddrshift(p1, 2), std::invalid_argument);
    // v2 at level 2, shift 2 also invalid (level 0)
    bddp p2 = bddprime(v2);
    EXPECT_THROW(bddrshift(p2, 2), std::invalid_argument);
}

// --- bdduniv ---

TEST_F(BDDTest, BddUnivNoCube) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bdduniv(p, bddfalse), p);
    EXPECT_EQ(bdduniv(bddfalse, bddfalse), bddfalse);
    EXPECT_EQ(bdduniv(bddtrue, bddfalse), bddtrue);
}

TEST_F(BDDTest, BddUnivTerminalF) {
    bddvar v = BDD_NewVar();
    bddp cube = bddprime(v);
    EXPECT_EQ(bdduniv(bddfalse, cube), bddfalse);
    EXPECT_EQ(bdduniv(bddtrue, cube), bddtrue);
}

TEST_F(BDDTest, BddUnivSingleVar) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // forall v. v = (v|_{v=0}) & (v|_{v=1}) = 0 & 1 = 0
    EXPECT_EQ(bdduniv(p, bddprime(v)), bddfalse);
    // forall v. ~v = 1 & 0 = 0
    EXPECT_EQ(bdduniv(bddnot(p), bddprime(v)), bddfalse);
}

TEST_F(BDDTest, BddUnivAndOneVar) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    // forall v1. (v1 & v2) = (0 & v2) & (1 & v2) = 0 & v2 = 0
    EXPECT_EQ(bdduniv(f, bddprime(v1)), bddfalse);
    // forall v2. (v1 & v2) = (v1 & 0) & (v1 & 1) = 0 & v1 = 0
    EXPECT_EQ(bdduniv(f, bddprime(v2)), bddfalse);
}

TEST_F(BDDTest, BddUnivOrOneVar) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddor(p1, p2);
    // forall v1. (v1 | v2) = (0 | v2) & (1 | v2) = v2 & 1 = v2
    EXPECT_EQ(bdduniv(f, bddprime(v1)), p2);
    // forall v2. (v1 | v2) = (v1 | 0) & (v1 | 1) = v1 & 1 = v1
    EXPECT_EQ(bdduniv(f, bddprime(v2)), p1);
}

TEST_F(BDDTest, BddUnivNotInF) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    // v2 does not appear in p1
    EXPECT_EQ(bdduniv(p1, bddprime(v2)), p1);
}

TEST_F(BDDTest, BddUnivThreeVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    // f = v1 | v2 | v3
    bddp f = bddor(bddor(p1, p2), p3);
    // forall v1. (v1|v2|v3) = (v2|v3) & (1) = v2|v3
    EXPECT_EQ(bdduniv(f, bddprime(v1)), bddor(p2, p3));
    // forall v1,v2. (v1|v2|v3) = forall v2. (v2|v3) = v3 & 1 = v3
    bddp cube12 = getnode(v2, bddprime(v1), bddtrue);
    EXPECT_EQ(bdduniv(f, cube12), p3);
    // forall v1,v2,v3. (v1|v2|v3) = false (e.g. all 0)
    EXPECT_EQ(bdduniv(f, bddsupport(f)), bddfalse);
}

TEST_F(BDDTest, BddUnivTautology) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // forall v. (v | ~v) = forall v. true = true
    bddp f = bddor(p, bddnot(p));
    EXPECT_EQ(f, bddtrue);
    EXPECT_EQ(bdduniv(f, bddprime(v)), bddtrue);
}

TEST_F(BDDTest, BddUnivDualOfExist) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddxor(p1, p2);
    bddp cube = bddprime(v1);
    // forall v. f = ~(exist v. ~f)
    EXPECT_EQ(bdduniv(f, cube), bddnot(bddexist(bddnot(f), cube)));
}

// --- bddimply ---

TEST_F(BDDTest, BddImplyTerminals) {
    EXPECT_EQ(bddimply(bddfalse, bddfalse), 1);  // false->false = true
    EXPECT_EQ(bddimply(bddfalse, bddtrue), 1);   // false->true = true
    EXPECT_EQ(bddimply(bddtrue, bddfalse), 0);   // true->false = false
    EXPECT_EQ(bddimply(bddtrue, bddtrue), 1);    // true->true = true
}

TEST_F(BDDTest, BddImplySelf) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddimply(p, p), 1);            // x -> x
    EXPECT_EQ(bddimply(bddnot(p), bddnot(p)), 1);  // ~x -> ~x
}

TEST_F(BDDTest, BddImplyComplement) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddimply(p, bddnot(p)), 0);   // x -> ~x fails (x=1)
    EXPECT_EQ(bddimply(bddnot(p), p), 0);   // ~x -> x fails (x=0)
}

TEST_F(BDDTest, BddImplyWithTerminal) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddimply(bddfalse, p), 1);    // false -> anything
    EXPECT_EQ(bddimply(p, bddtrue), 1);     // anything -> true
    EXPECT_EQ(bddimply(bddtrue, p), 0);     // true -> x fails (x=0)
    EXPECT_EQ(bddimply(p, bddfalse), 0);    // x -> false fails (x=1)
}

TEST_F(BDDTest, BddImplyAndImpliesOr) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f_and = bddand(p1, p2);
    bddp f_or  = bddor(p1, p2);
    // (x1 & x2) -> (x1 | x2) is always true
    EXPECT_EQ(bddimply(f_and, f_or), 1);
    // (x1 | x2) -> (x1 & x2) is NOT always true
    EXPECT_EQ(bddimply(f_or, f_and), 0);
}

TEST_F(BDDTest, BddImplyVarImpliesOr) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    // x1 -> (x1 | x2) is always true
    EXPECT_EQ(bddimply(p1, bddor(p1, p2)), 1);
    // x2 -> (x1 | x2) is always true
    EXPECT_EQ(bddimply(p2, bddor(p1, p2)), 1);
    // (x1 | x2) -> x1 is NOT always true
    EXPECT_EQ(bddimply(bddor(p1, p2), p1), 0);
}

TEST_F(BDDTest, BddImplyThreeVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    // (x1 & x2 & x3) -> x1 is always true
    EXPECT_EQ(bddimply(bddand(bddand(p1, p2), p3), p1), 1);
    // x1 -> (x1 & x2 & x3) is NOT always true
    EXPECT_EQ(bddimply(p1, bddand(bddand(p1, p2), p3)), 0);
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

// --- bddexport ---

// Helper: split a string into lines
static std::vector<std::string> split_lines(const std::string& s) {
    std::vector<std::string> lines;
    std::istringstream iss(s);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    return lines;
}

TEST_F(BDDTest, ExportTerminalFalse) {
    bddp p[] = { bddfalse };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::vector<std::string> lines = split_lines(oss.str());
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(lines[0], "_i 0");
    EXPECT_EQ(lines[1], "_o 1");
    EXPECT_EQ(lines[2], "_n 0");
    EXPECT_EQ(lines[3], "F");
}

TEST_F(BDDTest, ExportTerminalTrue) {
    bddp p[] = { bddtrue };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::vector<std::string> lines = split_lines(oss.str());
    ASSERT_EQ(lines.size(), 4u);
    EXPECT_EQ(lines[0], "_i 0");
    EXPECT_EQ(lines[1], "_o 1");
    EXPECT_EQ(lines[2], "_n 0");
    EXPECT_EQ(lines[3], "T");
}

TEST_F(BDDTest, ExportSingleVariable) {
    bddvar v1 = bddnewvar();
    bddp f = bddprime(v1);  // x1
    bddp p[] = { f };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::vector<std::string> lines = split_lines(oss.str());
    // Header: _i 1, _o 1, _n 1
    EXPECT_EQ(lines[0], "_i 1");
    EXPECT_EQ(lines[1], "_o 1");
    EXPECT_EQ(lines[2], "_n 1");
    // 1 node + 1 root = 2 lines after header
    ASSERT_EQ(lines.size(), 5u);
    // Node line: <node_id> 1 F T
    bddp node = f & ~BDD_COMP_FLAG;
    std::ostringstream expected;
    expected << node << " 1 F T";
    EXPECT_EQ(lines[3], expected.str());
    // Root: non-negated node id
    std::ostringstream root_expected;
    root_expected << node;
    EXPECT_EQ(lines[4], root_expected.str());
}

TEST_F(BDDTest, ExportNegatedVariable) {
    bddvar v1 = bddnewvar();
    bddp f = bddnot(bddprime(v1));  // NOT x1
    bddp p[] = { f };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::vector<std::string> lines = split_lines(oss.str());
    ASSERT_EQ(lines.size(), 5u);
    EXPECT_EQ(lines[2], "_n 1");
    // Root should be odd (negated)
    bddp node = f & ~BDD_COMP_FLAG;
    std::ostringstream root_expected;
    root_expected << (node | 1);
    EXPECT_EQ(lines[4], root_expected.str());
}

TEST_F(BDDTest, ExportAndOfTwoVars) {
    // x1 AND x2: should produce 2 internal nodes
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v2));
    bddp p[] = { f };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::vector<std::string> lines = split_lines(oss.str());
    // Header
    EXPECT_EQ(lines[0], "_i 2");
    EXPECT_EQ(lines[1], "_o 1");
    EXPECT_EQ(lines[2], "_n 2");
    // 3 header + 2 nodes + 1 root = 6 lines
    ASSERT_EQ(lines.size(), 6u);
}

TEST_F(BDDTest, ExportMultipleBDDs) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f1 = bddprime(v1);
    bddp f2 = bddprime(v2);
    bddp p[] = { f1, f2 };
    std::ostringstream oss;
    bddexport(oss, p, 2);
    std::vector<std::string> lines = split_lines(oss.str());
    EXPECT_EQ(lines[0], "_i 2");
    EXPECT_EQ(lines[1], "_o 2");
    EXPECT_EQ(lines[2], "_n 2");
    // 3 header + 2 nodes + 2 roots = 7 lines
    ASSERT_EQ(lines.size(), 7u);
}

TEST_F(BDDTest, ExportSharedNodes) {
    // f1 = x1 AND x2, f2 = x1 OR x2 share the same internal nodes
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp a = bddprime(v1);
    bddp b = bddprime(v2);
    bddp f1 = bddand(a, b);
    bddp f2 = bddor(a, b);
    bddp p[] = { f1, f2 };
    std::ostringstream oss;
    bddexport(oss, p, 2);
    std::vector<std::string> lines = split_lines(oss.str());
    // Shared node count should match bddvsize
    uint64_t vsize = bddvsize(p, 2);
    std::ostringstream node_count_str;
    node_count_str << "_n " << vsize;
    EXPECT_EQ(lines[2], node_count_str.str());
}

TEST_F(BDDTest, ExportVectorOverload) {
    bddvar v1 = bddnewvar();
    bddp f = bddprime(v1);
    std::vector<bddp> v = { f };
    std::ostringstream oss;
    bddexport(oss, v);
    std::vector<std::string> lines = split_lines(oss.str());
    EXPECT_EQ(lines[0], "_i 1");
    EXPECT_EQ(lines[1], "_o 1");
    EXPECT_EQ(lines[2], "_n 1");
}

TEST_F(BDDTest, ExportFilePtr) {
    bddvar v1 = bddnewvar();
    bddp f = bddprime(v1);
    bddp p[] = { f };
    // Write to a temp file, then read back and compare with ostream version
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::string expected = oss.str();

    FILE* tmp = std::tmpfile();
    ASSERT_NE(tmp, nullptr);
    bddexport(tmp, p, 1);
    std::rewind(tmp);
    std::string actual;
    char buf[256];
    while (std::fgets(buf, sizeof(buf), tmp)) {
        actual += buf;
    }
    std::fclose(tmp);
    EXPECT_EQ(actual, expected);
}

TEST_F(BDDTest, ExportFilePtrVector) {
    bddvar v1 = bddnewvar();
    bddp f = bddprime(v1);
    std::vector<bddp> v = { f };
    std::ostringstream oss;
    bddexport(oss, v);
    std::string expected = oss.str();

    FILE* tmp = std::tmpfile();
    ASSERT_NE(tmp, nullptr);
    bddexport(tmp, v);
    std::rewind(tmp);
    std::string actual;
    char buf[256];
    while (std::fgets(buf, sizeof(buf), tmp)) {
        actual += buf;
    }
    std::fclose(tmp);
    EXPECT_EQ(actual, expected);
}

TEST_F(BDDTest, ExportPostOrder) {
    // Verify nodes appear in post-order:
    // each node's children must appear before it.
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddand(bddprime(v2), bddprime(v3)));
    bddp p[] = { f };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::vector<std::string> lines = split_lines(oss.str());

    // Parse node IDs from node section
    int node_count = 0;
    std::istringstream nc(lines[2].substr(3));
    nc >> node_count;

    std::unordered_set<std::string> seen;
    seen.insert("F");
    seen.insert("T");
    for (int i = 0; i < node_count; i++) {
        std::istringstream iss(lines[3 + i]);
        std::string nid, lev, arc0, arc1;
        iss >> nid >> lev >> arc0 >> arc1;
        // arc0 and arc1 (stripped of negation) must already be seen
        // For odd arc1, the node_id is arc1-1
        uint64_t a1_val = 0;
        bool a1_is_terminal = (arc1 == "F" || arc1 == "T");
        if (!a1_is_terminal) {
            std::istringstream(arc1) >> a1_val;
            if (a1_val % 2 == 1) {
                std::ostringstream even;
                even << (a1_val - 1);
                EXPECT_TRUE(seen.count(even.str()) > 0)
                    << "arc1 node " << (a1_val - 1) << " not yet seen for node " << nid;
            } else {
                EXPECT_TRUE(seen.count(arc1) > 0)
                    << "arc1 " << arc1 << " not yet seen for node " << nid;
            }
        }
        EXPECT_TRUE(seen.count(arc0) > 0)
            << "arc0 " << arc0 << " not yet seen for node " << nid;
        seen.insert(nid);
    }
}
