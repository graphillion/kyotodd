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
#include <functional>
#include <limits>
#include <unistd.h>
#include <fcntl.h>

using namespace kyotodd;

class BDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

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
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddmaximal(z_v1), z_v1);
}

TEST_F(BDDTest, BddMaximalSubsetRemoved) {
    // F = {{v1}, {v1,v2}} → {v1} ⊊ {v1,v2}, so maximal = {{v1,v2}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp F = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddmaximal(F), z_v1v2);
}

TEST_F(BDDTest, BddMaximalNoSubsets) {
    // F = {{v1}, {v2}} → neither is subset of other → maximal = F
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddmaximal(F), F);
}

TEST_F(BDDTest, BddMaximalChain) {
    // F = {{v1}, {v1,v2}, {v1,v2,v3}} → maximal = {{v1,v2,v3}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v1v2v3 = ZDD::getnode(v3, bddempty,
        ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    bddp F = bddunion(bddunion(z_v1, z_v1v2), z_v1v2v3);
    EXPECT_EQ(bddmaximal(F), z_v1v2v3);
}

TEST_F(BDDTest, BddMaximalWithEmptySet) {
    // F = {∅, {v1}} → ∅ ⊊ {v1}, so maximal = {{v1}}
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp F = bddunion(bddsingle, z_v1);
    EXPECT_EQ(bddmaximal(F), z_v1);
}

TEST_F(BDDTest, BddMaximalMixed) {
    // F = {{v1}, {v2}, {v1,v2}} → maximal = {{v1,v2}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
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
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddminimal(z_v1), z_v1);
}

TEST_F(BDDTest, BddMinimalSupersetRemoved) {
    // F = {{v1}, {v1,v2}} → {v1} ⊊ {v1,v2}, so minimal = {{v1}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp F = bddunion(z_v1, z_v1v2);
    EXPECT_EQ(bddminimal(F), z_v1);
}

TEST_F(BDDTest, BddMinimalNoSubsets) {
    // F = {{v1}, {v2}} → minimal = F
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddminimal(F), F);
}

TEST_F(BDDTest, BddMinimalChain) {
    // F = {{v1}, {v1,v2}, {v1,v2,v3}} → minimal = {{v1}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v1v2v3 = ZDD::getnode(v3, bddempty,
        ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    bddp F = bddunion(bddunion(z_v1, z_v1v2), z_v1v2v3);
    EXPECT_EQ(bddminimal(F), z_v1);
}

TEST_F(BDDTest, BddMinimalWithEmptySet) {
    // F = {∅, {v1}} → minimal = {∅}
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp F = bddunion(bddsingle, z_v1);
    EXPECT_EQ(bddminimal(F), bddsingle);
}

TEST_F(BDDTest, BddMinimalMixed) {
    // F = {{v1}, {v2}, {v1,v2}} → minimal = {{v1}, {v2}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v1v2);
    bddp expected = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddminimal(F), expected);
}

TEST_F(BDDTest, BddMaximalMinimalInverse) {
    // For any F: minimal(maximal(F)) = maximal(F) and maximal(minimal(F)) = minimal(F)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
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
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddminhit(z_v1), z_v1);
}

TEST_F(BDDTest, BddMinhitTwoDisjoint) {
    // minhit({{v1}, {v2}}) = {{v1,v2}} (must hit both)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(bddminhit(F), z_v1v2);
}

TEST_F(BDDTest, BddMinhitTwoOverlapping) {
    // minhit({{v1,v2}, {v2,v3}}) = {{v2}, {v1,v3}}
    // {v2} hits both, {v1,v3} also hits both ({v1}∩{v1,v2}≠∅, {v3}∩{v2,v3}≠∅)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
    bddp F = bddunion(z_v1v2, z_v2v3);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp expected = bddunion(z_v2, z_v1v3);
    EXPECT_EQ(bddminhit(F), expected);
}

TEST_F(BDDTest, BddMinhitMultipleSolutions) {
    // minhit({{v1}, {v2}}) = {{v1,v2}}
    // But minhit({{v1,v2}}) = {{v1}, {v2}} (pick either element)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp expected = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddminhit(z_v1v2), expected);
}

TEST_F(BDDTest, BddMinhitWithEmptySet) {
    // minhit({∅, {v1}}) = ∅ (∅ ∈ F, impossible to hit)
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp F = bddunion(bddsingle, z_v1);
    EXPECT_EQ(bddminhit(F), bddempty);
}

TEST_F(BDDTest, BddMinhitDoubleApplication) {
    // minhit(minhit(F)) relates back to F in specific ways
    // For F = {{v1}, {v2}}: minhit = {{v1,v2}}, minhit(minhit) = {{v1},{v2}} = F
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    EXPECT_EQ(bddminhit(bddminhit(F)), F);
}

TEST_F(BDDTest, BddMinhitThreeWay) {
    // minhit({{v1}, {v2}, {v3}}) = {{v1,v2,v3}}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp F = bddunion(bddunion(z_v1, z_v2), z_v3);
    bddp z_v1v2v3 = ZDD::getnode(v3, bddempty,
        ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
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
    // closure({{v1}}) = {{v1}, ∅}
    bddvar v1 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp expected = bddunion(z_v1, bddsingle);
    EXPECT_EQ(bddclosure(z_v1), expected);
}

TEST_F(BDDTest, BddClosureTwoSets) {
    // closure({{v1}, {v2}}) = {{v1}, {v2}, ∅} ({v1}∩{v2} = ∅)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    bddp expected = bddunion(bddunion(z_v1, z_v2), bddsingle);
    EXPECT_EQ(bddclosure(F), expected);
}

TEST_F(BDDTest, BddClosureSubsets) {
    // closure({{v1}, {v1,v2}}) = {{v1,v2}, {v1}, {v2}, ∅}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp F = bddunion(z_v1, z_v1v2);
    bddp expected = bddunion(bddunion(z_v1v2, bddunion(z_v1, z_v2)), bddsingle);
    EXPECT_EQ(bddclosure(F), expected);
}

TEST_F(BDDTest, BddClosureThreeSets) {
    // closure({{v1,v2}, {v2,v3}}) = {{v1,v2}, {v2,v3}, {v1}, {v2}, {v3}, ∅}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp z_v3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
    bddp F = bddunion(z_v1v2, z_v2v3);
    bddp expected = bddunion(bddunion(bddunion(z_v1v2, z_v2v3),
                              bddunion(z_v1, bddunion(z_v2, z_v3))),
                              bddsingle);
    EXPECT_EQ(bddclosure(F), expected);
}

TEST_F(BDDTest, BddClosureIdempotent) {
    // closure(closure(F)) = closure(F)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp F = bddunion(z_v1, z_v2);
    bddp C = bddclosure(F);
    EXPECT_EQ(bddclosure(C), C);
}

TEST_F(BDDTest, BddClosureContainsOriginal) {
    // closure(F) ⊇ F always
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp z_v1v2 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp z_v2v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
    bddp z_v1v3 = ZDD::getnode(v3, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
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
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddcard(x1), 1u);

    // {{v2}} -> 1 element
    bddp x2 = ZDD::getnode(v2, bddempty, bddsingle);
    EXPECT_EQ(bddcard(x2), 1u);
}

TEST_F(BDDTest, BddCardUnion) {
    bddvar v1 = bddnewvar();

    // {{}, {v1}} -> 2 elements
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp f = bddunion(x1, bddsingle);  // {{v1}} ∪ {{}} = {{}, {v1}}
    EXPECT_EQ(bddcard(f), 2u);
}

TEST_F(BDDTest, BddCardMultipleVariables) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // {{v1}, {v2}, {v3}} -> 3 elements
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp x2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp x3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp f = bddunion(bddunion(x1, x2), x3);
    EXPECT_EQ(bddcard(f), 3u);
}

TEST_F(BDDTest, BddCardPowerSet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Power set of {v1,v2} = {{}, {v1}, {v2}, {v1,v2}} -> 4 elements
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp x2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp x12 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp f = bddunion(bddunion(bddunion(bddsingle, x1), x2), x12);
    EXPECT_EQ(bddcard(f), 4u);
}

TEST_F(BDDTest, BddCardComplement) {
    bddvar v1 = bddnewvar();

    // bddnot on ZDD toggles empty set membership
    // {{v1}} -> complement -> {{}, {v1}} if ∅ was not in the family
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
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
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp f = bddunion(x1, bddsingle);  // {{}, {v1}}
    EXPECT_EQ(bddcard(f), 2u);
    EXPECT_EQ(bddcard(bddnot(f)), 1u);
}

TEST_F(BDDTest, BddCardLargerFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // Build {{v1}, {v2}, {v3}, {v1,v2}, {v1,v3}, {v2,v3}, {v1,v2,v3}} = 7 elements
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp x2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp x3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp x12 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp x13 = ZDD::getnode(v3, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp x23 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
    bddp x123 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));

    bddp f = bddunion(bddunion(bddunion(x1, x2), bddunion(x3, x12)),
                       bddunion(bddunion(x13, x23), x123));
    EXPECT_EQ(bddcard(f), 7u);

    // Add empty set -> full power set: 8 elements
    bddp g = bddunion(f, bddsingle);
    EXPECT_EQ(bddcard(g), 8u);
}

// --- Issue #1: node_max exhaustion returns bddnull instead of OOB write ---

TEST_F(BDDTest, GetNodeThrowsWhenNodeMaxExhausted) {
    bddinit(1, 1);  // capacity=1, max=1
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // First prime uses 1 node — should succeed
    bddp p1 = bddprime(v1);
    bddgc_protect(&p1);
    EXPECT_NE(p1, bddnull);
    // Second prime needs another node but max is reached — should throw
    EXPECT_THROW(bddprime(v2), std::overflow_error);
    bddgc_unprotect(&p1);
}

TEST_F(BDDTest, GetZNodeThrowsWhenNodeMaxExhausted) {
    bddinit(1, 1);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddgc_protect(&z1);
    EXPECT_NE(z1, bddnull);
    EXPECT_THROW(ZDD::getnode(v2, bddempty, bddsingle), std::overflow_error);
    bddgc_unprotect(&z1);
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
    EXPECT_EQ(bddlshiftb(bddnull, 1), bddnull);
    EXPECT_EQ(bddrshiftb(bddnull, 1), bddnull);
    EXPECT_EQ(bddlshiftz(bddnull, 1), bddnull);
    EXPECT_EQ(bddrshiftz(bddnull, 1), bddnull);

    // support
    EXPECT_EQ(bddsupport(bddnull), bddnull);
    EXPECT_TRUE(bddsupport_vec(bddnull).empty());
}

TEST_F(BDDTest, BddNullPropagatesThroughZddOps) {
    bddvar v = bddnewvar();
    bddp z = ZDD::getnode(v, bddempty, bddsingle);

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

// --- Issue 4: bddvar range checks ---
TEST_F(BDDTest, BddVarRangeCheckThrowsForInvalidVar) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddprime(v1);
    bddp z = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));

    // var == 0 is invalid
    EXPECT_THROW(bddprime(0), std::invalid_argument);
    EXPECT_THROW(bddat0(f, 0), std::invalid_argument);
    EXPECT_THROW(bddat1(f, 0), std::invalid_argument);
    EXPECT_THROW(bddoffset(z, 0), std::invalid_argument);
    EXPECT_THROW(bddonset(z, 0), std::invalid_argument);
    EXPECT_THROW(bddonset0(z, 0), std::invalid_argument);
    EXPECT_THROW(bddchange(z, 0), std::invalid_argument);

    // var > bdd_varcount: bddprime auto-expands, others throw
    bddvar bad = bddvarused() + 1;
    EXPECT_NO_THROW(bddprime(bad));  // auto-expands variables
    EXPECT_THROW(bddat0(f, bad + 1), std::invalid_argument);
    EXPECT_THROW(bddat1(f, bad + 1), std::invalid_argument);
    EXPECT_THROW(bddoffset(z, bad + 1), std::invalid_argument);
    EXPECT_THROW(bddonset(z, bad + 1), std::invalid_argument);
    EXPECT_THROW(bddonset0(z, bad + 1), std::invalid_argument);

    // very large var: bddprime rejects > 65536
    EXPECT_THROW(bddprime(999999), std::invalid_argument);
    EXPECT_THROW(bddat0(f, 999999), std::invalid_argument);
    EXPECT_THROW(bddat1(f, 999999), std::invalid_argument);

    // bddexist/bdduniv with vector containing invalid var
    std::vector<bddvar> bad_vars = {v1, 0};
    EXPECT_THROW(bddexist(f, bad_vars), std::invalid_argument);
    EXPECT_THROW(bdduniv(f, bad_vars), std::invalid_argument);

    std::vector<bddvar> bad_vars2 = {999999};
    EXPECT_THROW(bddexist(f, bad_vars2), std::invalid_argument);
    EXPECT_THROW(bdduniv(f, bad_vars2), std::invalid_argument);
}

TEST_F(BDDTest, BddVarRangeCheckValidVarsStillWork) {
    bddvar v1 = bddnewvar();
    (void)bddnewvar();

    // Valid operations should still work
    bddp f = bddprime(v1);
    EXPECT_NE(f, bddnull);

    bddp r0 = bddat0(f, v1);
    EXPECT_EQ(r0, bddfalse);

    bddp r1 = bddat1(f, v1);
    EXPECT_EQ(r1, bddtrue);

    // ZDD ops with valid vars
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_NO_THROW(bddoffset(z, v1));
    EXPECT_NO_THROW(bddonset(z, v1));
    EXPECT_NO_THROW(bddonset0(z, v1));
    EXPECT_NO_THROW(bddchange(z, v1));

    // bddexist/bdduniv with valid vars
    std::vector<bddvar> vars = {v1};
    EXPECT_NO_THROW(bddexist(bddprime(v1), vars));
    EXPECT_NO_THROW(bdduniv(bddprime(v1), vars));
}

// --- bddite: post-normalization g == h check ---

TEST_F(BDDTest, BddItePostNormalizationGEqualsH) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp p3 = bddprime(v3);

    // After f-complement normalization (swap g,h) and g-complement
    // normalization (negate g,h), g == h may arise.
    // ITE(~f, g, ~g) -> normalize f: ITE(f, ~g, g) -> normalize g: comp=true, g'=g, h'=~g
    // Then g' == ~h' ... not equal.
    // But ITE(~f, g, g) = g is caught before normalization.
    // Construct a case: ITE(f, ~a, a) with f complemented:
    // After f-norm: ITE(~f_raw, ~a, a) -> swap: ITE(f_raw, a, ~a)
    // After g-norm: g=a (non-comp), h=~a, comp=false. g != h, normal recursion.
    // Now: ITE(~f, a, ~a):
    // After f-norm: swap -> ITE(f, ~a, a)
    // After g-norm: g was ~a, so negate both: g=a, h=~a, comp=true. g != h.

    // Best case: ITE(~p1, p2, bddnot(p2))
    // = ~p1 ? p2 : ~p2 = p1 XOR p2 XOR p2... no.
    // = ITE(~p1, p2, ~p2) = XOR(p1, p2)... wait:
    // ITE(f, g, h) = f ? g : h. ITE(~p1, p2, ~p2) = ~p1 ? p2 : ~p2
    // = ~(p1 XOR p2)... no. Let me just verify correctness.
    // ITE(~p1, p2, ~p2) = ~p1 ? p2 : ~p2 = XOR(p1, p2)
    bddp r1 = bddite(bddnot(p1), p2, bddnot(p2));
    EXPECT_EQ(r1, bddxor(p1, p2));

    // ITE(p3, ~p1, ~p2): verify via cofactors
    bddp r2 = bddite(p3, bddnot(p1), bddnot(p2));
    EXPECT_EQ(bddat1(r2, v3), bddnot(p1));
    EXPECT_EQ(bddat0(r2, v3), bddnot(p2));

    // g == h caught before normalization
    EXPECT_EQ(bddite(p1, bddnot(p2), bddnot(p2)), bddnot(p2));
    EXPECT_EQ(bddite(bddnot(p1), p2, p2), p2);
}

// --- bddexport: vector with large-ish data ---

TEST_F(BDDTest, ExportImportVectorRoundtrip) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);

    std::vector<bddp> roots = {bddand(p1, p2), bddor(p1, p2), bddtrue, bddfalse};
    std::ostringstream oss;
    bddexport(oss, roots);

    std::istringstream iss(oss.str());
    std::vector<bddp> imported;
    int ret = bddimport(iss, imported);
    EXPECT_EQ(ret, 4);
    ASSERT_EQ(imported.size(), 4u);
    EXPECT_EQ(imported[0], roots[0]);
    EXPECT_EQ(imported[1], roots[1]);
    EXPECT_EQ(imported[2], bddtrue);
    EXPECT_EQ(imported[3], bddfalse);
}

// --- BDD class high-level member functions ---

TEST_F(BDDTest, BDDClassAt0At1) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    EXPECT_EQ(f.At0(v1), BDD::False);
    EXPECT_EQ(f.At1(v1).GetID(), b.GetID());
    EXPECT_EQ(f.At0(v2).GetID(), BDD::False.GetID());
    EXPECT_EQ(f.At1(v2).GetID(), a.GetID());
}

TEST_F(BDDTest, BDDClassExist) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;

    // Exist with cube
    BDD cube = BDD_ID(bddprime(v1));
    EXPECT_EQ(f.Exist(cube).GetID(), b.GetID());

    // Exist with vector
    std::vector<bddvar> vars = {v1};
    EXPECT_EQ(f.Exist(vars).GetID(), b.GetID());

    // Exist with single variable
    EXPECT_EQ(f.Exist(v1).GetID(), b.GetID());
}

TEST_F(BDDTest, BDDClassUniv) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a | b;

    // Univ with cube
    BDD cube = BDD_ID(bddprime(v1));
    EXPECT_EQ(f.Univ(cube).GetID(), b.GetID());

    // Univ with vector
    std::vector<bddvar> vars = {v1};
    EXPECT_EQ(f.Univ(vars).GetID(), b.GetID());

    // Univ with single variable
    EXPECT_EQ(f.Univ(v1).GetID(), b.GetID());
}

TEST_F(BDDTest, BDDClassExistDuplicateVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;

    std::vector<bddvar> vars_dup = {v1, v1};
    std::vector<bddvar> vars_single = {v1};
    EXPECT_EQ(f.Exist(vars_dup).GetID(), f.Exist(vars_single).GetID());
}

TEST_F(BDDTest, BDDClassUnivDuplicateVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a | b;

    std::vector<bddvar> vars_dup = {v1, v1};
    std::vector<bddvar> vars_single = {v1};
    EXPECT_EQ(f.Univ(vars_dup).GetID(), f.Univ(vars_single).GetID());
}

TEST_F(BDDTest, BDDClassCofactor) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    EXPECT_EQ(f.Cofactor(a).GetID(), b.GetID());
}

TEST_F(BDDTest, BDDClassSupport) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;

    BDD sup = f.Support();
    EXPECT_EQ(sup.GetID(), bddsupport(f.GetID()));

    std::vector<bddvar> sv = f.SupportVec();
    EXPECT_EQ(sv.size(), 2u);
}

TEST_F(BDDTest, BDDClassImply) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    EXPECT_EQ(f.Imply(a), 1);
    EXPECT_EQ(a.Imply(f), 0);
}

TEST_F(BDDTest, BDDClassSize) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    EXPECT_EQ(f.Size(), bddsize(f.GetID()));
    EXPECT_EQ(BDD::True.Size(), 0u);
}

TEST_F(BDDTest, BDDClassIte) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);

    // ITE(a, b, False) = a & b
    BDD r = BDD::Ite(a, b, BDD::False);
    EXPECT_EQ(r.GetID(), (a & b).GetID());

    // ITE(a, True, b) = a | b
    BDD r2 = BDD::Ite(a, BDD::True, b);
    EXPECT_EQ(r2.GetID(), (a | b).GetID());
}

TEST_F(BDDTest, BDDClassPrint) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & BDDvar(v2);

    // Capture stdout
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    f.Print();
    std::cout.rdbuf(old);

    std::string out = oss.str();
    bddvar top = bddtop(f.GetID());

    // Build expected string
    std::ostringstream expected;
    expected << "[ " << f.GetID()
             << " Var:" << top << "(" << bddlevofvar(top) << ")"
             << " Size:" << bddsize(f.GetID())
             << " ]";
    EXPECT_EQ(out, expected.str());
}

TEST_F(BDDTest, BDDClassPrintConstant) {
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    BDD::False.Print();
    std::cout.rdbuf(old);

    // Constant: Var:0(0) Size:0
    std::string out = oss.str();
    EXPECT_NE(out.find("Var:0(0)"), std::string::npos);
    EXPECT_NE(out.find("Size:0"), std::string::npos);
}

// --- BDD::Swap ---

TEST_F(BDDTest, BDDClassSwapSameVar) {
    bddvar v1 = BDD_NewVar();
    BDD a = BDDvar(v1);
    EXPECT_EQ(a.Swap(v1, v1), a);
}

TEST_F(BDDTest, BDDClassSwapSimple) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    // Swap(v1, v2) on x1 should give x2
    BDD swapped = a.Swap(v1, v2);
    EXPECT_EQ(swapped, BDDvar(v2));
}

TEST_F(BDDTest, BDDClassSwapAnd) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD c = BDDvar(v3);
    // f = v1 & v2, swap(v1, v3) should give v3 & v2
    BDD f = a & b;
    BDD swapped = f.Swap(v1, v3);
    EXPECT_EQ(swapped, c & b);
}

TEST_F(BDDTest, BDDClassSwapSymmetric) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & BDDvar(v2);
    // Swapping twice should restore original
    EXPECT_EQ(f.Swap(v1, v2).Swap(v1, v2), f);
}

TEST_F(BDDTest, BDDClassSwapCommutative) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) | ~BDDvar(v2);
    // Swap(v1,v2) == Swap(v2,v1)
    EXPECT_EQ(f.Swap(v1, v2), f.Swap(v2, v1));
}

TEST_F(BDDTest, BDDClassSwapConstants) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    EXPECT_EQ(BDD::True.Swap(v1, v2), BDD::True);
    EXPECT_EQ(BDD::False.Swap(v1, v2), BDD::False);
    EXPECT_EQ(bddswap(bddnull, v1, v2), bddnull);
}

TEST_F(BDDTest, BDDClassSwapComplement) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & ~BDDvar(v2);
    // swap(~f, v1, v2) == ~swap(f, v1, v2)
    EXPECT_EQ((~f).Swap(v1, v2), ~f.Swap(v1, v2));
}

TEST_F(BDDTest, BDDClassSwapNonAdjacent) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddvar v4 = BDD_NewVar();
    bddvar v5 = BDD_NewVar();
    BDD a = BDDvar(v1), b = BDDvar(v2), c = BDDvar(v3);
    BDD d = BDDvar(v4), e = BDDvar(v5);
    // Swap v1 and v5 (4 variables between them)
    BDD f = (a & b) | (c & d) | e;
    BDD swapped = f.Swap(v1, v5);
    BDD expected = (e & b) | (c & d) | a;
    EXPECT_EQ(swapped, expected);
    // Involution: swap twice restores original
    EXPECT_EQ(swapped.Swap(v1, v5), f);
}

TEST_F(BDDTest, BDDClassSwapNonAdjacentComplex) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddvar v4 = BDD_NewVar();
    BDD a = BDDvar(v1), b = BDDvar(v2), c = BDDvar(v3), d = BDDvar(v4);
    // f involves v1 and v3 with v2 between them
    BDD f = (a & b & c) | (~a & d);
    BDD swapped = f.Swap(v1, v3);
    BDD expected = (c & b & a) | (~c & d);
    EXPECT_EQ(swapped, expected);
    // Commutativity
    EXPECT_EQ(f.Swap(v1, v3), f.Swap(v3, v1));
    // Involution
    EXPECT_EQ(f.Swap(v1, v3).Swap(v1, v3), f);
}

TEST_F(BDDTest, BDDClassSwapVariableOutOfRange) {
    bddvar v1 = BDD_NewVar();
    BDD f = BDDvar(v1);
    EXPECT_THROW(f.Swap(0, v1), std::invalid_argument);
    EXPECT_THROW(f.Swap(v1, bdd_varcount + 1), std::invalid_argument);
}

// --- BDD::Smooth ---

TEST_F(BDDTest, BDDClassSmoothConstant) {
    bddvar v1 = BDD_NewVar();
    EXPECT_EQ(BDD::True.Smooth(v1), BDD::True);
    EXPECT_EQ(BDD::False.Smooth(v1), BDD::False);
}

TEST_F(BDDTest, BDDClassSmoothSingleVar) {
    bddvar v1 = BDD_NewVar();
    BDD a = BDDvar(v1);
    // ∃v1. v1 = true
    EXPECT_EQ(a.Smooth(v1), BDD::True);
}

TEST_F(BDDTest, BDDClassSmoothNotInBDD) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    // ∃v2. v1 = v1  (v2 not in BDD)
    EXPECT_EQ(a.Smooth(v2), a);
}

TEST_F(BDDTest, BDDClassSmoothAnd) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    // ∃v1. (v1 ∧ v2) = v2
    EXPECT_EQ(f.Smooth(v1), b);
}

TEST_F(BDDTest, BDDClassSmoothMatchesExist) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    BDD f = (BDDvar(v1) & BDDvar(v2)) | BDDvar(v3);
    // Smooth(v) should be the same as Exist(v)
    EXPECT_EQ(f.Smooth(v1), f.Exist(v1));
    EXPECT_EQ(f.Smooth(v2), f.Exist(v2));
    EXPECT_EQ(f.Smooth(v3), f.Exist(v3));
}

// --- BDD::Spread ---

TEST_F(BDDTest, BDDClassSpreadZero) {
    bddvar v1 = BDD_NewVar();
    BDD a = BDDvar(v1);
    // Spread(0) = identity
    EXPECT_EQ(a.Spread(0), a);
}

TEST_F(BDDTest, BDDClassSpreadConstant) {
    (void)BDD_NewVar();
    EXPECT_EQ(BDD::True.Spread(1), BDD::True);
    EXPECT_EQ(BDD::False.Spread(1), BDD::False);
}

TEST_F(BDDTest, BDDClassSpreadNegativeThrows) {
    bddvar v1 = BDD_NewVar();
    BDD a = BDDvar(v1);
    EXPECT_THROW(a.Spread(-1), std::invalid_argument);
}

TEST_F(BDDTest, BDDClassSpreadSingleVar) {
    bddvar v1 = BDD_NewVar();
    BDD a = BDDvar(v1);
    // Spread(1) on v1: at node v1, f0=false, f1=true
    // lo = f0.Spread(1) | f1.Spread(0) = false | true = true
    // hi = f1.Spread(1) | f0.Spread(0) = true | false = true
    // BDD::getnode(v1, true, true) = true (reduced)
    EXPECT_EQ(a.Spread(1), BDD::True);
}

TEST_F(BDDTest, BDDClassSpreadTwoVars) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    // f = v1, Spread(1): at top variable v2 (or v1 depending on ordering),
    // the 0-branch and 1-branch mix via OR
    // For a single variable f = v1 with 2 vars:
    // BDD has top = v2 (higher level), At0(v2) = v1, At1(v2) = v1
    // Wait, v1 doesn't depend on v2, so the BDD just has v1 at its node.
    // Actually the BDD of v1 only has one node with var v1.
    // Spread(1) decomposes by v1: f0 = false, f1 = true
    // lo = f0.Spread(1) | f1.Spread(0) = false | true = true
    // hi = f1.Spread(1) | f0.Spread(0) = true | false = true
    // result = BDD::getnode(v1, true, true) = true
    // So Spread(1) on a single var BDD with >1 vars defined = true
    BDD result = a.Spread(1);
    EXPECT_EQ(result, BDD::True);
}

TEST_F(BDDTest, BDDClassSpreadAnd) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & BDDvar(v2);
    // Spread should make result >= f (more true values)
    // f => Spread(f, k) for all k >= 0
    BDD spread1 = f.Spread(1);
    EXPECT_EQ(f.Imply(spread1), 1);
}

TEST_F(BDDTest, BDDClassXPrint0) {
    BDD f = BDDvar(BDD_NewVar());
    EXPECT_THROW(f.XPrint0(), std::logic_error);
}

TEST_F(BDDTest, BDDClassXPrint) {
    BDD f = BDDvar(BDD_NewVar());
    EXPECT_THROW(f.XPrint(), std::logic_error);
}

TEST_F(BDDTest, BDDClassExportOstream) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & BDDvar(v2);

    std::ostringstream oss1;
    f.Export(oss1);

    std::ostringstream oss2;
    bddp p = f.GetID();
    bddexport(oss2, &p, 1);

    EXPECT_EQ(oss1.str(), oss2.str());
}

TEST_F(BDDTest, BDDClassExportFilePtr) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & BDDvar(v2);

    FILE* tmp = std::tmpfile();
    ASSERT_NE(tmp, nullptr);
    f.Export(tmp);
    long len = std::ftell(tmp);
    EXPECT_GT(len, 0);

    std::rewind(tmp);
    bddp p;
    int ret = bddimport(tmp, &p, 1);
    std::fclose(tmp);
    EXPECT_EQ(ret, 1);
    EXPECT_EQ(p, f.GetID());
}

// --- ZDD class high-level member functions ---

TEST_F(BDDTest, ZDDClassMaximalMinimal) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    // {∅, {1}, {2}, {1,2}} → maximal = {{1,2}}, minimal = {∅}
    bddp z1 = bddchange(bddsingle, v1);  // {{1}}
    bddp z2 = bddchange(bddsingle, v2);  // {{2}}
    bddp z12 = bddchange(z1, v2);        // {{1,2}}
    bddp all = bddunion(bddunion(bddunion(bddsingle, z1), z2), z12);

    ZDD zall = ZDD_ID(all);
    EXPECT_EQ(zall.maximal().GetID(), z12);
    EXPECT_EQ(zall.minimal().GetID(), bddsingle);
}

TEST_F(BDDTest, ZDDClassMinhitClosure) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    // {{1}, {2}} → minhit = {{1,2}}
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);
    bddp f = bddunion(z1, z2);

    ZDD zf = ZDD_ID(f);
    ZDD mh = zf.minhit();
    bddp z12 = bddchange(z1, v2);
    EXPECT_EQ(mh.GetID(), z12);

    // closure of {{1}} = {∅, {1}}
    ZDD z1w = ZDD_ID(z1);
    ZDD cl = z1w.closure();
    EXPECT_EQ(cl.GetID(), bddclosure(z1));
}

TEST_F(BDDTest, ZDDClassCard) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);
    bddp f = bddunion(bddunion(bddsingle, z1), z2);

    ZDD zf = ZDD_ID(f);
    EXPECT_EQ(zf.Card(), 3u);
    EXPECT_EQ(ZDD::Empty.Card(), 0u);
    EXPECT_EQ(ZDD::Single.Card(), 1u);
}

TEST_F(BDDTest, ZDDClassRestrictPermit) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);
    bddp z12 = bddchange(z1, v2);
    bddp f = bddunion(bddunion(z1, z2), z12);

    ZDD zf = ZDD_ID(f);
    ZDD zp = ZDD_ID(z1);  // permit set: {{1}}

    ZDD restricted = zf.Restrict(zp);
    EXPECT_EQ(restricted.GetID(), bddrestrict(f, z1));

    ZDD permitted = zf.Permit(zp);
    EXPECT_EQ(permitted.GetID(), bddpermit(f, z1));
}

TEST_F(BDDTest, ZDDClassNonsupNonsub) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);
    bddp z12 = bddchange(z1, v2);
    bddp f = bddunion(bddunion(z1, z2), z12);

    ZDD zf = ZDD_ID(f);
    ZDD zg = ZDD_ID(z12);

    EXPECT_EQ(zf.remove_supersets(zg).GetID(), bddnonsup(f, z12));
    EXPECT_EQ(zf.remove_subsets(zg).GetID(), bddnonsub(f, z12));
}

TEST_F(BDDTest, ZDDClassDisjoinJointjoin) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);

    ZDD za = ZDD_ID(z1);
    ZDD zb = ZDD_ID(z2);

    EXPECT_EQ(za.disjoin(zb).GetID(), bdddisjoin(z1, z2));
    EXPECT_EQ(za.joint_join(zb).GetID(), bddjointjoin(z1, z2));
}

TEST_F(BDDTest, ZDDClassDelta) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);
    bddp z12 = bddchange(z1, v2);
    bddp f = bddunion(z1, z12);

    ZDD zf = ZDD_ID(f);
    ZDD zg = ZDD_ID(z2);

    EXPECT_EQ(zf.delta(zg).GetID(), bdddelta(f, z2));
}

// --- Garbage collection tests ---

TEST_F(BDDTest, GCBasicProtectUnprotect) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp a = bddand(p1, p2);

    // Protect a, then GC — a should survive
    bddgc_protect(&a);
    uint64_t live_before = bddlive();
    bddgc();
    uint64_t live_after = bddlive();
    // a and its children (p1, p2) are reachable — they survive
    // p1 and p2 are also reachable from a
    EXPECT_LE(live_after, live_before);
    // The node for a should still be valid
    EXPECT_EQ(bddtop(a), v1 > v2 ? v1 : v2);
    bddgc_unprotect(&a);
}

TEST_F(BDDTest, GCCollectsUnreachableNodes) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // Create nodes that are only referenced by raw bddp (not protected)
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp unreachable = bddand(p1, p2);
    (void)unreachable;

    // Create a protected node
    bddp kept = bddprime(v3);
    bddgc_protect(&kept);

    uint64_t live_before = bddlive();
    bddgc();
    uint64_t live_after = bddlive();

    // Unprotected nodes (p1, p2, unreachable) should be collected
    // Only kept (v3 prime) should survive
    EXPECT_LT(live_after, live_before);
    EXPECT_EQ(bddtop(kept), v3);

    bddgc_unprotect(&kept);
}

TEST_F(BDDTest, GCFreeListReuse) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp a = bddand(p1, p2);
    (void)a;

    uint64_t used_before = bddused();

    // Nothing protected — GC should free all nodes
    bddgc();
    uint64_t live_after = bddlive();
    EXPECT_EQ(live_after, 0u);

    // Allocating new nodes should reuse freed slots
    bddp p3 = bddprime(v1);
    bddgc_protect(&p3);
    uint64_t used_after = bddused();
    // bddused() is high-water mark, should not increase
    EXPECT_EQ(used_after, used_before);

    bddgc_unprotect(&p3);
}

TEST_F(BDDTest, GCBDDClassAutoProtection) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // BDD objects automatically protect their root
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD c = a & b;

    bddp expected_root = c.GetID();

    bddgc();

    // c should survive because BDD object protects &c.root
    EXPECT_EQ(c.GetID(), expected_root);
    EXPECT_EQ(bddtop(c.GetID()), v1 > v2 ? v1 : v2);
}

TEST_F(BDDTest, GCBDDClassScopeProtection) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    uint64_t live_after_create;
    {
        BDD a = BDDvar(v1);
        BDD b = BDDvar(v2);
        BDD c = a & b;
        live_after_create = bddlive();
        EXPECT_GT(live_after_create, 0u);
    }
    // a, b, c are out of scope — destructors unprotected their roots

    bddgc();
    uint64_t live_after_gc = bddlive();
    EXPECT_EQ(live_after_gc, 0u);
}

TEST_F(BDDTest, GCZDDClassAutoProtection) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    ZDD z1 = ZDD_ID(bddchange(bddsingle, v1));
    ZDD z2 = ZDD_ID(bddchange(bddsingle, v2));
    ZDD z3 = z1 + z2;

    bddp expected_root = z3.GetID();

    bddgc();

    // z3 should survive because ZDD object protects &z3.root
    EXPECT_EQ(z3.GetID(), expected_root);
}

TEST_F(BDDTest, GCAutoTriggerOnExhaustion) {
    // Use a small node_max so GC is triggered automatically
    bddinit(4, 8);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddvar v4 = bddnewvar();

    // Fill up nodes in a scope so they become unreachable
    {
        BDD a = BDDvar(v1);
        BDD b = BDDvar(v2);
        BDD c = a & b;
        BDD d = a | b;
        (void)c;
        (void)d;
    }
    // All BDD objects out of scope — nodes are unprotected

    // This operation should succeed because GC frees dead nodes
    // even though we're at the node limit
    BDD e = BDDvar(v3);
    BDD f = BDDvar(v4);
    BDD g = e & f;

    EXPECT_EQ(bddtop(g.GetID()), v3 > v4 ? v3 : v4);
}

TEST_F(BDDTest, GCPreservesOperationCorrectness) {
    bddinit(16, 32);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD c = BDDvar(v3);

    // Build some intermediate results that will require GC
    BDD r1 = a & b;
    BDD r2 = b | c;
    BDD r3 = r1 ^ r2;

    // Force GC
    bddgc();

    // Verify results are still correct after GC
    // r3 = (v1 & v2) ^ (v2 | v3)
    // Check by evaluating at all variable assignments
    for (int i = 0; i < 8; i++) {
        bool x1 = (i >> 0) & 1;
        bool x2 = (i >> 1) & 1;
        bool x3 = (i >> 2) & 1;

        bddp f = r3.GetID();
        f = x1 ? bddat1(f, v1) : bddat0(f, v1);
        f = x2 ? bddat1(f, v2) : bddat0(f, v2);
        f = x3 ? bddat1(f, v3) : bddat0(f, v3);

        bool expected = (x1 && x2) ^ (x2 || x3);
        EXPECT_EQ(f, expected ? bddtrue : bddfalse);
    }
}

TEST_F(BDDTest, GCManualProtectRawBddp) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Manual protect for raw bddp values
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp a = bddand(p1, p2);

    bddgc_protect(&a);
    bddgc();

    // a is still valid
    EXPECT_EQ(bddtop(a), v1 > v2 ? v1 : v2);

    // Unprotect and GC again — should be collected
    bddgc_unprotect(&a);
    bddgc();
    EXPECT_EQ(bddlive(), 0u);
}

TEST_F(BDDTest, GCDepthPreventsCollection) {
    bddvar v1 = bddnewvar();
    bddp p1 = bddprime(v1);
    (void)p1;

    uint64_t live_before = bddlive();
    EXPECT_GT(live_before, 0u);

    // NOTE: Directly manipulating internal bdd_gc_depth to test GC guard behavior.
    // This couples the test to the implementation detail that bddgc() is a no-op
    // when bdd_gc_depth > 0.
    bdd_gc_depth = 1;
    bddgc();
    // GC should be a no-op at depth > 0
    EXPECT_EQ(bddlive(), live_before);

    bdd_gc_depth = 0;
    bddgc();
    // Now GC runs — p1 is unprotected
    EXPECT_EQ(bddlive(), 0u);
}

TEST_F(BDDTest, GCThresholdSetting) {
    double orig = bddgc_getthreshold();
    bddgc_setthreshold(0.5);
    EXPECT_DOUBLE_EQ(bddgc_getthreshold(), 0.5);
    bddgc_setthreshold(orig);
    EXPECT_DOUBLE_EQ(bddgc_getthreshold(), orig);
}

TEST_F(BDDTest, GCLiveCount) {
    EXPECT_EQ(bddlive(), 0u);

    bddvar v1 = bddnewvar();
    bddp p1 = bddprime(v1);
    bddgc_protect(&p1);

    EXPECT_EQ(bddlive(), 1u);

    bddvar v2 = bddnewvar();
    bddp p2 = bddprime(v2);
    bddgc_protect(&p2);

    EXPECT_EQ(bddlive(), 2u);

    bddgc_unprotect(&p2);
    bddgc();
    // p2 collected, p1 survives
    EXPECT_EQ(bddlive(), 1u);

    bddgc_unprotect(&p1);
    bddgc();
    EXPECT_EQ(bddlive(), 0u);
}

TEST_F(BDDTest, GCReinitClearsState) {
    bddvar v1 = bddnewvar();
    bddp p1 = bddprime(v1);
    bddgc_protect(&p1);

    EXPECT_GT(bddlive(), 0u);

    // Re-initialize throws while non-terminal roots exist
    EXPECT_THROW(bddinit(256), std::runtime_error);

    // After unprotecting, re-initialize succeeds
    bddgc_unprotect(&p1);
    bddinit(256);
    EXPECT_EQ(bddlive(), 0u);
    EXPECT_EQ(bddused(), 0u);
}

TEST_F(BDDTest, GCZDDAutoTrigger) {
    bddinit(4, 8);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // Create and discard ZDD nodes
    {
        ZDD z1 = ZDD_ID(bddchange(bddsingle, v1));
        ZDD z2 = ZDD_ID(bddchange(bddsingle, v2));
        ZDD z3 = z1 + z2;
        (void)z3;
    }
    // All out of scope

    // Should succeed via auto GC
    ZDD z4 = ZDD_ID(bddchange(bddsingle, v3));
    ZDD z5 = ZDD_ID(bddchange(bddsingle, v1));
    ZDD z6 = z4 + z5;

    EXPECT_NE(z6.GetID(), bddempty);
}

// --- Reduced flag tests ---

TEST_F(BDDTest, ReducedFlag_TerminalsAreReduced) {
    EXPECT_TRUE(bddp_is_reduced(bddfalse));
    EXPECT_TRUE(bddp_is_reduced(bddtrue));
    EXPECT_TRUE(bddp_is_reduced(bddempty));
    EXPECT_TRUE(bddp_is_reduced(bddsingle));
}

TEST_F(BDDTest, ReducedFlag_PrimeNodeIsReduced) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    EXPECT_TRUE(bddp_is_reduced(p));
    EXPECT_TRUE(node_is_reduced(p));
}

TEST_F(BDDTest, ReducedFlag_BDDOperations) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddp x1 = bddprime(v1);
    bddp x2 = bddprime(v2);
    bddp x3 = bddprime(v3);

    // AND
    bddp a = bddand(x1, x2);
    EXPECT_TRUE(bddp_is_reduced(a));
    EXPECT_TRUE(bdd_check_reduced(a));

    // OR
    bddp o = bddor(x1, x2);
    EXPECT_TRUE(bddp_is_reduced(o));
    EXPECT_TRUE(bdd_check_reduced(o));

    // XOR
    bddp x = bddxor(x1, x2);
    EXPECT_TRUE(bddp_is_reduced(x));
    EXPECT_TRUE(bdd_check_reduced(x));

    // NOT (complement edge)
    bddp n = bddnot(x1);
    EXPECT_TRUE(bddp_is_reduced(n));
    EXPECT_TRUE(bdd_check_reduced(n));

    // ITE
    bddp ite = bddite(x1, x2, x3);
    EXPECT_TRUE(bddp_is_reduced(ite));
    EXPECT_TRUE(bdd_check_reduced(ite));

    // Cofactor
    bddp c0 = bddat0(a, v1);
    EXPECT_TRUE(bddp_is_reduced(c0));
    bddp c1 = bddat1(a, v1);
    EXPECT_TRUE(bddp_is_reduced(c1));

    // Quantification
    bddp ex = bddexistvar(a, v1);
    EXPECT_TRUE(bddp_is_reduced(ex));
    bddp un = bddunivvar(a, v1);
    EXPECT_TRUE(bddp_is_reduced(un));
}

TEST_F(BDDTest, ReducedFlag_ZDDOperations) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();

    // Build ZDD sets: {v1}, {v2}, {v3}
    bddp s1 = bddchange(bddsingle, v1);
    bddp s2 = bddchange(bddsingle, v2);
    bddp s3 = bddchange(bddsingle, v3);

    EXPECT_TRUE(bddp_is_reduced(s1));
    EXPECT_TRUE(bddp_is_reduced(s2));
    EXPECT_TRUE(bddp_is_reduced(s3));

    // Union
    bddp u = bddunion(s1, s2);
    EXPECT_TRUE(bddp_is_reduced(u));
    EXPECT_TRUE(bdd_check_reduced(u));

    // Intersect
    bddp i = bddintersec(u, s1);
    EXPECT_TRUE(bddp_is_reduced(i));

    // Subtract
    bddp sub = bddsubtract(u, s1);
    EXPECT_TRUE(bddp_is_reduced(sub));

    // Onset/Offset
    bddp on = bddonset(u, v1);
    EXPECT_TRUE(bddp_is_reduced(on));
    bddp off = bddoffset(u, v1);
    EXPECT_TRUE(bddp_is_reduced(off));

    // Change
    bddp ch = bddchange(u, v3);
    EXPECT_TRUE(bddp_is_reduced(ch));
    EXPECT_TRUE(bdd_check_reduced(ch));
}

TEST_F(BDDTest, ReducedFlag_ComplexBDD_CheckReduced) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    bddvar v4 = BDD_NewVar();
    bddp x1 = bddprime(v1);
    bddp x2 = bddprime(v2);
    bddp x3 = bddprime(v3);
    bddp x4 = bddprime(v4);

    // Build complex BDD: (x1 & x2) | (x3 ^ x4)
    bddp complex = bddor(bddand(x1, x2), bddxor(x3, x4));
    EXPECT_TRUE(bdd_check_reduced(complex));

    // bdd_check_reduced on terminals
    EXPECT_TRUE(bdd_check_reduced(bddfalse));
    EXPECT_TRUE(bdd_check_reduced(bddtrue));
    EXPECT_TRUE(bdd_check_reduced(bddnull));
}

TEST_F(BDDTest, ReducedFlag_ComplexZDD_CheckReduced) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();

    // Build ZDD family: {{v1,v2}, {v2,v3}, {v1}}
    bddp s12 = bddchange(bddchange(bddsingle, v1), v2);
    bddp s23 = bddchange(bddchange(bddsingle, v2), v3);
    bddp s1 = bddchange(bddsingle, v1);
    bddp family = bddunion(bddunion(s12, s23), s1);
    EXPECT_TRUE(bdd_check_reduced(family));
}

TEST_F(BDDTest, ReducedFlag_GCPreservesFlag) {
    BDD_Init(64, 64);
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp x1 = bddprime(v1);
    bddp x2 = bddprime(v2);

    bddp result = bddand(x1, x2);
    bddgc_protect(&result);
    bddgc();
    EXPECT_TRUE(bddp_is_reduced(result));
    EXPECT_TRUE(bdd_check_reduced(result));
    bddgc_unprotect(&result);
}

TEST_F(BDDTest, ReducedFlag_ImportExport) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddp x1 = bddprime(v1);
    bddp x2 = bddprime(v2);
    bddp orig = bddand(x1, x2);

    // Export
    std::ostringstream oss;
    bddp arr[1] = { orig };
    bddexport(oss, arr, 1);

    // Re-init and import
    BDD_Init(1024, UINT64_MAX);
    BDD_NewVar();
    BDD_NewVar();
    std::istringstream iss(oss.str());
    bddp imported;
    int ret = bddimport(iss, &imported, 1);
    EXPECT_EQ(ret, 1);
    EXPECT_TRUE(bddp_is_reduced(imported));
    EXPECT_TRUE(bdd_check_reduced(imported));
}

TEST_F(BDDTest, ReducedFlag_ZDD_ID) {
    bddvar v = BDD_NewVar();
    bddp p = bddchange(bddsingle, v);
    ZDD z = ZDD_ID(p);
    EXPECT_EQ(z.GetID(), p);
}

TEST_F(BDDTest, ReducedFlag_BDD_ID_ValidatesReduced) {
    bddvar v = BDD_NewVar();
    bddp p = bddprime(v);
    // Should succeed — node is reduced
    EXPECT_NO_THROW(BDD_ID(p));
    // Terminals should succeed
    EXPECT_NO_THROW(BDD_ID(bddfalse));
    EXPECT_NO_THROW(BDD_ID(bddtrue));
}

// --- bdddump / bddvdump ---

static std::string capture_stdout(std::function<void()> fn) {
    fflush(stdout);
    int pipefd[2];
    if (pipe(pipefd) != 0) return "";
    int saved = dup(STDOUT_FILENO);
    if (saved < 0) { close(pipefd[0]); close(pipefd[1]); return ""; }
    if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
        close(saved); close(pipefd[0]); close(pipefd[1]); return "";
    }
    close(pipefd[1]);
    fn();
    fflush(stdout);
    dup2(saved, STDOUT_FILENO);
    close(saved);
    std::string result;
    char buf[1024];
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0) {
        result.append(buf, n);
    }
    close(pipefd[0]);
    return result;
}

TEST_F(BDDTest, Bdddump_Null) {
    std::string out = capture_stdout([]{ bdddump(bddnull); });
    EXPECT_EQ(out, "RT = NULL\n\n");
}

TEST_F(BDDTest, Bdddump_False) {
    std::string out = capture_stdout([]{ bdddump(bddfalse); });
    EXPECT_EQ(out, "RT = 0\n\n");
}

TEST_F(BDDTest, Bdddump_True) {
    std::string out = capture_stdout([]{ bdddump(bddtrue); });
    EXPECT_EQ(out, "RT = ~0\n\n");
}

TEST_F(BDDTest, Bdddump_SingleVar) {
    bddvar v1 = bddnewvar();
    bddp f = bddprime(v1);  // V1: lo=false, hi=true
    std::string out = capture_stdout([f]{ bdddump(f); });
    // f is node_id=2, index=1, var=1, lev=1
    // lo=bddfalse(val 0), hi=bddtrue=~bddfalse -> complement of const 0
    // bddtrue = BDD_CONST_FLAG | 1, but with complement edge representation:
    // BDD::getnode(v, bddfalse, bddtrue): lo=bddfalse (no comp), hi=bddtrue
    // bddtrue = 0x800000000001, which has BDD_CONST_FLAG set and value=1
    // So hi is a constant with value 1, no complement edge (BDD_COMP_FLAG=bit0, but
    // BDD_CONST_FLAG is bit 47). bddtrue has bit 0 set but it's a constant value, not comp flag.
    // Actually: bddtrue = BDD_CONST_FLAG | 1. The "1" here IS the value.
    // For display: f1 = bddtrue. Is BDD_COMP_FLAG set? BDD_COMP_FLAG = bit 0 = 1.
    // bddtrue & BDD_COMP_FLAG = 1. So neg1 = true.
    // f1abs = bddtrue & ~BDD_COMP_FLAG = BDD_CONST_FLAG | 0 = bddfalse.
    // f1abs & BDD_CONST_FLAG = true. val = f1abs & ~BDD_CONST_FLAG = 0.
    // So output: ~0. This matches the spec (bddtrue displays as ~0).
    uint64_t ndx = (f & ~BDD_COMP_FLAG) / 2;
    std::string expected = "N" + std::to_string(ndx) + " = [V1(1), 0, ~0]\n"
                           "RT = N" + std::to_string(ndx) + "\n\n";
    EXPECT_EQ(out, expected);
}

TEST_F(BDDTest, Bdddump_TwoVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp p1 = bddprime(v1);  // V1
    bddp p2 = bddprime(v2);  // V2
    bddp f = bddxor(p1, p2); // V1 XOR V2
    std::string out = capture_stdout([f]{ bdddump(f); });
    // Should contain node lines and RT line, ending with blank line
    EXPECT_NE(out.find("RT = "), std::string::npos);
    EXPECT_TRUE(out.size() >= 2 && out.substr(out.size()-2) == "\n\n");
}

TEST_F(BDDTest, Bdddump_NegatedRoot) {
    bddvar v1 = bddnewvar();
    bddp f = bddprime(v1);
    bddp nf = bddnot(f);
    std::string out = capture_stdout([nf]{ bdddump(nf); });
    uint64_t ndx = (f & ~BDD_COMP_FLAG) / 2;
    std::string expected = "N" + std::to_string(ndx) + " = [V1(1), 0, ~0]\n"
                           "RT = ~N" + std::to_string(ndx) + "\n\n";
    EXPECT_EQ(out, expected);
}

TEST_F(BDDTest, Bddvdump_Basic) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    bddp arr[2] = {f, p1};
    std::string out = capture_stdout([&arr]{ bddvdump(arr, 2); });
    EXPECT_NE(out.find("RT0 = "), std::string::npos);
    EXPECT_NE(out.find("RT1 = "), std::string::npos);
    EXPECT_TRUE(out.size() >= 2 && out.substr(out.size()-2) == "\n\n");
}

TEST_F(BDDTest, Bddvdump_NullSentinel) {
    bddvar v1 = bddnewvar();
    bddp p1 = bddprime(v1);
    bddp arr[3] = {p1, bddnull, bddfalse};
    std::string out = capture_stdout([&arr]{ bddvdump(arr, 3); });
    // RT0 should show the node, RT1 should show NULL, RT2 should not appear
    EXPECT_NE(out.find("RT0 = "), std::string::npos);
    EXPECT_NE(out.find("RT1 = NULL"), std::string::npos);
    EXPECT_EQ(out.find("RT2"), std::string::npos);
}

TEST_F(BDDTest, Bddvdump_SharedNodes) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp p1 = bddprime(v1);
    bddp p2 = bddprime(v2);
    bddp f = bddand(p1, p2);
    // f shares p1 as a subnode; shared nodes should be printed only once
    bddp arr[2] = {f, bddnot(p1)};
    std::string out = capture_stdout([&arr]{ bddvdump(arr, 2); });
    uint64_t ndx1 = (p1 & ~BDD_COMP_FLAG) / 2;
    std::string nstr = "N" + std::to_string(ndx1) + " = ";
    // Count occurrences of the node line
    size_t count = 0;
    size_t pos = 0;
    while ((pos = out.find(nstr, pos)) != std::string::npos) {
        count++;
        pos += nstr.size();
    }
    EXPECT_EQ(count, 1u);
}

// Redirect stdout to /dev/null during fn() so that voluminous output does
// not block on a full pipe buffer (as capture_stdout would).
static void silence_stdout(std::function<void()> fn) {
    fflush(stdout);
    int saved = dup(STDOUT_FILENO);
    int devnull = open("/dev/null", O_WRONLY);
    if (devnull < 0) { if (saved >= 0) close(saved); fn(); return; }
    dup2(devnull, STDOUT_FILENO);
    close(devnull);
    fn();
    fflush(stdout);
    if (saved >= 0) {
        dup2(saved, STDOUT_FILENO);
        close(saved);
    }
}

TEST_F(BDDTest, Bdddump_DeepBDDExceedsRecurLimit) {
    // Deep chain BDD (level > BDD_RecurLimit=8192). bdddump must dispatch
    // to the iterative implementation without stack overflow. Output is
    // discarded to avoid pipe-buffer deadlock; the test only checks
    // that no exception is thrown.
    const int depth = 9000;
    for (int i = bdd_varcount; i < depth; ++i) bddnewvar();
    bddp f = bddfalse;
    for (int v = 1; v <= depth; ++v) {
        f = BDD::getnode_raw(v, bddfalse, (v == 1) ? bddtrue : f);
    }
    EXPECT_NO_THROW(silence_stdout([f]{ bdddump(f); }));
}

TEST_F(BDDTest, Bddvdump_DeepBDDExceedsRecurLimit) {
    // Deep chain BDD for bddvdump's iterative path.
    const int depth = 9000;
    for (int i = bdd_varcount; i < depth; ++i) bddnewvar();
    bddp f = bddfalse;
    for (int v = 1; v <= depth; ++v) {
        f = BDD::getnode_raw(v, bddfalse, (v == 1) ? bddtrue : f);
    }
    bddp arr[1] = {f};
    EXPECT_NO_THROW(silence_stdout([&arr]{ bddvdump(arr, 1); }));
}

TEST_F(BDDTest, Bddgraph0_Throws) {
    EXPECT_THROW(bddgraph0(bddfalse), std::logic_error);
}

TEST_F(BDDTest, Bddgraph_Throws) {
    EXPECT_THROW(bddgraph(bddfalse), std::logic_error);
}

TEST_F(BDDTest, Bddvgraph0_Throws) {
    bddp arr[1] = {bddfalse};
    EXPECT_THROW(bddvgraph0(arr, 1), std::logic_error);
}

TEST_F(BDDTest, Bddvgraph_Throws) {
    bddp arr[1] = {bddfalse};
    EXPECT_THROW(bddvgraph(arr, 1), std::logic_error);
}

