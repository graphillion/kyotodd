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

class BDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

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

// --- bddlshiftb ---

TEST_F(BDDTest, BddLshiftTerminals) {
    EXPECT_EQ(bddlshiftb(bddfalse, 1), bddfalse);
    EXPECT_EQ(bddlshiftb(bddtrue, 1), bddtrue);
    EXPECT_EQ(bddlshiftb(bddfalse, 5), bddfalse);
}

TEST_F(BDDTest, BddLshiftZero) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddlshiftb(p, 0), p);
}

TEST_F(BDDTest, BddLshiftSingleVar) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    // level 1 + 1 = level 2 -> v2
    EXPECT_EQ(bddlshiftb(p1, 1), p2);
}

TEST_F(BDDTest, BddLshiftComplement) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddlshiftb(bddnot(p1), 1), bddnot(p2));
}

TEST_F(BDDTest, BddLshiftTwoVars) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddvar v3 = BDD_NewVar();  // level 3
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    // v1 & v2, shift 1: level 1->2(v2), level 2->3(v3) => v2 & v3
    EXPECT_EQ(bddlshiftb(bddand(p1, p2), 1), bddand(p2, p3));
}

TEST_F(BDDTest, BddLshiftAutoNewVar) {
    (void)BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddvarused(), 2u);
    // Shift v2 (level 2) by 1 -> level 3, doesn't exist yet
    bddp result = bddlshiftb(p2, 1);
    EXPECT_EQ(bddvarused(), 3u);
    bddvar v3 = bddvaroflev(3);
    EXPECT_EQ(result, bddprime(v3));
}

TEST_F(BDDTest, BddLshiftAutoNewVarMultiple) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddp p1 = bddprime(v1);
    EXPECT_EQ(bddvarused(), 1u);
    // Shift v1 (level 1) by 3 -> level 4, need vars at levels 2,3,4
    bddp result = bddlshiftb(p1, 3);
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
    EXPECT_EQ(bddlshiftb(bddor(p1, p2), 2), bddor(p3, p4));
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
    EXPECT_EQ(bddlshiftb(bddxor(p1, p2), 2), bddxor(p3, p4));
}

// --- bddrshiftb ---

TEST_F(BDDTest, BddRshiftTerminals) {
    EXPECT_EQ(bddrshiftb(bddfalse, 1), bddfalse);
    EXPECT_EQ(bddrshiftb(bddtrue, 1), bddtrue);
}

TEST_F(BDDTest, BddRshiftZero) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_EQ(bddrshiftb(p, 0), p);
}

TEST_F(BDDTest, BddRshiftSingleVar) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    // level 2 - 1 = level 1 -> v1
    EXPECT_EQ(bddrshiftb(p2, 1), p1);
}

TEST_F(BDDTest, BddRshiftComplement) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    EXPECT_EQ(bddrshiftb(bddnot(p2), 1), bddnot(p1));
}

TEST_F(BDDTest, BddRshiftTwoVars) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddvar v3 = BDD_NewVar();  // level 3
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);
    // v2 & v3, shift 1: level 2->1(v1), level 3->2(v2) => v1 & v2
    EXPECT_EQ(bddrshiftb(bddand(p2, p3), 1), bddand(p1, p2));
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
    EXPECT_EQ(bddrshiftb(bddor(p3, p4), 2), bddor(p1, p2));
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
    EXPECT_EQ(bddrshiftb(bddxor(p3, p4), 2), bddxor(p1, p2));
}

TEST_F(BDDTest, BddRshiftInvertsLshift) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    (void)BDD_NewVar();  // level 3
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    // lshift then rshift should be identity
    EXPECT_EQ(bddrshiftb(bddlshiftb(f, 1), 1), f);
}

TEST_F(BDDTest, BddRshiftTooLarge) {
    bddvar v1 = BDD_NewVar();  // level 1
    bddvar v2 = BDD_NewVar();  // level 2
    bddp p1 = bddprime(v1);
    // v1 is at level 1, shift 1 would go to level 0 (invalid)
    EXPECT_THROW(bddrshiftb(p1, 1), std::invalid_argument);
    // v1 at level 1, shift 2 also invalid
    EXPECT_THROW(bddrshiftb(p1, 2), std::invalid_argument);
    // v2 at level 2, shift 2 also invalid (level 0)
    bddp p2 = bddprime(v2);
    EXPECT_THROW(bddrshiftb(p2, 2), std::invalid_argument);
}

TEST_F(BDDTest, BddLshiftRshiftDeprecated) {
    bddvar v1 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    EXPECT_THROW(bddlshift(p1, 1), std::logic_error);
    EXPECT_THROW(bddrshift(p1, 1), std::logic_error);
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
    bddp cube12 = BDD::getnode(v2, bddprime(v1), bddtrue);
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
    EXPECT_EQ(ZDD::getnode(v1, bddempty, bddempty), bddempty);
    EXPECT_EQ(ZDD::getnode(v1, bddsingle, bddempty), bddsingle);

    bddvar v2 = bddnewvar();
    bddp node = ZDD::getnode(v2, bddempty, bddsingle);
    // Validation runs before zero-suppression rule:
    // node is at level 2 > v1's level 1, so this is invalid
    EXPECT_THROW(ZDD::getnode(v1, node, bddempty), std::invalid_argument);
    // Valid case: lo child at lower level
    EXPECT_EQ(ZDD::getnode(v2, bddempty, bddempty), bddempty);
}

TEST_F(BDDTest, GetznodeNoReductionWhenLoEqualsHi) {
    // Unlike BDD, getznode does NOT reduce when lo == hi
    bddvar v1 = bddnewvar();
    bddp z = ZDD::getnode(v1, bddsingle, bddsingle);
    // This should create a node, not return bddsingle
    EXPECT_NE(z, bddsingle);
    EXPECT_FALSE(z & BDD_CONST_FLAG);
}

TEST_F(BDDTest, GetznodeSingleton) {
    // {{v1}} = ZDD::getnode(v1, bddempty, bddsingle)
    bddvar v1 = bddnewvar();
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_FALSE(z & BDD_CONST_FLAG);
    // lo should be bddempty, hi should be bddsingle
    bddp node = z & ~BDD_COMP_FLAG;
    EXPECT_EQ(z, node);  // no complement on result
}

TEST_F(BDDTest, GetznodeComplementNormalization) {
    // lo is complemented: strip it, complement the result
    // hi is NOT negated (unlike BDD getnode)
    bddvar v1 = bddnewvar();
    bddp z1 = ZDD::getnode(v1, bddempty, bddsingle);   // {{v1}}
    bddp z2 = ZDD::getnode(v1, bddsingle, bddsingle);  // {{}, {v1}}
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
    bddp inner = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}

    // ZDD::getnode(v2, bddsingle, inner)
    // lo=bddsingle is complemented → comp=true, lo=bddempty
    // hi=inner stays unchanged (ZDD rule)
    bddp z = ZDD::getnode(v2, bddsingle, inner);

    // Compare with BDD getnode which would negate hi too
    bddp b = BDD::getnode(v2, bddsingle, inner);

    // They should differ because BDD negates hi but ZDD doesn't
    // getznode stores (v2, bddempty, inner), getnode stores (v2, bddempty, ~inner)
    EXPECT_NE(z & ~BDD_COMP_FLAG, b & ~BDD_COMP_FLAG);
}

TEST_F(BDDTest, GetznodeUniqueTableSharing) {
    // Same (var, lo, hi) returns same node
    bddvar v1 = bddnewvar();
    bddp z1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z2 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(z1, z2);
}

TEST_F(BDDTest, GetznodeTwoElementFamily) {
    // {{v1}, {v2}} using ZDD
    bddvar v1 = bddnewvar();  // level 1
    bddvar v2 = bddnewvar();  // level 2
    // Build bottom-up: v2 is lower level, v1 is higher level
    // Node for v1: lo = ZDD::getnode(v1, bddempty, bddsingle) would be wrong ordering
    // ZDD for {{v1}, {v2}}: at v2 (level 2, top), lo = {{v1}}, hi = {{}}
    //   at v1 (level 1), lo = {}, hi = {{}}
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}} at level 1
    bddp z = ZDD::getnode(v2, z_v1, bddsingle);          // at level 2: lo={{v1}}, hi={{}}
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

TEST_F(BDDTest, ExportBddnullSentinel) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f1 = bddprime(v1);
    bddp f2 = bddprime(v2);
    // bddnull as sentinel: only f1 should be exported
    bddp p[] = { f1, bddnull, f2 };
    std::ostringstream oss;
    bddexport(oss, p, 3);
    std::vector<std::string> lines = split_lines(oss.str());
    EXPECT_EQ(lines[1], "_o 1");  // only 1 output
    // 3 header + 1 node + 1 root = 5 lines
    ASSERT_EQ(lines.size(), 5u);
}

TEST_F(BDDTest, ExportBddnullSentinelAtStart) {
    bddvar v1 = bddnewvar();
    bddp f1 = bddprime(v1);
    bddp p[] = { bddnull, f1 };
    std::ostringstream oss;
    bddexport(oss, p, 2);
    // bddnull at position 0: nothing to export
    EXPECT_TRUE(oss.str().empty());
}

TEST_F(BDDTest, ExportImportRoundtripBddnullSentinel) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f1 = bddand(bddprime(v1), bddprime(v2));
    bddp f2 = bddprime(v1);
    // Export 2 roots with sentinel at position 2
    bddp p_out[] = { f1, f2, bddnull };
    std::ostringstream oss;
    bddexport(oss, p_out, 3);
    // Import: should get 2 outputs
    std::istringstream iss(oss.str());
    std::vector<bddp> result;
    int ret = bddimport(iss, result);
    ASSERT_EQ(ret, 2);
    EXPECT_EQ(result[0], f1);
    EXPECT_EQ(result[1], f2);
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
    // ZDD {{v1}} = ZDD::getnode(v1, bddempty, bddsingle)
    bddvar v1 = bddnewvar();
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
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
    // ZDD {{}, {v1}} = ZDD::getnode(v1, bddsingle, bddsingle)
    bddvar v1 = bddnewvar();
    bddp z = ZDD::getnode(v1, bddsingle, bddsingle);
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
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z = ZDD::getnode(v2, z_v1, bddsingle);
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
    bddp node = ZDD::getnode(v1, bddsingle, bddsingle);
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
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
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
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
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
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
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

