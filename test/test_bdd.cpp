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

// --- BDD operator| ---

TEST_F(BDDTest, OperatorOr) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD c = a | b;
    EXPECT_EQ(c.root, bddor(a.root, b.root));
}

TEST_F(BDDTest, OperatorOrAssign) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    bddp expected = bddor(a.root, b.root);
    a |= b;
    EXPECT_EQ(a.root, expected);
}

// --- BDD operator^ ---

TEST_F(BDDTest, OperatorXor) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD c = a ^ b;
    EXPECT_EQ(c.root, bddxor(a.root, b.root));
}

TEST_F(BDDTest, OperatorXorAssign) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    bddp expected = bddxor(a.root, b.root);
    a ^= b;
    EXPECT_EQ(a.root, expected);
}

// --- BDD operator~ ---

TEST_F(BDDTest, OperatorNot) {
    bddvar v = BDD_NewVar();
    BDD a = BDDvar(v);
    BDD c = ~a;
    EXPECT_EQ(c.root, bddnot(a.root));
}

TEST_F(BDDTest, OperatorDoubleNot) {
    bddvar v = BDD_NewVar();
    BDD a = BDDvar(v);
    BDD c = ~(~a);
    EXPECT_EQ(c.root, a.root);
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
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD c = a << 1;
    EXPECT_EQ(c.root, bddlshift(a.root, 1));
}

TEST_F(BDDTest, OperatorLshiftAssign) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    bddp expected = bddlshift(a.root, 1);
    a <<= 1;
    EXPECT_EQ(a.root, expected);
}

TEST_F(BDDTest, OperatorRshift) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v2);
    BDD c = a >> 1;
    EXPECT_EQ(c.root, bddrshift(a.root, 1));
}

TEST_F(BDDTest, OperatorRshiftAssign) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v2);
    bddp expected = bddrshift(a.root, 1);
    a >>= 1;
    EXPECT_EQ(a.root, expected);
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

// --- getznode ---

TEST_F(BDDTest, GetznodeZeroSuppression) {
    // ZDD reduction: if hi == bddempty, return lo
    bddvar v1 = bddnewvar();
    EXPECT_EQ(getznode(v1, bddempty, bddempty), bddempty);
    EXPECT_EQ(getznode(v1, bddsingle, bddempty), bddsingle);

    bddvar v2 = bddnewvar();
    bddp node = getznode(v2, bddempty, bddsingle);
    EXPECT_EQ(getznode(v1, node, bddempty), node);
}

TEST_F(BDDTest, GetznodeNoReductionWhenLoEqualsHi) {
    // Unlike BDD, getznode does NOT reduce when lo == hi
    bddvar v1 = bddnewvar();
    bddp z = getznode(v1, bddsingle, bddsingle);
    // This should create a node, not return bddsingle
    EXPECT_NE(z, bddsingle);
    EXPECT_FALSE(z & BDD_CONST_FLAG);
}

TEST_F(BDDTest, GetznodeSingleton) {
    // {{v1}} = getznode(v1, bddempty, bddsingle)
    bddvar v1 = bddnewvar();
    bddp z = getznode(v1, bddempty, bddsingle);
    EXPECT_FALSE(z & BDD_CONST_FLAG);
    // lo should be bddempty, hi should be bddsingle
    bddp node = z & ~BDD_COMP_FLAG;
    EXPECT_EQ(z, node);  // no complement on result
}

TEST_F(BDDTest, GetznodeComplementNormalization) {
    // lo is complemented: strip it, complement the result
    // hi is NOT negated (unlike BDD getnode)
    bddvar v1 = bddnewvar();
    bddp z1 = getznode(v1, bddempty, bddsingle);   // {{v1}}
    bddp z2 = getznode(v1, bddsingle, bddsingle);  // {{}, {v1}}
    // z2 should be complement of z1, since:
    //   lo=bddsingle → comp=true, lo normalized to bddempty
    //   stored node is (v1, bddempty, bddsingle) = z1
    //   result = ~z1
    EXPECT_EQ(z2, bddnot(z1));
}

TEST_F(BDDTest, GetznodeComplementHiUnchanged) {
    // Verify hi is not negated during complement normalization
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp inner = getznode(v1, bddempty, bddsingle);  // {{v1}}

    // getznode(v2, bddsingle, inner)
    // lo=bddsingle is complemented → comp=true, lo=bddempty
    // hi=inner stays unchanged (ZDD rule)
    bddp z = getznode(v2, bddsingle, inner);

    // Compare with BDD getnode which would negate hi too
    bddp b = getnode(v2, bddsingle, inner);

    // They should differ because BDD negates hi but ZDD doesn't
    // getznode stores (v2, bddempty, inner), getnode stores (v2, bddempty, ~inner)
    EXPECT_NE(z & ~BDD_COMP_FLAG, b & ~BDD_COMP_FLAG);
}

TEST_F(BDDTest, GetznodeUniqueTableSharing) {
    // Same (var, lo, hi) returns same node
    bddvar v1 = bddnewvar();
    bddp z1 = getznode(v1, bddempty, bddsingle);
    bddp z2 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(z1, z2);
}

TEST_F(BDDTest, GetznodeTwoElementFamily) {
    // {{v1}, {v2}} using ZDD
    bddvar v1 = bddnewvar();  // level 1
    bddvar v2 = bddnewvar();  // level 2
    // Build bottom-up: v2 is lower level, v1 is higher level
    // Node for v1: lo = getznode(v1, bddempty, bddsingle) would be wrong ordering
    // ZDD for {{v1}, {v2}}: at v2 (level 2, top), lo = {{v1}}, hi = {{}}
    //   at v1 (level 1), lo = {}, hi = {{}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}} at level 1
    bddp z = getznode(v2, z_v1, bddsingle);          // at level 2: lo={{v1}}, hi={{}}
    // The family = {{v1}} ∪ {{v2 ∪ s} | s ∈ {{}}} = {{v1}} ∪ {{v2}} = {{v1}, {v2}}
    EXPECT_FALSE(z & BDD_CONST_FLAG);
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

// --- bddimport ---

TEST_F(BDDTest, ImportRoundtripTerminals) {
    bddp p[] = { bddfalse, bddtrue };
    std::ostringstream oss;
    bddexport(oss, p, 2);
    std::istringstream iss(oss.str());
    bddp q[2];
    int ret = bddimport(iss, q, 2);
    EXPECT_EQ(ret, 2);
    EXPECT_EQ(q[0], bddfalse);
    EXPECT_EQ(q[1], bddtrue);
}

TEST_F(BDDTest, ImportRoundtripSingleVar) {
    bddvar v1 = bddnewvar();
    bddp f = bddprime(v1);
    bddp p[] = { f };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::istringstream iss(oss.str());
    bddp q[1];
    int ret = bddimport(iss, q, 1);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(q[0], f);
}

TEST_F(BDDTest, ImportRoundtripNegated) {
    bddvar v1 = bddnewvar();
    bddp f = bddnot(bddprime(v1));
    bddp p[] = { f };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::istringstream iss(oss.str());
    bddp q[1];
    int ret = bddimport(iss, q, 1);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(q[0], f);
}

TEST_F(BDDTest, ImportRoundtripAnd) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v2));
    bddp p[] = { f };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::istringstream iss(oss.str());
    bddp q[1];
    int ret = bddimport(iss, q, 1);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(q[0], f);
}

TEST_F(BDDTest, ImportRoundtripOr) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddor(bddprime(v1), bddprime(v2));
    bddp p[] = { f };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::istringstream iss(oss.str());
    bddp q[1];
    int ret = bddimport(iss, q, 1);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(q[0], f);
}

TEST_F(BDDTest, ImportRoundtripXor) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp f = bddxor(bddprime(v1), bddxor(bddprime(v2), bddprime(v3)));
    bddp p[] = { f };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::istringstream iss(oss.str());
    bddp q[1];
    int ret = bddimport(iss, q, 1);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(q[0], f);
}

TEST_F(BDDTest, ImportRoundtripMultiple) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f1 = bddand(bddprime(v1), bddprime(v2));
    bddp f2 = bddor(bddprime(v1), bddprime(v2));
    bddp f3 = bddnot(bddprime(v1));
    bddp p[] = { f1, f2, f3 };
    std::ostringstream oss;
    bddexport(oss, p, 3);
    std::istringstream iss(oss.str());
    bddp q[3];
    int ret = bddimport(iss, q, 3);
    EXPECT_EQ(ret, 3);
    EXPECT_EQ(q[0], f1);
    EXPECT_EQ(q[1], f2);
    EXPECT_EQ(q[2], f3);
}

TEST_F(BDDTest, ImportLimSmaller) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f1 = bddprime(v1);
    bddp f2 = bddprime(v2);
    bddp p[] = { f1, f2 };
    std::ostringstream oss;
    bddexport(oss, p, 2);
    std::istringstream iss(oss.str());
    bddp q[1];
    int ret = bddimport(iss, q, 1);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(q[0], f1);
}

TEST_F(BDDTest, ImportVectorOverload) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v2));
    bddp p[] = { f };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::istringstream iss(oss.str());
    std::vector<bddp> v;
    int ret = bddimport(iss, v);
    EXPECT_EQ(ret, 1);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], f);
}

TEST_F(BDDTest, ImportFilePtr) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v2));
    bddp p[] = { f };

    FILE* tmp = std::tmpfile();
    ASSERT_NE(tmp, nullptr);
    bddexport(tmp, p, 1);
    std::rewind(tmp);
    bddp q[1];
    int ret = bddimport(tmp, q, 1);
    std::fclose(tmp);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(q[0], f);
}

TEST_F(BDDTest, ImportFilePtrVector) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddor(bddprime(v1), bddprime(v2));
    std::vector<bddp> orig = { f };

    FILE* tmp = std::tmpfile();
    ASSERT_NE(tmp, nullptr);
    bddexport(tmp, orig);
    std::rewind(tmp);
    std::vector<bddp> v;
    int ret = bddimport(tmp, v);
    std::fclose(tmp);
    EXPECT_EQ(ret, 1);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], f);
}

TEST_F(BDDTest, ImportCreatesVariables) {
    // Import into a fresh instance should auto-create variables
    bddp f = bddand(bddprime(bddnewvar()), bddprime(bddnewvar()));
    bddp p[] = { f };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::string data = oss.str();

    // Reinitialize (destroys all variables)
    bddinit(1024, UINT64_MAX);
    EXPECT_EQ(bddvarused(), 0u);

    std::istringstream iss(data);
    bddp q[1];
    int ret = bddimport(iss, q, 1);
    EXPECT_EQ(ret, 1);
    EXPECT_GE(bddvarused(), 2u);
    // Verify the imported BDD is x1 AND x2
    EXPECT_EQ(q[0], bddand(bddprime(1), bddprime(2)));
}

// --- bddimportz ---

TEST_F(BDDTest, ImportzRoundtripSingleton) {
    // ZDD {{v1}} = getznode(v1, bddempty, bddsingle)
    bddvar v1 = bddnewvar();
    bddp z = getznode(v1, bddempty, bddsingle);
    bddp p[] = { z };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::istringstream iss(oss.str());
    bddp q[1];
    int ret = bddimportz(iss, q, 1);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(q[0], z);
}

TEST_F(BDDTest, ImportzRoundtripWithComplement) {
    // ZDD {{}, {v1}} = getznode(v1, bddsingle, bddsingle)
    bddvar v1 = bddnewvar();
    bddp z = getznode(v1, bddsingle, bddsingle);
    bddp p[] = { z };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::istringstream iss(oss.str());
    bddp q[1];
    int ret = bddimportz(iss, q, 1);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(q[0], z);
}

TEST_F(BDDTest, ImportzRoundtripTwoVars) {
    // ZDD {{v1}, {v2}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z = getznode(v2, z_v1, bddsingle);
    bddp p[] = { z };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::istringstream iss(oss.str());
    bddp q[1];
    int ret = bddimportz(iss, q, 1);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(q[0], z);
}

TEST_F(BDDTest, ImportzDiffersFromImport) {
    // Same file imported as BDD vs ZDD may yield different results
    // because getnode and getznode have different reduction rules
    bddvar v1 = bddnewvar();
    // Create a node where lo == hi (BDD would reduce, ZDD would not if hi != empty)
    bddp node = getznode(v1, bddsingle, bddsingle);
    bddp p[] = { node };
    std::ostringstream oss;
    bddexport(oss, p, 1);

    // Import as BDD
    std::istringstream iss_bdd(oss.str());
    bddp q_bdd[1];
    bddimport(iss_bdd, q_bdd, 1);

    // Import as ZDD
    std::istringstream iss_zdd(oss.str());
    bddp q_zdd[1];
    bddimportz(iss_zdd, q_zdd, 1);

    EXPECT_EQ(q_zdd[0], node);
    // BDD import may reduce differently
}

TEST_F(BDDTest, ImportzFilePtr) {
    bddvar v1 = bddnewvar();
    bddp z = getznode(v1, bddempty, bddsingle);
    bddp p[] = { z };

    FILE* tmp = std::tmpfile();
    ASSERT_NE(tmp, nullptr);
    bddexport(tmp, p, 1);
    std::rewind(tmp);
    bddp q[1];
    int ret = bddimportz(tmp, q, 1);
    std::fclose(tmp);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(q[0], z);
}

TEST_F(BDDTest, ImportzVectorOverload) {
    bddvar v1 = bddnewvar();
    bddp z = getznode(v1, bddempty, bddsingle);
    std::vector<bddp> orig = { z };
    std::ostringstream oss;
    bddexport(oss, orig);
    std::istringstream iss(oss.str());
    std::vector<bddp> v;
    int ret = bddimportz(iss, v);
    EXPECT_EQ(ret, 1);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], z);
}

TEST_F(BDDTest, ImportzFilePtrVector) {
    bddvar v1 = bddnewvar();
    bddp z = getznode(v1, bddempty, bddsingle);
    std::vector<bddp> orig = { z };

    FILE* tmp = std::tmpfile();
    ASSERT_NE(tmp, nullptr);
    bddexport(tmp, orig);
    std::rewind(tmp);
    std::vector<bddp> v;
    int ret = bddimportz(tmp, v);
    std::fclose(tmp);
    EXPECT_EQ(ret, 1);
    ASSERT_EQ(v.size(), 1u);
    EXPECT_EQ(v[0], z);
}

// --- bddoffset ---

TEST_F(BDDTest, BddoffsetTerminal) {
    bddvar v1 = bddnewvar();
    // offset of empty family is empty
    EXPECT_EQ(bddoffset(bddempty, v1), bddempty);
    // offset of {{}} is {{}} (no item to remove)
    EXPECT_EQ(bddoffset(bddsingle, v1), bddsingle);
}

TEST_F(BDDTest, BddoffsetSingletonContaining) {
    bddvar v1 = bddnewvar();
    // {{v1}} = getznode(v1, bddempty, bddsingle)
    bddp z = getznode(v1, bddempty, bddsingle);
    // offset({{v1}}, v1) = sets not containing v1 = {} (empty family)
    EXPECT_EQ(bddoffset(z, v1), bddempty);
}

TEST_F(BDDTest, BddoffsetSingletonNotContaining) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}} — does not contain v2
    bddp z = getznode(v1, bddempty, bddsingle);
    // offset({{v1}}, v2) = {{v1}} (v2 not in any set)
    EXPECT_EQ(bddoffset(z, v2), z);
}

TEST_F(BDDTest, BddoffsetFamilyWithAndWithout) {
    bddvar v1 = bddnewvar();
    // {{}, {v1}} = getznode(v1, bddsingle, bddsingle)
    // lo = {{}} (sets without v1), hi = {{}} (sets with v1, v1 removed)
    bddp z = getznode(v1, bddsingle, bddsingle);
    // offset(z, v1) = sets not containing v1 = lo = {{}}
    EXPECT_EQ(bddoffset(z, v1), bddsingle);
}

TEST_F(BDDTest, BddoffsetTwoVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // Build {{v1, v2}}: at v1 (level 1), hi=bddsingle, lo=bddempty → {{v1}}
    // Then at v2 (level 2), lo=bddempty, hi={{v1}} → sets containing v2 AND v1
    bddp z_v1 = getznode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_v1v2 = getznode(v2, bddempty, z_v1);        // {{v1, v2}}
    // offset({{v1,v2}}, v2) = sets not containing v2 = empty
    EXPECT_EQ(bddoffset(z_v1v2, v2), bddempty);
    // offset({{v1,v2}}, v1) should recurse: at v2, lo=empty→offset=empty, hi={{v1}}→offset({{v1}},v1)=empty
    // result = getznode(v2, empty, empty) = empty
    EXPECT_EQ(bddoffset(z_v1v2, v1), bddempty);
}

TEST_F(BDDTest, BddoffsetMixedFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // Build {{v1}, {v2}}: at v2 (level 2, top), lo={{v1}}, hi={{}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z = getznode(v2, z_v1, bddsingle);             // {{v1}, {v2}}
    // offset(z, v2) = sets not containing v2 = lo = {{v1}}
    EXPECT_EQ(bddoffset(z, v2), z_v1);
    // offset(z, v1): at v2, lo=offset({{v1}},v1)=empty, hi=offset({{}},v1)={{}}
    // = getznode(v2, empty, single) = {{v2}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);     // {{v2}}
    EXPECT_EQ(bddoffset(z, v1), z_v2);
}

TEST_F(BDDTest, BddoffsetVarNotInFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // {{v1}} — only involves v1, ask about v3 (higher level)
    bddp z = getznode(v1, bddempty, bddsingle);
    // v3 has higher level than v1 → var above top, return f
    EXPECT_EQ(bddoffset(z, v3), z);
}

// --- bddonset ---

TEST_F(BDDTest, BddonsetTerminal) {
    bddvar v1 = bddnewvar();
    // onset of empty family is empty
    EXPECT_EQ(bddonset(bddempty, v1), bddempty);
    // onset of {{}} is empty (empty set doesn't contain v1)
    EXPECT_EQ(bddonset(bddsingle, v1), bddempty);
}

TEST_F(BDDTest, BddonsetSingletonContaining) {
    bddvar v1 = bddnewvar();
    // {{v1}}
    bddp z = getznode(v1, bddempty, bddsingle);
    // onset({{v1}}, v1) = {{v1}} (keeps var)
    EXPECT_EQ(bddonset(z, v1), z);
}

TEST_F(BDDTest, BddonsetSingletonNotContaining) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}} — does not contain v2
    bddp z = getznode(v1, bddempty, bddsingle);
    // onset({{v1}}, v2) = empty
    EXPECT_EQ(bddonset(z, v2), bddempty);
}

TEST_F(BDDTest, BddonsetFamilyWithAndWithout) {
    bddvar v1 = bddnewvar();
    // {{}, {v1}} = getznode(v1, bddsingle, bddsingle)
    bddp z = getznode(v1, bddsingle, bddsingle);
    // onset(z, v1) = {{v1}} (sets containing v1, var kept)
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    EXPECT_EQ(bddonset(z, v1), z_v1);
}

TEST_F(BDDTest, BddonsetTwoVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1, v2}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_v1v2 = getznode(v2, bddempty, z_v1);        // {{v1, v2}}
    // onset({{v1,v2}}, v2) = {{v1,v2}}
    EXPECT_EQ(bddonset(z_v1v2, v2), z_v1v2);
    // onset({{v1,v2}}, v1) = {{v1,v2}}
    EXPECT_EQ(bddonset(z_v1v2, v1), z_v1v2);
}

TEST_F(BDDTest, BddonsetMixedFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}, {v2}}: at v2 (top), lo={{v1}}, hi={{}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z = getznode(v2, z_v1, bddsingle);             // {{v1}, {v2}}
    // onset(z, v2) = {{v2}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    EXPECT_EQ(bddonset(z, v2), z_v2);
    // onset(z, v1) = {{v1}}
    EXPECT_EQ(bddonset(z, v1), z_v1);
}

TEST_F(BDDTest, BddonsetVarNotInFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // {{v1}} — v3 not present
    bddp z = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddonset(z, v3), bddempty);
}

// --- bddonset0 ---

TEST_F(BDDTest, Bddonset0Terminal) {
    bddvar v1 = bddnewvar();
    EXPECT_EQ(bddonset0(bddempty, v1), bddempty);
    EXPECT_EQ(bddonset0(bddsingle, v1), bddempty);
}

TEST_F(BDDTest, Bddonset0SingletonContaining) {
    bddvar v1 = bddnewvar();
    // {{v1}}
    bddp z = getznode(v1, bddempty, bddsingle);
    // onset0({{v1}}, v1) = {{}} (var removed)
    EXPECT_EQ(bddonset0(z, v1), bddsingle);
}

TEST_F(BDDTest, Bddonset0SingletonNotContaining) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddonset0(z, v2), bddempty);
}

TEST_F(BDDTest, Bddonset0FamilyWithAndWithout) {
    bddvar v1 = bddnewvar();
    // {{}, {v1}}
    bddp z = getznode(v1, bddsingle, bddsingle);
    // onset0(z, v1) = {{}} (sets containing v1 with v1 removed)
    EXPECT_EQ(bddonset0(z, v1), bddsingle);
}

TEST_F(BDDTest, Bddonset0TwoVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1, v2}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_v1v2 = getznode(v2, bddempty, z_v1);        // {{v1, v2}}
    // onset0({{v1,v2}}, v2) = {{v1}} (v2 removed)
    EXPECT_EQ(bddonset0(z_v1v2, v2), z_v1);
    // onset0({{v1,v2}}, v1) = {{v2}} (v1 removed)
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    EXPECT_EQ(bddonset0(z_v1v2, v1), z_v2);
}

TEST_F(BDDTest, Bddonset0MixedFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}, {v2}}: at v2 (top), lo={{v1}}, hi={{}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z = getznode(v2, z_v1, bddsingle);
    // onset0(z, v2) = {{}} (v2 removed from {v2})
    EXPECT_EQ(bddonset0(z, v2), bddsingle);
    // onset0(z, v1) = {{}} (v1 removed from {v1})
    EXPECT_EQ(bddonset0(z, v1), bddsingle);
}

TEST_F(BDDTest, Bddonset0VarNotInFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddonset0(z, v3), bddempty);
}

// --- bddchange ---

TEST_F(BDDTest, BddchangeTerminal) {
    bddvar v1 = bddnewvar();
    // change(empty, v1) = empty
    EXPECT_EQ(bddchange(bddempty, v1), bddempty);
    // change({{}}, v1) = {{v1}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddchange(bddsingle, v1), z_v1);
}

TEST_F(BDDTest, BddchangeSingletonToggle) {
    bddvar v1 = bddnewvar();
    // {{v1}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    // change({{v1}}, v1) = {{}} (remove v1)
    EXPECT_EQ(bddchange(z_v1, v1), bddsingle);
}

TEST_F(BDDTest, BddchangeDoubleToggle) {
    bddvar v1 = bddnewvar();
    // change twice should restore original
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddchange(bddchange(z_v1, v1), v1), z_v1);
    EXPECT_EQ(bddchange(bddchange(bddsingle, v1), v1), bddsingle);
}

TEST_F(BDDTest, BddchangeVarNotPresent) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}}: change with v2 → add v2 to each set → {{v1, v2}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, z_v1);  // {{v1, v2}}
    EXPECT_EQ(bddchange(z_v1, v2), z_v1v2);
}

TEST_F(BDDTest, BddchangeFamilyWithAndWithout) {
    bddvar v1 = bddnewvar();
    // {{}, {v1}} = getznode(v1, bddsingle, bddsingle)
    bddp z = getznode(v1, bddsingle, bddsingle);
    // change(z, v1): sets without v1 ({{}}) get v1 → {{v1}}
    //                sets with v1 ({v1}) lose v1 → {{}}
    //                result = {{}, {v1}} = z (same family, swapped roles)
    EXPECT_EQ(bddchange(z, v1), z);
}

TEST_F(BDDTest, BddchangeMixedFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}, {v2}}: at v2 (top), lo={{v1}}, hi={{}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z = getznode(v2, z_v1, bddsingle);
    // change(z, v1): toggle v1 in each set
    //   {v1} → {} (remove v1), {v2} → {v1, v2} (add v1)
    //   result = {{}, {v1, v2}}
    bddp z_v1v2 = getznode(v2, bddempty, z_v1);  // {{v1, v2}}
    bddp expected = getznode(v2, bddsingle, z_v1);  // {{}, {v1, v2}}
    EXPECT_EQ(bddchange(z, v1), expected);
}

TEST_F(BDDTest, BddchangeVarAboveTop) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // {{v1}}: change with v3 (higher level) → add v3 → {{v1, v3}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp expected = getznode(v3, bddempty, z_v1);  // {{v1, v3}}
    EXPECT_EQ(bddchange(z_v1, v3), expected);
}

// --- bddunion ---

TEST_F(BDDTest, BddunionTerminals) {
    // empty ∪ empty = empty
    EXPECT_EQ(bddunion(bddempty, bddempty), bddempty);
    // empty ∪ {{}} = {{}}
    EXPECT_EQ(bddunion(bddempty, bddsingle), bddsingle);
    EXPECT_EQ(bddunion(bddsingle, bddempty), bddsingle);
    // {{}} ∪ {{}} = {{}}
    EXPECT_EQ(bddunion(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, BddunionWithEmpty) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    EXPECT_EQ(bddunion(z_v1, bddempty), z_v1);
    EXPECT_EQ(bddunion(bddempty, z_v1), z_v1);
}

TEST_F(BDDTest, BddunionSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    EXPECT_EQ(bddunion(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, BddunionDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);  // {{v2}}
    // {{v1}} ∪ {{v2}} = {{v1}, {v2}}
    bddp expected = getznode(v2, z_v1, bddsingle);
    EXPECT_EQ(bddunion(z_v1, z_v2), expected);
    // Commutative
    EXPECT_EQ(bddunion(z_v2, z_v1), expected);
}

TEST_F(BDDTest, BddunionAddEmptySet) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    // {{v1}} ∪ {{}} = {{}, {v1}}
    bddp expected = getznode(v1, bddsingle, bddsingle);  // {{}, {v1}}
    EXPECT_EQ(bddunion(z_v1, bddsingle), expected);
}

TEST_F(BDDTest, BddunionOverlapping) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);     // {{v2}}
    bddp z_v1_v2 = getznode(v2, z_v1, bddsingle);      // {{v1}, {v2}}
    bddp z_v1v2 = getznode(v2, bddempty, z_v1);         // {{v1, v2}}
    // {{v1}, {v2}} ∪ {{v1, v2}} = {{v1}, {v2}, {v1, v2}}
    bddp result = bddunion(z_v1_v2, z_v1v2);
    // Expected: at v2, lo={{v1}} (sets without v2), hi=union({{},{}}, {{v1}})
    // hi = {{}, {v1}}
    bddp hi_expected = getznode(v1, bddsingle, bddsingle);  // {{}, {v1}}
    bddp expected = getznode(v2, z_v1, hi_expected);
    EXPECT_EQ(result, expected);
}

TEST_F(BDDTest, BddunionThreeVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_v3 = getznode(v3, bddempty, bddsingle);     // {{v3}}
    // {{v1}} ∪ {{v3}} = at v3, lo={{v1}}, hi={{}}
    bddp expected = getznode(v3, z_v1, bddsingle);
    EXPECT_EQ(bddunion(z_v1, z_v3), expected);
}

TEST_F(BDDTest, BddunionIdempotent) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp u = bddunion(z_v1, z_v2);
    // union(u, z_v1) == u (z_v1 already in u)
    EXPECT_EQ(bddunion(u, z_v1), u);
    EXPECT_EQ(bddunion(u, z_v2), u);
}

// --- bddintersec ---

TEST_F(BDDTest, BddintersecTerminals) {
    EXPECT_EQ(bddintersec(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddintersec(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddintersec(bddsingle, bddempty), bddempty);
    EXPECT_EQ(bddintersec(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, BddintersecWithEmpty) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddintersec(z_v1, bddempty), bddempty);
    EXPECT_EQ(bddintersec(bddempty, z_v1), bddempty);
}

TEST_F(BDDTest, BddintersecSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddintersec(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, BddintersecDisjoint) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);  // {{v2}}
    // {{v1}} ∩ {{v2}} = {} (no common sets)
    EXPECT_EQ(bddintersec(z_v1, z_v2), bddempty);
}

TEST_F(BDDTest, BddintersecOverlapping) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_v1_v2 = getznode(v2, z_v1, bddsingle);      // {{v1}, {v2}}
    // {{v1}, {v2}} ∩ {{v1}} = {{v1}}
    EXPECT_EQ(bddintersec(z_v1_v2, z_v1), z_v1);
    EXPECT_EQ(bddintersec(z_v1, z_v1_v2), z_v1);
}

TEST_F(BDDTest, BddintersecWithEmptySet) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);     // {{v1}}
    // {{v1}} ∩ {{}} = {} (no common sets)
    EXPECT_EQ(bddintersec(z_v1, bddsingle), bddempty);
    // {{}, {v1}} ∩ {{}} = {{}}
    bddp z_both = getznode(v1, bddsingle, bddsingle);  // {{}, {v1}}
    EXPECT_EQ(bddintersec(z_both, bddsingle), bddsingle);
}

TEST_F(BDDTest, BddintersecComplex) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);      // {{v1}}
    bddp z_v1_v2 = getznode(v2, z_v1, bddsingle);       // {{v1}, {v2}}
    bddp z_v1v2 = getznode(v2, bddempty, z_v1);          // {{v1, v2}}
    bddp z_v2_v1v2 = bddunion(
        getznode(v2, bddempty, bddsingle), z_v1v2);       // {{v2}, {v1, v2}}
    // {{v1}, {v2}} ∩ {{v2}, {v1, v2}} = {{v2}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    EXPECT_EQ(bddintersec(z_v1_v2, z_v2_v1v2), z_v2);
}

TEST_F(BDDTest, BddintersecUnionIdentity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp u = bddunion(z_v1, z_v2);  // {{v1}, {v2}}
    // intersec(union(a,b), a) == a
    EXPECT_EQ(bddintersec(u, z_v1), z_v1);
    EXPECT_EQ(bddintersec(u, z_v2), z_v2);
}

// --- bddsubtract ---

TEST_F(BDDTest, BddsubtractTerminals) {
    EXPECT_EQ(bddsubtract(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddsubtract(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddsubtract(bddsingle, bddempty), bddsingle);
    EXPECT_EQ(bddsubtract(bddsingle, bddsingle), bddempty);
}

TEST_F(BDDTest, BddsubtractWithEmpty) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddsubtract(z_v1, bddempty), z_v1);
    EXPECT_EQ(bddsubtract(bddempty, z_v1), bddempty);
}

TEST_F(BDDTest, BddsubtractSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddsubtract(z_v1, z_v1), bddempty);
}

TEST_F(BDDTest, BddsubtractDisjoint) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);  // {{v2}}
    // {{v1}} \ {{v2}} = {{v1}}
    EXPECT_EQ(bddsubtract(z_v1, z_v2), z_v1);
    // {{v2}} \ {{v1}} = {{v2}}
    EXPECT_EQ(bddsubtract(z_v2, z_v1), z_v2);
}

TEST_F(BDDTest, BddsubtractSubset) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp u = bddunion(z_v1, z_v2);  // {{v1}, {v2}}
    // {{v1}, {v2}} \ {{v1}} = {{v2}}
    EXPECT_EQ(bddsubtract(u, z_v1), z_v2);
    // {{v1}, {v2}} \ {{v2}} = {{v1}}
    EXPECT_EQ(bddsubtract(u, z_v2), z_v1);
    // {{v1}} \ {{v1}, {v2}} = {}
    EXPECT_EQ(bddsubtract(z_v1, u), bddempty);
}

TEST_F(BDDTest, BddsubtractEmptySetMember) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);     // {{v1}}
    bddp z_both = getznode(v1, bddsingle, bddsingle);   // {{}, {v1}}
    // {{}, {v1}} \ {{v1}} = {{}}
    EXPECT_EQ(bddsubtract(z_both, z_v1), bddsingle);
    // {{}, {v1}} \ {{}} = {{v1}}
    EXPECT_EQ(bddsubtract(z_both, bddsingle), z_v1);
}

TEST_F(BDDTest, BddsubtractUnionIdentity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    // union(a, b) \ b == subtract(a, b) when a ∩ b = empty
    bddp u = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddsubtract(u, z_v2), z_v1);
    // a == union(intersec(a,b), subtract(a,b))
    bddp z_v1v2 = getznode(v2, bddempty, z_v1);  // {{v1, v2}}
    bddp big = bddunion(u, z_v1v2);  // {{v1}, {v2}, {v1,v2}}
    bddp common = bddintersec(big, u);
    bddp diff = bddsubtract(big, u);
    EXPECT_EQ(bddunion(common, diff), big);
}

// --- bdddiv ---

TEST_F(BDDTest, BdddivTerminals) {
    // F / {∅} = F
    EXPECT_EQ(bdddiv(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bdddiv(bddsingle, bddsingle), bddsingle);
    // ∅ / G = ∅
    EXPECT_EQ(bdddiv(bddempty, bddempty), bddempty);
    // F / ∅ = ∅
    EXPECT_EQ(bdddiv(bddsingle, bddempty), bddempty);
    // {∅} / non-trivial = ∅ (G contains non-empty sets)
}

TEST_F(BDDTest, BdddivByEmptySet) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    // {{v1}} / {∅} = {{v1}}
    EXPECT_EQ(bdddiv(z_v1, bddsingle), z_v1);
}

TEST_F(BDDTest, BdddivSelf) {
    // F / F always contains {∅}
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    // {{v1}} / {{v1}}: x such that x ∪ {v1} ∈ {{v1}} and x ∩ {v1} = ∅
    // x = ∅: ∅ ∪ {v1} = {v1} ∈ F ✓ → {∅}
    EXPECT_EQ(bdddiv(z_v1, z_v1), bddsingle);
}

TEST_F(BDDTest, BdddivEmptySetInFNotG) {
    // {∅} / {{v1}} = ∅ (∅ ∪ {v1} = {v1} ∉ {∅})
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bdddiv(bddsingle, z_v1), bddempty);
}

TEST_F(BDDTest, BdddivGVarAboveF) {
    // G has variable not in F → ∅
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);  // {{v2}}
    // {{v1}} / {{v2}}: x ∪ {v2} ∈ {{v1}}, but no set in {{v1}} contains v2
    EXPECT_EQ(bdddiv(z_v1, z_v2), bddempty);
}

TEST_F(BDDTest, BdddivSimple) {
    // F = {{v1, v2}}, G = {{v2}} → F/G = {{v1}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);        // {{v1}}
    bddp z_v1v2 = getznode(v2, bddempty, z_v1);           // {{v1, v2}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);        // {{v2}}
    EXPECT_EQ(bdddiv(z_v1v2, z_v2), z_v1);
}

TEST_F(BDDTest, BdddivMultipleSets) {
    // F = {{a,c}, {a,d}, {c}, {d}}, G = {{c}, {d}}
    // F/G = {{a}, {∅}}:
    //   x={a}: {a}∪{c}={a,c}∈F, {a}∪{d}={a,d}∈F ✓
    //   x={∅}: {c}∈F, {d}∈F ✓
    bddvar a = bddnewvar();  // level 1
    bddvar c = bddnewvar();  // level 2
    bddvar d = bddnewvar();  // level 3

    bddp z_a = getznode(a, bddempty, bddsingle);   // {{a}}
    bddp z_ac = getznode(c, bddempty, z_a);        // {{a,c}}
    bddp z_ad = getznode(d, bddempty, z_a);        // {{a,d}}
    bddp z_c = getznode(c, bddempty, bddsingle);   // {{c}}
    bddp z_d = getznode(d, bddempty, bddsingle);   // {{d}}

    bddp F = bddunion(bddunion(z_ac, z_ad), bddunion(z_c, z_d));
    bddp G = bddunion(z_c, z_d);

    // Expected: {{∅}, {a}}
    bddp expected = getznode(a, bddsingle, bddsingle);
    EXPECT_EQ(bdddiv(F, G), expected);
}

TEST_F(BDDTest, BdddivPartialMatch) {
    // F = {{a,c}, {c}}, G = {{c}}
    // F/G: x ∪ {c} ∈ F and c ∉ x
    //   x={a}: {a,c} ∈ F ✓
    //   x={∅}: {c} ∈ F ✓
    // F/G = {{∅}, {a}}
    bddvar a = bddnewvar();
    bddvar c = bddnewvar();
    bddp z_a = getznode(a, bddempty, bddsingle);
    bddp z_ac = getznode(c, bddempty, z_a);         // {{a,c}}
    bddp z_c = getznode(c, bddempty, bddsingle);    // {{c}}
    bddp F = bddunion(z_ac, z_c);                  // {{a,c}, {c}}

    bddp expected = getznode(a, bddsingle, bddsingle);  // {{∅}, {a}}
    EXPECT_EQ(bdddiv(F, z_c), expected);
}

TEST_F(BDDTest, BdddivNoMatch) {
    // F = {{a}}, G = {{b}} where level(b) > level(a) → ∅
    bddvar a = bddnewvar();
    bddvar b = bddnewvar();
    bddp z_a = getznode(a, bddempty, bddsingle);
    bddp z_b = getznode(b, bddempty, bddsingle);
    EXPECT_EQ(bdddiv(z_a, z_b), bddempty);
}

TEST_F(BDDTest, BdddivQuotientTimesG) {
    // Verify: (F/G) · G ⊆ F  (via union/intersec identity)
    // F = {{a,b}, {a,c}, {b}, {c}}, G = {{b}, {c}}
    // F/G = {{a}, {∅}}
    // (F/G)·G = {{a,b}, {a,c}, {b}, {c}} = F
    // So F \ ((F/G)·G) should be ∅ (remainder = ∅)
    bddvar a = bddnewvar();
    bddvar b = bddnewvar();
    bddvar c = bddnewvar();
    bddp z_a = getznode(a, bddempty, bddsingle);
    bddp z_ab = getznode(b, bddempty, z_a);
    bddp z_ac = getznode(c, bddempty, z_a);
    bddp z_b = getznode(b, bddempty, bddsingle);
    bddp z_c = getznode(c, bddempty, bddsingle);

    bddp F = bddunion(bddunion(z_ab, z_ac), bddunion(z_b, z_c));
    bddp G = bddunion(z_b, z_c);
    bddp Q = bdddiv(F, G);

    bddp expected_q = getznode(a, bddsingle, bddsingle);  // {{∅}, {a}}
    EXPECT_EQ(Q, expected_q);
}

TEST_F(BDDTest, BdddivWithRemainder) {
    // F = {{a,b}, {c}}, G = {{b}}
    // F/G: x ∪ {b} ∈ F and b ∉ x
    //   x={a}: {a,b} ∈ F ✓
    //   x={c}: {b,c} ∉ F ✗
    // F/G = {{a}}
    bddvar a = bddnewvar();
    bddvar b = bddnewvar();
    bddvar c = bddnewvar();
    bddp z_a = getznode(a, bddempty, bddsingle);
    bddp z_ab = getznode(b, bddempty, z_a);
    bddp z_c = getznode(c, bddempty, bddsingle);
    bddp F = bddunion(z_ab, z_c);
    bddp G = getznode(b, bddempty, bddsingle);

    EXPECT_EQ(bdddiv(F, G), z_a);
}

TEST_F(BDDTest, ZDDOperatorDiv) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v1v2(0); z_v1v2.root = getznode(v2, bddempty, z_v1.root);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);

    // {{v1, v2}} / {{v2}} = {{v1}}
    ZDD result = z_v1v2 / z_v2;
    EXPECT_EQ(result, z_v1);
}

TEST_F(BDDTest, ZDDOperatorDivAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v1v2(0); z_v1v2.root = getznode(v2, bddempty, z_v1.root);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);

    z_v1v2 /= z_v2;
    EXPECT_EQ(z_v1v2, z_v1);
}

// --- bddsymdiff ---

TEST_F(BDDTest, ZDDSymdiffTerminalCases) {
    EXPECT_EQ(bddsymdiff(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddsymdiff(bddempty, bddsingle), bddsingle);
    EXPECT_EQ(bddsymdiff(bddsingle, bddempty), bddsingle);
    EXPECT_EQ(bddsymdiff(bddsingle, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDSymdiffWithSingleVar) {
    bddvar v1 = bddnewvar();
    // z_v1 = {{v1}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);

    // {{v1}} ^ {} = {{v1}}
    EXPECT_EQ(bddsymdiff(z_v1, bddempty), z_v1);
    EXPECT_EQ(bddsymdiff(bddempty, z_v1), z_v1);

    // {{v1}} ^ {{v1}} = {}
    EXPECT_EQ(bddsymdiff(z_v1, z_v1), bddempty);
}

TEST_F(BDDTest, ZDDSymdiffDisjointFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // z_v1 = {{v1}}, z_v2 = {{v2}}
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // {{v1}} ^ {{v2}} = {{v1}, {v2}} = union
    bddp result = bddsymdiff(z_v1, z_v2);
    bddp expected = bddunion(z_v1, z_v2);
    EXPECT_EQ(result, expected);
}

TEST_F(BDDTest, ZDDSymdiffOverlappingFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // F = {{v1}, {v2}}, G = {{v2}}
    bddp F = bddunion(z_v1, z_v2);
    bddp G = z_v2;

    // F ^ G = {{v1}}
    EXPECT_EQ(bddsymdiff(F, G), z_v1);
}

TEST_F(BDDTest, ZDDSymdiffEqualsSubtractUnion) {
    // Verify F ^ G = (F \ G) ∪ (G \ F)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);

    // F = {{v1}, {v2}}, G = {{v2}, {v3}}
    bddp F = bddunion(z_v1, z_v2);
    bddp G = bddunion(z_v2, z_v3);

    bddp result = bddsymdiff(F, G);
    bddp expected = bddunion(bddsubtract(F, G), bddsubtract(G, F));
    EXPECT_EQ(result, expected);
}

TEST_F(BDDTest, ZDDSymdiffCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    bddp F = bddunion(z_v1, bddsingle);  // {{v1}, {}}
    bddp G = z_v2;                         // {{v2}}

    EXPECT_EQ(bddsymdiff(F, G), bddsymdiff(G, F));
}

TEST_F(BDDTest, ZDDSymdiffWithEmptySet) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);

    // F = {{v1}, {}}, G = {{}}
    bddp F = bddunion(z_v1, bddsingle);
    bddp G = bddsingle;

    // F ^ G = {{v1}}
    EXPECT_EQ(bddsymdiff(F, G), z_v1);
}

TEST_F(BDDTest, ZDDOperatorSymdiff) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);

    ZDD F(0); F.root = bddunion(z_v1.root, z_v2.root);
    ZDD G = z_v2;

    // F ^ G = {{v1}}
    ZDD result = F ^ G;
    EXPECT_EQ(result, z_v1);
}

TEST_F(BDDTest, ZDDOperatorSymdiffAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);

    ZDD F(0); F.root = bddunion(z_v1.root, z_v2.root);
    F ^= z_v2;
    EXPECT_EQ(F, z_v1);
}

// --- bddjoin ---

TEST_F(BDDTest, ZDDJoinTerminalCases) {
    EXPECT_EQ(bddjoin(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddjoin(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddjoin(bddsingle, bddempty), bddempty);
    EXPECT_EQ(bddjoin(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDJoinIdentity) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}

    // {∅} ⊔ F = F
    EXPECT_EQ(bddjoin(bddsingle, z_v1), z_v1);
    EXPECT_EQ(bddjoin(z_v1, bddsingle), z_v1);
}

TEST_F(BDDTest, ZDDJoinDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);  // {{v2}}

    // {{v1}} ⊔ {{v2}} = {{v1, v2}}
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    EXPECT_EQ(bddjoin(z_v1, z_v2), z_v1v2);
}

TEST_F(BDDTest, ZDDJoinSameVar) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}

    // {{v1}} ⊔ {{v1}} = {{v1}} (union is idempotent)
    EXPECT_EQ(bddjoin(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDJoinFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);  // {{v2}}

    // F = {{v1}, {v2}}, G = {{v1}}
    bddp F = bddunion(z_v1, z_v2);
    bddp G = z_v1;

    // F ⊔ G = { A∪B | A∈F, B∈G }
    //   = {v1}∪{v1}, {v2}∪{v1} = {{v1}, {v1,v2}}
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp expected = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddjoin(F, G), expected);
}

TEST_F(BDDTest, ZDDJoinCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    bddp F = bddunion(z_v1, bddsingle);  // {{v1}, {}}
    bddp G = z_v2;                         // {{v2}}

    EXPECT_EQ(bddjoin(F, G), bddjoin(G, F));
}

TEST_F(BDDTest, ZDDJoinWithEmptySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // F = {{v1}, {}}, G = {{v2}}
    // F ⊔ G = {{v1,v2}, {v2}} ({}∪{v2}={v2}, {v1}∪{v2}={v1,v2})
    bddp F = bddunion(z_v1, bddsingle);
    bddp G = z_v2;
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp expected = bddunion(z_v2, z_v1v2);
    EXPECT_EQ(bddjoin(F, G), expected);
}

TEST_F(BDDTest, ZDDOperatorJoin) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);

    // {{v1}} * {{v2}} = {{v1, v2}}
    ZDD result = z_v1 * z_v2;
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    EXPECT_EQ(result.root, z_v1v2);
}

TEST_F(BDDTest, ZDDOperatorJoinAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);

    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    z_v1 *= z_v2;
    EXPECT_EQ(z_v1.root, z_v1v2);
}

// --- bddmeet ---

TEST_F(BDDTest, ZDDMeetTerminalCases) {
    EXPECT_EQ(bddmeet(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddmeet(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddmeet(bddsingle, bddempty), bddempty);
    // {∅} ⊓ {∅} = {∅∩∅} = {∅}
    EXPECT_EQ(bddmeet(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDMeetWithSingle) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}

    // {∅} ⊓ {{v1}} = {∅∩{v1}} = {∅}
    EXPECT_EQ(bddmeet(bddsingle, z_v1), bddsingle);
    EXPECT_EQ(bddmeet(z_v1, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDMeetSameFamily) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}

    // {{v1}} ⊓ {{v1}} = {{v1}∩{v1}} = {{v1}}
    EXPECT_EQ(bddmeet(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDMeetDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);  // {{v2}}

    // {{v1}} ⊓ {{v2}} = {{v1}∩{v2}} = {∅}
    EXPECT_EQ(bddmeet(z_v1, z_v2), bddsingle);
}

TEST_F(BDDTest, ZDDMeetOverlapping) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));  // {{v1,v2}}

    // {{v1,v2}} ⊓ {{v1}} = {{v1,v2}∩{v1}} = {{v1}}
    EXPECT_EQ(bddmeet(z_v1v2, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDMeetFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    // F = {{v1}, {v1,v2}}, G = {{v2}, {v1,v2}}
    bddp F = bddunion(z_v1, z_v1v2);
    bddp G = bddunion(z_v2, z_v1v2);

    // F ⊓ G = { A∩B | A∈F, B∈G }
    //   {v1}∩{v2}=∅, {v1}∩{v1,v2}={v1},
    //   {v1,v2}∩{v2}={v2}, {v1,v2}∩{v1,v2}={v1,v2}
    //   = {∅, {v1}, {v2}, {v1,v2}}
    bddp expected = bddunion(bddunion(bddsingle, z_v1), bddunion(z_v2, z_v1v2));
    EXPECT_EQ(bddmeet(F, G), expected);
}

TEST_F(BDDTest, ZDDMeetCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    bddp F = bddunion(z_v1, z_v1v2);
    bddp G = z_v2;

    EXPECT_EQ(bddmeet(F, G), bddmeet(G, F));
}

// --- bdddelta ---

TEST_F(BDDTest, ZDDDeltaTerminalCases) {
    EXPECT_EQ(bdddelta(bddempty, bddempty), bddempty);
    EXPECT_EQ(bdddelta(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bdddelta(bddsingle, bddempty), bddempty);
    // {∅} ⊞ {∅} = {∅⊕∅} = {∅}
    EXPECT_EQ(bdddelta(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDDeltaIdentity) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}

    // {∅} ⊞ F = F (∅⊕A = A)
    EXPECT_EQ(bdddelta(bddsingle, z_v1), z_v1);
    EXPECT_EQ(bdddelta(z_v1, bddsingle), z_v1);
}

TEST_F(BDDTest, ZDDDeltaSameSet) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}

    // {{v1}} ⊞ {{v1}} = {{v1}⊕{v1}} = {∅}
    EXPECT_EQ(bdddelta(z_v1, z_v1), bddsingle);
}

TEST_F(BDDTest, ZDDDeltaDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);  // {{v2}}

    // {{v1}} ⊞ {{v2}} = {{v1}⊕{v2}} = {{v1,v2}}
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    EXPECT_EQ(bdddelta(z_v1, z_v2), z_v1v2);
}

TEST_F(BDDTest, ZDDDeltaFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // F = {{v1}, {v2}}, G = {{v1}}
    bddp F = bddunion(z_v1, z_v2);
    bddp G = z_v1;

    // F ⊞ G = { A⊕B | A∈F, B∈G }
    //   {v1}⊕{v1}=∅, {v2}⊕{v1}={v1,v2}
    //   = {∅, {v1,v2}}
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp expected = bddunion(bddsingle, z_v1v2);
    EXPECT_EQ(bdddelta(F, G), expected);
}

TEST_F(BDDTest, ZDDDeltaCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    bddp F = bddunion(z_v1, bddsingle);  // {{v1}, {}}
    bddp G = z_v2;                         // {{v2}}

    EXPECT_EQ(bdddelta(F, G), bdddelta(G, F));
}

TEST_F(BDDTest, ZDDDeltaThreeVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    // F = {{v1,v2}}, G = {{v2,v3}}
    bddp z_v2v3 = getznode(v3, bddempty, getznode(v2, bddempty, bddsingle));
    bddp F = z_v1v2;
    bddp G = z_v2v3;

    // {v1,v2}⊕{v2,v3} = {v1,v3}
    bddp z_v1v3 = getznode(v3, bddempty, getznode(v1, bddempty, bddsingle));
    EXPECT_EQ(bdddelta(F, G), z_v1v3);
}

// --- bddremainder ---

TEST_F(BDDTest, ZDDRemainderTerminalCases) {
    // F % G = F \ (G ⊔ (F / G))
    EXPECT_EQ(bddremainder(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddremainder(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddremainder(bddsingle, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDRemainderNoDivisor) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);  // {{v2}}

    // {{v1}} / {{v2}} = {} (no quotient), so remainder = {{v1}} \ ({v2} ⊔ {}) = {{v1}}
    EXPECT_EQ(bddremainder(z_v1, z_v2), z_v1);
}

TEST_F(BDDTest, ZDDRemainderExactDivision) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));  // {{v1,v2}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);  // {{v2}}

    // F = {{v1,v2}}, G = {{v2}}
    // F / G = {{v1}}, G ⊔ (F/G) = {{v2}} ⊔ {{v1}} = {{v1,v2}}
    // F % G = {{v1,v2}} \ {{v1,v2}} = {}
    EXPECT_EQ(bddremainder(z_v1v2, z_v2), bddempty);
}

TEST_F(BDDTest, ZDDRemainderPartialDivision) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp z_v1v3 = getznode(v3, bddempty, getznode(v1, bddempty, bddsingle));

    // F = {{v1,v2}, {v1,v3}}, G = {{v2}}
    bddp F = bddunion(z_v1v2, z_v1v3);

    // F / G = {{v1}} (only {v1,v2} is divisible by {v2})
    // G ⊔ (F/G) = {{v2}} ⊔ {{v1}} = {{v1,v2}}
    // F % G = {{v1,v2},{v1,v3}} \ {{v1,v2}} = {{v1,v3}}
    EXPECT_EQ(bddremainder(F, z_v2), z_v1v3);
}

TEST_F(BDDTest, ZDDRemainderDefinition) {
    // Verify F % G = F \ (G ⊔ (F / G)) with a complex example
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp z_v2v3 = getznode(v3, bddempty, getznode(v2, bddempty, bddsingle));

    // F = {{v1,v2}, {v2,v3}, {v1}}, G = {{v2}}
    bddp F = bddunion(bddunion(z_v1v2, z_v2v3), z_v1);
    bddp G = z_v2;

    bddp result = bddremainder(F, G);
    bddp expected = bddsubtract(F, bddjoin(G, bdddiv(F, G)));
    EXPECT_EQ(result, expected);
}

TEST_F(BDDTest, ZDDOperatorRemainder) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);

    // {{v1}} % {{v2}} = {{v1}} (no divisor)
    ZDD result = z_v1 % z_v2;
    EXPECT_EQ(result, z_v1);
}

TEST_F(BDDTest, ZDDOperatorRemainderAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    ZDD F(0); F.root = z_v1v2;  // {{v1,v2}}
    ZDD G(0); G.root = getznode(v2, bddempty, bddsingle);  // {{v2}}

    // {{v1,v2}} % {{v2}} = {} (exact division)
    F %= G;
    EXPECT_EQ(F, ZDD::Empty);
}

// --- ZDD operator+ / operator+= ---

TEST_F(BDDTest, ZDDOperatorPlus) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);
    ZDD result = z_v1 + z_v2;
    EXPECT_EQ(result.root, bddunion(z_v1.root, z_v2.root));
}

TEST_F(BDDTest, ZDDOperatorPlusAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);
    bddp expected = bddunion(z_v1.root, z_v2.root);
    z_v1 += z_v2;
    EXPECT_EQ(z_v1.root, expected);
}

// --- ZDD operator- / operator-= ---

TEST_F(BDDTest, ZDDOperatorMinus) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);
    ZDD F(0); F.root = bddunion(z_v1.root, z_v2.root);
    ZDD result = F - z_v2;
    EXPECT_EQ(result, z_v1);
}

TEST_F(BDDTest, ZDDOperatorMinusAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);
    ZDD F(0); F.root = bddunion(z_v1.root, z_v2.root);
    F -= z_v2;
    EXPECT_EQ(F, z_v1);
}

// --- ZDD operator& / operator&= ---

TEST_F(BDDTest, ZDDOperatorIntersec) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);
    ZDD F(0); F.root = bddunion(z_v1.root, z_v2.root);
    ZDD result = F & z_v1;
    EXPECT_EQ(result, z_v1);
}

TEST_F(BDDTest, ZDDOperatorIntersecAssign) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v2(0); z_v2.root = getznode(v2, bddempty, bddsingle);
    ZDD F(0); F.root = bddunion(z_v1.root, z_v2.root);
    F &= z_v1;
    EXPECT_EQ(F, z_v1);
}

// --- ZDD operator== / operator!= ---

TEST_F(BDDTest, ZDDOperatorEqual) {
    bddvar v1 = bddnewvar();
    ZDD a(0); a.root = getznode(v1, bddempty, bddsingle);
    ZDD b(0); b.root = getznode(v1, bddempty, bddsingle);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
}

TEST_F(BDDTest, ZDDOperatorNotEqual) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a(0); a.root = getznode(v1, bddempty, bddsingle);
    ZDD b(0); b.root = getznode(v2, bddempty, bddsingle);
    EXPECT_FALSE(a == b);
    EXPECT_TRUE(a != b);
}

TEST_F(BDDTest, ZDDOperatorEqualConstants) {
    EXPECT_TRUE(ZDD::Empty == ZDD::Empty);
    EXPECT_TRUE(ZDD::Single == ZDD::Single);
    EXPECT_FALSE(ZDD::Empty == ZDD::Single);
    EXPECT_TRUE(ZDD::Empty != ZDD::Single);
}

// --- ZDD::Change ---

TEST_F(BDDTest, ZDDChangeMethod) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD result = z_v1.Change(v2);
    EXPECT_EQ(result.root, bddchange(z_v1.root, v2));
}

// --- ZDD::Offset ---

TEST_F(BDDTest, ZDDOffsetMethod) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2(0); z_v1v2.root = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD F(0); F.root = bddunion(z_v1.root, z_v1v2.root);
    ZDD result = F.Offset(v2);
    EXPECT_EQ(result, z_v1);
}

// --- ZDD::OnSet ---

TEST_F(BDDTest, ZDDOnSetMethod) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v1v2(0); z_v1v2.root = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    ZDD F(0); F.root = bddunion(z_v1.root, z_v1v2.root);
    ZDD result = F.OnSet(v2);
    EXPECT_EQ(result, z_v1v2);
}

// --- ZDD::OnSet0 ---

TEST_F(BDDTest, ZDDOnSet0Method) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1(0); z_v1.root = getznode(v1, bddempty, bddsingle);
    ZDD z_v1v2(0); z_v1v2.root = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    ZDD F(0); F.root = bddunion(z_v1.root, z_v1v2.root);
    // OnSet0(v2) returns sets containing v2 with v2 removed = {{v1}}
    ZDD result = F.OnSet0(v2);
    EXPECT_EQ(result, z_v1);
}

// --- bdddisjoin ---

TEST_F(BDDTest, ZDDDisjoinTerminalCases) {
    EXPECT_EQ(bdddisjoin(bddempty, bddempty), bddempty);
    EXPECT_EQ(bdddisjoin(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bdddisjoin(bddsingle, bddempty), bddempty);
    EXPECT_EQ(bdddisjoin(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDDisjoinIdentity) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}

    // {∅} ▷◁˙ F = F
    EXPECT_EQ(bdddisjoin(bddsingle, z_v1), z_v1);
    EXPECT_EQ(bdddisjoin(z_v1, bddsingle), z_v1);
}

TEST_F(BDDTest, ZDDDisjoinDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // {{v1}} ▷◁˙ {{v2}} = {{v1,v2}} (disjoint, same as join)
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    EXPECT_EQ(bdddisjoin(z_v1, z_v2), z_v1v2);
}

TEST_F(BDDTest, ZDDDisjoinSameVar) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}

    // {{v1}} ▷◁˙ {{v1}} = {} (v1 ∩ v1 = {v1} ≠ ∅, excluded)
    EXPECT_EQ(bdddisjoin(z_v1, z_v1), bddempty);
}

TEST_F(BDDTest, ZDDDisjoinOverlappingFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // F = {{v1}, {v2}}, G = {{v1}}
    // Pairs: ({v1},{v1}): {v1}∩{v1}≠∅ excluded; ({v2},{v1}): disjoint → {v1,v2}
    bddp F = bddunion(z_v1, z_v2);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    EXPECT_EQ(bdddisjoin(F, z_v1), z_v1v2);
}

TEST_F(BDDTest, ZDDDisjoinCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    bddp F = bddunion(z_v1, bddsingle);
    bddp G = bddunion(z_v2, z_v1);

    EXPECT_EQ(bdddisjoin(F, G), bdddisjoin(G, F));
}

TEST_F(BDDTest, ZDDDisjoinJoinDecomposition) {
    // Join = Disjoint join ∪ Joint join
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    bddp F = bddunion(z_v1, z_v1v2);   // {{v1}, {v1,v2}}
    bddp G = bddunion(z_v2, z_v3);      // {{v2}, {v3}}

    bddp join = bddjoin(F, G);
    bddp disjoin = bdddisjoin(F, G);
    bddp jointjoin = bddjointjoin(F, G);
    EXPECT_EQ(bddunion(disjoin, jointjoin), join);
}

TEST_F(BDDTest, ZDDDisjoinThreeVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    // F = {{v1,v2}}, G = {{v2}, {v3}}
    bddp F = z_v1v2;
    bddp G = bddunion(z_v2, z_v3);

    // ({v1,v2},{v2}): {v1,v2}∩{v2}={v2}≠∅ → excluded
    // ({v1,v2},{v3}): disjoint → {v1,v2,v3}
    bddp z_v1v2v3 = getznode(v3, bddempty, getznode(v2, bddempty, getznode(v1, bddempty, bddsingle)));
    EXPECT_EQ(bdddisjoin(F, G), z_v1v2v3);
}

// --- bddjointjoin ---

TEST_F(BDDTest, ZDDJointjoinTerminalCases) {
    EXPECT_EQ(bddjointjoin(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddjointjoin(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddjointjoin(bddsingle, bddempty), bddempty);
    // {∅} ▷◁ˆ {∅}: ∅ ∩ ∅ = ∅, not ≠ ∅ → excluded
    EXPECT_EQ(bddjointjoin(bddsingle, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDJointjoinWithSingle) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);

    // {∅} ▷◁ˆ {{v1}}: ∅ ∩ {v1} = ∅ → excluded
    EXPECT_EQ(bddjointjoin(bddsingle, z_v1), bddempty);
    EXPECT_EQ(bddjointjoin(z_v1, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDJointjoinSameVar) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);

    // {{v1}} ▷◁ˆ {{v1}} = {{v1}} ({v1}∩{v1}={v1}≠∅, {v1}∪{v1}={v1})
    EXPECT_EQ(bddjointjoin(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDJointjoinDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // {{v1}} ▷◁ˆ {{v2}}: {v1}∩{v2}=∅ → excluded
    EXPECT_EQ(bddjointjoin(z_v1, z_v2), bddempty);
}

TEST_F(BDDTest, ZDDJointjoinOverlappingFamilies) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // F = {{v1}, {v2}}, G = {{v1}}
    // ({v1},{v1}): {v1}∩{v1}≠∅ → {v1}
    // ({v2},{v1}): {v2}∩{v1}=∅ → excluded
    bddp F = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddjointjoin(F, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDJointjoinCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    bddp F = bddunion(z_v1, bddsingle);
    bddp G = bddunion(z_v2, z_v1);

    EXPECT_EQ(bddjointjoin(F, G), bddjointjoin(G, F));
}

TEST_F(BDDTest, ZDDJointjoinComplex) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    // F = {{v1,v2}}, G = {{v2}, {v3}}
    bddp F = z_v1v2;
    bddp G = bddunion(z_v2, z_v3);

    // ({v1,v2},{v2}): {v1,v2}∩{v2}={v2}≠∅ → {v1,v2}
    // ({v1,v2},{v3}): {v1,v2}∩{v3}=∅ → excluded
    EXPECT_EQ(bddjointjoin(F, G), z_v1v2);
}

TEST_F(BDDTest, ZDDJointjoinSubsetOfJoin) {
    // Joint join ⊆ Join (every joint join result is also a join result)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    bddp F = bddunion(z_v1, z_v1v2);
    bddp G = bddunion(z_v2, z_v3);

    bddp join = bddjoin(F, G);
    bddp jointjoin = bddjointjoin(F, G);
    // jointjoin ⊆ join means jointjoin \ join = ∅
    EXPECT_EQ(bddsubtract(jointjoin, join), bddempty);
}

TEST_F(BDDTest, ZDDDisjoinJointjoinCoverJoin) {
    // Join = Disjoin ∪ Jointjoin (with different families to avoid overlap)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp z_v2v3 = getznode(v3, bddempty, getznode(v2, bddempty, bddsingle));

    // F = {{v1}}, G = {{v2}, {v2,v3}}
    bddp F = z_v1;
    bddp G = bddunion(z_v2, z_v2v3);

    // All pairs are disjoint (v1 doesn't appear in G)
    bddp join = bddjoin(F, G);
    bddp disjoin = bdddisjoin(F, G);
    bddp jointjoin = bddjointjoin(F, G);
    EXPECT_EQ(bddunion(disjoin, jointjoin), join);
    EXPECT_EQ(disjoin, join);
    EXPECT_EQ(jointjoin, bddempty);
}

// --- bddrestrict ---

TEST_F(BDDTest, ZDDRestrictTerminalCases) {
    EXPECT_EQ(bddrestrict(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddrestrict(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddrestrict(bddsingle, bddempty), bddempty);
    // {∅} △ {∅}: ∃B∈{∅}: B⊆∅ → yes → {∅}
    EXPECT_EQ(bddrestrict(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDRestrictEmptySetFilter) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}

    // G = {∅}: ∅ ⊆ every A → return F
    EXPECT_EQ(bddrestrict(z_v1, bddsingle), z_v1);
}

TEST_F(BDDTest, ZDDRestrictSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);

    EXPECT_EQ(bddrestrict(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDRestrictSubsetFilter) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);  // {{v2}}
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));  // {{v1,v2}}

    // F = {{v1}, {v2}, {v1,v2}}, G = {{v1}}
    // {v1}⊆{v1}? yes. {v1}⊆{v2}? no. {v1}⊆{v1,v2}? yes.
    // result = {{v1}, {v1,v2}}
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    bddp expected = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddrestrict(F, z_v1), expected);
}

TEST_F(BDDTest, ZDDRestrictNoMatch) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}
    bddp z_v2 = getznode(v2, bddempty, bddsingle);  // {{v2}}

    // F = {{v1}}, G = {{v2}}: {v2}⊆{v1}? no → {}
    EXPECT_EQ(bddrestrict(z_v1, z_v2), bddempty);
}

TEST_F(BDDTest, ZDDRestrictMultipleFilters) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp z_v2v3 = getznode(v3, bddempty, getznode(v2, bddempty, bddsingle));
    bddp z_v1v2v3 = getznode(v3, bddempty, getznode(v2, bddempty, getznode(v1, bddempty, bddsingle)));

    // F = {{v1,v2}, {v2,v3}, {v1,v2,v3}}, G = {{v1}, {v3}}
    // {v1}⊆{v1,v2}? yes. {v1}⊆{v2,v3}? no. {v3}⊆{v2,v3}? yes.
    // {v1}⊆{v1,v2,v3}? yes.
    // result = {{v1,v2}, {v2,v3}, {v1,v2,v3}}
    bddp F = bddunion(bddunion(z_v1v2, z_v2v3), z_v1v2v3);
    bddp G = bddunion(z_v1, z_v3);
    EXPECT_EQ(bddrestrict(F, G), F);
}

TEST_F(BDDTest, ZDDRestrictEmptySetInG) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // F = {{v1}}, G = {{v2}} → {v2}⊄{v1} → {}
    EXPECT_EQ(bddrestrict(z_v1, z_v2), bddempty);

    // G = {{v2}, {}} → ∅⊆{v1} → {{v1}}
    bddp G2 = bddunion(z_v2, bddsingle);
    EXPECT_EQ(bddrestrict(z_v1, G2), z_v1);
}

TEST_F(BDDTest, ZDDRestrictContainsEmptyCheck) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}

    // F = {∅}, G = {{v1}}: need B⊆∅, so B=∅. ∅∉G → {}
    EXPECT_EQ(bddrestrict(bddsingle, z_v1), bddempty);

    // G = {{v1}, {∅}}: ∅∈G → {∅}
    bddp G2 = bddunion(z_v1, bddsingle);
    EXPECT_EQ(bddrestrict(bddsingle, G2), bddsingle);
}

// --- bddpermit ---

TEST_F(BDDTest, ZDDPermitTerminalCases) {
    EXPECT_EQ(bddpermit(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddpermit(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddpermit(bddsingle, bddempty), bddempty);
    // {∅} ⊘ {∅}: ∅⊆∅ → {∅}
    EXPECT_EQ(bddpermit(bddsingle, bddsingle), bddsingle);
}

TEST_F(BDDTest, ZDDPermitEmptySetAlwaysPermitted) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);  // {{v1}}

    // F = {∅}: ∅ ⊆ any B → {∅}
    EXPECT_EQ(bddpermit(bddsingle, z_v1), bddsingle);
}

TEST_F(BDDTest, ZDDPermitSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);

    EXPECT_EQ(bddpermit(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDPermitSupersetFilter) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    // F = {{v1}, {v2}, {v1,v2}}, G = {{v1,v2}}
    // {v1}⊆{v1,v2}? yes. {v2}⊆{v1,v2}? yes. {v1,v2}⊆{v1,v2}? yes.
    // result = {{v1}, {v2}, {v1,v2}}
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    EXPECT_EQ(bddpermit(F, z_v1v2), F);
}

TEST_F(BDDTest, ZDDPermitStrictFilter) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    // F = {{v1}, {v2}, {v1,v2}}, G = {{v1}}
    // {v1}⊆{v1}? yes. {v2}⊆{v1}? no. {v1,v2}⊆{v1}? no.
    // result = {{v1}}
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    EXPECT_EQ(bddpermit(F, z_v1), z_v1);
}

TEST_F(BDDTest, ZDDPermitNoMatch) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // F = {{v1}}, G = {{v2}}: {v1}⊆{v2}? no → {}
    EXPECT_EQ(bddpermit(z_v1, z_v2), bddempty);
}

TEST_F(BDDTest, ZDDPermitWithEmptySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // F = {{v1}, {∅}}, G = {{v2}}
    // {v1}⊆{v2}? no. ∅⊆{v2}? yes.
    // result = {{∅}}
    bddp F = bddunion(z_v1, bddsingle);
    EXPECT_EQ(bddpermit(F, z_v2), bddsingle);
}

TEST_F(BDDTest, ZDDPermitMultipleOptions) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    // F = {{v1}, {v3}}, G = {{v1,v2}, {v3}}
    // {v1}⊆{v1,v2}? yes. {v3}⊆{v3}? yes.
    // result = {{v1}, {v3}}
    bddp F = bddunion(z_v1, z_v3);
    bddp G = bddunion(z_v1v2, z_v3);
    EXPECT_EQ(bddpermit(F, G), F);
}

TEST_F(BDDTest, ZDDPermitGSingletonOnly) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    // F = {{v1}, {v1,v2}}, G = {∅}
    // A ⊆ ∅ only when A = ∅. Neither {v1} nor {v1,v2} is ∅ → {}
    bddp F = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddpermit(F, bddsingle), bddempty);
}

// --- bddnonsup ---

TEST_F(BDDTest, ZDDNonsupTerminalCases) {
    EXPECT_EQ(bddnonsup(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddnonsup(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddnonsup(bddsingle, bddempty), bddsingle);
    // G={∅}: ∅⊆every A → none qualify
    EXPECT_EQ(bddnonsup(bddsingle, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDNonsupEmptyG) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);

    // G=∅ → all qualify
    EXPECT_EQ(bddnonsup(z_v1, bddempty), z_v1);
}

TEST_F(BDDTest, ZDDNonsupSingleG) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);

    // G={∅}: ∅⊆{v1} → none qualify
    EXPECT_EQ(bddnonsup(z_v1, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDNonsupSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);

    EXPECT_EQ(bddnonsup(z_v1, z_v1), bddempty);
}

TEST_F(BDDTest, ZDDNonsupNoSubset) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // F={{v1}}, G={{v2}}: {v2}⊄{v1} → {{v1}} qualifies
    EXPECT_EQ(bddnonsup(z_v1, z_v2), z_v1);
}

TEST_F(BDDTest, ZDDNonsupPartialMatch) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    // F={{v1},{v2},{v1,v2}}, G={{v1}}
    // {v1}⊆{v1}? yes→out. {v1}⊆{v2}? no→keep. {v1}⊆{v1,v2}? yes→out.
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    EXPECT_EQ(bddnonsup(F, z_v1), z_v2);
}

TEST_F(BDDTest, ZDDNonsupEqualsSubtractRestrict) {
    // nonsup(F,G) = F \ restrict(F,G)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    bddp F = bddunion(bddunion(z_v1, z_v2), bddunion(z_v1v2, z_v3));
    bddp G = bddunion(z_v1, z_v3);

    EXPECT_EQ(bddnonsup(F, G), bddsubtract(F, bddrestrict(F, G)));
}

TEST_F(BDDTest, ZDDNonsupCheckEmptyInG) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);

    // F={∅}, G={{v1}}: need ∀B∈G: B⊄∅. {v1}⊄∅? yes → {∅} qualifies
    EXPECT_EQ(bddnonsup(bddsingle, z_v1), bddsingle);

    // F={∅}, G={{v1},{∅}}: ∅⊆∅ → fails
    bddp G2 = bddunion(z_v1, bddsingle);
    EXPECT_EQ(bddnonsup(bddsingle, G2), bddempty);
}

// --- bddnonsub ---

TEST_F(BDDTest, ZDDNonsubTerminalCases) {
    EXPECT_EQ(bddnonsub(bddempty, bddempty), bddempty);
    EXPECT_EQ(bddnonsub(bddempty, bddsingle), bddempty);
    EXPECT_EQ(bddnonsub(bddsingle, bddempty), bddsingle);
    // F={∅}, G={∅}: ∅⊆∅ → fails
    EXPECT_EQ(bddnonsub(bddsingle, bddsingle), bddempty);
}

TEST_F(BDDTest, ZDDNonsubEmptyG) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);

    EXPECT_EQ(bddnonsub(z_v1, bddempty), z_v1);
}

TEST_F(BDDTest, ZDDNonsubSelf) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);

    EXPECT_EQ(bddnonsub(z_v1, z_v1), bddempty);
}

TEST_F(BDDTest, ZDDNonsubNoSuperset) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // F={{v1}}, G={{v2}}: {v1}⊄{v2} → {{v1}} qualifies
    EXPECT_EQ(bddnonsub(z_v1, z_v2), z_v1);
}

TEST_F(BDDTest, ZDDNonsubPartialMatch) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    // F={{v1},{v2},{v1,v2}}, G={{v1,v2}}
    // {v1}⊆{v1,v2}? yes→out. {v2}⊆{v1,v2}? yes→out. {v1,v2}⊆{v1,v2}? yes→out.
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    EXPECT_EQ(bddnonsub(F, z_v1v2), bddempty);
}

TEST_F(BDDTest, ZDDNonsubStrictFilter) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    // F={{v1},{v2},{v1,v2}}, G={{v1}}
    // {v1}⊆{v1}? yes→out. {v2}⊆{v1}? no→keep. {v1,v2}⊆{v1}? no→keep.
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    bddp expected = bddunion(z_v2, z_v1v2);
    EXPECT_EQ(bddnonsub(F, z_v1), expected);
}

TEST_F(BDDTest, ZDDNonsubEqualsSubtractPermit) {
    // nonsub(F,G) = F \ permit(F,G)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    bddp F = bddunion(bddunion(z_v1, z_v2), bddunion(z_v1v2, z_v3));
    bddp G = bddunion(z_v1v2, z_v3);

    EXPECT_EQ(bddnonsub(F, G), bddsubtract(F, bddpermit(F, G)));
}

TEST_F(BDDTest, ZDDNonsubFVarNotInG) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));

    // F={{v1},{v1,v2}}, G={{v2}}
    // {v1}⊆{v2}? no→keep. {v1,v2}⊆{v2}? no→keep.
    bddp F = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddnonsub(F, z_v2), F);
}

TEST_F(BDDTest, ZDDNonsubWithEmptySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);

    // F={{v1},{∅}}, G={{v2}}
    // {v1}⊆{v2}? no→keep. ∅⊆{v2}? yes→out.
    bddp F = bddunion(z_v1, bddsingle);
    EXPECT_EQ(bddnonsub(F, z_v2), z_v1);
}

// --- bddmaximal ---

TEST_F(BDDTest, BddMaximalEmpty) {
    EXPECT_EQ(bddmaximal(bddempty), bddempty);
}

TEST_F(BDDTest, BddMaximalSingle) {
    EXPECT_EQ(bddmaximal(bddsingle), bddsingle);
}

TEST_F(BDDTest, BddMaximalSingleton) {
    // {{v1}} → maximal = {{v1}}
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddmaximal(z_v1), z_v1);
}

TEST_F(BDDTest, BddMaximalSubsetRemoved) {
    // F = {{v1}, {v1,v2}} → {v1} ⊊ {v1,v2}, so maximal = {{v1,v2}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp F = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddmaximal(F), z_v1v2);
}

TEST_F(BDDTest, BddMaximalNoSubsets) {
    // F = {{v1}, {v2}} → neither is subset of other → maximal = F
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddmaximal(F), F);
}

TEST_F(BDDTest, BddMaximalChain) {
    // F = {{v1}, {v1,v2}, {v1,v2,v3}} → maximal = {{v1,v2,v3}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp z_v1v2v3 = getznode(v3, bddempty,
        getznode(v2, bddempty, getznode(v1, bddempty, bddsingle)));
    bddp F = bddunion(bddunion(z_v1, z_v1v2), z_v1v2v3);
    EXPECT_EQ(bddmaximal(F), z_v1v2v3);
}

TEST_F(BDDTest, BddMaximalWithEmptySet) {
    // F = {∅, {v1}} → ∅ ⊊ {v1}, so maximal = {{v1}}
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp F = bddunion(bddsingle, z_v1);
    EXPECT_EQ(bddmaximal(F), z_v1);
}

TEST_F(BDDTest, BddMaximalMixed) {
    // F = {{v1}, {v2}, {v1,v2}} → maximal = {{v1,v2}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    EXPECT_EQ(bddmaximal(F), z_v1v2);
}

// --- bddminimal ---

TEST_F(BDDTest, BddMinimalEmpty) {
    EXPECT_EQ(bddminimal(bddempty), bddempty);
}

TEST_F(BDDTest, BddMinimalSingle) {
    EXPECT_EQ(bddminimal(bddsingle), bddsingle);
}

TEST_F(BDDTest, BddMinimalSingleton) {
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddminimal(z_v1), z_v1);
}

TEST_F(BDDTest, BddMinimalSupersetRemoved) {
    // F = {{v1}, {v1,v2}} → {v1} ⊊ {v1,v2}, so minimal = {{v1}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp F = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddminimal(F), z_v1);
}

TEST_F(BDDTest, BddMinimalNoSubsets) {
    // F = {{v1}, {v2}} → minimal = F
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddminimal(F), F);
}

TEST_F(BDDTest, BddMinimalChain) {
    // F = {{v1}, {v1,v2}, {v1,v2,v3}} → minimal = {{v1}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp z_v1v2v3 = getznode(v3, bddempty,
        getznode(v2, bddempty, getznode(v1, bddempty, bddsingle)));
    bddp F = bddunion(bddunion(z_v1, z_v1v2), z_v1v2v3);
    EXPECT_EQ(bddminimal(F), z_v1);
}

TEST_F(BDDTest, BddMinimalWithEmptySet) {
    // F = {∅, {v1}} → minimal = {∅}
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp F = bddunion(bddsingle, z_v1);
    EXPECT_EQ(bddminimal(F), bddsingle);
}

TEST_F(BDDTest, BddMinimalMixed) {
    // F = {{v1}, {v2}, {v1,v2}} → minimal = {{v1}, {v2}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    bddp expected = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddminimal(F), expected);
}

TEST_F(BDDTest, BddMaximalMinimalInverse) {
    // For any F: minimal(maximal(F)) = maximal(F) and maximal(minimal(F)) = minimal(F)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp z_v2v3 = getznode(v3, bddempty, getznode(v2, bddempty, bddsingle));
    bddp F = bddunion(bddunion(z_v1, z_v2), bddunion(z_v1v2, z_v2v3));
    bddp maxF = bddmaximal(F);
    bddp minF = bddminimal(F);
    EXPECT_EQ(bddminimal(maxF), maxF);
    EXPECT_EQ(bddmaximal(minF), minF);
}

// --- bddminhit ---

TEST_F(BDDTest, BddMinhitEmpty) {
    // minhit(∅) = {∅} (no constraints → empty set hits everything vacuously)
    EXPECT_EQ(bddminhit(bddempty), bddsingle);
}

TEST_F(BDDTest, BddMinhitSingle) {
    // minhit({∅}) = ∅ (impossible to hit ∅)
    EXPECT_EQ(bddminhit(bddsingle), bddempty);
}

TEST_F(BDDTest, BddMinhitSingleton) {
    // minhit({{v1}}) = {{v1}} (must contain v1)
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddminhit(z_v1), z_v1);
}

TEST_F(BDDTest, BddMinhitTwoDisjoint) {
    // minhit({{v1}, {v2}}) = {{v1,v2}} (must hit both)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    EXPECT_EQ(bddminhit(F), z_v1v2);
}

TEST_F(BDDTest, BddMinhitTwoOverlapping) {
    // minhit({{v1,v2}, {v2,v3}}) = {{v2}, {v1,v3}}
    // {v2} hits both, {v1,v3} also hits both ({v1}∩{v1,v2}≠∅, {v3}∩{v2,v3}≠∅)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp z_v2v3 = getznode(v3, bddempty, getznode(v2, bddempty, bddsingle));
    bddp F = bddunion(z_v1v2, z_v2v3);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v1v3 = getznode(v3, bddempty, getznode(v1, bddempty, bddsingle));
    bddp expected = bddunion(z_v2, z_v1v3);
    EXPECT_EQ(bddminhit(F), expected);
}

TEST_F(BDDTest, BddMinhitMultipleSolutions) {
    // minhit({{v1}, {v2}}) = {{v1,v2}}
    // But minhit({{v1,v2}}) = {{v1}, {v2}} (pick either element)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp expected = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddminhit(z_v1v2), expected);
}

TEST_F(BDDTest, BddMinhitWithEmptySet) {
    // minhit({∅, {v1}}) = ∅ (∅ ∈ F, impossible to hit)
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp F = bddunion(bddsingle, z_v1);
    EXPECT_EQ(bddminhit(F), bddempty);
}

TEST_F(BDDTest, BddMinhitDoubleApplication) {
    // minhit(minhit(F)) relates back to F in specific ways
    // For F = {{v1}, {v2}}: minhit = {{v1,v2}}, minhit(minhit) = {{v1},{v2}} = F
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddminhit(bddminhit(F)), F);
}

TEST_F(BDDTest, BddMinhitThreeWay) {
    // minhit({{v1}, {v2}, {v3}}) = {{v1,v2,v3}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp z_v3 = getznode(v3, bddempty, bddsingle);
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v3);
    bddp z_v1v2v3 = getznode(v3, bddempty,
        getznode(v2, bddempty, getznode(v1, bddempty, bddsingle)));
    EXPECT_EQ(bddminhit(F), z_v1v2v3);
}

// --- bddclosure ---

TEST_F(BDDTest, BddClosureEmpty) {
    EXPECT_EQ(bddclosure(bddempty), bddempty);
}

TEST_F(BDDTest, BddClosureSingle) {
    EXPECT_EQ(bddclosure(bddsingle), bddsingle);
}

TEST_F(BDDTest, BddClosureSingleton) {
    // closure({{v1}}) = {{v1}}
    bddvar v1 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddclosure(z_v1), z_v1);
}

TEST_F(BDDTest, BddClosureTwoSets) {
    // closure({{v1}, {v2}}) = {{v1}, {v2}, ∅} ({v1}∩{v2} = ∅)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    bddp expected = bddunion(bddunion(z_v1, z_v2), bddsingle);
    EXPECT_EQ(bddclosure(F), expected);
}

TEST_F(BDDTest, BddClosureSubsets) {
    // closure({{v1}, {v1,v2}}) = {{v1}, {v1,v2}}
    // ({v1} ∩ {v1,v2} = {v1} already in F)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp F = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddclosure(F), F);
}

TEST_F(BDDTest, BddClosureThreeSets) {
    // closure({{v1,v2}, {v2,v3}}) = {{v1,v2}, {v2,v3}, {v2}}
    // ({v1,v2} ∩ {v2,v3} = {v2})
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp z_v2v3 = getznode(v3, bddempty, getznode(v2, bddempty, bddsingle));
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1v2, z_v2v3);
    bddp expected = bddunion(bddunion(z_v1v2, z_v2v3), z_v2);
    EXPECT_EQ(bddclosure(F), expected);
}

TEST_F(BDDTest, BddClosureIdempotent) {
    // closure(closure(F)) = closure(F)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = getznode(v1, bddempty, bddsingle);
    bddp z_v2 = getznode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    bddp C = bddclosure(F);
    EXPECT_EQ(bddclosure(C), C);
}

TEST_F(BDDTest, BddClosureContainsOriginal) {
    // closure(F) ⊇ F always
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1v2 = getznode(v2, bddempty, getznode(v1, bddempty, bddsingle));
    bddp z_v2v3 = getznode(v3, bddempty, getznode(v2, bddempty, bddsingle));
    bddp z_v1v3 = getznode(v3, bddempty, getznode(v1, bddempty, bddsingle));
    bddp F = bddunion(bddunion(z_v1v2, z_v2v3), z_v1v3);
    bddp C = bddclosure(F);
    EXPECT_EQ(bddunion(C, F), C);
}

// --- bddisbdd / bddiszbdd ---

TEST_F(BDDTest, BddIsBddNotSupported) {
    EXPECT_THROW(bddisbdd(bddfalse), std::logic_error);
    EXPECT_THROW(bddisbdd(bddtrue), std::logic_error);
}

TEST_F(BDDTest, BddIsZbddNotSupported) {
    EXPECT_THROW(bddiszbdd(bddfalse), std::logic_error);
    EXPECT_THROW(bddiszbdd(bddtrue), std::logic_error);
}

// --- bddcard tests ---

TEST_F(BDDTest, BddCardTerminals) {
    // Empty family has 0 elements
    EXPECT_EQ(bddcard(bddempty), 0u);
    // Single family {{}} has 1 element
    EXPECT_EQ(bddcard(bddsingle), 1u);
}

TEST_F(BDDTest, BddCardSingleVariable) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // {{v1}} -> 1 element
    bddp x1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddcard(x1), 1u);

    // {{v2}} -> 1 element
    bddp x2 = getznode(v2, bddempty, bddsingle);
    EXPECT_EQ(bddcard(x2), 1u);
}

TEST_F(BDDTest, BddCardUnion) {
    bddvar v1 = bddnewvar();

    // {{}, {v1}} -> 2 elements
    bddp x1 = getznode(v1, bddempty, bddsingle);
    bddp f = bddunion(x1, bddsingle);  // {{v1}} ∪ {{}} = {{}, {v1}}
    EXPECT_EQ(bddcard(f), 2u);
}

TEST_F(BDDTest, BddCardMultipleVariables) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // {{v1}, {v2}, {v3}} -> 3 elements
    bddp x1 = getznode(v1, bddempty, bddsingle);
    bddp x2 = getznode(v2, bddempty, bddsingle);
    bddp x3 = getznode(v3, bddempty, bddsingle);
    bddp f = bddunion(bddunion(x1, x2), x3);
    EXPECT_EQ(bddcard(f), 3u);
}

TEST_F(BDDTest, BddCardPowerSet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Power set of {v1,v2} = {{}, {v1}, {v2}, {v1,v2}} -> 4 elements
    bddp x1 = getznode(v1, bddempty, bddsingle);
    bddp x2 = getznode(v2, bddempty, bddsingle);
    bddp x12 = getznode(v1, bddempty, getznode(v2, bddempty, bddsingle));
    bddp f = bddunion(bddunion(bddunion(bddsingle, x1), x2), x12);
    EXPECT_EQ(bddcard(f), 4u);
}

TEST_F(BDDTest, BddCardComplement) {
    bddvar v1 = bddnewvar();

    // bddnot on ZDD toggles empty set membership
    // {{v1}} -> complement -> {{}, {v1}} if ∅ was not in the family
    bddp x1 = getznode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddcard(x1), 1u);
    EXPECT_EQ(bddcard(bddnot(x1)), 2u);

    // {{}} -> complement -> {} (empty family)
    EXPECT_EQ(bddcard(bddsingle), 1u);
    EXPECT_EQ(bddcard(bddnot(bddsingle)), 0u);

    // {} -> complement -> {{}}
    EXPECT_EQ(bddcard(bddempty), 0u);
    EXPECT_EQ(bddcard(bddnot(bddempty)), 1u);
}

TEST_F(BDDTest, BddCardComplementWithEmptySet) {
    bddvar v1 = bddnewvar();

    // Family {{}, {v1}} contains ∅ -> complement removes ∅ -> {{v1}}
    bddp x1 = getznode(v1, bddempty, bddsingle);
    bddp f = bddunion(x1, bddsingle);  // {{}, {v1}}
    EXPECT_EQ(bddcard(f), 2u);
    EXPECT_EQ(bddcard(bddnot(f)), 1u);
}

TEST_F(BDDTest, BddCardLargerFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // Build {{v1}, {v2}, {v3}, {v1,v2}, {v1,v3}, {v2,v3}, {v1,v2,v3}} = 7 elements
    bddp x1 = getznode(v1, bddempty, bddsingle);
    bddp x2 = getznode(v2, bddempty, bddsingle);
    bddp x3 = getznode(v3, bddempty, bddsingle);
    bddp x12 = getznode(v1, bddempty, getznode(v2, bddempty, bddsingle));
    bddp x13 = getznode(v1, bddempty, getznode(v3, bddempty, bddsingle));
    bddp x23 = getznode(v2, bddempty, getznode(v3, bddempty, bddsingle));
    bddp x123 = getznode(v1, bddempty, getznode(v2, bddempty, getznode(v3, bddempty, bddsingle)));

    bddp f = bddunion(bddunion(bddunion(x1, x2), bddunion(x3, x12)),
                       bddunion(bddunion(x13, x23), x123));
    EXPECT_EQ(bddcard(f), 7u);

    // Add empty set -> full power set: 8 elements
    bddp g = bddunion(f, bddsingle);
    EXPECT_EQ(bddcard(g), 8u);
}

// --- Issue #1: node_max exhaustion returns bddnull instead of OOB write ---

TEST_F(BDDTest, GetNodeReturnsNullWhenNodeMaxExhausted) {
    bddinit(1, 1);  // capacity=1, max=1
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // First prime uses 1 node — should succeed
    bddp p1 = bddprime(v1);
    EXPECT_NE(p1, bddnull);
    // Second prime needs another node but max is reached — should return bddnull
    bddp p2 = bddprime(v2);
    EXPECT_EQ(p2, bddnull);
}

TEST_F(BDDTest, GetZNodeReturnsNullWhenNodeMaxExhausted) {
    bddinit(1, 1);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z1 = getznode(v1, bddempty, bddsingle);
    EXPECT_NE(z1, bddnull);
    bddp z2 = getznode(v2, bddempty, bddsingle);
    EXPECT_EQ(z2, bddnull);
}

// --- Issue #2: bddnull propagation through all operations ---

TEST_F(BDDTest, BddNullPropagatesThroughBddOps) {
    bddvar v = bddnewvar();
    bddp f = bddprime(v);

    // bddnot
    EXPECT_EQ(bddnot(bddnull), bddnull);

    // BDD binary ops with bddnull as first arg
    EXPECT_EQ(bddand(bddnull, f), bddnull);
    EXPECT_EQ(bddor(bddnull, f), bddnull);
    EXPECT_EQ(bddxor(bddnull, f), bddnull);
    EXPECT_EQ(bddnand(bddnull, f), bddnull);
    EXPECT_EQ(bddnor(bddnull, f), bddnull);
    EXPECT_EQ(bddxnor(bddnull, f), bddnull);

    // BDD binary ops with bddnull as second arg
    EXPECT_EQ(bddand(f, bddnull), bddnull);
    EXPECT_EQ(bddor(f, bddnull), bddnull);
    EXPECT_EQ(bddxor(f, bddnull), bddnull);

    // bddite
    EXPECT_EQ(bddite(bddnull, f, bddfalse), bddnull);
    EXPECT_EQ(bddite(f, bddnull, bddfalse), bddnull);
    EXPECT_EQ(bddite(f, bddtrue, bddnull), bddnull);

    // bddimply
    EXPECT_EQ(bddimply(bddnull, f), -1);
    EXPECT_EQ(bddimply(f, bddnull), -1);

    // cofactor
    EXPECT_EQ(bddat0(bddnull, v), bddnull);
    EXPECT_EQ(bddat1(bddnull, v), bddnull);
    EXPECT_EQ(bddcofactor(bddnull, f), bddnull);
    EXPECT_EQ(bddcofactor(f, bddnull), bddnull);

    // quantification
    EXPECT_EQ(bddexist(bddnull, f), bddnull);
    EXPECT_EQ(bdduniv(bddnull, f), bddnull);

    // shift
    EXPECT_EQ(bddlshift(bddnull, 1), bddnull);
    EXPECT_EQ(bddrshift(bddnull, 1), bddnull);

    // support
    EXPECT_EQ(bddsupport(bddnull), bddnull);
    EXPECT_TRUE(bddsupport_vec(bddnull).empty());
}

TEST_F(BDDTest, BddNullPropagatesThroughZddOps) {
    bddvar v = bddnewvar();
    bddp z = getznode(v, bddempty, bddsingle);

    // ZDD unary-var ops
    EXPECT_EQ(bddoffset(bddnull, v), bddnull);
    EXPECT_EQ(bddonset(bddnull, v), bddnull);
    EXPECT_EQ(bddonset0(bddnull, v), bddnull);
    EXPECT_EQ(bddchange(bddnull, v), bddnull);

    // ZDD binary ops with bddnull as first arg
    EXPECT_EQ(bddunion(bddnull, z), bddnull);
    EXPECT_EQ(bddintersec(bddnull, z), bddnull);
    EXPECT_EQ(bddsubtract(bddnull, z), bddnull);
    EXPECT_EQ(bdddiv(bddnull, z), bddnull);
    EXPECT_EQ(bddsymdiff(bddnull, z), bddnull);
    EXPECT_EQ(bddjoin(bddnull, z), bddnull);
    EXPECT_EQ(bddmeet(bddnull, z), bddnull);
    EXPECT_EQ(bdddelta(bddnull, z), bddnull);
    EXPECT_EQ(bddremainder(bddnull, z), bddnull);
    EXPECT_EQ(bdddisjoin(bddnull, z), bddnull);
    EXPECT_EQ(bddjointjoin(bddnull, z), bddnull);
    EXPECT_EQ(bddrestrict(bddnull, z), bddnull);
    EXPECT_EQ(bddpermit(bddnull, z), bddnull);
    EXPECT_EQ(bddnonsup(bddnull, z), bddnull);
    EXPECT_EQ(bddnonsub(bddnull, z), bddnull);

    // ZDD binary ops with bddnull as second arg
    EXPECT_EQ(bddunion(z, bddnull), bddnull);
    EXPECT_EQ(bddintersec(z, bddnull), bddnull);
    EXPECT_EQ(bddsubtract(z, bddnull), bddnull);
    EXPECT_EQ(bdddiv(z, bddnull), bddnull);

    // ZDD unary ops
    EXPECT_EQ(bddmaximal(bddnull), bddnull);
    EXPECT_EQ(bddminimal(bddnull), bddnull);
    EXPECT_EQ(bddminhit(bddnull), bddnull);
    EXPECT_EQ(bddclosure(bddnull), bddnull);

    // bddcard
    EXPECT_EQ(bddcard(bddnull), 0u);
}
