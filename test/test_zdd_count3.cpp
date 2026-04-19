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

// --- BDD class count/exact_count ---

TEST_F(BDDTest, BDDCount_ClassMethod) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    EXPECT_EQ(f.count(2), 1.0);
    EXPECT_EQ(f.exact_count(2), bigint::BigInt(1));
    BDD nf = ~f;
    EXPECT_EQ(nf.count(2), 3.0);
    EXPECT_EQ(nf.exact_count(2), bigint::BigInt(3));
}

// --- bddhasempty / ZDD::has_empty ---

TEST_F(BDDTest, ZDD_HasEmpty_EmptyFamily) {
    // Empty family {} contains no sets at all
    EXPECT_FALSE(bddhasempty(bddempty));
    ZDD f(0);
    EXPECT_FALSE(f.has_empty());
}

TEST_F(BDDTest, ZDD_HasEmpty_SingleFamily) {
    // Unit family {∅} contains the empty set
    EXPECT_TRUE(bddhasempty(bddsingle));
    ZDD f(1);
    EXPECT_TRUE(f.has_empty());
}

TEST_F(BDDTest, ZDD_HasEmpty_SingletonSet) {
    // Family {{a}} does not contain the empty set
    bddp a = bddchange(bddsingle, 1);
    EXPECT_FALSE(bddhasempty(a));
    ZDD f = ZDD_ID(a);
    EXPECT_FALSE(f.has_empty());
}

TEST_F(BDDTest, ZDD_HasEmpty_UnionWithEmpty) {
    // Family {{a}, ∅} contains the empty set
    bddp a = bddchange(bddsingle, 1);
    bddp u = bddunion(a, bddsingle);
    EXPECT_TRUE(bddhasempty(u));
    ZDD z = ZDD_ID(u);
    EXPECT_TRUE(z.has_empty());
}

TEST_F(BDDTest, ZDD_HasEmpty_ComplementEdge) {
    // Complement toggles ∅ membership
    bddp a = bddchange(bddsingle, 1);  // {{a}}, no ∅
    EXPECT_FALSE(bddhasempty(a));
    EXPECT_TRUE(bddhasempty(bddnot(a)));  // complement adds ∅

    bddp u = bddunion(a, bddsingle);  // {{a}, ∅}, has ∅
    EXPECT_TRUE(bddhasempty(u));
    EXPECT_FALSE(bddhasempty(bddnot(u)));  // complement removes ∅
}

// --- bddcontains / ZDD::contains ---

TEST_F(BDDTest, ZDD_Contains_EmptyFamily) {
    EXPECT_FALSE(bddcontains(bddempty, {}));
    EXPECT_FALSE(bddcontains(bddempty, {1}));
}

TEST_F(BDDTest, ZDD_Contains_SingleFamily) {
    // {∅} contains ∅ but not {1}
    EXPECT_TRUE(bddcontains(bddsingle, {}));
    EXPECT_FALSE(bddcontains(bddsingle, {1}));
}

TEST_F(BDDTest, ZDD_Contains_SingletonSet) {
    bddp a = bddchange(bddsingle, 1);  // {{1}}
    EXPECT_TRUE(bddcontains(a, {1}));
    EXPECT_FALSE(bddcontains(a, {}));
    EXPECT_FALSE(bddcontains(a, {2}));
    EXPECT_FALSE(bddcontains(a, {1, 2}));
}

TEST_F(BDDTest, ZDD_Contains_MultipleVars) {
    // {{1,2}}
    bddp s = bddchange(bddchange(bddsingle, 1), 2);
    EXPECT_TRUE(bddcontains(s, {1, 2}));
    EXPECT_FALSE(bddcontains(s, {1}));
    EXPECT_FALSE(bddcontains(s, {2}));
    EXPECT_FALSE(bddcontains(s, {}));
}

TEST_F(BDDTest, ZDD_Contains_FamilyWithEmpty) {
    // {{1}, ∅}
    bddp a = bddchange(bddsingle, 1);
    bddp f = bddunion(a, bddsingle);
    EXPECT_TRUE(bddcontains(f, {1}));
    EXPECT_TRUE(bddcontains(f, {}));
    EXPECT_FALSE(bddcontains(f, {2}));
}

TEST_F(BDDTest, ZDD_Contains_DuplicateInput) {
    bddp a = bddchange(bddsingle, 1);  // {{1}}
    EXPECT_TRUE(bddcontains(a, {1, 1}));
}

TEST_F(BDDTest, ZDD_Contains_UnsortedInput) {
    bddp s = bddchange(bddchange(bddsingle, 1), 2);  // {{1,2}}
    EXPECT_TRUE(bddcontains(s, {2, 1}));
}

TEST_F(BDDTest, ZDD_Contains_PowerSet) {
    ZDD ps = ZDD::power_set(3);
    // All 8 subsets should be contained
    EXPECT_TRUE(bddcontains(ps.GetID(), {}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {1}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {2}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {3}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {1, 2}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {1, 3}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {2, 3}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {1, 2, 3}));
    // {4} should not be contained (variable 4 exists but not in power_set(3))
}

TEST_F(BDDTest, ZDD_Contains_ComplementEdge) {
    bddp a = bddchange(bddsingle, 1);  // {{1}}, no ∅
    // ~{{1}} toggles ∅ membership → {{1}, ∅} → has ∅
    EXPECT_TRUE(bddcontains(bddnot(a), {}));
    EXPECT_TRUE(bddcontains(bddnot(a), {1}));

    bddp u = bddunion(a, bddsingle);  // {{1}, ∅}
    // ~{{1}, ∅} removes ∅ → {{1}}
    EXPECT_FALSE(bddcontains(bddnot(u), {}));
    EXPECT_TRUE(bddcontains(bddnot(u), {1}));
}

TEST_F(BDDTest, ZDD_Contains_ConsistencyWithEnumerate) {
    ZDD ps = ZDD::power_set(3);
    auto all_sets = ps.enumerate();
    for (auto& s : all_sets) {
        EXPECT_TRUE(ps.contains(s));
    }
    // A non-member
    EXPECT_FALSE(ps.contains({4}));
}

TEST_F(BDDTest, ZDD_Contains_Null) {
    EXPECT_FALSE(bddcontains(bddnull, {}));
    EXPECT_FALSE(bddcontains(bddnull, {1}));
}

TEST_F(BDDTest, ZDD_Contains_MemberFunction) {
    ZDD ps = ZDD::power_set(3);
    EXPECT_TRUE(ps.contains({1, 2}));
    EXPECT_FALSE(ps.contains({4}));
}

// --- bddissubset / ZDD::is_subset_family ---

TEST_F(BDDTest, ZDD_IsSubsetFamily_EmptyIsSubsetOfAll) {
    ZDD e(0);
    ZDD u(1);
    ZDD ps = ZDD::power_set(3);
    EXPECT_TRUE(e.is_subset_family(e));
    EXPECT_TRUE(e.is_subset_family(u));
    EXPECT_TRUE(e.is_subset_family(ps));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_SelfSubset) {
    ZDD ps = ZDD::power_set(3);
    EXPECT_TRUE(ps.is_subset_family(ps));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_UnitFamily) {
    // {∅} ⊆ power_set(3)  → true
    ZDD u(1);
    ZDD ps = ZDD::power_set(3);
    EXPECT_TRUE(u.is_subset_family(ps));
    // power_set(3) ⊆ {∅}  → false
    EXPECT_FALSE(ps.is_subset_family(u));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_SingletonSubset) {
    // {{1}} ⊆ {{1}, {2}} → true
    ZDD s1 = ZDD::singleton(1);
    ZDD s2 = ZDD::singleton(2);
    ZDD f = s1 + s2;
    EXPECT_TRUE(s1.is_subset_family(f));
    EXPECT_TRUE(s2.is_subset_family(f));
    // {{1}, {2}} ⊆ {{1}} → false
    EXPECT_FALSE(f.is_subset_family(s1));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_NotSubset) {
    // {{1}} ⊆ {{2}} → false
    ZDD s1 = ZDD::singleton(1);
    ZDD s2 = ZDD::singleton(2);
    EXPECT_FALSE(s1.is_subset_family(s2));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_ChooseSubsetOfPowerSet) {
    // C(3,2) ⊆ power_set(3) → true
    ZDD ps = ZDD::power_set(3);
    ZDD c32 = ZDD::combination(3, 2);
    EXPECT_TRUE(c32.is_subset_family(ps));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_ComplementEdge) {
    // {{1}} complemented → {{1}, ∅}
    // {{1}, ∅} ⊆ power_set(3) → true
    ZDD s1 = ZDD::singleton(1);
    ZDD comp = ~s1;
    ZDD ps = ZDD::power_set(3);
    EXPECT_TRUE(comp.is_subset_family(ps));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_ConsistencyWithSubtract) {
    // F ⊆ G iff (F - G) == ∅
    ZDD ps = ZDD::power_set(3);
    ZDD c32 = ZDD::combination(3, 2);
    ZDD s1 = ZDD::singleton(1);

    EXPECT_EQ(c32.is_subset_family(ps), (c32 - ps).is_zero());
    EXPECT_EQ(ps.is_subset_family(c32), (ps - c32).is_zero());
    EXPECT_EQ(s1.is_subset_family(c32), (s1 - c32).is_zero());
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_Null) {
    ZDD n(-1);
    ZDD ps = ZDD::power_set(3);
    EXPECT_FALSE(n.is_subset_family(ps));
    EXPECT_FALSE(ps.is_subset_family(n));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_FreeFunction) {
    bddp ps = ZDD::power_set(3).GetID();
    bddp c32 = ZDD::combination(3, 2).GetID();
    EXPECT_TRUE(bddissubset(c32, ps));
    EXPECT_FALSE(bddissubset(ps, c32));
}

// --- bddflatten / ZDD::flatten ---

TEST_F(BDDTest, ZDD_Flatten_EmptyFamily) {
    ZDD e(0);
    ZDD result = e.flatten();
    EXPECT_TRUE(result.is_zero());
}

TEST_F(BDDTest, ZDD_Flatten_Null) {
    EXPECT_EQ(bddflatten(bddnull), bddnull);
}

TEST_F(BDDTest, ZDD_Flatten_UnitFamily) {
    // {∅} → {∅}
    ZDD u(1);
    ZDD result = u.flatten();
    EXPECT_TRUE(result.is_one());
}

TEST_F(BDDTest, ZDD_Flatten_SingletonSet) {
    // {{1}} → {{1}}
    ZDD s1 = ZDD::singleton(1);
    ZDD result = s1.flatten();
    EXPECT_EQ(result.GetID(), s1.GetID());
}

TEST_F(BDDTest, ZDD_Flatten_TwoDisjointSets) {
    // {{1}, {2}} → {{1,2}}
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    ZDD result = f.flatten();
    ZDD expected = ZDD::single_set({1, 2});
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Flatten_OverlappingSets) {
    // {{1,2}, {2,3}} → {{1,2,3}}
    ZDD f = ZDD::single_set({1, 2}) + ZDD::single_set({2, 3});
    ZDD result = f.flatten();
    ZDD expected = ZDD::single_set({1, 2, 3});
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Flatten_PowerSet) {
    // power_set(3) → {{1,2,3}}
    ZDD ps = ZDD::power_set(3);
    ZDD result = ps.flatten();
    ZDD expected = ZDD::single_set({1, 2, 3});
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Flatten_WithEmptySet) {
    // {{}, {1,2}} → {{1,2}} (∅ contributes nothing)
    ZDD f = ZDD(1) + ZDD::single_set({1, 2});
    ZDD result = f.flatten();
    ZDD expected = ZDD::single_set({1, 2});
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Flatten_ComplementEdge) {
    // ~{{1}} = {{1}, ∅} → flatten = {{1}}
    ZDD s1 = ZDD::singleton(1);
    ZDD comp = ~s1;
    ZDD result = comp.flatten();
    EXPECT_EQ(result.GetID(), s1.GetID());
}

TEST_F(BDDTest, ZDD_Flatten_ResultIsSingleSet) {
    // flatten always returns 0 or 1 sets
    ZDD ps = ZDD::power_set(4);
    ZDD result = ps.flatten();
    EXPECT_EQ(result.exact_count(), bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Flatten_FreeFunction) {
    bddp f = bddunion(bddchange(bddsingle, 1), bddchange(bddsingle, 2));
    bddp result = bddflatten(f);
    bddp expected = bddchange(bddchange(bddsingle, 1), 2);
    EXPECT_EQ(result, expected);
}

// --- bddcoalesce / ZDD::coalesce ---

TEST_F(BDDTest, ZDD_Coalesce_EmptyFamily) {
    ZDD e(0);
    EXPECT_TRUE(e.coalesce(1, 2).is_zero());
}

TEST_F(BDDTest, ZDD_Coalesce_UnitFamily) {
    // {∅} → {∅} (no variables to merge)
    ZDD u(1);
    EXPECT_TRUE(u.coalesce(1, 2).is_one());
}

TEST_F(BDDTest, ZDD_Coalesce_SameVariable) {
    // coalesce(v, v) = identity
    ZDD f = ZDD::power_set(3);
    EXPECT_EQ(f.coalesce(1, 1).GetID(), f.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_V2NotPresent) {
    // {{1}} coalesce(1,2): v2=2 allocated but not in any set → unchanged
    ZDD::new_var();  // var 1
    ZDD::new_var();  // var 2
    ZDD s1 = ZDD::singleton(1);
    ZDD result = s1.coalesce(1, 2);
    EXPECT_EQ(result.GetID(), s1.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_V1OutOfRange) {
    ZDD::new_var();
    ZDD s1 = ZDD::singleton(1);
    EXPECT_THROW(s1.coalesce(99, 1), std::invalid_argument);
}

TEST_F(BDDTest, ZDD_Coalesce_V2OutOfRange) {
    ZDD::new_var();
    ZDD s1 = ZDD::singleton(1);
    EXPECT_THROW(s1.coalesce(1, 99), std::invalid_argument);
}

TEST_F(BDDTest, ZDD_Coalesce_V1NotPresent) {
    // {{2}} coalesce(1,2): v2=2 replaced by v1=1 → {{1}}
    ZDD s2 = ZDD::singleton(2);
    ZDD result = s2.coalesce(1, 2);
    ZDD expected = ZDD::singleton(1);
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_BothPresent) {
    // {{1,2}} coalesce(1,2): both present → v2 removed, v1 stays → {{1}}
    ZDD s12 = ZDD::single_set({1, 2});
    ZDD result = s12.coalesce(1, 2);
    ZDD expected = ZDD::singleton(1);
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_MergesDuplicates) {
    // {{1}, {2}} coalesce(1,2): both become {{1}} → {{1}} (deduplicated)
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    ZDD result = f.coalesce(1, 2);
    ZDD expected = ZDD::singleton(1);
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_MixedSets) {
    // {{1,3}, {2,3}} coalesce(1,2): → {{1,3}} (deduplicated)
    ZDD f = ZDD::single_set({1, 3}) + ZDD::single_set({2, 3});
    ZDD result = f.coalesce(1, 2);
    ZDD expected = ZDD::single_set({1, 3});
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_PreservesOtherVariables) {
    // {{2,3}} coalesce(1,2): → {{1,3}}
    ZDD f = ZDD::single_set({2, 3});
    ZDD result = f.coalesce(1, 2);
    auto sets = result.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    std::vector<bddvar> expected = {1, 3};
    EXPECT_EQ(sets[0], expected);
}

TEST_F(BDDTest, ZDD_Coalesce_WithEmptySet) {
    // {{}, {1}, {2}} coalesce(1,2): ∅→∅, {1}→{1}, {2}→{1} → {{}, {1}}
    ZDD f = ZDD(1) + ZDD::singleton(1) + ZDD::singleton(2);
    ZDD result = f.coalesce(1, 2);
    ZDD expected = ZDD(1) + ZDD::singleton(1);
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_ConsistencyWithEnumerate) {
    // Verify by brute-force: enumerate, coalesce each set, compare
    // F = {{1,2}, {2,3}, {3}}
    ZDD f = ZDD::single_set({1, 2}) + ZDD::single_set({2, 3}) + ZDD::singleton(3);
    ZDD result = f.coalesce(1, 2);
    // {1,2} → v2 removed, v1 stays → {1}
    // {2,3} → v2 replaced by v1 → {1,3}
    // {3}   → no v1 or v2 → {3}
    ZDD expected = ZDD::singleton(1) + ZDD::single_set({1, 3}) + ZDD::singleton(3);
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_FreeFunction) {
    bddp f = bddunion(bddchange(bddsingle, 1), bddchange(bddsingle, 2));
    bddp result = bddcoalesce(f, 1, 2);
    bddp expected = bddchange(bddsingle, 1);
    EXPECT_EQ(result, expected);
}

// --- bddchoose / ZDD::choose ---

TEST_F(BDDTest, ZDD_Choose_TerminalCases) {
    EXPECT_EQ(bddchoose(bddnull, 0), bddnull);
    EXPECT_EQ(bddchoose(bddempty, 0), bddempty);
    EXPECT_EQ(bddchoose(bddempty, 1), bddempty);
    EXPECT_EQ(bddchoose(bddsingle, 0), bddsingle);
    EXPECT_EQ(bddchoose(bddsingle, 1), bddempty);
}

TEST_F(BDDTest, ZDD_Choose_NegativeK) {
    EXPECT_EQ(bddchoose(bddsingle, -1), bddempty);
    bddp a = bddchange(bddsingle, 1);
    EXPECT_EQ(bddchoose(a, -1), bddempty);
}

TEST_F(BDDTest, ZDD_Choose_ZeroK_WithEmpty) {
    // {{1}, ∅} → choose(0) = {∅}
    bddp a = bddchange(bddsingle, 1);
    bddp f = bddunion(a, bddsingle);
    EXPECT_EQ(bddchoose(f, 0), bddsingle);
}

TEST_F(BDDTest, ZDD_Choose_ZeroK_WithoutEmpty) {
    // {{1}} → choose(0) = {}
    bddp a = bddchange(bddsingle, 1);
    EXPECT_EQ(bddchoose(a, 0), bddempty);
}

TEST_F(BDDTest, ZDD_Choose_PowerSet) {
    ZDD ps = ZDD::power_set(3);
    // C(3,0)=1, C(3,1)=3, C(3,2)=3, C(3,3)=1
    ZDD c0 = ps.choose(0);
    ZDD c1 = ps.choose(1);
    ZDD c2 = ps.choose(2);
    ZDD c3 = ps.choose(3);

    EXPECT_EQ(c0.exact_count(), bigint::BigInt(1));
    EXPECT_EQ(c1.exact_count(), bigint::BigInt(3));
    EXPECT_EQ(c2.exact_count(), bigint::BigInt(3));
    EXPECT_EQ(c3.exact_count(), bigint::BigInt(1));

    // choose(k) should equal combination(3, k) for power_set(3)
    EXPECT_EQ(c1.GetID(), ZDD::combination(3, 1).GetID());
    EXPECT_EQ(c2.GetID(), ZDD::combination(3, 2).GetID());
    EXPECT_EQ(c3.GetID(), ZDD::combination(3, 3).GetID());
}

TEST_F(BDDTest, ZDD_Choose_KExceedsMax) {
    ZDD ps = ZDD::power_set(3);
    EXPECT_EQ(ps.choose(4), ZDD::Empty);
    EXPECT_EQ(ps.choose(100), ZDD::Empty);
}

TEST_F(BDDTest, ZDD_Choose_ComplementEdge) {
    bddp a = bddchange(bddsingle, 1);  // {{1}}
    // ~{{1}} = {{1}, ∅}
    bddp ca = bddnot(a);
    EXPECT_EQ(bddchoose(ca, 0), bddsingle);  // {∅}
    EXPECT_EQ(bddchoose(ca, 1), a);          // {{1}}

    // {{1}, ∅}: complement removes ∅ → {{1}}
    bddp u = bddunion(a, bddsingle);
    bddp cu = bddnot(u);
    EXPECT_EQ(bddchoose(cu, 0), bddempty);  // no ∅
    EXPECT_EQ(bddchoose(cu, 1), a);         // {{1}}
}

TEST_F(BDDTest, ZDD_Choose_FilterCorrectness) {
    // {{1}, {1,2}, {1,2,3}}
    bddp s1 = bddchange(bddsingle, 1);
    bddp s12 = bddchange(bddchange(bddsingle, 1), 2);
    bddp s123 = bddchange(bddchange(bddchange(bddsingle, 1), 2), 3);
    bddp f = bddunion(bddunion(s1, s12), s123);

    ZDD r1 = ZDD_ID(bddchoose(f, 1));
    ZDD r2 = ZDD_ID(bddchoose(f, 2));
    ZDD r3 = ZDD_ID(bddchoose(f, 3));

    EXPECT_EQ(r1.exact_count(), bigint::BigInt(1));  // {{1}}
    EXPECT_EQ(r2.exact_count(), bigint::BigInt(1));  // {{1,2}}
    EXPECT_EQ(r3.exact_count(), bigint::BigInt(1));  // {{1,2,3}}
    EXPECT_TRUE(r1.contains({1}));
    EXPECT_TRUE(r2.contains({1, 2}));
    EXPECT_TRUE(r3.contains({1, 2, 3}));
}

// --- bddprofile / ZDD::profile ---

TEST_F(BDDTest, ZDD_Profile_EmptyFamily) {
    auto p = bddprofile(bddempty);
    EXPECT_TRUE(p.empty());
}

TEST_F(BDDTest, ZDD_Profile_Null) {
    auto p = bddprofile(bddnull);
    EXPECT_TRUE(p.empty());
}

TEST_F(BDDTest, ZDD_Profile_SingleFamily) {
    // {∅} → profile = [1]
    auto p = bddprofile(bddsingle);
    ASSERT_EQ(p.size(), 1u);
    EXPECT_EQ(p[0], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Profile_SingletonSet) {
    // {{1}} → profile = [0, 1]
    bddp a = bddchange(bddsingle, 1);
    auto p = bddprofile(a);
    ASSERT_EQ(p.size(), 2u);
    EXPECT_EQ(p[0], bigint::BigInt(0));
    EXPECT_EQ(p[1], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Profile_MixedSizes) {
    // {{1}, {1,2}, {1,2,3}} → profile = [0, 1, 1, 1]
    bddp s1 = bddchange(bddsingle, 1);
    bddp s12 = bddchange(bddchange(bddsingle, 1), 2);
    bddp s123 = bddchange(bddchange(bddchange(bddsingle, 1), 2), 3);
    bddp f = bddunion(bddunion(s1, s12), s123);
    auto p = bddprofile(f);
    ASSERT_EQ(p.size(), 4u);
    EXPECT_EQ(p[0], bigint::BigInt(0));
    EXPECT_EQ(p[1], bigint::BigInt(1));
    EXPECT_EQ(p[2], bigint::BigInt(1));
    EXPECT_EQ(p[3], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Profile_WithEmpty) {
    // {{}, {1}, {1,2}} → profile = [1, 1, 1]
    bddp s1 = bddchange(bddsingle, 1);
    bddp s12 = bddchange(bddchange(bddsingle, 1), 2);
    bddp f = bddunion(bddunion(bddsingle, s1), s12);
    auto p = bddprofile(f);
    ASSERT_EQ(p.size(), 3u);
    EXPECT_EQ(p[0], bigint::BigInt(1));
    EXPECT_EQ(p[1], bigint::BigInt(1));
    EXPECT_EQ(p[2], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Profile_PowerSet) {
    // power_set(3) → [1, 3, 3, 1] (binomial coefficients)
    ZDD ps = ZDD::power_set(3);
    auto p = bddprofile(ps.GetID());
    ASSERT_EQ(p.size(), 4u);
    EXPECT_EQ(p[0], bigint::BigInt(1));
    EXPECT_EQ(p[1], bigint::BigInt(3));
    EXPECT_EQ(p[2], bigint::BigInt(3));
    EXPECT_EQ(p[3], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Profile_SumsToCount) {
    ZDD ps = ZDD::power_set(3);
    auto p = ps.profile();
    bigint::BigInt total(0);
    for (auto& v : p) total += v;
    EXPECT_EQ(total, ps.exact_count());
}

TEST_F(BDDTest, ZDD_Profile_ChooseConsistency) {
    ZDD ps = ZDD::power_set(3);
    auto p = ps.profile();
    for (size_t k = 0; k < p.size(); k++) {
        EXPECT_EQ(p[k], ps.choose(static_cast<int>(k)).exact_count());
    }
}

TEST_F(BDDTest, ZDD_Profile_ComplementEdge) {
    bddp a = bddchange(bddsingle, 1);  // {{1}}, profile = [0, 1]
    // ~{{1}} = {{1}, ∅}, profile = [1, 1]
    auto p = bddprofile(bddnot(a));
    ASSERT_EQ(p.size(), 2u);
    EXPECT_EQ(p[0], bigint::BigInt(1));
    EXPECT_EQ(p[1], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Profile_Double) {
    ZDD ps = ZDD::power_set(3);
    auto pd = ps.profile_double();
    ASSERT_EQ(pd.size(), 4u);
    EXPECT_DOUBLE_EQ(pd[0], 1.0);
    EXPECT_DOUBLE_EQ(pd[1], 3.0);
    EXPECT_DOUBLE_EQ(pd[2], 3.0);
    EXPECT_DOUBLE_EQ(pd[3], 1.0);
}

// --- bddelmfreq / ZDD::element_frequency ---

TEST_F(BDDTest, ZDD_ElementFrequency_EmptyFamily) {
    auto freq = bddelmfreq(bddempty);
    EXPECT_TRUE(freq.empty());
}

TEST_F(BDDTest, ZDD_ElementFrequency_Null) {
    auto freq = bddelmfreq(bddnull);
    EXPECT_TRUE(freq.empty());
}

TEST_F(BDDTest, ZDD_ElementFrequency_UnitFamily) {
    // {∅} → no variables → empty
    auto freq = bddelmfreq(bddsingle);
    EXPECT_TRUE(freq.empty());
}

TEST_F(BDDTest, ZDD_ElementFrequency_SingletonSet) {
    // {{1}} → freq[1] = 1
    bddp a = bddchange(bddsingle, 1);
    auto freq = bddelmfreq(a);
    ASSERT_GE(freq.size(), 2u);
    EXPECT_EQ(freq[0], bigint::BigInt(0));
    EXPECT_EQ(freq[1], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_ElementFrequency_TwoSets) {
    // {{1}, {1,2}} → freq[1] = 2, freq[2] = 1
    bddp s1 = bddchange(bddsingle, 1);
    bddp s12 = bddchange(bddchange(bddsingle, 1), 2);
    bddp f = bddunion(s1, s12);
    auto freq = bddelmfreq(f);
    ASSERT_GE(freq.size(), 3u);
    EXPECT_EQ(freq[1], bigint::BigInt(2));
    EXPECT_EQ(freq[2], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_ElementFrequency_PowerSet) {
    // power_set(3) = all subsets of {1,2,3}, 8 sets total
    // Each variable appears in exactly half (4) of the sets
    ZDD ps = ZDD::power_set(3);
    auto freq = ps.element_frequency();
    ASSERT_GE(freq.size(), 4u);
    EXPECT_EQ(freq[1], bigint::BigInt(4));
    EXPECT_EQ(freq[2], bigint::BigInt(4));
    EXPECT_EQ(freq[3], bigint::BigInt(4));
}

TEST_F(BDDTest, ZDD_ElementFrequency_WithEmpty) {
    // {{}, {1}, {1,2}} → freq[1] = 2, freq[2] = 1
    // Empty set doesn't affect frequency
    bddp s1 = bddchange(bddsingle, 1);
    bddp s12 = bddchange(bddchange(bddsingle, 1), 2);
    bddp f = bddunion(bddunion(bddsingle, s1), s12);
    auto freq = bddelmfreq(f);
    ASSERT_GE(freq.size(), 3u);
    EXPECT_EQ(freq[1], bigint::BigInt(2));
    EXPECT_EQ(freq[2], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_ElementFrequency_ComplementEdge) {
    // {{1}} complemented → {{1}, ∅}
    // freq[1] should still be 1 (∅ adds no variable frequency)
    bddp a = bddchange(bddsingle, 1);
    auto freq = bddelmfreq(bddnot(a));
    ASSERT_GE(freq.size(), 2u);
    EXPECT_EQ(freq[1], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_ElementFrequency_SumsToLit) {
    // Sum of all frequencies == total literal count (Lit)
    ZDD ps = ZDD::power_set(3);
    auto freq = ps.element_frequency();
    bigint::BigInt total(0);
    for (auto& v : freq) total += v;
    // Lit() returns total number of literals across all sets
    EXPECT_EQ(total, bigint::BigInt(ps.Lit()));
}

TEST_F(BDDTest, ZDD_ElementFrequency_ConsistencyWithEnumerate) {
    // Verify against brute-force enumeration
    // {{1}, {2}, {1,2,3}}
    bddp s1 = bddchange(bddsingle, 1);
    bddp s2 = bddchange(bddsingle, 2);
    bddp s123 = bddchange(bddchange(bddchange(bddsingle, 1), 2), 3);
    bddp f = bddunion(bddunion(s1, s2), s123);
    auto freq = bddelmfreq(f);
    // freq[1] = 2 (in {1} and {1,2,3})
    // freq[2] = 2 (in {2} and {1,2,3})
    // freq[3] = 1 (in {1,2,3})
    ASSERT_GE(freq.size(), 4u);
    EXPECT_EQ(freq[1], bigint::BigInt(2));
    EXPECT_EQ(freq[2], bigint::BigInt(2));
    EXPECT_EQ(freq[3], bigint::BigInt(1));
}

// --- bddproduct / ZDD::product ---

TEST_F(BDDTest, ZDD_Product_EmptyLeft) {
    ZDD e(0);
    ZDD s = ZDD::singleton(1);
    EXPECT_TRUE(e.product(s).is_zero());
}

TEST_F(BDDTest, ZDD_Product_EmptyRight) {
    ZDD s = ZDD::singleton(1);
    ZDD e(0);
    EXPECT_TRUE(s.product(e).is_zero());
}

TEST_F(BDDTest, ZDD_Product_UnitLeft) {
    // {∅} × G = G
    ZDD u(1);
    ZDD g = ZDD::singleton(2);
    ZDD result = u.product(g);
    EXPECT_EQ(result.GetID(), g.GetID());
}

TEST_F(BDDTest, ZDD_Product_UnitRight) {
    // F × {∅} = F
    ZDD f = ZDD::singleton(1);
    ZDD u(1);
    ZDD result = f.product(u);
    EXPECT_EQ(result.GetID(), f.GetID());
}

TEST_F(BDDTest, ZDD_Product_SingletonPair) {
    // {{1}} × {{2}} = {{1,2}}
    ZDD f = ZDD::singleton(1);
    ZDD g = ZDD::singleton(2);
    ZDD result = f.product(g);
    auto sets = result.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    ASSERT_EQ(sets[0].size(), 2u);
    std::vector<bddvar> expected = {1, 2};
    EXPECT_EQ(sets[0], expected);
}

TEST_F(BDDTest, ZDD_Product_CardinalityIsMultiplicative) {
    // |F × G| = |F| * |G| for disjoint variable sets
    // F = power_set({1,2}) = 4 sets, G = power_set({3,4}) = 4 sets
    ZDD f = ZDD::power_set({1, 2});
    ZDD g = ZDD::power_set({3, 4});
    ZDD result = f.product(g);
    EXPECT_EQ(result.exact_count(), f.exact_count() * g.exact_count());
}

TEST_F(BDDTest, ZDD_Product_ConsistencyWithJoin) {
    // For disjoint variable sets, product should equal join
    ZDD f = ZDD::power_set({1, 2});
    ZDD g = ZDD::power_set({3, 4});
    ZDD prod = f.product(g);
    ZDD join = f * g;
    EXPECT_EQ(prod.GetID(), join.GetID());
}

TEST_F(BDDTest, ZDD_Product_WithEmptySet) {
    // F = {{}, {1}}, G = {{3}} → {{3}, {1,3}}
    ZDD f = ZDD(1) + ZDD::singleton(1);
    ZDD g = ZDD::singleton(3);
    ZDD result = f.product(g);
    auto sets = result.enumerate();
    ASSERT_EQ(sets.size(), 2u);
}

TEST_F(BDDTest, ZDD_Product_LargerExample) {
    // F = {{1}, {2}}, G = {{3}, {4}}
    // Result = {{1,3}, {1,4}, {2,3}, {2,4}}
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    ZDD g = ZDD::singleton(3) + ZDD::singleton(4);
    ZDD result = f.product(g);
    EXPECT_EQ(result.exact_count(), bigint::BigInt(4));
    auto sets = result.enumerate();
    ASSERT_EQ(sets.size(), 4u);
    // Each set should have exactly 2 elements
    for (auto& s : sets) {
        EXPECT_EQ(s.size(), 2u);
    }
}

TEST_F(BDDTest, ZDD_Product_Commutative) {
    ZDD f = ZDD::power_set({1, 2});
    ZDD g = ZDD::power_set({3, 4});
    EXPECT_EQ(f.product(g).GetID(), g.product(f).GetID());
}

TEST_F(BDDTest, ZDD_Product_FreeFunction) {
    bddp f = ZDD::power_set({1, 2}).GetID();
    bddp g = ZDD::power_set({3, 4}).GetID();
    bddp result = bddproduct(f, g);
    bddp join = bddjoin(f, g);
    EXPECT_EQ(result, join);
}

TEST_F(BDDTest, ZDD_Product_ComplementEdge) {
    // F = power_set({1,2}) uses complement edges internally.
    // G = {{3}} is a simple singleton.
    // Result should equal join for disjoint variables.
    ZDD f = ZDD::power_set({1, 2});
    ZDD g = ZDD::singleton(3);
    ZDD prod = f.product(g);
    ZDD join = f * g;
    EXPECT_EQ(prod.GetID(), join.GetID());
    // Verify sets: {{3}, {1,3}, {2,3}, {1,2,3}}
    EXPECT_EQ(prod.exact_count(), bigint::BigInt(4));
    auto sets = prod.enumerate();
    for (auto& s : sets) {
        bool has3 = false;
        for (bddvar v : s) {
            if (v == 3) has3 = true;
        }
        EXPECT_TRUE(has3);
    }
}

TEST_F(BDDTest, ZDD_Product_NonDisjointThrows) {
    // product() requires disjoint variable sets.
    // Overlapping variables should throw.
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    ZDD g = ZDD::singleton(2) + ZDD::singleton(3);
    EXPECT_THROW(f.product(g), std::invalid_argument);
}

// --- ZDD::average_size ---

TEST_F(BDDTest, ZDD_AverageSize_EmptyFamily) {
    ZDD e(0);
    EXPECT_DOUBLE_EQ(e.average_size(), 0.0);
}

TEST_F(BDDTest, ZDD_AverageSize_UnitFamily) {
    // {∅} → average size = 0
    ZDD u(1);
    EXPECT_DOUBLE_EQ(u.average_size(), 0.0);
}

TEST_F(BDDTest, ZDD_AverageSize_SingletonSet) {
    // {{1}} → average size = 1
    ZDD s = ZDD::singleton(1);
    EXPECT_DOUBLE_EQ(s.average_size(), 1.0);
}

TEST_F(BDDTest, ZDD_AverageSize_MixedSizes) {
    // {{1}, {1,2}, {1,2,3}} → sizes 1,2,3 → avg = 2.0
    ZDD s1 = ZDD::singleton(1);
    ZDD s12 = ZDD::single_set({1, 2});
    ZDD s123 = ZDD::single_set({1, 2, 3});
    ZDD f = s1 + s12 + s123;
    EXPECT_DOUBLE_EQ(f.average_size(), 2.0);
}

TEST_F(BDDTest, ZDD_AverageSize_PowerSet) {
    // power_set(3): 8 sets, total elements = 12, avg = 1.5
    ZDD ps = ZDD::power_set(3);
    EXPECT_DOUBLE_EQ(ps.average_size(), 1.5);
}

TEST_F(BDDTest, ZDD_AverageSize_WithEmpty) {
    // {{}, {1,2}} → sizes 0,2 → avg = 1.0
    ZDD e(1);
    ZDD s12 = ZDD::single_set({1, 2});
    ZDD f = e + s12;
    EXPECT_DOUBLE_EQ(f.average_size(), 1.0);
}

// --- ZDD::singleton ---

TEST_F(BDDTest, ZDD_Singleton) {
    ZDD s1 = ZDD::singleton(1);
    auto sets1 = s1.enumerate();
    ASSERT_EQ(sets1.size(), 1u);
    ASSERT_EQ(sets1[0].size(), 1u);
    EXPECT_EQ(sets1[0][0], 1u);

    ZDD s3 = ZDD::singleton(3);
    auto sets3 = s3.enumerate();
    ASSERT_EQ(sets3.size(), 1u);
    ASSERT_EQ(sets3[0].size(), 1u);
    EXPECT_EQ(sets3[0][0], 3u);

    // singleton should not contain the empty set
    EXPECT_FALSE(s1.has_empty());
}

// --- ZDD::single_set ---

TEST_F(BDDTest, ZDD_SingleSet_Empty) {
    // Empty vars → {{∅}} (unit family)
    ZDD f = ZDD::single_set({});
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_TRUE(sets[0].empty());
}

TEST_F(BDDTest, ZDD_SingleSet_OneVar) {
    // {1} → {{1}}, same as singleton
    ZDD f = ZDD::single_set({1});
    ZDD s = ZDD::singleton(1);
    EXPECT_EQ(f, s);
}

TEST_F(BDDTest, ZDD_SingleSet_MultipleVars) {
    // {1, 2, 3} → {{1, 2, 3}}
    ZDD f = ZDD::single_set({1, 2, 3});
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    std::vector<bddvar> expected = {1, 2, 3};
    EXPECT_EQ(sets[0], expected);
    EXPECT_FALSE(f.has_empty());
}

// --- ZDD::power_set ---

TEST_F(BDDTest, ZDD_PowerSet_Zero) {
    // power_set(0) → {{∅}}
    ZDD f = ZDD::power_set(0);
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_TRUE(sets[0].empty());
}

TEST_F(BDDTest, ZDD_PowerSet_One) {
    // power_set(1) → {{}, {1}}
    ZDD f = ZDD::power_set(1);
    auto sets = f.enumerate();
    EXPECT_EQ(sets.size(), 2u);
    EXPECT_TRUE(f.has_empty());
}

TEST_F(BDDTest, ZDD_PowerSet_Three) {
    // power_set(3) → 2^3 = 8 sets
    ZDD f = ZDD::power_set(3);
    EXPECT_EQ(f.count(), 8.0);
    EXPECT_TRUE(f.has_empty());
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 8u);
}

TEST_F(BDDTest, ZDD_PowerSet_Vec_Empty) {
    // power_set({}) → {{∅}}
    ZDD f = ZDD::power_set(std::vector<bddvar>{});
    EXPECT_EQ(f, ZDD::Single);
}

TEST_F(BDDTest, ZDD_PowerSet_Vec_SameAsN) {
    // power_set({1,2,3}) should equal power_set(3)
    ZDD a = ZDD::power_set(std::vector<bddvar>{1, 2, 3});
    ZDD b = ZDD::power_set(3);
    EXPECT_EQ(a, b);
}

TEST_F(BDDTest, ZDD_PowerSet_Vec_NonContiguous) {
    // power_set({2, 5}) → {{}, {2}, {5}, {2,5}} = 4 sets
    ZDD f = ZDD::power_set(std::vector<bddvar>{2, 5});
    EXPECT_EQ(f.count(), 4.0);
    EXPECT_TRUE(f.has_empty());
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 4u);
}

// --- ZDD::from_sets ---

TEST_F(BDDTest, ZDD_FromSets_Empty) {
    // No sets → empty family
    ZDD f = ZDD::from_sets({});
    EXPECT_EQ(f, ZDD::Empty);
}

TEST_F(BDDTest, ZDD_FromSets_SingleEmpty) {
    // {{}} → unit family
    ZDD f = ZDD::from_sets({{}});
    EXPECT_EQ(f, ZDD::Single);
}

TEST_F(BDDTest, ZDD_FromSets_Roundtrip) {
    // Build a ZDD, enumerate, reconstruct, compare
    ZDD orig = ZDD::power_set(3);
    auto sets = orig.enumerate();
    ZDD rebuilt = ZDD::from_sets(sets);
    EXPECT_EQ(rebuilt, orig);
}

TEST_F(BDDTest, ZDD_FromSets_Duplicates) {
    // Duplicate sets are merged
    ZDD f = ZDD::from_sets({{1, 2}, {1, 2}, {3}});
    EXPECT_EQ(f.count(), 2.0);  // {{1,2}, {3}}
}

// --- ZDD::combination ---

TEST_F(BDDTest, ZDD_Combination_K0) {
    // C(n, 0) = {{}} for any n
    ZDD f = ZDD::combination(5, 0);
    EXPECT_EQ(f, ZDD::Single);
}

TEST_F(BDDTest, ZDD_Combination_KGreaterThanN) {
    // C(3, 5) = empty
    ZDD f = ZDD::combination(3, 5);
    EXPECT_EQ(f, ZDD::Empty);
}

TEST_F(BDDTest, ZDD_Combination_KEqN) {
    // C(3, 3) = {{1,2,3}}
    ZDD f = ZDD::combination(3, 3);
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    ASSERT_EQ(sets[0].size(), 3u);
}

TEST_F(BDDTest, ZDD_Combination_3_2) {
    // C(3, 2) = {{1,2}, {1,3}, {2,3}} → 3 sets
    ZDD f = ZDD::combination(3, 2);
    EXPECT_EQ(f.count(), 3.0);
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 3u);
    for (const auto& s : sets) {
        EXPECT_EQ(s.size(), 2u);
    }
}

TEST_F(BDDTest, ZDD_Combination_5_3) {
    // C(5, 3) = 10 sets
    ZDD f = ZDD::combination(5, 3);
    EXPECT_EQ(f.count(), 10.0);
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 10u);
    for (const auto& s : sets) {
        EXPECT_EQ(s.size(), 3u);
    }
}

TEST_F(BDDTest, ZDD_Combination_K1) {
    // C(4, 1) = {{1}, {2}, {3}, {4}} → 4 singletons
    ZDD f = ZDD::combination(4, 1);
    EXPECT_EQ(f.count(), 4.0);
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 4u);
    for (const auto& s : sets) {
        EXPECT_EQ(s.size(), 1u);
    }
}

// --- ZDD::random_family ---

TEST_F(BDDTest, ZDD_RandomFamily_Zero) {
    // n=0: result is either {} or {{∅}}
    std::mt19937_64 rng(42);
    bool saw_empty = false, saw_single = false;
    for (int i = 0; i < 100; ++i) {
        ZDD f = ZDD::random_family(0, rng);
        if (f == ZDD::Empty) saw_empty = true;
        if (f == ZDD::Single) saw_single = true;
    }
    EXPECT_TRUE(saw_empty);
    EXPECT_TRUE(saw_single);
}

TEST_F(BDDTest, ZDD_RandomFamily_One) {
    // n=1: 2^(2^1) = 4 possible families: {}, {{1}}, {{∅}}, {{∅},{1}}
    std::mt19937_64 rng(123);
    std::set<bddp> seen;
    for (int i = 0; i < 200; ++i) {
        ZDD f = ZDD::random_family(1, rng);
        seen.insert(f.GetID());
    }
    EXPECT_EQ(seen.size(), 4u);
}

TEST_F(BDDTest, ZDD_RandomFamily_ValidSets) {
    // All sets in the family should only contain variables in {1,...,n}
    std::mt19937_64 rng(999);
    bddvar n = 3;
    for (int i = 0; i < 50; ++i) {
        ZDD f = ZDD::random_family(n, rng);
        auto sets = f.enumerate();
        for (const auto& s : sets) {
            for (bddvar v : s) {
                EXPECT_GE(v, 1u);
                EXPECT_LE(v, n);
            }
        }
    }
}

TEST_F(BDDTest, ZDD_RandomFamily_CardinalityRange) {
    // Cardinality should be between 0 and 2^n
    std::mt19937_64 rng(777);
    bddvar n = 3;
    for (int i = 0; i < 50; ++i) {
        ZDD f = ZDD::random_family(n, rng);
        double card = f.count();
        EXPECT_GE(card, 0.0);
        EXPECT_LE(card, 8.0);  // 2^3 = 8
    }
}

// --- ZDD::enumerate ---

TEST_F(BDDTest, ZDD_Enumerate_EmptyFamily) {
    ZDD f(0);  // empty family
    auto sets = f.enumerate();
    EXPECT_TRUE(sets.empty());
}

TEST_F(BDDTest, ZDD_Enumerate_SingleEmptySet) {
    ZDD f(1);  // { {} }
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_TRUE(sets[0].empty());
}

TEST_F(BDDTest, ZDD_Enumerate_SingleVariable) {
    bddvar v = bddnewvar();
    ZDD f = ZDD_ID(bddchange(bddsingle, v));  // { {v} }
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    ASSERT_EQ(sets[0].size(), 1u);
    EXPECT_EQ(sets[0][0], v);
}

TEST_F(BDDTest, ZDD_Enumerate_TwoSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD_ID(bddchange(bddsingle, v1));  // { {v1} }
    ZDD b = ZDD_ID(bddchange(bddsingle, v2));  // { {v2} }
    ZDD f = a + b;  // { {v1}, {v2} }
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 2u);
    // Sort for deterministic comparison
    std::sort(sets.begin(), sets.end());
    EXPECT_EQ(sets[0], std::vector<bddvar>({v1}));
    EXPECT_EQ(sets[1], std::vector<bddvar>({v2}));
}

TEST_F(BDDTest, ZDD_Enumerate_PowerSet) {
    // Power set of {v1, v2} = { {}, {v1}, {v2}, {v1,v2} }
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD s1 = ZDD_ID(bddchange(bddsingle, v1));
    ZDD s2 = ZDD_ID(bddchange(bddsingle, v2));
    ZDD s12 = s1 * s2;  // join: { {v1, v2} }
    ZDD f = ZDD(1) + s1 + s2 + s12;
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 4u);
    // Each inner vector is now sorted ascending; sort outer by lex order
    std::sort(sets.begin(), sets.end());
    EXPECT_EQ(sets[0], std::vector<bddvar>({}));
    EXPECT_EQ(sets[1], std::vector<bddvar>({v1}));
    EXPECT_EQ(sets[2], (std::vector<bddvar>{v1, v2}));
    EXPECT_EQ(sets[3], std::vector<bddvar>({v2}));
}

TEST_F(BDDTest, ZDD_Enumerate_ComplementEdge) {
    // ~ZDD::Empty = { {} }, ~ZDD::Single = empty
    ZDD f = ~ZDD(0);  // complement of empty = { {} }
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_TRUE(sets[0].empty());

    ZDD g = ~ZDD(1);  // complement of { {} } = empty
    auto sets2 = g.enumerate();
    EXPECT_TRUE(sets2.empty());
}

TEST_F(BDDTest, ZDD_Enumerate_ConsistentWithCard) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD s1 = ZDD_ID(bddchange(bddsingle, v1));
    ZDD s2 = ZDD_ID(bddchange(bddsingle, v2));
    ZDD s3 = ZDD_ID(bddchange(bddsingle, v3));
    ZDD f = s1 + s2 + s3 + (s1 * s2) + (s2 * s3);
    auto sets = f.enumerate();
    EXPECT_EQ(static_cast<uint64_t>(sets.size()), f.Card());
}

// --- ZDD::print_sets ---

TEST_F(BDDTest, ZDD_PrintSets_Null) {
    ZDD n(-1);
    std::ostringstream oss;
    n.print_sets(oss);
    EXPECT_EQ(oss.str(), "N");
}

TEST_F(BDDTest, ZDD_PrintSets_Empty) {
    ZDD e(0);
    std::ostringstream oss;
    e.print_sets(oss);
    EXPECT_EQ(oss.str(), "E");
}

TEST_F(BDDTest, ZDD_PrintSets_Single) {
    ZDD s(1);  // {∅}
    std::ostringstream oss;
    s.print_sets(oss);
    EXPECT_EQ(oss.str(), "{}");
}

TEST_F(BDDTest, ZDD_PrintSets_OneSingleton) {
    // {{1}}
    ZDD f = ZDD::singleton(1);
    std::ostringstream oss;
    f.print_sets(oss);
    EXPECT_EQ(oss.str(), "{1}");
}

TEST_F(BDDTest, ZDD_PrintSets_MultipleElements) {
    // {{3,2,1}} - single set with 3 elements
    ZDD f = ZDD::single_set({1, 2, 3});
    std::ostringstream oss;
    f.print_sets(oss);
    EXPECT_EQ(oss.str(), "{3,2,1}");
}

TEST_F(BDDTest, ZDD_PrintSets_PowerSet2) {
    // power_set(2) = {{},{1},{2},{2,1}} (lo-first order)
    ZDD f = ZDD::power_set(2);
    std::ostringstream oss;
    f.print_sets(oss);
    EXPECT_EQ(oss.str(), "{},{1},{2},{2,1}");
}

TEST_F(BDDTest, ZDD_PrintSets_CustomDelimiters) {
    ZDD f = ZDD::power_set(2);
    std::ostringstream oss;
    f.print_sets(oss, "};{", ",");
    EXPECT_EQ(oss.str(), "};{1};{2};{2,1");
}

TEST_F(BDDTest, ZDD_PrintSets_CustomDelimiters2) {
    ZDD f = ZDD::power_set(2);
    std::ostringstream oss;
    f.print_sets(oss, " | ", "-");
    EXPECT_EQ(oss.str(), " | 1 | 2 | 2-1");
}

TEST_F(BDDTest, ZDD_PrintSets_VarNameMap) {
    ZDD f = ZDD::single_set({1, 2, 3});
    std::vector<std::string> names = {"", "a", "b", "c"};
    std::ostringstream oss;
    f.print_sets(oss, ",", ",", names);
    EXPECT_EQ(oss.str(), "c,b,a");
}

TEST_F(BDDTest, ZDD_PrintSets_VarNameMap_Fallback) {
    ZDD f = ZDD::single_set({1, 2, 3});
    // var 3 has no mapping (out of range), falls back to number
    std::vector<std::string> names = {"", "x", "y"};
    std::ostringstream oss;
    f.print_sets(oss, ",", ",", names);
    EXPECT_EQ(oss.str(), "3,y,x");
}

TEST_F(BDDTest, ZDD_PrintSets_ComplementEdge) {
    // ~ZDD(0) = ~bddempty = {∅}
    ZDD f = ~ZDD(0);
    std::ostringstream oss;
    f.print_sets(oss);
    EXPECT_EQ(oss.str(), "{}");
}

TEST_F(BDDTest, ZDD_PrintSets_NullDelimiters) {
    ZDD n(-1);
    std::ostringstream oss;
    n.print_sets(oss, ";", ",");
    EXPECT_EQ(oss.str(), "N");
}

TEST_F(BDDTest, ZDD_PrintSets_EmptyDelimiters) {
    ZDD e(0);
    std::ostringstream oss;
    e.print_sets(oss, ";", ",");
    EXPECT_EQ(oss.str(), "E");
}

// --- ZDD::to_str ---

TEST_F(BDDTest, ZDD_ToStr_Null) {
    ZDD n(-1);
    EXPECT_EQ(n.to_str(), "N");
}

TEST_F(BDDTest, ZDD_ToStr_Empty) {
    ZDD e(0);
    EXPECT_EQ(e.to_str(), "E");
}

TEST_F(BDDTest, ZDD_ToStr_Single) {
    ZDD s(1);
    EXPECT_EQ(s.to_str(), "{}");
}

TEST_F(BDDTest, ZDD_ToStr_PowerSet2) {
    ZDD f = ZDD::power_set(2);
    EXPECT_EQ(f.to_str(), "{},{1},{2},{2,1}");
}

// ============================================================
// Parameterized mode tests: Recursive / Iterative / Auto must
// agree for all zdd_adv_count operations (batch 4 _iter additions).
// ============================================================

#define EXPECT_COUNT_MODE_EQ(expr_mode, expr_default)         \
    do {                                                      \
        bdd_cache_clear();                                    \
        auto _actual = (expr_mode);                           \
        bdd_cache_clear();                                    \
        auto _expected = (expr_default);                      \
        EXPECT_EQ(_actual, _expected);                        \
    } while (0)

class ZddAdvCountModeTest : public ::testing::TestWithParam<BddExecMode> {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

INSTANTIATE_TEST_SUITE_P(
    ExecModes,
    ZddAdvCountModeTest,
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

namespace {
struct CountFamilies {
    bddvar v1, v2, v3, v4;
    bddp s_v1, s_v2, s_v3, s_v4;
    bddp v1v2;          // {{v1,v2}}
    bddp v2v3;          // {{v2,v3}}
    bddp v3v4;          // {{v3,v4}}
    bddp v1v2v3;        // {{v1,v2,v3}}
    bddp F;             // {{v1},{v1,v2},{v2,v3}}
    bddp G;             // {{v1,v2},{v2,v3},{v3,v4}}
    bddp H;             // {{v1},{v2},{v1,v2},{v2,v3}}
    bddp pow3;          // powerset over v1,v2,v3 (contains ∅)
    bddp pow3_noempty;  // powerset minus ∅
    bddp comp_F;        // ~F (complement edge)
};

static CountFamilies make_count_families() {
    CountFamilies t;
    t.v1 = bddnewvar();
    t.v2 = bddnewvar();
    t.v3 = bddnewvar();
    t.v4 = bddnewvar();
    t.s_v1 = ZDD::getnode(t.v1, bddempty, bddsingle);
    t.s_v2 = ZDD::getnode(t.v2, bddempty, bddsingle);
    t.s_v3 = ZDD::getnode(t.v3, bddempty, bddsingle);
    t.s_v4 = ZDD::getnode(t.v4, bddempty, bddsingle);
    t.v1v2 = bddjoin(t.s_v1, t.s_v2);
    t.v2v3 = bddjoin(t.s_v2, t.s_v3);
    t.v3v4 = bddjoin(t.s_v3, t.s_v4);
    t.v1v2v3 = bddjoin(t.v1v2, t.s_v3);
    t.F = bddunion(bddunion(t.s_v1, t.v1v2), t.v2v3);
    t.G = bddunion(bddunion(t.v1v2, t.v2v3), t.v3v4);
    t.H = bddunion(bddunion(bddunion(t.s_v1, t.s_v2), t.v1v2), t.v2v3);
    // powerset over v1,v2,v3 using union composition
    t.pow3 = bddunion(bddunion(bddunion(bddsingle, t.s_v1),
                                bddunion(t.s_v2, t.v1v2)),
                      bddunion(bddunion(t.s_v3, bddjoin(t.s_v1, t.s_v3)),
                               bddunion(t.v2v3, t.v1v2v3)));
    t.pow3_noempty = bddsubtract(t.pow3, bddsingle);
    t.comp_F = bddnot(t.F);
    bdd_cache_clear();
    return t;
}
} // namespace

TEST_P(ZddAdvCountModeTest, Choose) {
    auto t = make_count_families();
    BddExecMode m = GetParam();
    // Terminal / degenerate
    EXPECT_EQ(bddchoose(bddempty, 0, m), bddempty);
    EXPECT_EQ(bddchoose(bddempty, 2, m), bddempty);
    EXPECT_EQ(bddchoose(bddsingle, 0, m), bddsingle);
    EXPECT_EQ(bddchoose(bddsingle, 1, m), bddempty);
    // Non-trivial
    for (int k = 0; k <= 3; ++k) {
        EXPECT_COUNT_MODE_EQ(bddchoose(t.F, k, m), bddchoose(t.F, k));
        EXPECT_COUNT_MODE_EQ(bddchoose(t.G, k, m), bddchoose(t.G, k));
        EXPECT_COUNT_MODE_EQ(bddchoose(t.H, k, m), bddchoose(t.H, k));
        EXPECT_COUNT_MODE_EQ(bddchoose(t.pow3, k, m), bddchoose(t.pow3, k));
        // ∅-containing complement case
        EXPECT_COUNT_MODE_EQ(bddchoose(t.comp_F, k, m), bddchoose(t.comp_F, k));
    }
}

TEST_P(ZddAdvCountModeTest, MinSize) {
    auto t = make_count_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddminsize(bddempty, m), 0u);
    EXPECT_EQ(bddminsize(bddsingle, m), 0u);
    EXPECT_COUNT_MODE_EQ(bddminsize(t.F, m), bddminsize(t.F));
    EXPECT_COUNT_MODE_EQ(bddminsize(t.G, m), bddminsize(t.G));
    EXPECT_COUNT_MODE_EQ(bddminsize(t.H, m), bddminsize(t.H));
    EXPECT_COUNT_MODE_EQ(bddminsize(t.pow3_noempty, m),
                         bddminsize(t.pow3_noempty));
    EXPECT_COUNT_MODE_EQ(bddminsize(t.v1v2v3, m), bddminsize(t.v1v2v3));
    EXPECT_COUNT_MODE_EQ(bddminsize(t.comp_F, m), bddminsize(t.comp_F));
}

TEST_P(ZddAdvCountModeTest, Count) {
    auto t = make_count_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddcount(bddempty, m), 0.0);
    EXPECT_EQ(bddcount(bddsingle, m), 1.0);
    EXPECT_EQ(bddcount(t.F, m), bddcount(t.F));
    EXPECT_EQ(bddcount(t.G, m), bddcount(t.G));
    EXPECT_EQ(bddcount(t.H, m), bddcount(t.H));
    EXPECT_EQ(bddcount(t.pow3, m), bddcount(t.pow3));
    EXPECT_EQ(bddcount(t.comp_F, m), bddcount(t.comp_F));
}

TEST_P(ZddAdvCountModeTest, ExactCount) {
    auto t = make_count_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddexactcount(bddempty, m), bigint::BigInt(0));
    EXPECT_EQ(bddexactcount(bddsingle, m), bigint::BigInt(1));
    EXPECT_EQ(bddexactcount(t.F, m), bddexactcount(t.F));
    EXPECT_EQ(bddexactcount(t.G, m), bddexactcount(t.G));
    EXPECT_EQ(bddexactcount(t.H, m), bddexactcount(t.H));
    EXPECT_EQ(bddexactcount(t.pow3, m), bddexactcount(t.pow3));
    EXPECT_EQ(bddexactcount(t.comp_F, m), bddexactcount(t.comp_F));
    // memo-variant overload
    CountMemoMap memo;
    EXPECT_EQ(bddexactcount(t.F, memo, m), bddexactcount(t.F));
    EXPECT_EQ(bddexactcount(t.G, memo, m), bddexactcount(t.G));
}

TEST_P(ZddAdvCountModeTest, Profile) {
    auto t = make_count_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddprofile(bddempty, m).size(), 0u);
    EXPECT_EQ(bddprofile(bddsingle, m).size(), 1u);
    EXPECT_EQ(bddprofile(t.F, m), bddprofile(t.F));
    EXPECT_EQ(bddprofile(t.G, m), bddprofile(t.G));
    EXPECT_EQ(bddprofile(t.H, m), bddprofile(t.H));
    EXPECT_EQ(bddprofile(t.pow3, m), bddprofile(t.pow3));
    EXPECT_EQ(bddprofile(t.comp_F, m), bddprofile(t.comp_F));
}

TEST_P(ZddAdvCountModeTest, ElmFreq) {
    auto t = make_count_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddelmfreq(bddempty, m).size(), 0u);
    EXPECT_EQ(bddelmfreq(bddsingle, m).size(), 0u);
    EXPECT_EQ(bddelmfreq(t.F, m), bddelmfreq(t.F));
    EXPECT_EQ(bddelmfreq(t.G, m), bddelmfreq(t.G));
    EXPECT_EQ(bddelmfreq(t.H, m), bddelmfreq(t.H));
    EXPECT_EQ(bddelmfreq(t.pow3, m), bddelmfreq(t.pow3));
    EXPECT_EQ(bddelmfreq(t.comp_F, m), bddelmfreq(t.comp_F));
}

TEST_P(ZddAdvCountModeTest, CountIntersec) {
    auto t = make_count_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddcountintersec(bddempty, t.F, m), bigint::BigInt(0));
    EXPECT_EQ(bddcountintersec(t.F, bddempty, m), bigint::BigInt(0));
    EXPECT_EQ(bddcountintersec(t.F, t.F, m), bddexactcount(t.F));
    EXPECT_EQ(bddcountintersec(t.F, t.G, m), bddcountintersec(t.F, t.G));
    EXPECT_EQ(bddcountintersec(t.G, t.F, m), bddcountintersec(t.F, t.G));
    EXPECT_EQ(bddcountintersec(t.F, t.H, m), bddcountintersec(t.F, t.H));
    EXPECT_EQ(bddcountintersec(t.pow3, t.pow3_noempty, m),
              bddcountintersec(t.pow3, t.pow3_noempty));
    EXPECT_EQ(bddcountintersec(t.pow3, t.comp_F, m),
              bddcountintersec(t.pow3, t.comp_F));
}

// Cross-validation test: mid-depth family exercising both lo/hi paths
// and complement interactions across all count ops.
TEST_P(ZddAdvCountModeTest, CrossValidationLinearChain) {
    const int n = 8;
    std::vector<bddvar> vars;
    for (int i = 0; i < n; ++i) vars.push_back(bddnewvar());

    bddp s = bddsingle;
    bddp fam = bddempty;
    for (int i = 0; i < n; ++i) {
        s = bddjoin(s, ZDD::getnode(vars[i], bddempty, bddsingle));
        fam = bddunion(fam, s);
    }
    bddp other = bddunion(ZDD::getnode(vars[2], bddempty, bddsingle),
                          bddjoin(ZDD::getnode(vars[5], bddempty, bddsingle),
                                  ZDD::getnode(vars[7], bddempty, bddsingle)));

    BddExecMode m = GetParam();
    EXPECT_EQ(bddcount(fam, m), bddcount(fam));
    EXPECT_EQ(bddexactcount(fam, m), bddexactcount(fam));
    EXPECT_EQ(bddminsize(fam, m), bddminsize(fam));
    EXPECT_EQ(bddprofile(fam, m), bddprofile(fam));
    EXPECT_EQ(bddelmfreq(fam, m), bddelmfreq(fam));
    EXPECT_EQ(bddcountintersec(fam, other, m),
              bddcountintersec(fam, other));
    for (int k = 0; k <= 4; ++k) {
        EXPECT_COUNT_MODE_EQ(bddchoose(fam, k, m), bddchoose(fam, k));
    }
}

