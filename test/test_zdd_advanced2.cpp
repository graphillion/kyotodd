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

// ============================================================
// min_size tests
// ============================================================

TEST_F(BDDTest, MinSize_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    EXPECT_EQ(empty.min_size(), 0u);
    EXPECT_EQ(bddminsize(empty.GetID()), 0u);
}

TEST_F(BDDTest, MinSize_UnitFamily) {
    bddnewvar();
    ZDD unit(1);  // {∅}
    EXPECT_EQ(unit.min_size(), 0u);
}

TEST_F(BDDTest, MinSize_SingletonSet) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);  // {{v1}}
    EXPECT_EQ(f.min_size(), 1u);
}

TEST_F(BDDTest, MinSize_MultipleSets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // f = {{v1}, {v1, v2}, {v1, v2, v3}}
    ZDD f = ZDD::singleton(v1)
          + ZDD::single_set({v1, v2})
          + ZDD::single_set({v1, v2, v3});
    EXPECT_EQ(f.min_size(), 1u);
    EXPECT_EQ(f.Len(), 3u);
}

TEST_F(BDDTest, MinSize_WithEmpty) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // f = {∅, {v1}, {v1, v2}}
    ZDD f = ZDD::Single + ZDD::singleton(v1) + ZDD::single_set({v1, v2});
    EXPECT_EQ(f.min_size(), 0u);
}

TEST_F(BDDTest, MinSize_PowerSet) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(3);
    EXPECT_EQ(f.min_size(), 0u);
    EXPECT_EQ(f.Len(), 3u);
}

TEST_F(BDDTest, MinSize_PowerSetWithoutEmpty) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(3) - ZDD::Single;
    EXPECT_EQ(f.min_size(), 1u);
}

TEST_F(BDDTest, MinSize_ComplementEdge) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD all = ZDD::power_set(3);
    ZDD f = all - ZDD::single_set({v1, v2, v3});
    // Still has ∅, so min should be 0
    EXPECT_EQ(f.min_size(), 0u);
}

TEST_F(BDDTest, MinSize_OnlyLargeSets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // f = {{v1,v2}, {v1,v3}, {v2,v3}, {v1,v2,v3}}
    ZDD f = ZDD::single_set({v1, v2})
          + ZDD::single_set({v1, v3})
          + ZDD::single_set({v2, v3})
          + ZDD::single_set({v1, v2, v3});
    EXPECT_EQ(f.min_size(), 2u);
    EXPECT_EQ(f.Len(), 3u);
}

TEST_F(BDDTest, MinSize_ConsistencyWithProfile) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(3) - ZDD::Single;
    auto prof = f.profile();
    uint64_t expected_min = 0;
    for (size_t k = 0; k < prof.size(); ++k) {
        if (prof[k] > bigint::BigInt(0)) {
            expected_min = k;
            break;
        }
    }
    EXPECT_EQ(f.min_size(), expected_min);
}

TEST_F(BDDTest, MinSize_LenConsistency) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2})
          + ZDD::single_set({v1, v2, v3});
    EXPECT_LE(f.min_size(), f.Len());
}

// ============================================================
// support_vars tests
// ============================================================

TEST_F(BDDTest, ZDD_SupportVars_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    auto sv = empty.support_vars();
    EXPECT_TRUE(sv.empty());
}

TEST_F(BDDTest, ZDD_SupportVars_UnitFamily) {
    bddnewvar();
    ZDD unit(1);  // {∅}
    auto sv = unit.support_vars();
    EXPECT_TRUE(sv.empty());
}

TEST_F(BDDTest, ZDD_SupportVars_Singleton) {
    bddvar v1 = bddnewvar();
    bddnewvar();  // v2 unused
    ZDD f = ZDD::singleton(v1);
    auto sv = f.support_vars();
    EXPECT_EQ(sv.size(), 1u);
    EXPECT_EQ(sv[0], v1);
}

TEST_F(BDDTest, ZDD_SupportVars_MultipleSets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v2, v3});
    auto sv = f.support_vars();
    EXPECT_EQ(sv.size(), 3u);
    // Sorted by level descending (v3 > v2 > v1)
    std::vector<bddvar> expected = {v3, v2, v1};
    EXPECT_EQ(sv, expected);
}

TEST_F(BDDTest, ZDD_SupportVars_ConsistencyWithSupport) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::power_set(3);
    auto sv = f.support_vars();
    EXPECT_EQ(sv.size(), 3u);
    // Check all variables are present
    std::set<bddvar> sv_set(sv.begin(), sv.end());
    EXPECT_TRUE(sv_set.count(v1));
    EXPECT_TRUE(sv_set.count(v2));
    EXPECT_TRUE(sv_set.count(v3));
}

// ============================================================
// supersets_of tests
// ============================================================

TEST_F(BDDTest, SupersetsOf_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    EXPECT_EQ(empty.supersets_of({1}), ZDD::Empty);
}

TEST_F(BDDTest, SupersetsOf_EmptySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2});
    // Every set is a superset of ∅
    EXPECT_EQ(f.supersets_of({}), f);
}

TEST_F(BDDTest, SupersetsOf_Basic) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // F = {{v1,v2}, {v1,v3}, {v2,v3}}
    ZDD f = ZDD::single_set({v1, v2})
          + ZDD::single_set({v1, v3})
          + ZDD::single_set({v2, v3});
    ZDD result = f.supersets_of({v1});
    // Sets containing v1: {v1,v2}, {v1,v3}
    ZDD expected = ZDD::single_set({v1, v2}) + ZDD::single_set({v1, v3});
    EXPECT_EQ(result, expected);
}

TEST_F(BDDTest, SupersetsOf_NoMatch) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);
    EXPECT_EQ(f.supersets_of({v3}), ZDD::Empty);
}

TEST_F(BDDTest, SupersetsOf_PowerSet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD ps = ZDD::power_set(3);
    ZDD result = ps.supersets_of({v1, v2});
    // Supersets of {v1,v2}: {v1,v2}, {v1,v2,v3}
    ZDD expected = ZDD::single_set({v1, v2}) + ZDD::single_set({v1, v2, v3});
    EXPECT_EQ(result, expected);
}

TEST_F(BDDTest, SupersetsOf_UnitFamily) {
    bddvar v1 = bddnewvar();
    ZDD unit(1);  // {∅}
    // ∅ is not a superset of {v1}
    EXPECT_EQ(unit.supersets_of({v1}), ZDD::Empty);
    // ∅ is a superset of ∅
    EXPECT_EQ(unit.supersets_of({}), unit);
}

// ============================================================
// subsets_of tests
// ============================================================

TEST_F(BDDTest, SubsetsOf_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    EXPECT_EQ(empty.subsets_of({1}), ZDD::Empty);
}

TEST_F(BDDTest, SubsetsOf_Basic) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // F = {{v1,v2,v3}, {v1,v2}, {v1}}
    ZDD f = ZDD::single_set({v1, v2, v3})
          + ZDD::single_set({v1, v2})
          + ZDD::singleton(v1);
    ZDD result = f.subsets_of({v1, v2});
    // Subsets of {v1,v2}: {v1,v2}, {v1}
    ZDD expected = ZDD::single_set({v1, v2}) + ZDD::singleton(v1);
    EXPECT_EQ(result, expected);
}

TEST_F(BDDTest, SubsetsOf_EmptySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // F = {∅, {v1}, {v1,v2}}
    ZDD f = ZDD::Single + ZDD::singleton(v1) + ZDD::single_set({v1, v2});
    // Only ∅ is a subset of ∅
    EXPECT_EQ(f.subsets_of({}), ZDD::Single);
}

TEST_F(BDDTest, SubsetsOf_PowerSet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD ps = ZDD::power_set(3);
    ZDD result = ps.subsets_of({v1, v2});
    // Subsets of {v1,v2}: ∅, {v1}, {v2}, {v1,v2}
    ZDD expected = ZDD::Single + ZDD::singleton(v1) + ZDD::singleton(v2)
                 + ZDD::single_set({v1, v2});
    EXPECT_EQ(result, expected);
}

TEST_F(BDDTest, SubsetsOf_ConsistencyWithPermit) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::power_set(3) - ZDD::Single;
    std::vector<bddvar> s = {v1, v3};
    ZDD result = f.subsets_of(s);
    ZDD permit_result = f.Permit(ZDD::single_set(s));
    EXPECT_EQ(result, permit_result);
}

// ============================================================
// project tests
// ============================================================

TEST_F(BDDTest, Project_EmptyFamily) {
    bddvar v1 = bddnewvar();
    ZDD empty(0);
    EXPECT_EQ(empty.project({v1}), ZDD::Empty);
}

TEST_F(BDDTest, Project_EmptyVars) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    EXPECT_EQ(f.project({}), f);
}

TEST_F(BDDTest, Project_SingleVar) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::single_set({v1, v2, v3}) + ZDD::single_set({v1, v3});
    ZDD result = f.project({v2});
    // Remove v2: {{v1,v3}} (duplicates merge)
    ZDD expected_manual = f.OffSet(v2) + f.OnSet0(v2);
    EXPECT_EQ(result, expected_manual);
}

TEST_F(BDDTest, Project_MultipleVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // F = {{v1,v2,v3}, {v1}}
    ZDD f = ZDD::single_set({v1, v2, v3}) + ZDD::singleton(v1);
    ZDD result = f.project({v2, v3});
    // Remove v2 and v3: {{v1}} (both sets collapse to {v1})
    EXPECT_EQ(result, ZDD::singleton(v1));
}

TEST_F(BDDTest, Project_AllVars) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::single_set({v1, v2}) + ZDD::singleton(v1);
    ZDD result = f.project({v1, v2});
    // Remove all vars: {∅}
    EXPECT_EQ(result, ZDD::Single);
}

TEST_F(BDDTest, Project_Idempotent) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::power_set(3);
    std::vector<bddvar> vars = {v2};
    EXPECT_EQ(f.project(vars).project(vars), f.project(vars));
}

// ============================================================
// to_bdd / to_zdd tests
// ============================================================

TEST_F(BDDTest, ZDD_ToBdd_EmptyFamily) {
    bddnewvar();
    bddnewvar();
    ZDD empty(0);
    BDD bdd = empty.to_bdd();
    EXPECT_TRUE(bdd.is_zero());
}

TEST_F(BDDTest, ZDD_ToBdd_UnitFamily) {
    bddnewvar();
    bddnewvar();
    ZDD unit(1);  // {∅}
    BDD bdd = unit.to_bdd();
    // ∅ corresponds to all variables false
    // BDD should be true only when all vars are 0
    // That's ~x1 & ~x2
    BDD x1 = BDD::prime(1);
    BDD x2 = BDD::prime(2);
    EXPECT_EQ(bdd, ~x1 & ~x2);
}

TEST_F(BDDTest, ZDD_ToBdd_Singleton) {
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1);  // {{1}}
    BDD bdd = f.to_bdd();
    // {1} corresponds to x1=1, x2=0
    BDD x1 = BDD::prime(1);
    BDD x2 = BDD::prime(2);
    EXPECT_EQ(bdd, x1 & ~x2);
}

TEST_F(BDDTest, ZDD_ToBdd_PowerSet) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD ps = ZDD::power_set(3);
    BDD bdd = ps.to_bdd();
    // Power set = every assignment is a valid set → BDD is true
    EXPECT_TRUE(bdd.is_one());
}

TEST_F(BDDTest, ZDD_ToBdd_ConsistencyWithQDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::single_set({v1, v2}) + ZDD::singleton(v3);
    BDD direct = f.to_bdd();
    BDD via_qdd = f.to_qdd().to_bdd();
    EXPECT_EQ(direct, via_qdd);
}

TEST_F(BDDTest, BDD_ToZdd_False) {
    bddnewvar();
    bddnewvar();
    BDD f = BDD::False;
    ZDD z = f.to_zdd();
    EXPECT_TRUE(z.is_zero());
}

TEST_F(BDDTest, BDD_ToZdd_True) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    BDD f = BDD::True;
    ZDD z = f.to_zdd();
    // True BDD → power set (every assignment maps to a set)
    EXPECT_EQ(z, ZDD::power_set(3));
}

TEST_F(BDDTest, BDD_ToZdd_ConsistencyWithQDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    BDD x2 = BDD::prime(v2);
    BDD f = x1 & ~x2;
    ZDD direct = f.to_zdd();
    ZDD via_qdd = f.to_qdd().to_zdd();
    EXPECT_EQ(direct, via_qdd);
}

TEST_F(BDDTest, ZDD_ToBdd_RoundTrip) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::single_set({v1, v3}) + ZDD::singleton(v2) + ZDD::Single;
    int n = 3;
    ZDD roundtrip = f.to_bdd(n).to_zdd(n);
    EXPECT_EQ(roundtrip, f);
}

TEST_F(BDDTest, BDD_ToZdd_RoundTrip) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    BDD x1 = BDD::prime(v1);
    BDD x2 = BDD::prime(v2);
    BDD x3 = BDD::prime(v3);
    BDD f = (x1 | x2) & ~x3;
    int n = 3;
    BDD roundtrip = f.to_zdd(n).to_bdd(n);
    EXPECT_EQ(roundtrip, f);
}

TEST_F(BDDTest, ZDD_ToBdd_WithN) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1);  // {{1}}
    // Convert over 2 variables (not all 3)
    BDD bdd2 = f.to_bdd(2);
    BDD x1 = BDD::prime(1);
    BDD x2 = BDD::prime(2);
    EXPECT_EQ(bdd2, x1 & ~x2);
}

TEST_F(BDDTest, ZDD_ToBdd_NSmallerThanTopLevel_Throws) {
    // Top level is v3 (level 3) but caller asks for n = 2 variables.
    // The fill loop cannot cover variable 3, so the result would be a
    // silent structural drop. Must throw instead.
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    (void)v1; (void)v2;
    ZDD f = ZDD::singleton(v3);
    EXPECT_THROW(f.to_bdd(2), std::invalid_argument);
    EXPECT_THROW(f.to_bdd(1), std::invalid_argument);
    // n = top level must succeed.
    EXPECT_NO_THROW(f.to_bdd(3));
}

TEST_F(BDDTest, BDD_ToZdd_NSmallerThanTopLevel_Throws) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    (void)v1; (void)v2;
    BDD f = BDD::prime(v3);
    EXPECT_THROW(f.to_zdd(2), std::invalid_argument);
    EXPECT_THROW(f.to_zdd(1), std::invalid_argument);
    EXPECT_NO_THROW(f.to_zdd(3));
}

// --- ZDD::sample_k ---

TEST_F(BDDTest, ZDD_SampleK_EmptyFamily) {
    ZDD f(0);  // empty family
    ZddCountMemo memo(f);
    std::mt19937_64 rng(42);
    ZDD result = f.sample_k(3, rng, memo);
    EXPECT_EQ(result, ZDD::Empty);
}

TEST_F(BDDTest, ZDD_SampleK_ZeroK) {
    bddnewvar();
    ZDD f = ZDD::singleton(1);
    ZddCountMemo memo(f);
    std::mt19937_64 rng(42);
    ZDD result = f.sample_k(0, rng, memo);
    EXPECT_EQ(result, ZDD::Empty);
}

TEST_F(BDDTest, ZDD_SampleK_KGreaterThanTotal) {
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);  // {{1}, {2}}
    ZddCountMemo memo(f);
    std::mt19937_64 rng(42);
    ZDD result = f.sample_k(10, rng, memo);
    EXPECT_EQ(result, f);
}

TEST_F(BDDTest, ZDD_SampleK_KEqualsTotal) {
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    ZddCountMemo memo(f);
    std::mt19937_64 rng(42);
    ZDD result = f.sample_k(2, rng, memo);
    EXPECT_EQ(result, f);
}

TEST_F(BDDTest, ZDD_SampleK_SingleElement) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2) + ZDD::singleton(3);
    ZddCountMemo memo(f);
    std::mt19937_64 rng(42);
    ZDD result = f.sample_k(1, rng, memo);
    EXPECT_EQ(result.exact_count(), bigint::BigInt(1));
    // Result must be a subset of f
    EXPECT_TRUE(result.is_subset_family(f));
}

TEST_F(BDDTest, ZDD_SampleK_CorrectCount) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // Power set of {1,2,3} = 8 sets
    ZDD f = ZDD::power_set(3);
    ZddCountMemo memo(f);
    std::mt19937_64 rng(123);
    ZDD result = f.sample_k(5, rng, memo);
    EXPECT_EQ(result.exact_count(), bigint::BigInt(5));
    EXPECT_TRUE(result.is_subset_family(f));
}

TEST_F(BDDTest, ZDD_SampleK_SubsetOfOriginal) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // Power set of {1,2,3,4} = 16 sets
    ZDD f = ZDD::power_set(4);
    ZddCountMemo memo(f);
    std::mt19937_64 rng(999);
    for (int k = 1; k <= 15; ++k) {
        ZDD result = f.sample_k(k, rng, memo);
        EXPECT_EQ(result.exact_count(), bigint::BigInt(k));
        EXPECT_TRUE(result.is_subset_family(f));
    }
}

TEST_F(BDDTest, ZDD_SampleK_WithEmptySet) {
    bddnewvar();
    bddnewvar();
    // F = {∅, {1}, {2}} — 3 sets including ∅
    ZDD f = ZDD::Single + ZDD::singleton(1) + ZDD::singleton(2);
    ZddCountMemo memo(f);
    std::mt19937_64 rng(77);
    ZDD result = f.sample_k(2, rng, memo);
    EXPECT_EQ(result.exact_count(), bigint::BigInt(2));
    EXPECT_TRUE(result.is_subset_family(f));
}

TEST_F(BDDTest, ZDD_SampleK_DifferentSeeds) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // Power set of {1,..,5} = 32 sets, sample 3
    ZDD f = ZDD::power_set(5);
    ZddCountMemo memo(f);

    std::mt19937_64 rng1(111);
    ZDD r1 = f.sample_k(3, rng1, memo);

    std::mt19937_64 rng2(222);
    ZDD r2 = f.sample_k(3, rng2, memo);

    // Both are valid samples
    EXPECT_EQ(r1.exact_count(), bigint::BigInt(3));
    EXPECT_EQ(r2.exact_count(), bigint::BigInt(3));
    EXPECT_TRUE(r1.is_subset_family(f));
    EXPECT_TRUE(r2.is_subset_family(f));
    // Very unlikely to be identical with different seeds
    // (not guaranteed but extremely improbable for 32-choose-3 = 4960)
    EXPECT_NE(r1, r2);
}

TEST_F(BDDTest, ZDD_SampleK_NullThrows) {
    ZDD f;
    f = ZDD::Null;
    ZddCountMemo memo(f);
    std::mt19937_64 rng(42);
    EXPECT_THROW(f.sample_k(1, rng, memo), std::invalid_argument);
}

TEST_F(BDDTest, ZDD_SampleK_NegativeKThrows) {
    bddnewvar();
    ZDD f = ZDD::singleton(1);
    ZddCountMemo memo(f);
    std::mt19937_64 rng(42);
    EXPECT_THROW(f.sample_k(-1, rng, memo), std::invalid_argument);
}

TEST_F(BDDTest, ZDD_SampleK_Uniformity) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // F = {{1}, {2}, {3}}, sample k=1 many times
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2) + ZDD::singleton(3);
    ZddCountMemo memo(f);

    int counts[4] = {};  // counts[1..3]
    const int trials = 3000;
    std::mt19937_64 rng(12345);
    for (int i = 0; i < trials; ++i) {
        ZDD result = f.sample_k(1, rng, memo);
        auto sets = result.enumerate();
        ASSERT_EQ(sets.size(), 1u);
        ASSERT_EQ(sets[0].size(), 1u);
        counts[sets[0][0]]++;
    }
    // Each should be ~1000 (±150)
    for (int v = 1; v <= 3; ++v) {
        EXPECT_GT(counts[v], 700);
        EXPECT_LT(counts[v], 1300);
    }
}

TEST_F(BDDTest, ZDD_SampleK_LargerFamily) {
    for (int i = 0; i < 6; ++i) bddnewvar();
    // Power set of {1,..,6} = 64 sets
    ZDD f = ZDD::power_set(6);
    ZddCountMemo memo(f);
    std::mt19937_64 rng(42);

    ZDD result = f.sample_k(20, rng, memo);
    EXPECT_EQ(result.exact_count(), bigint::BigInt(20));
    EXPECT_TRUE(result.is_subset_family(f));
}

// --- ZDD::weighted_sample ---

TEST_F(BDDTest, ZDD_WeightedSample_Product_Singleton) {
    bddnewvar();
    ZDD f = ZDD::singleton(1);  // {{1}}
    std::vector<double> w = {0.0, 3.0};
    WeightedSampleMemo memo(f, w, WeightMode::Product);
    std::mt19937_64 rng(42);
    auto s = f.weighted_sample(w, WeightMode::Product, rng, memo);
    ASSERT_EQ(s.size(), 1u);
    EXPECT_EQ(s[0], 1u);
}

TEST_F(BDDTest, ZDD_WeightedSample_Product_TwoSets) {
    bddnewvar();
    bddnewvar();
    // F = {{1}, {2}}, w[1]=1, w[2]=3 → P({1})=1/4, P({2})=3/4
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<double> w = {0.0, 1.0, 3.0};
    WeightedSampleMemo memo(f, w, WeightMode::Product);
    std::mt19937_64 rng(123);

    int count1 = 0, count2 = 0;
    const int trials = 4000;
    for (int i = 0; i < trials; ++i) {
        auto s = f.weighted_sample(w, WeightMode::Product, rng, memo);
        ASSERT_EQ(s.size(), 1u);
        if (s[0] == 1) count1++;
        else count2++;
    }
    // P({1}) ≈ 25% → expect ~1000
    EXPECT_GT(count1, 700);
    EXPECT_LT(count1, 1400);
    // P({2}) ≈ 75% → expect ~3000
    EXPECT_GT(count2, 2600);
    EXPECT_LT(count2, 3300);
}

TEST_F(BDDTest, ZDD_WeightedSample_Sum_TwoSets) {
    bddnewvar();
    bddnewvar();
    // F = {{1}, {2}}, w[1]=2, w[2]=6 → P({1})=2/8=25%, P({2})=6/8=75%
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<double> w = {0.0, 2.0, 6.0};
    WeightedSampleMemo memo(f, w, WeightMode::Sum);
    std::mt19937_64 rng(456);

    int count1 = 0, count2 = 0;
    const int trials = 4000;
    for (int i = 0; i < trials; ++i) {
        auto s = f.weighted_sample(w, WeightMode::Sum, rng, memo);
        ASSERT_EQ(s.size(), 1u);
        if (s[0] == 1) count1++;
        else count2++;
    }
    EXPECT_GT(count1, 700);
    EXPECT_LT(count1, 1400);
    EXPECT_GT(count2, 2600);
    EXPECT_LT(count2, 3300);
}

TEST_F(BDDTest, ZDD_WeightedSample_Sum_MultiElementSets) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // F = {{1,2}, {3}}, w[1]=1, w[2]=2, w[3]=4
    // w_sum({1,2}) = 3, w_sum({3}) = 4 → P({1,2})=3/7≈43%, P({3})=4/7≈57%
    ZDD s12 = ZDD::single_set({1, 2});
    ZDD s3 = ZDD::singleton(3);
    ZDD f = s12 + s3;
    std::vector<double> w = {0.0, 1.0, 2.0, 4.0};
    WeightedSampleMemo memo(f, w, WeightMode::Sum);
    std::mt19937_64 rng(789);

    int count_12 = 0, count_3 = 0;
    const int trials = 4000;
    for (int i = 0; i < trials; ++i) {
        auto s = f.weighted_sample(w, WeightMode::Sum, rng, memo);
        if (s.size() == 2) count_12++;
        else count_3++;
    }
    // P({1,2}) ≈ 43% → ~1714
    EXPECT_GT(count_12, 1300);
    EXPECT_LT(count_12, 2200);
    // P({3}) ≈ 57% → ~2286
    EXPECT_GT(count_3, 1800);
    EXPECT_LT(count_3, 2700);
}

TEST_F(BDDTest, ZDD_WeightedSample_Product_WithEmptySet) {
    bddnewvar();
    // F = {∅, {1}}, w[1]=2
    // Product: w(∅)=1, w({1})=2 → P(∅)=1/3≈33%, P({1})=2/3≈67%
    ZDD f = ZDD::Single + ZDD::singleton(1);
    std::vector<double> w = {0.0, 2.0};
    WeightedSampleMemo memo(f, w, WeightMode::Product);
    std::mt19937_64 rng(42);

    int count_empty = 0, count_1 = 0;
    const int trials = 3000;
    for (int i = 0; i < trials; ++i) {
        auto s = f.weighted_sample(w, WeightMode::Product, rng, memo);
        if (s.empty()) count_empty++;
        else count_1++;
    }
    // P(∅) ≈ 33% → ~1000
    EXPECT_GT(count_empty, 700);
    EXPECT_LT(count_empty, 1400);
    EXPECT_GT(count_1, 1600);
    EXPECT_LT(count_1, 2300);
}

TEST_F(BDDTest, ZDD_WeightedSample_Sum_WithEmptySet) {
    bddnewvar();
    // F = {∅, {1}}, w[1]=2
    // Sum: w(∅)=0, w({1})=2 → P(∅)=0, P({1})=1
    ZDD f = ZDD::Single + ZDD::singleton(1);
    std::vector<double> w = {0.0, 2.0};
    WeightedSampleMemo memo(f, w, WeightMode::Sum);
    std::mt19937_64 rng(42);

    for (int i = 0; i < 100; ++i) {
        auto s = f.weighted_sample(w, WeightMode::Sum, rng, memo);
        ASSERT_EQ(s.size(), 1u);
        EXPECT_EQ(s[0], 1u);
    }
}

TEST_F(BDDTest, ZDD_WeightedSample_EmptyFamily_Throws) {
    ZDD f(0);  // empty family
    std::vector<double> w = {0.0};
    WeightedSampleMemo memo(f, w, WeightMode::Product);
    std::mt19937_64 rng(42);
    EXPECT_THROW(
        f.weighted_sample(w, WeightMode::Product, rng, memo),
        std::invalid_argument);
}

TEST_F(BDDTest, ZDD_WeightedSample_NullZDD_Throws) {
    ZDD f = ZDD::Null;
    std::vector<double> w = {0.0};
    WeightedSampleMemo memo(f, w, WeightMode::Product);
    std::mt19937_64 rng(42);
    EXPECT_THROW(
        f.weighted_sample(w, WeightMode::Product, rng, memo),
        std::invalid_argument);
}

TEST_F(BDDTest, ZDD_WeightedSample_NegativeWeight_Throws) {
    bddnewvar();
    ZDD f = ZDD::singleton(1);
    std::vector<double> w = {0.0, -1.0};
    WeightedSampleMemo memo(f, w, WeightMode::Product);
    std::mt19937_64 rng(42);
    EXPECT_THROW(
        f.weighted_sample(w, WeightMode::Product, rng, memo),
        std::invalid_argument);
}

TEST_F(BDDTest, ZDD_WeightedSample_ZeroTotal_Throws) {
    bddnewvar();
    bddnewvar();
    // F = {{1}, {2}}, all weights 0 → Sum mode total = 0
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<double> w = {0.0, 0.0, 0.0};
    WeightedSampleMemo memo(f, w, WeightMode::Sum);
    std::mt19937_64 rng(42);
    EXPECT_THROW(
        f.weighted_sample(w, WeightMode::Sum, rng, memo),
        std::invalid_argument);
}

TEST_F(BDDTest, ZDD_WeightedSample_InfWeight_Throws) {
    bddnewvar();
    ZDD f = ZDD::singleton(1);
    std::vector<double> w = {0.0, std::numeric_limits<double>::infinity()};
    WeightedSampleMemo memo(f, w, WeightMode::Product);
    std::mt19937_64 rng(42);
    EXPECT_THROW(
        f.weighted_sample(w, WeightMode::Product, rng, memo),
        std::invalid_argument);
}

TEST_F(BDDTest, ZDD_WeightedSample_NaNWeight_Throws) {
    bddnewvar();
    ZDD f = ZDD::singleton(1);
    std::vector<double> w = {0.0, std::numeric_limits<double>::quiet_NaN()};
    WeightedSampleMemo memo(f, w, WeightMode::Sum);
    std::mt19937_64 rng(42);
    EXPECT_THROW(
        f.weighted_sample(w, WeightMode::Sum, rng, memo),
        std::invalid_argument);
}

TEST_F(BDDTest, ZDD_BoltzmannWeights_Overflow_Throws) {
    // beta < 0 with large weight causes exp() to overflow to inf
    bddnewvar();
    std::vector<double> w = {0.0, 1000.0};
    EXPECT_THROW(
        ZDD::boltzmann_weights(w, -100.0),
        std::invalid_argument);
}

TEST_F(BDDTest, ZDD_WeightedSample_MemoReuse) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(3);
    std::vector<double> w = {0.0, 1.0, 2.0, 3.0};
    WeightedSampleMemo memo(f, w, WeightMode::Product);
    std::mt19937_64 rng(42);

    for (int i = 0; i < 100; ++i) {
        auto s = f.weighted_sample(w, WeightMode::Product, rng, memo);
        // Result should be a valid set (each var in {1,2,3})
        for (bddvar v : s) {
            EXPECT_GE(v, 1u);
            EXPECT_LE(v, 3u);
        }
    }
}

TEST_F(BDDTest, ZDD_BoltzmannSample_Basic) {
    bddnewvar();
    bddnewvar();
    // F = {{1}, {2}}, w[1]=1, w[2]=2, beta=1
    // P({1}) ∝ exp(-1) ≈ 0.368, P({2}) ∝ exp(-2) ≈ 0.135
    // P({1}) ≈ 73%, P({2}) ≈ 27%
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<double> orig_w = {0.0, 1.0, 2.0};
    double beta = 1.0;
    auto tw = ZDD::boltzmann_weights(orig_w, beta);
    WeightedSampleMemo memo(f, tw, WeightMode::Product);
    std::mt19937_64 rng(42);

    int count1 = 0, count2 = 0;
    const int trials = 3000;
    for (int i = 0; i < trials; ++i) {
        auto s = f.boltzmann_sample(orig_w, beta, rng, memo);
        ASSERT_EQ(s.size(), 1u);
        if (s[0] == 1) count1++;
        else count2++;
    }
    // P({1}) ≈ 73% → ~2190
    EXPECT_GT(count1, 1800);
    EXPECT_LT(count1, 2600);
    // P({2}) ≈ 27% → ~810
    EXPECT_GT(count2, 400);
    EXPECT_LT(count2, 1200);
}

TEST_F(BDDTest, ZDD_BoltzmannSample_ZeroBeta) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // F = {{1}, {2}, {3}}, beta=0 → exp(0)=1 for all → uniform
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2) + ZDD::singleton(3);
    std::vector<double> orig_w = {0.0, 5.0, 10.0, 15.0};
    double beta = 0.0;
    auto tw = ZDD::boltzmann_weights(orig_w, beta);
    WeightedSampleMemo memo(f, tw, WeightMode::Product);
    std::mt19937_64 rng(42);

    int counts[4] = {};
    const int trials = 3000;
    for (int i = 0; i < trials; ++i) {
        auto s = f.boltzmann_sample(orig_w, beta, rng, memo);
        ASSERT_EQ(s.size(), 1u);
        counts[s[0]]++;
    }
    // Each ≈ 1000
    for (int v = 1; v <= 3; ++v) {
        EXPECT_GT(counts[v], 700);
        EXPECT_LT(counts[v], 1300);
    }
}

TEST_F(BDDTest, ZDD_BoltzmannSample_WrongMemoMode) {
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<double> w = {0.0, 1.0, 2.0};
    // Sum mode memo is invalid for boltzmann_sample
    WeightedSampleMemo memo(f, w, WeightMode::Sum);
    std::mt19937_64 rng(42);
    EXPECT_THROW(f.boltzmann_sample(w, 1.0, rng, memo), std::invalid_argument);
}

TEST_F(BDDTest, ZDD_BoltzmannSample_WeightsMismatch) {
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<double> w1 = {0.0, 1.0, 2.0};
    std::vector<double> w2 = {0.0, 5.0, 10.0};
    double beta = 1.0;
    // Memo created with w1, but boltzmann_sample called with w2
    auto tw1 = ZDD::boltzmann_weights(w1, beta);
    WeightedSampleMemo memo(f, tw1, WeightMode::Product);
    std::mt19937_64 rng(42);
    EXPECT_THROW(f.boltzmann_sample(w2, beta, rng, memo), std::invalid_argument);
}

TEST_F(BDDTest, ZDD_BoltzmannSample_BetaMismatch) {
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<double> w = {0.0, 1.0, 2.0};
    // Memo created with beta=1.0, but boltzmann_sample called with beta=2.0
    auto tw = ZDD::boltzmann_weights(w, 1.0);
    WeightedSampleMemo memo(f, tw, WeightMode::Product);
    std::mt19937_64 rng(42);
    EXPECT_THROW(f.boltzmann_sample(w, 2.0, rng, memo), std::invalid_argument);
}

TEST_F(BDDTest, ZDD_WeightedSample_MemoWeightsMismatch) {
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<double> w1 = {0.0, 1.0, 2.0};
    std::vector<double> w2 = {0.0, 3.0, 4.0};
    WeightedSampleMemo memo(f, w1, WeightMode::Sum);
    std::mt19937_64 rng(42);
    // Passing different weights than memo was created with should throw
    EXPECT_THROW(f.weighted_sample(w2, WeightMode::Sum, rng, memo),
                 std::invalid_argument);
}

TEST_F(BDDTest, ZDD_WeightedSample_Product_LargerFamily) {
    for (int i = 0; i < 3; ++i) bddnewvar();
    // Power set of {1,2,3} = 8 sets, w = [0, 1, 2, 3]
    // Product mode: w({}) = 1, w({1}) = 1, w({2}) = 2, w({3}) = 3,
    //               w({1,2}) = 2, w({1,3}) = 3, w({2,3}) = 6, w({1,2,3}) = 6
    // Total = 1+1+2+3+2+3+6+6 = 24
    ZDD f = ZDD::power_set(3);
    std::vector<double> w = {0.0, 1.0, 2.0, 3.0};
    WeightedSampleMemo memo(f, w, WeightMode::Product);
    std::mt19937_64 rng(42);

    std::map<std::vector<bddvar>, int> freq;
    const int trials = 6000;
    for (int i = 0; i < trials; ++i) {
        auto s = f.weighted_sample(w, WeightMode::Product, rng, memo);
        freq[s]++;
    }
    // All 8 sets should be reachable
    EXPECT_EQ(freq.size(), 8u);

    // w({1,2,3})=6 → P=6/24=25%, expect ~1500
    std::vector<bddvar> set123 = {1, 2, 3};
    EXPECT_GT(freq[set123], 1100);
    EXPECT_LT(freq[set123], 1900);
}

TEST_F(BDDTest, ZDD_WeightedSample_Sum_OnlyEmptySet) {
    // F = {∅}, Sum mode: w(∅)=0, but only 1 set → should return ∅
    ZDD f = ZDD::Single;
    std::vector<double> w = {0.0};
    WeightedSampleMemo memo(f, w, WeightMode::Sum);
    std::mt19937_64 rng(42);
    auto s = f.weighted_sample(w, WeightMode::Sum, rng, memo);
    EXPECT_TRUE(s.empty());
}

// ============================================================
// max_size tests
// ============================================================

TEST_F(BDDTest, MaxSize_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    EXPECT_EQ(empty.max_size(), 0u);
    EXPECT_EQ(bddmaxsize(empty.GetID()), 0u);
}

TEST_F(BDDTest, MaxSize_UnitFamily) {
    bddnewvar();
    ZDD unit(1);  // {∅}
    EXPECT_EQ(unit.max_size(), 0u);
}

TEST_F(BDDTest, MaxSize_SingletonSet) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);  // {{v1}}
    EXPECT_EQ(f.max_size(), 1u);
}

TEST_F(BDDTest, MaxSize_MultipleSets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // f = {{v1}, {v1, v2}, {v1, v2, v3}}
    ZDD f = ZDD::singleton(v1)
          + ZDD::single_set({v1, v2})
          + ZDD::single_set({v1, v2, v3});
    EXPECT_EQ(f.max_size(), 3u);
}

TEST_F(BDDTest, MaxSize_WithEmpty) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // f = {∅, {v1}, {v1, v2}}
    ZDD f = ZDD::Single + ZDD::singleton(v1) + ZDD::single_set({v1, v2});
    EXPECT_EQ(f.max_size(), 2u);
}

TEST_F(BDDTest, MaxSize_PowerSet) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(3);
    EXPECT_EQ(f.max_size(), 3u);
}

TEST_F(BDDTest, MaxSize_EqualsLen) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2})
          + ZDD::single_set({v1, v2, v3});
    EXPECT_EQ(f.max_size(), f.Len());
}

TEST_F(BDDTest, MaxSize_MinMaxConsistency) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2})
          + ZDD::single_set({v1, v2, v3});
    EXPECT_LE(f.min_size(), f.max_size());
}

TEST_F(BDDTest, MaxSize_ConsistencyWithProfile) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(3) - ZDD::Single;
    auto prof = f.profile();
    uint64_t expected_max = 0;
    for (size_t k = 0; k < prof.size(); ++k) {
        if (prof[k] > bigint::BigInt(0)) {
            expected_max = k;
        }
    }
    EXPECT_EQ(f.max_size(), expected_max);
}

// ============================================================
// is_disjoint tests
// ============================================================

TEST_F(BDDTest, IsDisjoint_BothEmpty) {
    bddnewvar();
    ZDD a(0), b(0);
    EXPECT_TRUE(a.is_disjoint(b));
    EXPECT_TRUE(bddisdisjoint(a.GetID(), b.GetID()));
}

TEST_F(BDDTest, IsDisjoint_OneEmpty) {
    bddvar v1 = bddnewvar();
    ZDD a = ZDD::singleton(v1);
    ZDD b(0);
    EXPECT_TRUE(a.is_disjoint(b));
    EXPECT_TRUE(b.is_disjoint(a));
}

TEST_F(BDDTest, IsDisjoint_Identical) {
    bddvar v1 = bddnewvar();
    ZDD a = ZDD::singleton(v1);
    EXPECT_FALSE(a.is_disjoint(a));
}

TEST_F(BDDTest, IsDisjoint_TrueCase) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // a = {{v1}}, b = {{v2}}
    ZDD a = ZDD::singleton(v1);
    ZDD b = ZDD::singleton(v2);
    EXPECT_TRUE(a.is_disjoint(b));
    EXPECT_TRUE(b.is_disjoint(a));
}

TEST_F(BDDTest, IsDisjoint_FalseCase) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // a = {{v1}, {v2}}, b = {{v2}, {v1, v2}}
    ZDD a = ZDD::singleton(v1) + ZDD::singleton(v2);
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v1, v2});
    EXPECT_FALSE(a.is_disjoint(b));
}

TEST_F(BDDTest, IsDisjoint_EmptySetOverlap) {
    bddvar v1 = bddnewvar();
    // a = {∅, {v1}}, b = {∅}
    ZDD a = ZDD::Single + ZDD::singleton(v1);
    ZDD b = ZDD::Single;
    EXPECT_FALSE(a.is_disjoint(b));
}

TEST_F(BDDTest, IsDisjoint_EmptySetNoOverlap) {
    bddvar v1 = bddnewvar();
    // a = {{v1}}, b = {∅}
    ZDD a = ZDD::singleton(v1);
    ZDD b = ZDD::Single;
    EXPECT_TRUE(a.is_disjoint(b));
}

TEST_F(BDDTest, IsDisjoint_ConsistentWithIntersection) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // a = {{v1}, {v1, v2}}, b = {{v2}, {v2, v3}}
    ZDD a = ZDD::singleton(v1) + ZDD::single_set({v1, v2});
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v2, v3});
    bool disjoint = a.is_disjoint(b);
    bool intersec_empty = (a & b).is_zero();
    EXPECT_EQ(disjoint, intersec_empty);
}

TEST_F(BDDTest, IsDisjoint_ConsistentWithIntersection_Overlap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // a = {{v1}, {v1, v2}}, b = {{v1}, {v2}}
    ZDD a = ZDD::singleton(v1) + ZDD::single_set({v1, v2});
    ZDD b = ZDD::singleton(v1) + ZDD::singleton(v2);
    bool disjoint = a.is_disjoint(b);
    bool intersec_empty = (a & b).is_zero();
    EXPECT_EQ(disjoint, intersec_empty);
}

TEST_F(BDDTest, IsDisjoint_PowerSetVsComplement) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD all = ZDD::power_set(3);
    ZDD a = ZDD::Single;  // {∅}
    ZDD b = all - a;  // everything except ∅
    EXPECT_TRUE(a.is_disjoint(b));
    EXPECT_FALSE(a.is_disjoint(all));
}

TEST_F(BDDTest, IsDisjoint_LargerExample) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddvar v4 = bddnewvar();
    // a = all sets of size <= 2
    ZDD all = ZDD::power_set(4);
    ZDD a = all.size_le(2);
    // b = all sets of size >= 3
    ZDD b = all.size_ge(3);
    EXPECT_TRUE(a.is_disjoint(b));
    // Verify with intersection
    EXPECT_TRUE((a & b).is_zero());
}

TEST_F(BDDTest, IsDisjoint_Symmetric) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD a = ZDD::singleton(v1) + ZDD::single_set({v2, v3});
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v1, v3});
    EXPECT_EQ(a.is_disjoint(b), b.is_disjoint(a));
}

TEST_F(BDDTest, IsDisjoint_Null) {
    bddvar v1 = bddnewvar();
    ZDD a = ZDD::singleton(v1);
    // null is not a valid family; is_disjoint should return false
    // (consistent with is_subset_family returning false for null)
    EXPECT_FALSE(ZDD::Null.is_disjoint(a));
    EXPECT_FALSE(a.is_disjoint(ZDD::Null));
    EXPECT_FALSE(ZDD::Null.is_disjoint(ZDD::Null));
}

// ============================================================
// count_intersec tests
// ============================================================

TEST_F(BDDTest, CountIntersec_BothEmpty) {
    bddnewvar();
    ZDD a(0), b(0);
    EXPECT_EQ(a.count_intersec(b), bigint::BigInt(0));
}

TEST_F(BDDTest, CountIntersec_OneEmpty) {
    bddvar v1 = bddnewvar();
    ZDD a = ZDD::singleton(v1);
    ZDD b(0);
    EXPECT_EQ(a.count_intersec(b), bigint::BigInt(0));
    EXPECT_EQ(b.count_intersec(a), bigint::BigInt(0));
}

TEST_F(BDDTest, CountIntersec_Identical) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD ps = ZDD::power_set(3);
    EXPECT_EQ(ps.count_intersec(ps), ps.exact_count());
}

TEST_F(BDDTest, CountIntersec_Disjoint) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD::singleton(v1);
    ZDD b = ZDD::singleton(v2);
    EXPECT_EQ(a.count_intersec(b), bigint::BigInt(0));
}

TEST_F(BDDTest, CountIntersec_PartialOverlap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // a = {{v1}, {v2}}, b = {{v2}, {v1,v2}}
    ZDD a = ZDD::singleton(v1) + ZDD::singleton(v2);
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v1, v2});
    EXPECT_EQ(a.count_intersec(b), bigint::BigInt(1));
}

TEST_F(BDDTest, CountIntersec_EmptySetOverlap) {
    bddvar v1 = bddnewvar();
    // a = {∅, {v1}}, b = {∅}
    ZDD a = ZDD::Single + ZDD::singleton(v1);
    ZDD b = ZDD::Single;
    EXPECT_EQ(a.count_intersec(b), bigint::BigInt(1));
}

TEST_F(BDDTest, CountIntersec_ConsistentWithIntersection) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD ps = ZDD::power_set(3);
    ZDD c32 = ZDD::combination(3, 2);
    EXPECT_EQ(ps.count_intersec(c32), (ps & c32).exact_count());
}

TEST_F(BDDTest, CountIntersec_Symmetric) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD a = ZDD::singleton(v1) + ZDD::single_set({v2, v3});
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v1, v3});
    EXPECT_EQ(a.count_intersec(b), b.count_intersec(a));
}

TEST_F(BDDTest, CountIntersec_Complement) {
    bddvar v1 = bddnewvar();
    ZDD s1 = ZDD::singleton(v1);
    ZDD comp = ~s1;  // ~{{v1}} = {∅, {v1}}
    EXPECT_EQ(comp.count_intersec(s1), bigint::BigInt(1));
}

TEST_F(BDDTest, CountIntersec_Null) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD ps = ZDD::power_set(3);
    EXPECT_EQ(ZDD::Null.count_intersec(ps), bigint::BigInt(0));
    EXPECT_EQ(ps.count_intersec(ZDD::Null), bigint::BigInt(0));
}

TEST_F(BDDTest, CountIntersec_FreeFunction) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddp ps = ZDD::power_set(3).GetID();
    bddp c32 = ZDD::combination(3, 2).GetID();
    EXPECT_EQ(bddcountintersec(ps, c32),
              bddexactcount(bddintersec(ps, c32)));
}

TEST_F(BDDTest, CountIntersec_LargeExample) {
    for (int i = 0; i < 10; ++i) bddnewvar();
    ZDD ps = ZDD::power_set(10);
    ZDD c105 = ZDD::combination(10, 5);
    EXPECT_EQ(ps.count_intersec(c105), bigint::BigInt(252));
}

// ============================================================
// jaccard_index tests
// ============================================================

TEST_F(BDDTest, JaccardIndex_BothEmpty) {
    bddnewvar();
    ZDD a(0), b(0);
    EXPECT_DOUBLE_EQ(a.jaccard_index(b), 1.0);
}

TEST_F(BDDTest, JaccardIndex_OneEmpty) {
    bddvar v1 = bddnewvar();
    ZDD a = ZDD::singleton(v1);
    ZDD b(0);
    EXPECT_DOUBLE_EQ(a.jaccard_index(b), 0.0);
    EXPECT_DOUBLE_EQ(b.jaccard_index(a), 0.0);
}

TEST_F(BDDTest, JaccardIndex_Identical) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD ps = ZDD::power_set(3);
    EXPECT_DOUBLE_EQ(ps.jaccard_index(ps), 1.0);
}

TEST_F(BDDTest, JaccardIndex_Disjoint) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD::singleton(v1);
    ZDD b = ZDD::singleton(v2);
    EXPECT_DOUBLE_EQ(a.jaccard_index(b), 0.0);
}

TEST_F(BDDTest, JaccardIndex_PartialOverlap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // a = {{v1}, {v2}}, b = {{v2}, {v1,v2}}
    // |a ∩ b| = 1, |a ∪ b| = 3 → J = 1/3
    ZDD a = ZDD::singleton(v1) + ZDD::singleton(v2);
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v1, v2});
    EXPECT_NEAR(a.jaccard_index(b), 1.0 / 3.0, 1e-15);
}

TEST_F(BDDTest, JaccardIndex_Symmetric) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD a = ZDD::singleton(v1) + ZDD::single_set({v2, v3});
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v1, v3});
    EXPECT_DOUBLE_EQ(a.jaccard_index(b), b.jaccard_index(a));
}

TEST_F(BDDTest, JaccardIndex_SubsetCase) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // a ⊆ b → J = |a| / |b|
    ZDD a = ZDD::combination(3, 2);  // 3 sets
    ZDD b = ZDD::power_set(3);       // 8 sets
    EXPECT_NEAR(a.jaccard_index(b), 3.0 / 8.0, 1e-15);
}

TEST_F(BDDTest, JaccardIndex_Null) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD ps = ZDD::power_set(3);
    EXPECT_DOUBLE_EQ(ZDD::Null.jaccard_index(ps), 0.0);
    EXPECT_DOUBLE_EQ(ps.jaccard_index(ZDD::Null), 0.0);
}

TEST_F(BDDTest, JaccardIndex_FreeFunction) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddp ps = ZDD::power_set(3).GetID();
    bddp c32 = ZDD::combination(3, 2).GetID();
    EXPECT_NEAR(bddjaccardindex(ps, c32), 3.0 / 8.0, 1e-15);
}

TEST_F(BDDTest, JaccardIndex_UnitFamilies) {
    bddnewvar();
    EXPECT_DOUBLE_EQ(ZDD::Single.jaccard_index(ZDD::Single), 1.0);
}

