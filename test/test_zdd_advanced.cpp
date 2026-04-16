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

// ---- min_weight / max_weight tests ----

TEST_F(BDDTest, MinMaxWeight_EmptyFamilyThrows) {
    bddnewvar();
    std::vector<int> w = {0, 1};
    ZDD empty(0);  // bddempty
    EXPECT_THROW(empty.min_weight(w), std::invalid_argument);
    EXPECT_THROW(empty.max_weight(w), std::invalid_argument);
    EXPECT_THROW(empty.min_weight_set(w), std::invalid_argument);
    EXPECT_THROW(empty.max_weight_set(w), std::invalid_argument);
}

TEST_F(BDDTest, MinMaxWeight_OnlyEmptySet) {
    bddnewvar();
    std::vector<int> w = {0, 5};
    ZDD single(1);  // {∅}
    EXPECT_EQ(single.min_weight(w), 0);
    EXPECT_EQ(single.max_weight(w), 0);
    EXPECT_TRUE(single.min_weight_set(w).empty());
    EXPECT_TRUE(single.max_weight_set(w).empty());
}

TEST_F(BDDTest, MinMaxWeight_SingleVariable) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, 7};
    ZDD f = ZDD::singleton(v1);  // {{v1}}
    EXPECT_EQ(f.min_weight(w), 7);
    EXPECT_EQ(f.max_weight(w), 7);
    std::vector<bddvar> expected = {v1};
    EXPECT_EQ(f.min_weight_set(w), expected);
    EXPECT_EQ(f.max_weight_set(w), expected);
}

TEST_F(BDDTest, MinMaxWeight_TwoSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, 3, 8};
    // {{v1}, {v2}}
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);
    EXPECT_EQ(f.min_weight(w), 3);
    EXPECT_EQ(f.max_weight(w), 8);
    std::vector<bddvar> min_set = {v1};
    std::vector<bddvar> max_set = {v2};
    EXPECT_EQ(f.min_weight_set(w), min_set);
    EXPECT_EQ(f.max_weight_set(w), max_set);
}

TEST_F(BDDTest, MinMaxWeight_NegativeWeights) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, -5, 3};
    // {{v1}, {v2}, {v1, v2}}
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2)
          + ZDD::single_set({v1, v2});
    // weights: {v1}=-5, {v2}=3, {v1,v2}=-5+3=-2
    EXPECT_EQ(f.min_weight(w), -5);
    EXPECT_EQ(f.max_weight(w), 3);
    std::vector<bddvar> min_set = {v1};
    std::vector<bddvar> max_set = {v2};
    EXPECT_EQ(f.min_weight_set(w), min_set);
    EXPECT_EQ(f.max_weight_set(w), max_set);
}

TEST_F(BDDTest, MinMaxWeight_FamilyWithEmptySet) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, 4};
    // {{}, {v1}}
    ZDD f = ZDD(1) + ZDD::singleton(v1);
    // weights: {}=0, {v1}=4
    EXPECT_EQ(f.min_weight(w), 0);
    EXPECT_EQ(f.max_weight(w), 4);
    EXPECT_TRUE(f.min_weight_set(w).empty());
    std::vector<bddvar> max_set = {v1};
    EXPECT_EQ(f.max_weight_set(w), max_set);
}

TEST_F(BDDTest, MinMaxWeight_FamilyWithEmptySetNegativeWeight) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, -3};
    // {{}, {v1}}
    ZDD f = ZDD(1) + ZDD::singleton(v1);
    // weights: {}=0, {v1}=-3
    EXPECT_EQ(f.min_weight(w), -3);
    EXPECT_EQ(f.max_weight(w), 0);
    std::vector<bddvar> min_set = {v1};
    EXPECT_EQ(f.min_weight_set(w), min_set);
    EXPECT_TRUE(f.max_weight_set(w).empty());
}

TEST_F(BDDTest, MinMaxWeight_CompoundFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    std::vector<int> w = {0, 2, 5, 1};
    // {{v1}, {v2}, {v3}, {v1,v2}, {v1,v3}, {v2,v3}, {v1,v2,v3}}
    ZDD f = ZDD::power_set(3) - ZDD(1);  // power set minus empty set
    // weights: {v1}=2, {v2}=5, {v3}=1,
    //          {v1,v2}=7, {v1,v3}=3, {v2,v3}=6, {v1,v2,v3}=8
    EXPECT_EQ(f.min_weight(w), 1);
    EXPECT_EQ(f.max_weight(w), 8);
    std::vector<bddvar> min_set = {v3};
    std::vector<bddvar> max_set = {v1, v2, v3};
    EXPECT_EQ(f.min_weight_set(w), min_set);
    EXPECT_EQ(f.max_weight_set(w), max_set);
}

TEST_F(BDDTest, MinMaxWeight_SetWeightMatchesValue) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    std::vector<int> w = {0, -1, 4, 2};
    ZDD f = ZDD::power_set(3);
    // Verify: the weight of min_weight_set equals min_weight
    long long min_val = f.min_weight(w);
    std::vector<bddvar> min_set = f.min_weight_set(w);
    long long computed = 0;
    for (bddvar v : min_set) computed += w[v];
    EXPECT_EQ(computed, min_val);
    // Same for max
    long long max_val = f.max_weight(w);
    std::vector<bddvar> max_set = f.max_weight_set(w);
    computed = 0;
    for (bddvar v : max_set) computed += w[v];
    EXPECT_EQ(computed, max_val);
}

TEST_F(BDDTest, MinMaxWeight_CrossValidateWithEnumerate) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    std::vector<int> w = {0, 3, -2, 7};
    ZDD f = ZDD::power_set(3);
    // Enumerate all sets and compute min/max manually
    auto sets = f.enumerate();
    long long expected_min = LLONG_MAX;
    long long expected_max = LLONG_MIN;
    for (const auto& s : sets) {
        long long sum = 0;
        for (bddvar v : s) sum += w[v];
        if (sum < expected_min) expected_min = sum;
        if (sum > expected_max) expected_max = sum;
    }
    EXPECT_EQ(f.min_weight(w), expected_min);
    EXPECT_EQ(f.max_weight(w), expected_max);
}

TEST_F(BDDTest, MinMaxWeight_WeightsTooSmallThrows) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0};  // too small (size=1, need > 2)
    ZDD f = ZDD::singleton(v1);
    EXPECT_THROW(f.min_weight(w), std::invalid_argument);
    EXPECT_THROW(f.max_weight(w), std::invalid_argument);
}

TEST_F(BDDTest, MinMaxWeight_UnrelatedNewVarDoesNotBreak) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    std::vector<int> w = {0, 5};
    EXPECT_EQ(f.min_weight(w), 5);

    // Creating more variables should not break existing ZDD's weight ops.
    bddnewvar();
    bddnewvar();
    EXPECT_EQ(f.min_weight(w), 5);
    EXPECT_EQ(f.max_weight(w), 5);
    std::vector<bddvar> expected = {v1};
    EXPECT_EQ(f.min_weight_set(w), expected);
    EXPECT_EQ(f.max_weight_set(w), expected);
}

// --- CostBound (BkTrk-IntervalMemo) tests ---

TEST_F(BDDTest, CostBound_EmptyFamily) {
    bddnewvar();
    std::vector<int> w = {0, 1};
    ZDD f = ZDD::Empty;
    EXPECT_EQ(f.cost_bound_le(w, 100), ZDD::Empty);
    EXPECT_EQ(f.cost_bound_le(w, -1), ZDD::Empty);
}

TEST_F(BDDTest, CostBound_UnitFamily) {
    bddnewvar();
    std::vector<int> w = {0, 1};
    ZDD f = ZDD::Single;  // {∅}
    // Empty set has cost 0
    EXPECT_EQ(f.cost_bound_le(w, 0), ZDD::Single);
    EXPECT_EQ(f.cost_bound_le(w, 100), ZDD::Single);
    EXPECT_EQ(f.cost_bound_le(w, -1), ZDD::Empty);
}

TEST_F(BDDTest, CostBound_SingleVariable) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, 5};
    ZDD f = ZDD::singleton(v1);  // {{v1}}
    // cost({v1}) = 5
    EXPECT_EQ(f.cost_bound_le(w, 5), f);       // 5 <= 5: accepted
    EXPECT_EQ(f.cost_bound_le(w, 6), f);       // 5 <= 6: accepted
    EXPECT_EQ(f.cost_bound_le(w, 4), ZDD::Empty);  // 5 > 4: rejected
}

TEST_F(BDDTest, CostBound_MultipleSets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, 3, 8};
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);  // {{v1}, {v2}}
    // cost({v1}) = 3, cost({v2}) = 8

    ZDD h5 = f.cost_bound_le(w, 5);
    EXPECT_EQ(h5, ZDD::singleton(v1));  // only {v1} accepted

    ZDD h10 = f.cost_bound_le(w, 10);
    EXPECT_EQ(h10, f);  // both accepted

    ZDD h2 = f.cost_bound_le(w, 2);
    EXPECT_EQ(h2, ZDD::Empty);  // both rejected
}

TEST_F(BDDTest, CostBound_FamilyWithEmptySet) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, 4};
    // {{}, {v1}}
    ZDD f = ZDD::Single + ZDD::singleton(v1);
    // cost({}) = 0, cost({v1}) = 4

    ZDD h3 = f.cost_bound_le(w, 3);
    EXPECT_EQ(h3, ZDD::Single);  // only {} accepted

    ZDD h4 = f.cost_bound_le(w, 4);
    EXPECT_EQ(h4, f);  // both accepted

    ZDD hm1 = f.cost_bound_le(w, -1);
    EXPECT_EQ(hm1, ZDD::Empty);  // both rejected (0 > -1)
}

TEST_F(BDDTest, CostBound_NegativeWeights) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, -3, 5};
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);  // {{v1}, {v2}}
    // cost({v1}) = -3, cost({v2}) = 5

    ZDD h0 = f.cost_bound_le(w, 0);
    EXPECT_EQ(h0, ZDD::singleton(v1));  // -3 <= 0, 5 > 0

    ZDD hm4 = f.cost_bound_le(w, -4);
    EXPECT_EQ(hm4, ZDD::Empty);  // -3 > -4, 5 > -4
}

TEST_F(BDDTest, CostBound_CrossValidateWithEnumerate) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    std::vector<int> w = {0, 3, -2, 7};
    ZDD f = ZDD::power_set(3);  // all subsets of {v1, v2, v3}

    for (long long b = -5; b <= 12; ++b) {
        ZDD h = f.cost_bound_le(w, b);
        // Cross-validate: enumerate and filter manually
        auto all_sets = f.enumerate();
        auto bounded_sets = h.enumerate();

        // Collect expected sets
        std::vector<std::vector<bddvar>> expected;
        for (const auto& s : all_sets) {
            long long cost = 0;
            for (bddvar v : s) cost += w[v];
            if (cost <= b) expected.push_back(s);
        }
        EXPECT_EQ(bounded_sets.size(), expected.size())
            << "Mismatch at bound=" << b;
        // Sort for comparison
        auto sort_sets = [](std::vector<std::vector<bddvar>>& sets) {
            for (auto& s : sets) std::sort(s.begin(), s.end());
            std::sort(sets.begin(), sets.end());
        };
        sort_sets(bounded_sets);
        sort_sets(expected);
        EXPECT_EQ(bounded_sets, expected)
            << "Sets mismatch at bound=" << b;
    }
}

TEST_F(BDDTest, CostBound_MemoReuse) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    std::vector<int> w = {0, 10, 20, 30};
    ZDD f = ZDD::power_set(3);

    CostBoundMemo memo;
    // Call with different bounds, reusing the memo
    ZDD h5 = f.cost_bound_le(w, 5, memo);
    ZDD h15 = f.cost_bound_le(w, 15, memo);
    ZDD h25 = f.cost_bound_le(w, 25, memo);
    ZDD h35 = f.cost_bound_le(w, 35, memo);
    ZDD h100 = f.cost_bound_le(w, 100, memo);

    // Verify correctness of each
    EXPECT_EQ(h5.enumerate().size(), 1u);    // only {}
    EXPECT_EQ(h15.enumerate().size(), 2u);   // {}, {v1}
    EXPECT_EQ(h25.enumerate().size(), 3u);   // {}, {v1}, {v2}
    EXPECT_EQ(h35.enumerate().size(), 5u);   // {}, {v1}, {v2}, {v3}, {v1,v2}
    EXPECT_EQ(h100.enumerate().size(), 8u);  // all subsets
}

TEST_F(BDDTest, CostBound_SameIntervalHit) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, 10};
    ZDD f = ZDD::singleton(v1);  // {{v1}}, cost=10

    CostBoundMemo memo;
    ZDD h10 = f.cost_bound_le(w, 10, memo);
    ZDD h15 = f.cost_bound_le(w, 15, memo);  // same interval, should hit memo
    ZDD h100 = f.cost_bound_le(w, 100, memo);
    EXPECT_EQ(h10, f);
    EXPECT_EQ(h15, f);
    EXPECT_EQ(h100, f);
}

TEST_F(BDDTest, CostBound_ValidationErrors) {
    bddvar v1 = bddnewvar();
    std::vector<int> w_small = {0};  // too small
    ZDD f = ZDD::singleton(v1);
    EXPECT_THROW(f.cost_bound_le(w_small, 5), std::invalid_argument);
}

TEST_F(BDDTest, CostBound_ComplementEdge) {
    bddvar v1 = bddnewvar();
    bddnewvar();
    std::vector<int> w = {0, 3, 7};
    // all subsets of {v1,v2} except {{v1}}
    // = {{}, {v2}, {v1,v2}}
    ZDD f = ZDD::power_set(2) - ZDD::singleton(v1);
    // cost: {} = 0, {v2} = 7, {v1,v2} = 10

    ZDD h5 = f.cost_bound_le(w, 5);
    EXPECT_EQ(h5, ZDD::Single);  // only {}

    ZDD h7 = f.cost_bound_le(w, 7);
    auto sets7 = h7.enumerate();
    EXPECT_EQ(sets7.size(), 2u);  // {}, {v2}

    ZDD h10 = f.cost_bound_le(w, 10);
    EXPECT_EQ(h10, f);  // all accepted
}

TEST_F(BDDTest, CostBound_SimpleOverload) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, 3, 8};
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);

    // Simple overload (no memo parameter) should work
    ZDD h = f.cost_bound_le(w, 5);
    EXPECT_EQ(h, ZDD::singleton(v1));
}

TEST_F(BDDTest, CostBound_MemoSurvivesGC) {
    // Verify that memo invalidation works correctly after GC.
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddnewvar();
    std::vector<int> w = {0, 1, 2, 4, 8};
    ZDD f = ZDD::power_set(4);

    CostBoundMemo memo;
    ZDD h1 = f.cost_bound_le(w, 5, memo);
    auto sets1 = h1.enumerate();
    size_t count1 = sets1.size();

    // Force GC — memo entries become stale
    DDBase::gc();

    // Second call should produce the same correct result
    // (memo is invalidated and recomputed)
    ZDD h2 = f.cost_bound_le(w, 5, memo);
    auto sets2 = h2.enumerate();
    EXPECT_EQ(sets2.size(), count1);
    EXPECT_EQ(h2, h1);
}

TEST_F(BDDTest, CostBound_MemoRejectsDifferentWeights) {
    bddvar v1 = bddnewvar();
    std::vector<int> w1 = {0, 1};
    std::vector<int> w2 = {0, 100};
    ZDD f = ZDD::singleton(v1);

    CostBoundMemo memo;
    f.cost_bound_le(w1, 50, memo);
    // Using the same memo with different weights must throw
    EXPECT_THROW(f.cost_bound_le(w2, 50, memo), std::invalid_argument);
}

TEST_F(BDDTest, CostBound_ReorderedVariables) {
    // Create variables with non-default level ordering
    bddvar v1 = bddnewvar();               // var 1, level 1
    bddnewvar();                            // var 2, level 2
    bddvar v3 = DDBase::new_var(true);      // var 3, level 1 (pushes others up)
    // Now: level 3 = var 2, level 2 = var 1, level 1 = var 3

    std::vector<int> w = {0, 5, 10, 20};
    ZDD f = ZDD::single_set({v1, v3});      // {{1, 3}}, cost = 5 + 20 = 25

    ZDD h30 = f.cost_bound_le(w, 30);
    EXPECT_EQ(h30, f);                      // 25 <= 30: accepted

    ZDD h20 = f.cost_bound_le(w, 20);
    EXPECT_EQ(h20, ZDD::Empty);             // 25 > 20: rejected
}

// ==================== CostBoundGe ====================

TEST_F(BDDTest, CostBoundGe_EmptyFamily) {
    bddnewvar();
    std::vector<int> w = {0, 1};
    ZDD f = ZDD::Empty;
    EXPECT_EQ(f.cost_bound_ge(w, 0), ZDD::Empty);
    EXPECT_EQ(f.cost_bound_ge(w, -1), ZDD::Empty);
}

TEST_F(BDDTest, CostBoundGe_UnitFamily) {
    bddnewvar();
    std::vector<int> w = {0, 1};
    ZDD f = ZDD::Single;  // {∅}, cost(∅) = 0
    EXPECT_EQ(f.cost_bound_ge(w, 0), ZDD::Single);   // 0 >= 0: accepted
    EXPECT_EQ(f.cost_bound_ge(w, -1), ZDD::Single);  // 0 >= -1: accepted
    EXPECT_EQ(f.cost_bound_ge(w, 1), ZDD::Empty);    // 0 < 1: rejected
}

TEST_F(BDDTest, CostBoundGe_SingleVariable) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, 5};
    ZDD f = ZDD::singleton(v1);  // {{v1}}, cost = 5
    EXPECT_EQ(f.cost_bound_ge(w, 5), f);             // 5 >= 5: accepted
    EXPECT_EQ(f.cost_bound_ge(w, 4), f);             // 5 >= 4: accepted
    EXPECT_EQ(f.cost_bound_ge(w, 6), ZDD::Empty);    // 5 < 6: rejected
}

TEST_F(BDDTest, CostBoundGe_MultipleSets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, 3, 8};
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);  // {{v1},{v2}}, costs 3, 8

    // b=5: only {v2} (cost 8) has cost >= 5
    ZDD h5 = f.cost_bound_ge(w, 5);
    EXPECT_EQ(h5, ZDD::singleton(v2));

    // b=3: both have cost >= 3
    ZDD h3 = f.cost_bound_ge(w, 3);
    EXPECT_EQ(h3, f);

    // b=9: neither has cost >= 9
    ZDD h9 = f.cost_bound_ge(w, 9);
    EXPECT_EQ(h9, ZDD::Empty);
}

TEST_F(BDDTest, CostBoundGe_FamilyWithEmptySet) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, 4};
    ZDD f = ZDD::Single + ZDD::singleton(v1);  // {{},{v1}}, costs 0, 4

    // b=0: both have cost >= 0
    ZDD h0 = f.cost_bound_ge(w, 0);
    EXPECT_EQ(h0, f);

    // b=1: only {v1} (cost 4) has cost >= 1
    ZDD h1 = f.cost_bound_ge(w, 1);
    EXPECT_EQ(h1, ZDD::singleton(v1));

    // b=5: neither has cost >= 5
    ZDD h5 = f.cost_bound_ge(w, 5);
    EXPECT_EQ(h5, ZDD::Empty);
}

TEST_F(BDDTest, CostBoundGe_CrossValidateWithEnumerate) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    std::vector<int> w = {0, 2, 5, 10};
    ZDD f = ZDD::power_set(3);  // all subsets of {1,2,3}

    auto all_sets = f.enumerate();

    for (long long b = -1; b <= 18; ++b) {
        ZDD h = f.cost_bound_ge(w, b);
        auto result_sets = h.enumerate();

        // Filter manually: keep sets with cost >= b.
        std::vector<std::vector<bddvar>> expected;
        for (const auto& s : all_sets) {
            long long cost = 0;
            for (bddvar v : s) cost += w[v];
            if (cost >= b) expected.push_back(s);
        }

        std::vector<std::vector<bddvar>> sorted_result = result_sets;
        std::vector<std::vector<bddvar>> sorted_expected = expected;
        std::sort(sorted_result.begin(), sorted_result.end());
        std::sort(sorted_expected.begin(), sorted_expected.end());
        EXPECT_EQ(sorted_result, sorted_expected)
            << "Mismatch at b=" << b;
    }
}

TEST_F(BDDTest, CostBoundGe_MemoReuse) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    std::vector<int> w = {0, 5, 10, 20};
    ZDD f = ZDD::power_set(3);

    CostBoundMemo memo;
    ZDD h5 = f.cost_bound_ge(w, 5, memo);
    ZDD h15 = f.cost_bound_ge(w, 15, memo);
    ZDD h25 = f.cost_bound_ge(w, 25, memo);

    // Verify against non-memo version
    EXPECT_EQ(h5, f.cost_bound_ge(w, 5));
    EXPECT_EQ(h15, f.cost_bound_ge(w, 15));
    EXPECT_EQ(h25, f.cost_bound_ge(w, 25));
}

TEST_F(BDDTest, CostBoundGe_SimpleOverload) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, 5};
    ZDD f = ZDD::singleton(v1);
    ZDD h = f.cost_bound_ge(w, 5);
    EXPECT_EQ(h, f);
}

// ==================== CostBoundEq ====================

TEST_F(BDDTest, CostBoundEq_EmptyFamily) {
    bddnewvar();
    std::vector<int> w = {0, 1};
    ZDD f = ZDD::Empty;
    EXPECT_EQ(f.cost_bound_eq(w, 0), ZDD::Empty);
    EXPECT_EQ(f.cost_bound_eq(w, 1), ZDD::Empty);
}

TEST_F(BDDTest, CostBoundEq_UnitFamily) {
    bddnewvar();
    std::vector<int> w = {0, 1};
    ZDD f = ZDD::Single;  // {∅}, cost(∅) = 0
    EXPECT_EQ(f.cost_bound_eq(w, 0), ZDD::Single);  // 0 == 0: accepted
    EXPECT_EQ(f.cost_bound_eq(w, 1), ZDD::Empty);   // 0 != 1: rejected
}

TEST_F(BDDTest, CostBoundEq_SingleVariable) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, 5};
    ZDD f = ZDD::singleton(v1);  // {{v1}}, cost = 5
    EXPECT_EQ(f.cost_bound_eq(w, 5), f);             // exact match
    EXPECT_EQ(f.cost_bound_eq(w, 4), ZDD::Empty);    // too low
    EXPECT_EQ(f.cost_bound_eq(w, 6), ZDD::Empty);    // too high
}

TEST_F(BDDTest, CostBoundEq_MultipleSets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, 3, 8};
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);  // costs 3, 8

    EXPECT_EQ(f.cost_bound_eq(w, 3), ZDD::singleton(v1));
    EXPECT_EQ(f.cost_bound_eq(w, 8), ZDD::singleton(v2));
    EXPECT_EQ(f.cost_bound_eq(w, 5), ZDD::Empty);
}

TEST_F(BDDTest, CostBoundEq_CrossValidateWithEnumerate) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    std::vector<int> w = {0, 2, 5, 10};
    ZDD f = ZDD::power_set(3);

    auto all_sets = f.enumerate();

    for (long long b = -1; b <= 18; ++b) {
        ZDD h = f.cost_bound_eq(w, b);
        auto result_sets = h.enumerate();

        std::vector<std::vector<bddvar>> expected;
        for (const auto& s : all_sets) {
            long long cost = 0;
            for (bddvar v : s) cost += w[v];
            if (cost == b) expected.push_back(s);
        }

        std::sort(result_sets.begin(), result_sets.end());
        std::sort(expected.begin(), expected.end());
        EXPECT_EQ(result_sets, expected) << "Mismatch at b=" << b;
    }
}

TEST_F(BDDTest, CostBoundEq_MemoReuse) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    std::vector<int> w = {0, 5, 10, 20};
    ZDD f = ZDD::power_set(3);

    CostBoundMemo memo;
    ZDD h5 = f.cost_bound_eq(w, 5, memo);
    ZDD h10 = f.cost_bound_eq(w, 10, memo);
    ZDD h15 = f.cost_bound_eq(w, 15, memo);

    EXPECT_EQ(h5, f.cost_bound_eq(w, 5));
    EXPECT_EQ(h10, f.cost_bound_eq(w, 10));
    EXPECT_EQ(h15, f.cost_bound_eq(w, 15));
}

TEST_F(BDDTest, CostBoundEq_SimpleOverload) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, 5};
    ZDD f = ZDD::singleton(v1);
    EXPECT_EQ(f.cost_bound_eq(w, 5), f);
}

// ---- size_le / size_ge tests ----

TEST_F(BDDTest, SizeLe_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    EXPECT_EQ(empty.size_le(0), ZDD::Empty);
    EXPECT_EQ(empty.size_le(5), ZDD::Empty);
}

TEST_F(BDDTest, SizeLe_UnitFamily) {
    bddnewvar();
    ZDD unit(1);  // {∅}
    EXPECT_EQ(unit.size_le(0), unit);   // |∅| = 0 <= 0
    EXPECT_EQ(unit.size_le(5), unit);
}

TEST_F(BDDTest, SizeLe_SingleVariable) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);  // {{v1}}
    EXPECT_EQ(f.size_le(0), ZDD::Empty);  // |{v1}| = 1 > 0
    EXPECT_EQ(f.size_le(1), f);           // |{v1}| = 1 <= 1
    EXPECT_EQ(f.size_le(5), f);
}

TEST_F(BDDTest, SizeLe_MultipleSets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // f = {{v1}, {v1, v2}, {v1, v2, v3}}
    ZDD f = ZDD::singleton(v1)
          + ZDD::single_set({v1, v2})
          + ZDD::single_set({v1, v2, v3});

    EXPECT_EQ(f.size_le(0), ZDD::Empty);
    EXPECT_EQ(f.size_le(1), ZDD::singleton(v1));
    EXPECT_EQ(f.size_le(2), ZDD::singleton(v1) + ZDD::single_set({v1, v2}));
    EXPECT_EQ(f.size_le(3), f);
    EXPECT_EQ(f.size_le(10), f);
}

TEST_F(BDDTest, SizeLe_WithEmptySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // f = {∅, {v1}, {v1, v2}}
    ZDD f = ZDD::Single + ZDD::singleton(v1) + ZDD::single_set({v1, v2});

    EXPECT_EQ(f.size_le(0), ZDD::Single);  // only ∅
    EXPECT_EQ(f.size_le(1), ZDD::Single + ZDD::singleton(v1));
    EXPECT_EQ(f.size_le(2), f);
}

TEST_F(BDDTest, SizeLe_CrossValidateWithChoose) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::power_set(3);  // all subsets of {v1, v2, v3}

    // size_le(k) should equal union of choose(0) + choose(1) + ... + choose(k)
    for (int k = 0; k <= 3; ++k) {
        ZDD expected(0);
        for (int j = 0; j <= k; ++j) {
            expected = expected + f.choose(j);
        }
        EXPECT_EQ(f.size_le(k), expected);
    }
}

TEST_F(BDDTest, SizeGe_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    EXPECT_EQ(empty.size_ge(0), ZDD::Empty);
    EXPECT_EQ(empty.size_ge(5), ZDD::Empty);
}

TEST_F(BDDTest, SizeGe_UnitFamily) {
    bddnewvar();
    ZDD unit(1);  // {∅}
    EXPECT_EQ(unit.size_ge(0), unit);     // |∅| = 0 >= 0
    EXPECT_EQ(unit.size_ge(1), ZDD::Empty);  // |∅| = 0 < 1
}

TEST_F(BDDTest, SizeGe_SingleVariable) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);  // {{v1}}
    EXPECT_EQ(f.size_ge(0), f);   // |{v1}| = 1 >= 0
    EXPECT_EQ(f.size_ge(1), f);   // |{v1}| = 1 >= 1
    EXPECT_EQ(f.size_ge(2), ZDD::Empty);
}

TEST_F(BDDTest, SizeGe_MultipleSets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // f = {{v1}, {v1, v2}, {v1, v2, v3}}
    ZDD f = ZDD::singleton(v1)
          + ZDD::single_set({v1, v2})
          + ZDD::single_set({v1, v2, v3});

    EXPECT_EQ(f.size_ge(0), f);
    EXPECT_EQ(f.size_ge(1), f);
    EXPECT_EQ(f.size_ge(2), ZDD::single_set({v1, v2}) + ZDD::single_set({v1, v2, v3}));
    EXPECT_EQ(f.size_ge(3), ZDD::single_set({v1, v2, v3}));
    EXPECT_EQ(f.size_ge(4), ZDD::Empty);
}

TEST_F(BDDTest, SizeGe_CrossValidateWithChoose) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::power_set(3);  // all subsets of {v1, v2, v3}

    // size_ge(k) should equal union of choose(k) + choose(k+1) + ... + choose(3)
    for (int k = 0; k <= 4; ++k) {
        ZDD expected(0);
        for (int j = k; j <= 3; ++j) {
            expected = expected + f.choose(j);
        }
        EXPECT_EQ(f.size_ge(k), expected);
    }
}

TEST_F(BDDTest, SizeLeGe_Complement) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::power_set(3);

    // size_le(k) + size_ge(k+1) == f for all k
    for (int k = 0; k <= 3; ++k) {
        EXPECT_EQ(f.size_le(k) + f.size_ge(k + 1), f);
    }
}

// ---- get_sum / bddweightsum tests ----

TEST_F(BDDTest, WeightSum_EmptyFamily) {
    bddnewvar();
    std::vector<int> w = {0, 1};
    ZDD empty(0);  // bddempty
    EXPECT_EQ(empty.get_sum(w), bigint::BigInt(0));
}

TEST_F(BDDTest, WeightSum_OnlyEmptySet) {
    bddnewvar();
    std::vector<int> w = {0, 5};
    ZDD single(1);  // {∅}
    EXPECT_EQ(single.get_sum(w), bigint::BigInt(0));
}

TEST_F(BDDTest, WeightSum_Singleton) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, 10};
    ZDD f = ZDD::singleton(v1);  // {{v1}}
    // sum = 10
    EXPECT_EQ(f.get_sum(w), bigint::BigInt(10));
}

TEST_F(BDDTest, WeightSum_MultipleSets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    std::vector<int> w = {0, 10, 20, 30};
    // {{v1, v2}, {v2, v3}}
    ZDD f = ZDD::single_set({v1, v2}) + ZDD::single_set({v2, v3});
    // {v1,v2}: 10+20=30, {v2,v3}: 20+30=50, total=80
    EXPECT_EQ(f.get_sum(w), bigint::BigInt(80));
}

TEST_F(BDDTest, WeightSum_FamilyWithEmptySet) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0, 7};
    // {{}, {v1}}
    ZDD f = ZDD(1) + ZDD::singleton(v1);
    // {}: 0, {v1}: 7, total=7
    EXPECT_EQ(f.get_sum(w), bigint::BigInt(7));
}

TEST_F(BDDTest, WeightSum_NegativeWeights) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, -5, 3};
    // {{v1}, {v2}, {v1, v2}}
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2)
          + ZDD::single_set({v1, v2});
    // {v1}: -5, {v2}: 3, {v1,v2}: -5+3=-2, total=-4
    EXPECT_EQ(f.get_sum(w), bigint::BigInt(-4));
}

TEST_F(BDDTest, WeightSum_ComplementInvariance) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, 3, 5};
    // {{v1}, {v2}}
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);
    // complement adds/removes ∅ which has weight 0
    ZDD fc = ~f;  // {{}, {v1, v2}}
    // {}: 0, {v1,v2}: 3+5=8, total=8
    // Original: {v1}: 3, {v2}: 5, total=8
    EXPECT_EQ(f.get_sum(w), bigint::BigInt(8));
    EXPECT_EQ(fc.get_sum(w), bigint::BigInt(8));
}

TEST_F(BDDTest, WeightSum_PowerSet) {
    const int N = 10;
    for (int i = 0; i < N; ++i) bddnewvar();
    std::vector<int> w(N + 1, 0);
    for (int i = 1; i <= N; ++i) w[i] = 1;

    ZDD ps = ZDD::power_set(N);
    // Each variable appears in 2^(N-1) sets
    // Total = N * 1 * 2^(N-1) = 10 * 512 = 5120
    bigint::BigInt expected = bigint::BigInt(N) * (bigint::BigInt(1) << (N - 1));
    EXPECT_EQ(ps.get_sum(w), expected);
}

TEST_F(BDDTest, WeightSum_PowerSetLarge) {
    const int N = 65;
    for (int i = 0; i < N; ++i) bddnewvar();
    std::vector<int> w(N + 1, 0);
    for (int i = 1; i <= N; ++i) w[i] = 1;

    ZDD ps = ZDD::power_set(N);
    // Total = N * 2^(N-1) = 65 * 2^64
    bigint::BigInt expected = bigint::BigInt(N) * (bigint::BigInt(1) << (N - 1));
    EXPECT_EQ(ps.get_sum(w), expected);
}

TEST_F(BDDTest, WeightSum_FreeFunctionNull) {
    std::vector<int> w = {0};
    EXPECT_THROW(bddweightsum(bddnull, w), std::invalid_argument);
}

TEST_F(BDDTest, WeightSum_WeightsTooSmallThrows) {
    bddvar v1 = bddnewvar();
    std::vector<int> w = {0};  // too small: need > v1
    ZDD f = ZDD::singleton(v1);
    EXPECT_THROW(f.get_sum(w), std::invalid_argument);
}

// ---------------------------------------------------------------
// ZDD Rank tests
// ---------------------------------------------------------------

TEST_F(BDDTest, ZDD_Rank_EmptyFamily) {
    // rank in empty family always returns -1
    ZDD f = ZDD::Empty;
    EXPECT_EQ(f.rank({}), -1);
    bddvar v1 = bddnewvar();
    EXPECT_EQ(f.rank({v1}), -1);
}

TEST_F(BDDTest, ZDD_Rank_SingleFamily) {
    // {∅} — only the empty set
    ZDD f = ZDD::Single;
    EXPECT_EQ(f.rank({}), 0);
    bddvar v1 = bddnewvar();
    EXPECT_EQ(f.rank({v1}), -1);
}

TEST_F(BDDTest, ZDD_Rank_SingleVariable) {
    // {{1}} — only {1}
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    EXPECT_EQ(f.rank({v1}), 0);
    EXPECT_EQ(f.rank({}), -1);
}

TEST_F(BDDTest, ZDD_Rank_EmptySetFirst) {
    // {{1}, ∅} — ∅ gets index 0, {1} gets index 1
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::Single;
    EXPECT_EQ(f.rank({}), 0);
    EXPECT_EQ(f.rank({v1}), 1);
}

TEST_F(BDDTest, ZDD_Rank_NotInFamily) {
    // {{1}} — {2} is not a member
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    EXPECT_EQ(f.rank({v2}), -1);
    EXPECT_EQ(f.rank({v1, v2}), -1);
}

TEST_F(BDDTest, ZDD_Rank_DuplicateInput) {
    // Duplicate and unsorted input should be normalized
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::single_set({v1, v2});
    int64_t r = f.rank({v2, v1, v2, v1});  // duplicates and unsorted
    EXPECT_EQ(r, f.rank({v1, v2}));
}

TEST_F(BDDTest, ZDD_Rank_PowerSet3_RoundtripWithEnumerate) {
    // power_set(3) has 8 sets. Verify rank matches enumerate order.
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::power_set(3);

    auto sets = f.enumerate();
    // For each set from enumerate, rank should give consecutive indices
    // that match the ZDD structure-based ordering.
    std::vector<int64_t> ranks;
    for (const auto& s : sets) {
        int64_t r = f.rank(s);
        EXPECT_GE(r, 0);
        ranks.push_back(r);
    }
    // ranks should be a permutation of 0..7
    std::sort(ranks.begin(), ranks.end());
    for (int64_t i = 0; i < 8; ++i) {
        EXPECT_EQ(ranks[i], i);
    }
}

TEST_F(BDDTest, ZDD_ExactRank_Basic) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::Single;
    EXPECT_EQ(f.exact_rank({}), bigint::BigInt(0));
    EXPECT_EQ(f.exact_rank({v1}), bigint::BigInt(1));
    EXPECT_EQ(f.exact_rank({999}), bigint::BigInt(-1));
}

TEST_F(BDDTest, ZDD_ExactRank_WithMemo) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::power_set(2);
    ZddCountMemo memo(f);
    bigint::BigInt r1 = f.exact_rank({v1}, memo);
    bigint::BigInt r2 = f.exact_rank({v2}, memo);
    // Different sets should have different ranks
    EXPECT_NE(r1, r2);
    EXPECT_GE(r1, bigint::BigInt(0));
    EXPECT_GE(r2, bigint::BigInt(0));
}

TEST_F(BDDTest, ZDD_Rank_NullThrows) {
    ZDD f = ZDD::Null;
    EXPECT_THROW(f.rank({}), std::invalid_argument);
    EXPECT_THROW(f.exact_rank({}), std::invalid_argument);
}

// ---------------------------------------------------------------
// ZDD Unrank tests
// ---------------------------------------------------------------

TEST_F(BDDTest, ZDD_Unrank_EmptyFamily) {
    ZDD f = ZDD::Empty;
    EXPECT_THROW(f.unrank(0), std::out_of_range);
}

TEST_F(BDDTest, ZDD_Unrank_SingleFamily) {
    ZDD f = ZDD::Single;
    auto s = f.unrank(0);
    EXPECT_TRUE(s.empty());  // ∅
    EXPECT_THROW(f.unrank(1), std::out_of_range);
}

TEST_F(BDDTest, ZDD_Unrank_SingleVariable) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    auto s = f.unrank(0);
    EXPECT_EQ(s, std::vector<bddvar>({v1}));
    EXPECT_THROW(f.unrank(1), std::out_of_range);
}

TEST_F(BDDTest, ZDD_Unrank_EmptySetFirst) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::Single;
    auto s0 = f.unrank(0);
    auto s1 = f.unrank(1);
    EXPECT_TRUE(s0.empty());  // ∅ at index 0
    EXPECT_EQ(s1, std::vector<bddvar>({v1}));
}

TEST_F(BDDTest, ZDD_Unrank_NegativeThrows) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    EXPECT_THROW(f.unrank(-1), std::out_of_range);
}

TEST_F(BDDTest, ZDD_Unrank_OutOfRangeThrows) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    EXPECT_THROW(f.unrank(1), std::out_of_range);
    EXPECT_THROW(f.unrank(100), std::out_of_range);
}

TEST_F(BDDTest, ZDD_Unrank_OutputSorted) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::single_set({v1, v2, v3});
    auto s = f.unrank(0);
    EXPECT_TRUE(std::is_sorted(s.begin(), s.end()));
}

TEST_F(BDDTest, ZDD_ExactUnrank_Basic) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::Single;
    auto s0 = f.exact_unrank(bigint::BigInt(0));
    auto s1 = f.exact_unrank(bigint::BigInt(1));
    EXPECT_TRUE(s0.empty());
    EXPECT_EQ(s1, std::vector<bddvar>({v1}));
}

TEST_F(BDDTest, ZDD_ExactUnrank_WithMemo) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::power_set(2);
    ZddCountMemo memo(f);
    auto s0 = f.exact_unrank(bigint::BigInt(0), memo);
    auto s1 = f.exact_unrank(bigint::BigInt(1), memo);
    // Should produce different sets
    EXPECT_NE(s0, s1);
}

TEST_F(BDDTest, ZDD_ExactUnrank_OutOfRange) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    EXPECT_THROW(f.exact_unrank(bigint::BigInt(-1)), std::out_of_range);
    EXPECT_THROW(f.exact_unrank(bigint::BigInt(1)), std::out_of_range);
}

TEST_F(BDDTest, ZDD_Unrank_NullThrows) {
    ZDD f = ZDD::Null;
    EXPECT_THROW(f.unrank(0), std::invalid_argument);
    EXPECT_THROW(f.exact_unrank(bigint::BigInt(0)), std::invalid_argument);
}

// ---------------------------------------------------------------
// ZDD Rank/Unrank roundtrip tests
// ---------------------------------------------------------------

TEST_F(BDDTest, ZDD_RankUnrank_Roundtrip) {
    // rank(unrank(i)) == i and unrank(rank(s)) == s
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::power_set(3);
    uint64_t count = bddcard(f.id());

    for (uint64_t i = 0; i < count; ++i) {
        auto s = f.unrank(static_cast<int64_t>(i));
        int64_t r = f.rank(s);
        EXPECT_EQ(r, static_cast<int64_t>(i))
            << "rank(unrank(" << i << ")) != " << i;
    }
}

TEST_F(BDDTest, ZDD_RankUnrank_RoundtripEnumerate) {
    // unrank should produce the same sets as enumerate (in rank order)
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::power_set(3);
    auto enumerated = f.enumerate();
    uint64_t count = bddcard(f.id());

    // Collect all sets via unrank
    std::vector<std::vector<bddvar>> unranked;
    for (uint64_t i = 0; i < count; ++i) {
        unranked.push_back(f.unrank(static_cast<int64_t>(i)));
    }

    // enumerated and unranked should contain the same sets (possibly different order)
    auto sorted_enum = enumerated;
    auto sorted_unrank = unranked;
    std::sort(sorted_enum.begin(), sorted_enum.end());
    std::sort(sorted_unrank.begin(), sorted_unrank.end());
    EXPECT_EQ(sorted_enum, sorted_unrank);
}

TEST_F(BDDTest, ZDD_RankUnrank_ExactRoundtrip) {
    // BigInt version roundtrip
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::power_set(2);
    ZddCountMemo memo(f);
    bigint::BigInt count = f.exact_count(memo);

    for (int i = 0; i < 4; ++i) {
        bigint::BigInt bi(i);
        auto s = f.exact_unrank(bi, memo);
        bigint::BigInt r = f.exact_rank(s, memo);
        EXPECT_EQ(r, bi);
    }
}

TEST_F(BDDTest, ZDD_RankUnrank_ComplementEdge) {
    // Build a family that uses complement edges
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // ~singleton = all sets except {v1} → complement has complement edge
    ZDD f = ~ZDD::singleton(v1);
    uint64_t count = bddcard(f.id());
    EXPECT_GT(count, 0u);

    for (uint64_t i = 0; i < count; ++i) {
        auto s = f.unrank(static_cast<int64_t>(i));
        int64_t r = f.rank(s);
        EXPECT_EQ(r, static_cast<int64_t>(i));
    }
}

TEST_F(BDDTest, ZDD_RankUnrank_Combination) {
    // C(4,2) = 6 sets
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddvar v4 = bddnewvar();
    ZDD f = ZDD::combination(4, 2);
    uint64_t count = bddcard(f.id());
    EXPECT_EQ(count, 6u);

    for (uint64_t i = 0; i < count; ++i) {
        auto s = f.unrank(static_cast<int64_t>(i));
        EXPECT_EQ(s.size(), 2u);  // all sets have exactly 2 elements
        int64_t r = f.rank(s);
        EXPECT_EQ(r, static_cast<int64_t>(i));
    }
}

TEST_F(BDDTest, ZDD_Rank_FreeFunctions) {
    // Test C-level free functions
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::power_set(2);
    bddp fp = f.id();

    // bddrank
    EXPECT_GE(bddrank(fp, {v1}), 0);
    EXPECT_EQ(bddrank(fp, {999}), -1);

    // bddexactrank
    EXPECT_GE(bddexactrank(fp, {v1}), bigint::BigInt(0));

    // bddunrank
    auto s = bddunrank(fp, 0);
    EXPECT_EQ(bddrank(fp, s), 0);

    // bddexactunrank
    auto s2 = bddexactunrank(fp, bigint::BigInt(0));
    EXPECT_EQ(bddexactrank(fp, s2), bigint::BigInt(0));
}

// ---------------------------------------------------------------
// get_k_sets tests
// ---------------------------------------------------------------

TEST_F(BDDTest, ZDD_GetKSets_EmptyFamily) {
    ZDD f = ZDD::Empty;
    EXPECT_EQ(f.get_k_sets(0), ZDD::Empty);
    EXPECT_EQ(f.get_k_sets(1), ZDD::Empty);
    EXPECT_EQ(f.get_k_sets(100), ZDD::Empty);
}

TEST_F(BDDTest, ZDD_GetKSets_SingleFamily) {
    ZDD f = ZDD::Single;  // {∅}
    EXPECT_EQ(f.get_k_sets(0), ZDD::Empty);
    EXPECT_EQ(f.get_k_sets(1), ZDD::Single);
    EXPECT_EQ(f.get_k_sets(2), ZDD::Single);
}

TEST_F(BDDTest, ZDD_GetKSets_KZero) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::power_set(2);
    EXPECT_EQ(f.get_k_sets(0), ZDD::Empty);
}

TEST_F(BDDTest, ZDD_GetKSets_KGreaterThanCard) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::power_set(2);
    // power_set(2) has 4 sets, k=10 should return entire family
    EXPECT_EQ(f.get_k_sets(10), f);
    EXPECT_EQ(f.get_k_sets(4), f);
}

TEST_F(BDDTest, ZDD_GetKSets_PowerSet3_Incremental) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::power_set(3);  // 8 sets
    auto all_sets = f.enumerate();
    ASSERT_EQ(all_sets.size(), 8u);

    for (size_t k = 0; k <= 8; ++k) {
        ZDD g = f.get_k_sets(static_cast<int64_t>(k));
        auto g_sets = g.enumerate();
        ASSERT_EQ(g_sets.size(), k) << "k=" << k;

        // Verify these are the first k sets in structure order
        for (size_t i = 0; i < k; ++i) {
            auto expected = f.unrank(static_cast<int64_t>(i));
            // Find this set in g_sets
            bool found = false;
            for (const auto& s : g_sets) {
                if (s == expected) { found = true; break; }
            }
            EXPECT_TRUE(found) << "k=" << k << " i=" << i;
        }
    }
}

TEST_F(BDDTest, ZDD_GetKSets_MatchesUnrank) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::power_set(3);

    for (int64_t k = 0; k <= 8; ++k) {
        ZDD g = f.get_k_sets(k);
        auto g_sets = g.enumerate();
        // Build expected set from unrank
        std::vector<std::vector<bddvar>> expected;
        for (int64_t i = 0; i < k; ++i) {
            expected.push_back(f.unrank(i));
        }
        // Sort both for comparison
        std::sort(g_sets.begin(), g_sets.end());
        std::sort(expected.begin(), expected.end());
        EXPECT_EQ(g_sets, expected) << "k=" << k;
    }
}

TEST_F(BDDTest, ZDD_GetKSets_ComplementEdge) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // ~singleton(v1) toggles ∅ membership: {{v1}} → {∅, {v1}}
    ZDD f = ~ZDD::singleton(v1);
    uint64_t count = f.Card();
    ASSERT_EQ(count, 2u);

    for (int64_t k = 0; k <= static_cast<int64_t>(count); ++k) {
        ZDD g = f.get_k_sets(k);
        auto g_sets = g.enumerate();
        ASSERT_EQ(g_sets.size(), static_cast<size_t>(k)) << "k=" << k;

        std::vector<std::vector<bddvar>> expected;
        for (int64_t i = 0; i < k; ++i) {
            expected.push_back(f.unrank(i));
        }
        std::sort(g_sets.begin(), g_sets.end());
        std::sort(expected.begin(), expected.end());
        EXPECT_EQ(g_sets, expected) << "k=" << k;
    }
}

TEST_F(BDDTest, ZDD_GetKSets_ComplementComplex) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // Build a family with complement edges at multiple levels
    ZDD a = ZDD::singleton(v1);
    ZDD b = ZDD::singleton(v2);
    ZDD c = ZDD::singleton(v3);
    ZDD f = ~(a * b + c);  // complement of join + union
    uint64_t count = f.Card();
    ASSERT_GT(count, 0u);

    for (uint64_t k = 0; k <= count; ++k) {
        ZDD g = f.get_k_sets(static_cast<int64_t>(k));
        auto g_sets = g.enumerate();
        ASSERT_EQ(g_sets.size(), k) << "k=" << k;
    }
}

TEST_F(BDDTest, ZDD_GetKSets_BigInt) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::power_set(2);

    for (int k = 0; k <= 4; ++k) {
        ZDD g1 = f.get_k_sets(static_cast<int64_t>(k));
        ZDD g2 = f.get_k_sets(bigint::BigInt(k));
        EXPECT_EQ(g1, g2) << "k=" << k;
    }
}

TEST_F(BDDTest, ZDD_GetKSets_WithMemo) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::power_set(3);
    ZddCountMemo memo(f);

    for (int k = 0; k <= 8; ++k) {
        ZDD g = f.get_k_sets(bigint::BigInt(k), memo);
        auto g_sets = g.enumerate();
        EXPECT_EQ(g_sets.size(), static_cast<size_t>(k)) << "k=" << k;
    }
}

TEST_F(BDDTest, ZDD_GetKSets_NegativeThrows) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    EXPECT_THROW(f.get_k_sets(-1), std::invalid_argument);
    EXPECT_THROW(f.get_k_sets(bigint::BigInt(-1)), std::invalid_argument);
}

TEST_F(BDDTest, ZDD_GetKSets_FreeFunctions) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::power_set(2);
    bddp fp = f.id();

    bddp g = bddgetksets(fp, static_cast<int64_t>(2));
    EXPECT_EQ(bddcard(g), 2u);

    bddp g2 = bddgetksets(fp, bigint::BigInt(3));
    EXPECT_EQ(bddcard(g2), 3u);

    CountMemoMap memo;
    bddp g3 = bddgetksets(fp, bigint::BigInt(1), memo);
    EXPECT_EQ(bddcard(g3), 1u);
}

TEST_F(BDDTest, ZDD_GetKSets_Combination) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddvar v4 = bddnewvar();
    bddvar v5 = bddnewvar();
    (void)v1; (void)v2; (void)v3; (void)v4; (void)v5;
    ZDD f = ZDD::combination(5, 2);  // C(5,2) = 10 sets
    ASSERT_EQ(f.Card(), 10u);

    for (int64_t k = 0; k <= 10; ++k) {
        ZDD g = f.get_k_sets(k);
        auto g_sets = g.enumerate();
        ASSERT_EQ(g_sets.size(), static_cast<size_t>(k)) << "k=" << k;

        // All sets should have exactly 2 elements
        for (const auto& s : g_sets) {
            EXPECT_EQ(s.size(), 2u);
        }

        // Verify matches unrank
        std::vector<std::vector<bddvar>> expected;
        for (int64_t i = 0; i < k; ++i) {
            expected.push_back(f.unrank(i));
        }
        std::sort(g_sets.begin(), g_sets.end());
        std::sort(expected.begin(), expected.end());
        EXPECT_EQ(g_sets, expected) << "k=" << k;
    }
}

// ---------------------------------------------------------------
// get_k_lightest / get_k_heaviest tests
// ---------------------------------------------------------------

TEST_F(BDDTest, ZDD_GetKLightest_EmptyFamily) {
    std::vector<int> w = {0};
    ZDD f = ZDD::Empty;
    EXPECT_EQ(f.get_k_lightest(0, w), ZDD::Empty);
    EXPECT_EQ(f.get_k_lightest(1, w), ZDD::Empty);
}

TEST_F(BDDTest, ZDD_GetKLightest_SingleFamily) {
    std::vector<int> w = {0};
    ZDD f = ZDD::Single;
    EXPECT_EQ(f.get_k_lightest(0, w), ZDD::Empty);
    EXPECT_EQ(f.get_k_lightest(1, w), ZDD::Single);
    EXPECT_EQ(f.get_k_lightest(2, w), ZDD::Single);
}

TEST_F(BDDTest, ZDD_GetKLightest_KGreaterThanCard) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    (void)v1; (void)v2;
    std::vector<int> w = {0, 3, 5};
    ZDD f = ZDD::power_set(2);
    EXPECT_EQ(f.get_k_lightest(10, w), f);
    EXPECT_EQ(f.get_k_lightest(4, w), f);
}

TEST_F(BDDTest, ZDD_GetKLightest_KnownFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, 3, 5};

    ZDD s1 = ZDD::singleton(v1);     // cost 3
    ZDD s2 = ZDD::singleton(v2);     // cost 5
    ZDD s12 = s1 * s2;               // cost 8
    ZDD f = s1 + s2 + s12;

    // lightest 1: {{v1}} (cost 3)
    ZDD g1 = f.get_k_lightest(1, w);
    auto g1_sets = g1.enumerate();
    ASSERT_EQ(g1_sets.size(), 1u);
    EXPECT_EQ(g1_sets[0], std::vector<bddvar>({v1}));

    // lightest 2: {{v1}, {v2}} (costs 3, 5)
    ZDD g2 = f.get_k_lightest(2, w);
    auto g2_sets = g2.enumerate();
    ASSERT_EQ(g2_sets.size(), 2u);
    std::sort(g2_sets.begin(), g2_sets.end());
    std::vector<std::vector<bddvar>> expected2 = {{v1}, {v2}};
    std::sort(expected2.begin(), expected2.end());
    EXPECT_EQ(g2_sets, expected2);

    // lightest 3: entire family
    EXPECT_EQ(f.get_k_lightest(3, w), f);
}

TEST_F(BDDTest, ZDD_GetKLightest_StrictBoundary) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    (void)v1; (void)v2; (void)v3;
    std::vector<int> w = {0, 5, 5, 10};

    // family: {{v1}, {v2}, {v3}}  costs: 5, 5, 10
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2) + ZDD::singleton(v3);

    // k=1: boundary tier = cost 5, which has 2 sets
    EXPECT_EQ(f.get_k_lightest(1, w, 0).Card(), 1u);
    EXPECT_EQ(f.get_k_lightest(1, w, -1).Card(), 0u);
    EXPECT_EQ(f.get_k_lightest(1, w, 1).Card(), 2u);
}

TEST_F(BDDTest, ZDD_GetKLightest_WithEmptySet) {
    bddvar v1 = bddnewvar();
    (void)v1;
    std::vector<int> w = {0, 10};

    ZDD f = ZDD::Single + ZDD::singleton(v1);
    ASSERT_EQ(f.Card(), 2u);

    ZDD g = f.get_k_lightest(1, w);
    EXPECT_TRUE(g.has_empty());
    EXPECT_EQ(g.Card(), 1u);
}

TEST_F(BDDTest, ZDD_GetKLightest_NegativeWeights) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, -5, 3};

    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);

    ZDD g = f.get_k_lightest(1, w);
    auto g_sets = g.enumerate();
    ASSERT_EQ(g_sets.size(), 1u);
    EXPECT_EQ(g_sets[0], std::vector<bddvar>({v1}));
}

TEST_F(BDDTest, ZDD_GetKLightest_AllSameWeight) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    (void)v1; (void)v2;
    std::vector<int> w = {0, 5, 5};

    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);

    EXPECT_EQ(f.get_k_lightest(1, w, 0).Card(), 1u);
    EXPECT_EQ(f.get_k_lightest(1, w, -1).Card(), 0u);
    EXPECT_EQ(f.get_k_lightest(1, w, 1).Card(), 2u);
}

TEST_F(BDDTest, ZDD_GetKHeaviest_Basic) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, 3, 5};

    ZDD s1 = ZDD::singleton(v1);
    ZDD s2 = ZDD::singleton(v2);
    ZDD s12 = s1 * s2;
    ZDD f = s1 + s2 + s12;

    // heaviest 1: {{v1,v2}} (cost 8)
    ZDD g1 = f.get_k_heaviest(1, w);
    auto g1_sets = g1.enumerate();
    ASSERT_EQ(g1_sets.size(), 1u);
    std::vector<bddvar> expected_set = {v1, v2};
    std::sort(expected_set.begin(), expected_set.end());
    std::sort(g1_sets[0].begin(), g1_sets[0].end());
    EXPECT_EQ(g1_sets[0], expected_set);

    // heaviest 2: {{v2}, {v1,v2}} (costs 5, 8)
    ZDD g2 = f.get_k_heaviest(2, w);
    EXPECT_EQ(g2.Card(), 2u);
}

TEST_F(BDDTest, ZDD_GetKHeaviest_CrossValidate) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    (void)v1; (void)v2; (void)v3;
    std::vector<int> w = {0, 2, 3, 7};

    ZDD f = ZDD::power_set(3);
    uint64_t total = f.Card();

    for (uint64_t k = 0; k <= total; ++k) {
        ZDD heaviest = f.get_k_heaviest(static_cast<int64_t>(k), w, 0);
        ZDD lightest_comp = f.get_k_lightest(
            static_cast<int64_t>(total - k), w, 0);
        ZDD expected = f - lightest_comp;
        EXPECT_EQ(heaviest, expected) << "k=" << k;
    }
}

TEST_F(BDDTest, ZDD_GetKLightest_BigInt) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    (void)v1; (void)v2;
    std::vector<int> w = {0, 3, 5};

    ZDD f = ZDD::power_set(2);

    for (int k = 0; k <= 4; ++k) {
        ZDD g1 = f.get_k_lightest(static_cast<int64_t>(k), w);
        ZDD g2 = f.get_k_lightest(bigint::BigInt(k), w);
        EXPECT_EQ(g1, g2) << "k=" << k;
    }
}

TEST_F(BDDTest, ZDD_GetKLightest_NegativeKThrows) {
    bddvar v1 = bddnewvar();
    (void)v1;
    std::vector<int> w = {0, 1};
    ZDD f = ZDD::singleton(v1);
    EXPECT_THROW(f.get_k_lightest(-1, w), std::invalid_argument);
    EXPECT_THROW(f.get_k_heaviest(-1, w), std::invalid_argument);
}

TEST_F(BDDTest, ZDD_GetKLightest_FreeFunctions) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, 3, 5};

    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);
    bddp fp = f.id();

    bddp g = bddgetklightest(fp, static_cast<int64_t>(1), w, 0);
    EXPECT_EQ(bddcard(g), 1u);

    bddp h = bddgetkheaviest(fp, bigint::BigInt(1), w, 0);
    EXPECT_EQ(bddcard(h), 1u);
}

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

// ============================================================
// to_cnf tests
// ============================================================

TEST_F(BDDTest, ToCnf_SingleClause) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1, v2}} → one clause: (v2 | v1) — higher level first
    ZDD f = ZDD::single_set({v1, v2});
    EXPECT_EQ(f.to_cnf(), "(" + std::to_string(v2) + " | " +
                           std::to_string(v1) + ")");
}

TEST_F(BDDTest, ToCnf_MultipleClauses) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // {{v1, v3}, {v2}} → (v1 | v3) & (v2)
    ZDD f = ZDD::single_set({v1, v3}) + ZDD::singleton(v2);
    std::string result = f.to_cnf();
    // Both clauses must appear, connected by &
    EXPECT_NE(result.find(" & "), std::string::npos);
    EXPECT_NE(result.find("|"), std::string::npos);
}

TEST_F(BDDTest, ToCnf_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    EXPECT_EQ(empty.to_cnf(), "T");
}

TEST_F(BDDTest, ToCnf_Null) {
    EXPECT_EQ(ZDD::Null.to_cnf(), "N");
}

TEST_F(BDDTest, ToCnf_EmptySetClause) {
    bddvar v1 = bddnewvar();
    // {∅, {v1}} → two clauses: () & (v1)
    ZDD f = ZDD::Single + ZDD::singleton(v1);
    std::string result = f.to_cnf();
    EXPECT_NE(result.find("()"), std::string::npos);
}

TEST_F(BDDTest, ToCnf_SingleVariable) {
    bddvar v1 = bddnewvar();
    // {{v1}} → (v1)
    ZDD f = ZDD::singleton(v1);
    EXPECT_EQ(f.to_cnf(), "(" + std::to_string(v1) + ")");
}

TEST_F(BDDTest, ToCnf_UnitFamily) {
    bddnewvar();
    // {∅} → empty clause
    EXPECT_EQ(ZDD::Single.to_cnf(), "()");
}

TEST_F(BDDTest, ToCnf_VarNameMap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::single_set({v1, v2});
    std::vector<std::string> names(v2 + 1);
    names[v1] = "a";
    names[v2] = "b";
    // Higher level (v2) appears first in ZDD traversal
    EXPECT_EQ(f.to_cnf(names), "(b | a)");
}

// ============================================================
// to_dnf tests
// ============================================================

TEST_F(BDDTest, ToDnf_SingleTerm) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1, v2}} → one term: (v2 & v1) — higher level first
    ZDD f = ZDD::single_set({v1, v2});
    EXPECT_EQ(f.to_dnf(), "(" + std::to_string(v2) + " & " +
                           std::to_string(v1) + ")");
}

TEST_F(BDDTest, ToDnf_MultipleTerms) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // {{v1, v3}, {v2}} → (v1 & v3) | (v2)
    ZDD f = ZDD::single_set({v1, v3}) + ZDD::singleton(v2);
    std::string result = f.to_dnf();
    EXPECT_NE(result.find(" | "), std::string::npos);
    EXPECT_NE(result.find("&"), std::string::npos);
}

TEST_F(BDDTest, ToDnf_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    EXPECT_EQ(empty.to_dnf(), "F");
}

TEST_F(BDDTest, ToDnf_Null) {
    EXPECT_EQ(ZDD::Null.to_dnf(), "N");
}

TEST_F(BDDTest, ToDnf_EmptySetTerm) {
    bddvar v1 = bddnewvar();
    // {∅, {v1}} → two terms: () | (v1)
    ZDD f = ZDD::Single + ZDD::singleton(v1);
    std::string result = f.to_dnf();
    EXPECT_NE(result.find("()"), std::string::npos);
}

TEST_F(BDDTest, ToDnf_SingleVariable) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    EXPECT_EQ(f.to_dnf(), "(" + std::to_string(v1) + ")");
}

TEST_F(BDDTest, ToDnf_UnitFamily) {
    bddnewvar();
    EXPECT_EQ(ZDD::Single.to_dnf(), "()");
}

TEST_F(BDDTest, ToDnf_VarNameMap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::single_set({v1, v2});
    std::vector<std::string> names(v2 + 1);
    names[v1] = "x";
    names[v2] = "y";
    // Higher level (v2) appears first
    EXPECT_EQ(f.to_dnf(names), "(y & x)");
}

TEST_F(BDDTest, ToCnfDnf_DifferentOutput) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}, {v2}} → CNF: (v1) & (v2), DNF: (v1) | (v2)
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);
    std::string cnf = f.to_cnf();
    std::string dnf = f.to_dnf();
    EXPECT_NE(cnf, dnf);
    EXPECT_NE(cnf.find(" & "), std::string::npos);
    EXPECT_NE(dnf.find(" | "), std::string::npos);
}

// ============================================================
// random_subset tests
// ============================================================

TEST_F(BDDTest, RandomSubset_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    std::mt19937_64 rng(42);
    ZDD result = empty.random_subset(0.5, rng);
    EXPECT_TRUE(result.is_zero());
}

TEST_F(BDDTest, RandomSubset_ProbZero) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::power_set(2);
    std::mt19937_64 rng(42);
    ZDD result = f.random_subset(0.0, rng);
    EXPECT_TRUE(result.is_zero());
}

TEST_F(BDDTest, RandomSubset_ProbOne) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::power_set(2);
    std::mt19937_64 rng(42);
    ZDD result = f.random_subset(1.0, rng);
    EXPECT_EQ(result, f);
}

TEST_F(BDDTest, RandomSubset_InvalidProb) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    std::mt19937_64 rng(42);
    EXPECT_THROW(f.random_subset(-0.1, rng), std::invalid_argument);
    EXPECT_THROW(f.random_subset(1.1, rng), std::invalid_argument);
}

TEST_F(BDDTest, RandomSubset_NullZDD) {
    ZDD f = ZDD::Null;
    std::mt19937_64 rng(42);
    EXPECT_THROW(f.random_subset(0.5, rng), std::invalid_argument);
}

TEST_F(BDDTest, RandomSubset_UnitFamily) {
    bddnewvar();
    ZDD f = ZDD::Single;  // {∅}
    std::mt19937_64 rng(42);
    // Run several times: should get either Empty or Single
    bool got_empty = false, got_single = false;
    for (int i = 0; i < 100; ++i) {
        ZDD result = f.random_subset(0.5, rng);
        if (result.is_zero()) got_empty = true;
        else if (result == ZDD::Single) got_single = true;
        else FAIL() << "Unexpected result from random_subset on {∅}";
    }
    EXPECT_TRUE(got_empty);
    EXPECT_TRUE(got_single);
}

TEST_F(BDDTest, RandomSubset_IsSubfamily) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(3);
    std::mt19937_64 rng(42);
    for (int i = 0; i < 20; ++i) {
        ZDD result = f.random_subset(0.5, rng);
        EXPECT_TRUE(result.is_subset_family(f));
    }
}

TEST_F(BDDTest, RandomSubset_SizeRange) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(4);  // 16 sets
    std::mt19937_64 rng(42);
    for (int i = 0; i < 20; ++i) {
        ZDD result = f.random_subset(0.5, rng);
        double cnt = result.count();
        EXPECT_GE(cnt, 0.0);
        EXPECT_LE(cnt, 16.0);
    }
}

TEST_F(BDDTest, RandomSubset_StatisticalMean) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(5);  // 32 sets
    double p = 0.3;
    double expected_mean = p * 32.0;
    int trials = 1000;
    double total = 0.0;
    std::mt19937_64 rng(123);
    for (int i = 0; i < trials; ++i) {
        ZDD result = f.random_subset(p, rng);
        total += result.count();
    }
    double mean = total / trials;
    // Allow ±3 standard deviations: stdev ≈ sqrt(n*p*(1-p)) ≈ sqrt(32*0.3*0.7) ≈ 2.59
    // stdev of mean ≈ 2.59 / sqrt(1000) ≈ 0.082
    // 5-sigma margin: 0.41
    EXPECT_NEAR(mean, expected_mean, 1.0);
}

TEST_F(BDDTest, RandomSubset_DifferentSeeds) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(4);  // 16 sets
    std::mt19937_64 rng1(1);
    std::mt19937_64 rng2(2);
    // With 16 sets and p=0.5, probability of identical results is (C(16,k)/2^16)^2
    // summed over k, which is very small. Run a few times to be safe.
    bool found_different = false;
    for (int i = 0; i < 10; ++i) {
        ZDD r1 = f.random_subset(0.5, rng1);
        ZDD r2 = f.random_subset(0.5, rng2);
        if (r1 != r2) {
            found_different = true;
            break;
        }
    }
    EXPECT_TRUE(found_different);
}

TEST_F(BDDTest, ZDD_SampleK_DeepZDD) {
    // Create a deep ZDD with 200 variables to exercise BDD_RecurGuard
    const int n = 200;
    for (int i = 0; i < n; ++i) bddnewvar();
    // Build a chain: {{1,2,...,n}} - a single large set
    ZDD f = ZDD::single_set({});
    std::vector<bddvar> vars;
    for (int i = 1; i <= n; ++i) vars.push_back(i);
    f = ZDD::single_set(vars);
    // Add a few smaller sets
    f = f + ZDD::single_set({1, 2, 3});
    f = f + ZDD::single_set({100, 150, 200});
    f = f + ZDD(1);  // add empty set

    ZddCountMemo memo(f);
    std::mt19937_64 rng(42);

    ZDD result = f.sample_k(2, rng, memo);
    EXPECT_EQ(result.exact_count(), bigint::BigInt(2));
    EXPECT_TRUE(result.is_subset_family(f));
}

TEST_F(BDDTest, RandomSubset_DeepZDD) {
    // Create a deep ZDD with 200 variables to exercise BDD_RecurGuard
    const int n = 200;
    for (int i = 0; i < n; ++i) bddnewvar();
    ZDD f = ZDD::single_set({});
    std::vector<bddvar> vars;
    for (int i = 1; i <= n; ++i) vars.push_back(i);
    f = ZDD::single_set(vars);
    f = f + ZDD::single_set({1, 2, 3});
    f = f + ZDD::single_set({50, 100, 150});
    f = f + ZDD(1);  // add empty set

    std::mt19937_64 rng(42);

    ZDD result = f.random_subset(0.5, rng);
    EXPECT_TRUE(result.is_subset_family(f));
    // With p=0.5 and 4 sets, expected ~2 sets
    EXPECT_LE(result.count(), 4.0);
}

// ============================================================
// hamming_distance tests
// ============================================================

TEST_F(BDDTest, HammingDistance_BothEmpty) {
    ZDD a(0), b(0);
    EXPECT_EQ(a.hamming_distance(b), bigint::BigInt(0));
}

TEST_F(BDDTest, HammingDistance_Identical) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD ps = ZDD::power_set(3);
    EXPECT_EQ(ps.hamming_distance(ps), bigint::BigInt(0));
}

TEST_F(BDDTest, HammingDistance_Disjoint) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD::singleton(v1);  // {{v1}}
    ZDD b = ZDD::singleton(v2);  // {{v2}}
    // |a △ b| = |a| + |b| - 2*0 = 2
    EXPECT_EQ(a.hamming_distance(b), bigint::BigInt(2));
}

TEST_F(BDDTest, HammingDistance_PartialOverlap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // a = {{v1}, {v2}}, b = {{v2}, {v1,v2}}
    // |a ∩ b| = 1 ({{v2}}), |a △ b| = 2 + 2 - 2*1 = 2
    ZDD a = ZDD::singleton(v1) + ZDD::singleton(v2);
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v1, v2});
    EXPECT_EQ(a.hamming_distance(b), bigint::BigInt(2));
}

TEST_F(BDDTest, HammingDistance_Symmetric) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD::singleton(v1);
    ZDD b = ZDD::singleton(v2);
    EXPECT_EQ(a.hamming_distance(b), b.hamming_distance(a));
}

TEST_F(BDDTest, HammingDistance_SubsetCase) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD a = ZDD::combination(3, 2);  // 3 sets
    ZDD b = ZDD::power_set(3);       // 8 sets
    // a ⊆ b → |a △ b| = |b| - |a| = 5
    EXPECT_EQ(a.hamming_distance(b), bigint::BigInt(5));
}

TEST_F(BDDTest, HammingDistance_ConsistentWithSymDiff) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD a = ZDD::combination(3, 1);
    ZDD b = ZDD::combination(3, 2);
    EXPECT_EQ(a.hamming_distance(b), (a ^ b).exact_count());
}

TEST_F(BDDTest, HammingDistance_OneEmpty) {
    bddnewvar();
    bddnewvar();
    ZDD a = ZDD::power_set(2);  // 4 sets
    ZDD b(0);                    // empty
    EXPECT_EQ(a.hamming_distance(b), bigint::BigInt(4));
}

TEST_F(BDDTest, HammingDistance_Null) {
    bddnewvar();
    ZDD ps = ZDD::power_set(1);
    EXPECT_EQ(ZDD::Null.hamming_distance(ps), bigint::BigInt(0));
    EXPECT_EQ(ps.hamming_distance(ZDD::Null), bigint::BigInt(0));
}

TEST_F(BDDTest, HammingDistance_FreeFunction) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddp ps = ZDD::power_set(3).GetID();
    bddp c32 = ZDD::combination(3, 2).GetID();
    EXPECT_EQ(bddhammingdist(ps, c32), bigint::BigInt(5));
}

// ============================================================
// overlap_coefficient tests
// ============================================================

TEST_F(BDDTest, OverlapCoefficient_BothEmpty) {
    ZDD a(0), b(0);
    EXPECT_DOUBLE_EQ(a.overlap_coefficient(b), 1.0);
}

TEST_F(BDDTest, OverlapCoefficient_Identical) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD ps = ZDD::power_set(3);
    EXPECT_DOUBLE_EQ(ps.overlap_coefficient(ps), 1.0);
}

TEST_F(BDDTest, OverlapCoefficient_Disjoint) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD::singleton(v1);
    ZDD b = ZDD::singleton(v2);
    EXPECT_DOUBLE_EQ(a.overlap_coefficient(b), 0.0);
}

TEST_F(BDDTest, OverlapCoefficient_SubsetCase) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // a ⊆ b → O(a,b) = |a| / min(|a|,|b|) = |a|/|a| = 1.0
    ZDD a = ZDD::combination(3, 2);  // 3 sets
    ZDD b = ZDD::power_set(3);       // 8 sets
    EXPECT_DOUBLE_EQ(a.overlap_coefficient(b), 1.0);
}

TEST_F(BDDTest, OverlapCoefficient_PartialOverlap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // a = {{v1}, {v2}}, b = {{v2}, {v1,v2}}
    // |a ∩ b| = 1, min(|a|,|b|) = 2 → O = 0.5
    ZDD a = ZDD::singleton(v1) + ZDD::singleton(v2);
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v1, v2});
    EXPECT_DOUBLE_EQ(a.overlap_coefficient(b), 0.5);
}

TEST_F(BDDTest, OverlapCoefficient_Symmetric) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD a = ZDD::singleton(v1) + ZDD::single_set({v2, v3});
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v1, v3});
    EXPECT_DOUBLE_EQ(a.overlap_coefficient(b), b.overlap_coefficient(a));
}

TEST_F(BDDTest, OverlapCoefficient_OneEmpty) {
    bddnewvar();
    ZDD a = ZDD::power_set(1);
    ZDD b(0);
    EXPECT_DOUBLE_EQ(a.overlap_coefficient(b), 0.0);
}

TEST_F(BDDTest, OverlapCoefficient_Null) {
    bddnewvar();
    ZDD ps = ZDD::power_set(1);
    EXPECT_DOUBLE_EQ(ZDD::Null.overlap_coefficient(ps), 0.0);
    EXPECT_DOUBLE_EQ(ps.overlap_coefficient(ZDD::Null), 0.0);
}

TEST_F(BDDTest, OverlapCoefficient_FreeFunction) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddp ps = ZDD::power_set(3).GetID();
    bddp c32 = ZDD::combination(3, 2).GetID();
    // a ⊆ b → O = 1.0
    EXPECT_DOUBLE_EQ(bddoverlapcoeff(c32, ps), 1.0);
}

// ============================================================
// entropy tests
// ============================================================

TEST_F(BDDTest, Entropy_EmptyFamily) {
    ZDD e(0);
    EXPECT_DOUBLE_EQ(e.entropy(), 0.0);
}

TEST_F(BDDTest, Entropy_UnitFamily) {
    // {∅} — no elements at all
    ZDD u(1);
    EXPECT_DOUBLE_EQ(u.entropy(), 0.0);
}

TEST_F(BDDTest, Entropy_SingletonSet) {
    // {{v1}} — only one element, entropy = 0
    ZDD s = ZDD::singleton(bddnewvar());
    EXPECT_DOUBLE_EQ(s.entropy(), 0.0);
}

TEST_F(BDDTest, Entropy_UniformFrequency) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // Power set of 3 vars: each var appears in 4 of 8 sets
    // All frequencies equal → H = log2(3) ≈ 1.585
    ZDD ps = ZDD::power_set(3);
    EXPECT_NEAR(ps.entropy(), std::log2(3.0), 1e-10);
}

TEST_F(BDDTest, Entropy_TwoVarsUniform) {
    bddnewvar();
    bddnewvar();
    // Power set of 2 vars: each var appears in 2 of 4 sets
    // H = log2(2) = 1.0
    ZDD ps = ZDD::power_set(2);
    EXPECT_NEAR(ps.entropy(), 1.0, 1e-10);
}

TEST_F(BDDTest, Entropy_NonUniform) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // F = {{v1}, {v1, v2}} — freq(v1)=2, freq(v2)=1
    // total_lit = 3, p(v1)=2/3, p(v2)=1/3
    // H = -(2/3)log2(2/3) - (1/3)log2(1/3)
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2});
    double p1 = 2.0 / 3.0, p2 = 1.0 / 3.0;
    double expected = -(p1 * std::log2(p1) + p2 * std::log2(p2));
    EXPECT_NEAR(f.entropy(), expected, 1e-10);
}

// ============================================================
// variance_size tests
// ============================================================

TEST_F(BDDTest, VarianceSize_EmptyFamily) {
    ZDD e(0);
    EXPECT_DOUBLE_EQ(e.variance_size(), 0.0);
}

TEST_F(BDDTest, VarianceSize_UnitFamily) {
    ZDD u(1);  // {∅}, all sets have size 0
    EXPECT_DOUBLE_EQ(u.variance_size(), 0.0);
}

TEST_F(BDDTest, VarianceSize_SingletonSet) {
    ZDD s = ZDD::singleton(bddnewvar());  // {{v1}}, all sets have size 1
    EXPECT_DOUBLE_EQ(s.variance_size(), 0.0);
}

TEST_F(BDDTest, VarianceSize_UniformSizes) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::combination(3, 2);  // all 3 sets have size 2
    EXPECT_DOUBLE_EQ(f.variance_size(), 0.0);
}

TEST_F(BDDTest, VarianceSize_MixedSizes) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // F = {{v1}, {v1,v2}, {v1,v2,v3}}, sizes = {1, 2, 3}, mean = 2
    // variance = ((1-2)^2 + (2-2)^2 + (3-2)^2) / 3 = 2/3
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2}) +
            ZDD::single_set({v1, v2, v3});
    EXPECT_NEAR(f.variance_size(), 2.0 / 3.0, 1e-10);
}

TEST_F(BDDTest, VarianceSize_PowerSet) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // Power set of {1,2,3}: 8 sets with sizes 0,1,1,1,2,2,2,3
    // Profile: {1, 3, 3, 1}, mean = 12/8 = 1.5
    // E[X^2] = (0*1 + 1*3 + 4*3 + 9*1)/8 = 24/8 = 3.0
    // Var = 3.0 - 1.5^2 = 0.75
    ZDD ps = ZDD::power_set(3);
    EXPECT_NEAR(ps.variance_size(), 0.75, 1e-10);
}

TEST_F(BDDTest, VarianceSize_WithEmptySet) {
    bddvar v1 = bddnewvar();
    // F = {∅, {v1}}, sizes = {0, 1}, mean = 0.5
    // Var = (0.25 + 0.25) / 2 = 0.25
    ZDD f = ZDD(1) + ZDD::singleton(v1);
    EXPECT_NEAR(f.variance_size(), 0.25, 1e-10);
}

// ============================================================
// median_size tests
// ============================================================

TEST_F(BDDTest, MedianSize_EmptyFamily) {
    ZDD e(0);
    EXPECT_DOUBLE_EQ(e.median_size(), 0.0);
}

TEST_F(BDDTest, MedianSize_UnitFamily) {
    ZDD u(1);  // {∅}
    EXPECT_DOUBLE_EQ(u.median_size(), 0.0);
}

TEST_F(BDDTest, MedianSize_SingletonSet) {
    ZDD s = ZDD::singleton(bddnewvar());  // {{v1}}
    EXPECT_DOUBLE_EQ(s.median_size(), 1.0);
}

TEST_F(BDDTest, MedianSize_OddCount) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // F = {{v1}, {v1,v2}, {v1,v2,v3}}, sizes = {1,2,3}, median = 2
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2}) +
            ZDD::single_set({v1, v2, v3});
    EXPECT_DOUBLE_EQ(f.median_size(), 2.0);
}

TEST_F(BDDTest, MedianSize_EvenCount) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // F = {∅, {v1}, {v2}, {v1,v2}}, sizes = {0,1,1,2}
    // sorted: {0,1,1,2}, median = (1+1)/2 = 1.0
    ZDD f = ZDD::power_set(2);
    EXPECT_DOUBLE_EQ(f.median_size(), 1.0);
}

TEST_F(BDDTest, MedianSize_EvenCountAveraging) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // F = {∅, {v1,v2,v3}}, sizes = {0, 3}, median = (0+3)/2 = 1.5
    ZDD f = ZDD(1) + ZDD::single_set({v1, v2, v3});
    EXPECT_DOUBLE_EQ(f.median_size(), 1.5);
}

TEST_F(BDDTest, MedianSize_PowerSet3) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // Power set of {1,2,3}: 8 sets, profile {1,3,3,1}
    // sorted sizes: {0,1,1,1,2,2,2,3}, median = (1+2)/2 = 1.5
    ZDD ps = ZDD::power_set(3);
    EXPECT_DOUBLE_EQ(ps.median_size(), 1.5);
}

TEST_F(BDDTest, MedianSize_AllSameSize) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::combination(3, 2);  // all sets have size 2
    EXPECT_DOUBLE_EQ(f.median_size(), 2.0);
}

// ============================================================
// cost_bound_range tests
// ============================================================

TEST_F(BDDTest, CostBoundRange_EmptyFamily) {
    std::vector<int> w = {0, 5, 10};
    bddnewvar();
    bddnewvar();
    ZDD f(0);
    EXPECT_EQ(f.cost_bound_range(w, 0, 100), ZDD::Empty);
}

TEST_F(BDDTest, CostBoundRange_UnitFamily) {
    std::vector<int> w = {0, 5, 10};
    bddnewvar();
    bddnewvar();
    ZDD f(1);  // {∅}, cost = 0
    EXPECT_EQ(f.cost_bound_range(w, 0, 100), ZDD::Single);
    EXPECT_EQ(f.cost_bound_range(w, 1, 100), ZDD::Empty);
    EXPECT_EQ(f.cost_bound_range(w, -5, -1), ZDD::Empty);
}

TEST_F(BDDTest, CostBoundRange_SingleVariable) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, 5, 10};
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);  // {{v1}, {v2}}
    // cost(v1) = 5, cost(v2) = 10
    EXPECT_EQ(f.cost_bound_range(w, 5, 5), ZDD::singleton(v1));
    EXPECT_EQ(f.cost_bound_range(w, 10, 10), ZDD::singleton(v2));
    EXPECT_EQ(f.cost_bound_range(w, 5, 10), f);
    EXPECT_EQ(f.cost_bound_range(w, 6, 9), ZDD::Empty);
}

TEST_F(BDDTest, CostBoundRange_CrossValidateWithLeGe) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    std::vector<int> w = {0, 3, -2, 7};
    ZDD f = ZDD::power_set(3);
    for (long long lo = -5; lo <= 10; lo += 3) {
        for (long long hi = lo; hi <= 12; hi += 3) {
            ZDD range = f.cost_bound_range(w, lo, hi);
            ZDD expected = f.cost_bound_le(w, hi) & f.cost_bound_ge(w, lo);
            EXPECT_EQ(range, expected)
                << "lo=" << lo << " hi=" << hi;
        }
    }
}

TEST_F(BDDTest, CostBoundRange_WithMemo) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    std::vector<int> w = {0, 3, -2, 7};
    ZDD f = ZDD::power_set(3);
    CostBoundMemo memo;
    ZDD r1 = f.cost_bound_range(w, 0, 5, memo);
    ZDD r2 = f.cost_bound_range(w, 3, 10, memo);
    ZDD r3 = f.cost_bound_range(w, -2, 0, memo);
    // Verify results match non-memo versions
    EXPECT_EQ(r1, f.cost_bound_range(w, 0, 5));
    EXPECT_EQ(r2, f.cost_bound_range(w, 3, 10));
    EXPECT_EQ(r3, f.cost_bound_range(w, -2, 0));
}

TEST_F(BDDTest, CostBoundRange_EquivalentToEq) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    std::vector<int> w = {0, 3, -2, 7};
    ZDD f = ZDD::power_set(3);
    // range(b, b) should equal eq(b)
    for (long long b = -5; b <= 12; ++b) {
        EXPECT_EQ(f.cost_bound_range(w, b, b), f.cost_bound_eq(w, b))
            << "b=" << b;
    }
}

// --- Review fixes: RecurGuard for weighted sampling ---

TEST_F(BDDTest, ZDD_WeightedSample_RecurGuard_DeepZDD) {
    // Build a deep chain ZDD that exceeds BDD_RecurLimit (8192)
    const int depth = 9000;
    for (int i = bdd_varcount; i < depth; ++i) bddnewvar();
    // Build single-chain ZDD: {1,2,...,depth}
    ZDD f = ZDD::Single;
    for (int v = depth; v >= 1; --v) {
        f = ZDD_ID(ZDD::getnode_raw(v, bddempty, f.GetID()));
    }
    std::vector<double> w(depth + 1, 1.0);
    w[0] = 0.0;
    std::mt19937_64 rng(42);
    WeightedSampleMemo memo_prod(f, w, WeightMode::Product);
    EXPECT_THROW(f.weighted_sample(w, WeightMode::Product, rng, memo_prod),
                 std::overflow_error);
    WeightedSampleMemo memo_sum(f, w, WeightMode::Sum);
    EXPECT_THROW(f.weighted_sample(w, WeightMode::Sum, rng, memo_sum),
                 std::overflow_error);
}

// --- Review fixes: boltzmann_sample mismatch after memo stored ---

TEST_F(BDDTest, ZDD_BoltzmannSample_WeightsMismatch_AfterStore) {
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<double> w1 = {0.0, 1.0, 2.0};
    std::vector<double> w2 = {0.0, 5.0, 10.0};
    double beta = 1.0;
    auto tw1 = ZDD::boltzmann_weights(w1, beta);
    WeightedSampleMemo memo(f, tw1, WeightMode::Product);
    std::mt19937_64 rng(42);
    // First call succeeds and populates memo
    EXPECT_NO_THROW(f.boltzmann_sample(w1, beta, rng, memo));
    EXPECT_TRUE(memo.stored());
    // Second call with different weights must throw
    EXPECT_THROW(f.boltzmann_sample(w2, beta, rng, memo),
                 std::invalid_argument);
}

TEST_F(BDDTest, ZDD_BoltzmannSample_BetaMismatch_AfterStore) {
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<double> w = {0.0, 1.0, 2.0};
    auto tw = ZDD::boltzmann_weights(w, 1.0);
    WeightedSampleMemo memo(f, tw, WeightMode::Product);
    std::mt19937_64 rng(42);
    // First call succeeds and populates memo
    EXPECT_NO_THROW(f.boltzmann_sample(w, 1.0, rng, memo));
    EXPECT_TRUE(memo.stored());
    // Second call with different beta must throw
    EXPECT_THROW(f.boltzmann_sample(w, 2.0, rng, memo),
                 std::invalid_argument);
}

// --- Review fixes: BigInt overflow in statistics/similarity APIs ---

TEST_F(BDDTest, ZDD_JaccardIndex_LargeBigInt) {
    // power_set(1100) has 2^1100 sets — far beyond double range
    const int n = 1100;
    for (int i = bdd_varcount; i < n; ++i) bddnewvar();
    ZDD f = ZDD::power_set(n);
    // jaccard(f, f) = 1.0 always
    EXPECT_DOUBLE_EQ(f.jaccard_index(f), 1.0);
    // jaccard with a subset
    ZDD g = ZDD::singleton(1) + ZDD::singleton(2);
    double j = f.jaccard_index(g);
    EXPECT_GE(j, 0.0);
    EXPECT_LE(j, 1.0);
}

TEST_F(BDDTest, ZDD_OverlapCoeff_LargeBigInt) {
    const int n = 1100;
    for (int i = bdd_varcount; i < n; ++i) bddnewvar();
    ZDD f = ZDD::power_set(n);
    EXPECT_DOUBLE_EQ(f.overlap_coefficient(f), 1.0);
    ZDD g = ZDD::singleton(1) + ZDD::singleton(2);
    double o = f.overlap_coefficient(g);
    // g is subset of f, so overlap = |g ∩ f| / min(|g|, |f|) = |g| / |g| = 1.0
    EXPECT_DOUBLE_EQ(o, 1.0);
}

TEST_F(BDDTest, ZDD_Entropy_LargeBigInt) {
    const int n = 1100;
    for (int i = bdd_varcount; i < n; ++i) bddnewvar();
    ZDD f = ZDD::power_set(n);
    double h = f.entropy();
    // For power_set, all variables have equal frequency → max entropy
    EXPECT_GT(h, 0.0);
    EXPECT_TRUE(std::isfinite(h));
}

TEST_F(BDDTest, ZDD_VarianceSize_LargeBigInt) {
    const int n = 1100;
    for (int i = bdd_varcount; i < n; ++i) bddnewvar();
    ZDD f = ZDD::power_set(n);
    double v = f.variance_size();
    // power_set(n) has binomial(n,k) sets of size k.
    // Mean = n/2, Variance = n/4
    EXPECT_GT(v, 0.0);
    EXPECT_TRUE(std::isfinite(v));
    EXPECT_NEAR(v, n / 4.0, 1.0);
}

// --- ZDD lowercase snake_case method tests ---

TEST_F(BDDTest, ZDD_LowercaseId) {
    bddvar v1 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(z_v1.id(), z_v1.GetID());
    EXPECT_EQ(z_v1.id(), z_v1.id());
    EXPECT_EQ(ZDD(0).id(), bddempty);
    EXPECT_EQ(ZDD(1).id(), bddsingle);
}

TEST_F(BDDTest, ZDD_LowercaseChange) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(z_v1.change(v2), z_v1.Change(v2));
}

TEST_F(BDDTest, ZDD_LowercaseOffset) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    EXPECT_EQ(F.offset(v2), F.OffSet(v2));
    EXPECT_EQ(F.offset(v2), z_v1);
}

TEST_F(BDDTest, ZDD_LowercaseOnset) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    EXPECT_EQ(F.onset(v2), F.OnSet(v2));
    EXPECT_EQ(F.onset(v2), z_v1v2);
}

TEST_F(BDDTest, ZDD_LowercaseOnset0) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    EXPECT_EQ(F.onset0(v2), F.OnSet0(v2));
    EXPECT_EQ(F.onset0(v2), z_v1);
}

TEST_F(BDDTest, ZDD_LowercaseIntersec) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    EXPECT_EQ(F.intersec(z_v1), F.Intersec(z_v1));
    EXPECT_EQ(F.intersec(z_v1), z_v1);
}

TEST_F(BDDTest, ZDD_LowercaseMeet) {
    bddvar v1 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD s = ZDD(1);  // {∅}
    EXPECT_EQ(z_v1.meet(s), z_v1.Meet(s));
}

TEST_F(BDDTest, ZDD_LowercaseCard) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2 + ZDD(1);
    EXPECT_EQ(F.card(), F.Card());
    EXPECT_EQ(F.card(), 3u);
}

TEST_F(BDDTest, ZDD_LowercaseRestrictOp) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    ZDD G = z_v1;  // filter: subsets of {v1}
    EXPECT_EQ(F.restrict_op(G), F.Restrict(G));
}

TEST_F(BDDTest, ZDD_LowercasePermit) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    ZDD G = z_v1;  // permit: only v1 allowed
    EXPECT_EQ(F.permit(G), F.Permit(G));
}

TEST_F(BDDTest, ZDD_RemoveSupersets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    // F = {{v1}, {v1,v2}}, remove supersets of {v1} → removes {v1,v2}
    ZDD F = z_v1 + z_v1v2;
    ZDD result = F.remove_supersets(z_v1);
    EXPECT_EQ(result.id(), bddnonsup(F.id(), z_v1.id()));
}

TEST_F(BDDTest, ZDD_RemoveSubsets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    // F = {{v1}, {v1,v2}}, remove subsets of {v1,v2} → removes {v1}
    ZDD F = z_v1 + z_v1v2;
    ZDD result = F.remove_subsets(z_v1v2);
    EXPECT_EQ(result.id(), bddnonsub(F.id(), z_v1v2.id()));
}

TEST_F(BDDTest, ZDD_LowercaseSupport) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(z_v1v2.support(), z_v1v2.Support());
}

TEST_F(BDDTest, ZDD_LowercaseLit) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    EXPECT_EQ(F.lit(), F.Lit());
}

TEST_F(BDDTest, ZDD_LowercaseLen) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(z_v1v2.len(), z_v1v2.Len());
    EXPECT_EQ(z_v1v2.len(), 2u);
}

TEST_F(BDDTest, ZDD_LowercaseIsPoly) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.is_poly(), F.IsPoly());
    EXPECT_TRUE(F.is_poly());
    EXPECT_FALSE(z_v1.is_poly());
}

TEST_F(BDDTest, ZDD_LowercaseSwap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(z_v1.swap(v1, v2), z_v1.Swap(v1, v2));
}

TEST_F(BDDTest, ZDD_LowercaseImplyChk) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // F = {{v1, v2}} → v1 implies v2
    ZDD F = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(F.imply_chk(v1, v2), F.ImplyChk(v1, v2));
    EXPECT_TRUE(F.imply_chk(v1, v2));
}

TEST_F(BDDTest, ZDD_LowercaseCoImplyChk) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(z_v1v2.co_imply_chk(v1, v2), z_v1v2.CoImplyChk(v1, v2));
}

TEST_F(BDDTest, ZDD_LowercasePermitSym) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2 + ZDD(1);
    EXPECT_EQ(F.permit_sym(1), F.PermitSym(1));
}

TEST_F(BDDTest, ZDD_LowercaseAlways) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // F = {{v1, v2}, {v1}} → v1 is always present
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    EXPECT_EQ(F.always(), F.Always());
}

TEST_F(BDDTest, ZDD_LowercaseSymChk) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // F = {{v1}, {v2}} → v1 and v2 are symmetric
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.sym_chk(v1, v2), F.SymChk(v1, v2));
    EXPECT_TRUE(F.sym_chk(v1, v2));
}

TEST_F(BDDTest, ZDD_LowercaseImplySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(z_v1v2.imply_set(v1), z_v1v2.ImplySet(v1));
}

TEST_F(BDDTest, ZDD_LowercaseSymGrp) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.sym_grp(), F.SymGrp());
}

TEST_F(BDDTest, ZDD_LowercaseSymGrpNaive) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.sym_grp_naive(), F.SymGrpNaive());
}

TEST_F(BDDTest, ZDD_LowercaseSymSet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.sym_set(v1), F.SymSet(v1));
}

TEST_F(BDDTest, ZDD_LowercaseCoImplySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(z_v1v2.co_imply_set(v1), z_v1v2.CoImplySet(v1));
}

TEST_F(BDDTest, ZDD_LowercaseDivisor) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.divisor(), F.Divisor());
}

TEST_F(BDDTest, ZDD_LowercasePrintPla) {
    // Test print_pla with deprecated PrintPla wrapper
    {
        ZDD e(0);
        std::ostringstream oss;
        std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
        e.PrintPla();
        std::cout.rdbuf(old);
        EXPECT_EQ(oss.str(), ".i 0\n.o 1\n0\n.e\n");
    }
}

TEST_F(BDDTest, ZDD_PrintPla_Null) {
    // bddnull: no output
    ZDD n(-1);
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    n.print_pla();
    std::cout.rdbuf(old);
    EXPECT_EQ(oss.str(), "");
}

TEST_F(BDDTest, ZDD_PrintPla_Empty) {
    // Empty family: tlev==0, output "0"
    ZDD e(0);
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    e.print_pla();
    std::cout.rdbuf(old);
    EXPECT_EQ(oss.str(), ".i 0\n.o 1\n0\n.e\n");
}

TEST_F(BDDTest, ZDD_PrintPla_Single) {
    // Single (unit family containing empty set): tlev==0, output "1"
    ZDD s(1);
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    s.print_pla();
    std::cout.rdbuf(old);
    EXPECT_EQ(oss.str(), ".i 0\n.o 1\n1\n.e\n");
}

TEST_F(BDDTest, ZDD_PrintPla_Example1) {
    // Example from spec: {{x1, x3}, {x2}} with 3 variables
    // x1=var at level 1, x2=var at level 2, x3=var at level 3
    bddvar v1 = bddnewvar();  // level 1
    bddvar v2 = bddnewvar();  // level 2
    bddvar v3 = bddnewvar();  // level 3

    // {x1, x3}
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v3 = ZDD_ID(ZDD::getnode(v3, bddempty, bddsingle));
    ZDD set_x1_x3 = z_v1 * z_v3;  // join = {x1, x3}

    // {x2}
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));

    ZDD F = set_x1_x3 + z_v2;  // {{x1, x3}, {x2}}

    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    F.print_pla();
    std::cout.rdbuf(old);
    EXPECT_EQ(oss.str(), ".i 3\n.o 1\n101 1\n010 1\n.e\n");
}

TEST_F(BDDTest, ZDD_PrintPla_SingleVar) {
    // Single variable: {{x1}}
    bddvar v1 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    z_v1.print_pla();
    std::cout.rdbuf(old);
    EXPECT_EQ(oss.str(), ".i 1\n.o 1\n1 1\n.e\n");
}

TEST_F(BDDTest, ZDD_PrintPla_AllCombinations) {
    // {{x1, x2}, {x1}, {x2}, {}} with 2 variables
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD all = (z_v1 + ZDD(1)) * (z_v2 + ZDD(1));  // power set of {v1, v2}

    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    all.print_pla();
    std::cout.rdbuf(old);
    // OnSet0 first (1), then OffSet (0) at each level
    // Level 2=1: Level 1=1 -> "11 1", Level 1=0 -> "01 1"
    // Level 2=0: Level 1=1 -> "10 1", Level 1=0 -> "00 1"
    EXPECT_EQ(oss.str(), ".i 2\n.o 1\n11 1\n01 1\n10 1\n00 1\n.e\n");
}

TEST_F(BDDTest, ZDD_PrintPla_VarsAboveSupport) {
    // When variables exist above the ZDD's support, .i should still
    // reflect the total number of declared variables.
    bddvar v1 = bddnewvar();  // level 1
    bddvar v2 = bddnewvar();  // level 2 (not used in ZDD)
    (void)v2;

    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));  // {{v1}}

    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    z_v1.print_pla();
    std::cout.rdbuf(old);
    // .i should be 2 (total declared vars), not 1 (top level of ZDD)
    // cube "10" means v1=1, v2=0
    EXPECT_EQ(oss.str(), ".i 2\n.o 1\n10 1\n.e\n");
}

TEST_F(BDDTest, ZDD_PrintPla_EmptySetWithExtraVars) {
    // {∅} (single/unit family) with extra declared variables
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    (void)v1;
    (void)v2;

    ZDD s(1);  // {∅}

    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    s.print_pla();
    std::cout.rdbuf(old);
    // ∅ = all variables absent → "00 1"
    EXPECT_EQ(oss.str(), ".i 2\n.o 1\n00 1\n.e\n");
}

TEST_F(BDDTest, ZDD_Zlev_Empty) {
    ZDD e(0);
    // zlev on empty family: always returns empty
    EXPECT_EQ(e.zlev(0, 0), ZDD(0));
    EXPECT_EQ(e.zlev(5, 0), ZDD(0));
}

TEST_F(BDDTest, ZDD_Zlev_Single) {
    ZDD s(1);
    // zlev on {∅}: lev <= 0 returns {∅} (contains empty set)
    EXPECT_EQ(s.zlev(0, 0), ZDD(1));
    EXPECT_EQ(s.zlev(5, 0), ZDD(1));
}

TEST_F(BDDTest, ZDD_Zlev_BasicDescent) {
    // Create variables at levels 1, 2, 3
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // F = {{v1}, {v2}, {v3}} - top level is 3
    ZDD z1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD z3 = ZDD_ID(ZDD::getnode(v3, bddempty, bddsingle));
    ZDD F = z1 + z2 + z3;

    // zlev(3, 0): top level is already 3, no descent needed
    EXPECT_EQ(F.zlev(3, 0), F);

    // zlev(2, 0): remove v3 via OffSet, result should be {{v1}, {v2}}
    ZDD result2 = F.zlev(2, 0);
    EXPECT_EQ(result2, z1 + z2);

    // zlev(1, 0): remove v3 and v2, result should be {{v1}}
    ZDD result1 = F.zlev(1, 0);
    EXPECT_EQ(result1, z1);

    // zlev(0, 0): returns {∅} if empty set is in F, else empty
    // F = {{v1},{v2},{v3}} does not contain ∅
    EXPECT_EQ(F.zlev(0, 0), ZDD(0));
}

TEST_F(BDDTest, ZDD_Zlev_WithEmptySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    // F = {{v1}, {v2}, {}} - contains empty set
    ZDD F = z1 + z2 + ZDD(1);

    // zlev(0, 0): F contains ∅, so returns {∅}
    EXPECT_EQ(F.zlev(0, 0), ZDD(1));

    // zlev(1, 0): should include {v1} and {∅}
    ZDD result1 = F.zlev(1, 0);
    EXPECT_EQ(result1, z1 + ZDD(1));
}

TEST_F(BDDTest, ZDD_Zlev_LastParameter) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // F = {{v3}} - top at level 3
    ZDD z3 = ZDD_ID(ZDD::getnode(v3, bddempty, bddsingle));

    // zlev(2, 1): last=1, target level=2
    // After OffSet(v3), result is empty (level 0), which is < 2
    // So return the backup (before the OffSet), which is z3 itself
    ZDD result = z3.zlev(2, 1);
    EXPECT_EQ(result, z3);

    // zlev(3, 1): already at level 3, so return as-is
    EXPECT_EQ(z3.zlev(3, 1), z3);
}

TEST_F(BDDTest, ZDD_Zlev_DeprecatedWrapper) {
    ZDD e(0);
    EXPECT_EQ(e.ZLev(0, 0), e.zlev(0, 0));
}

TEST_F(BDDTest, ZDD_SetZskip_Empty) {
    // set_zskip on empty/single should not throw
    ZDD e(0);
    e.set_zskip();  // no-op (level 0)
    ZDD s(1);
    s.set_zskip();  // no-op (level 0)
}

TEST_F(BDDTest, ZDD_SetZskip_DeprecatedWrapper) {
    ZDD e(0);
    e.SetZSkip();  // should call set_zskip(), no throw
}

TEST_F(BDDTest, ZDD_SetZskip_AcceleratesZlev) {
    // Create a ZDD with many levels and verify that
    // set_zskip + zlev produces the same result as zlev alone
    const int N = 20;
    std::vector<bddvar> vars(N);
    for (int i = 0; i < N; ++i) {
        vars[i] = bddnewvar();
    }
    // Build a singleton set containing the top variable
    ZDD F = ZDD_ID(ZDD::getnode(vars[N-1], bddempty, bddsingle));

    // Without skip: zlev should work correctly
    ZDD r1 = F.zlev(5, 0);

    // With skip: set_zskip then zlev should give same result
    F.set_zskip();
    ZDD r2 = F.zlev(5, 0);

    EXPECT_EQ(r1, r2);
}

TEST_F(BDDTest, ZDD_Zlev_Null) {
    // bddnull should propagate as bddnull
    ZDD n(-1);
    EXPECT_EQ(n.zlev(0, 0), ZDD(-1));
    EXPECT_EQ(n.zlev(5, 0), ZDD(-1));
    EXPECT_EQ(n.zlev(5, 1), ZDD(-1));
}

TEST_F(BDDTest, ZDD_Zlev_Complement) {
    // Complement edge should be handled correctly in zlev
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    ZDD z1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD z3 = ZDD_ID(ZDD::getnode(v3, bddempty, bddsingle));
    ZDD F = z1 + z2 + z3;  // {{v1},{v2},{v3}}
    ZDD Fc = ~F;            // complement: ∅ membership toggled

    // F does not contain ∅, so ~F does contain ∅
    EXPECT_FALSE(F.has_empty());
    EXPECT_TRUE(Fc.has_empty());

    // zlev on complement should equal complement of zlev
    EXPECT_EQ(Fc.zlev(2, 0), ~(F.zlev(2, 0)));
    EXPECT_EQ(Fc.zlev(1, 0), ~(F.zlev(1, 0)));
    EXPECT_EQ(Fc.zlev(0, 0), ~(F.zlev(0, 0)));
}

TEST_F(BDDTest, ZDD_Zlev_LastNoLoop) {
    // When target level > current top, last=1 should return self
    bddvar v1 = bddnewvar();
    ZDD z1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    // z1 has top at level 1, zlev(5, 1) should return z1 (not empty)
    EXPECT_EQ(z1.zlev(5, 1), z1);
}

TEST_F(BDDTest, ZDD_SetZskip_Null) {
    // set_zskip on bddnull should be a no-op
    ZDD n(-1);
    n.set_zskip();  // should not throw
}

TEST_F(BDDTest, ZDD_SetZskip_Complement) {
    // set_zskip on complement should work correctly
    const int N = 20;
    std::vector<bddvar> vars(N);
    for (int i = 0; i < N; ++i) {
        vars[i] = bddnewvar();
    }
    ZDD F = ZDD_ID(ZDD::getnode(vars[N-1], bddempty, bddsingle));
    ZDD Fc = ~F;

    // Reference: zlev without skip
    ZDD ref_f = F.zlev(5, 0);
    ZDD ref_fc = Fc.zlev(5, 0);

    // After set_zskip on F, zlev on both F and ~F should still be correct
    F.set_zskip();
    EXPECT_EQ(F.zlev(5, 0), ref_f);
    EXPECT_EQ(Fc.zlev(5, 0), ref_fc);

    // After set_zskip on ~F, results should still be correct
    Fc.set_zskip();
    EXPECT_EQ(F.zlev(5, 0), ref_f);
    EXPECT_EQ(Fc.zlev(5, 0), ref_fc);
}

TEST_F(BDDTest, ZDD_SetZskip_ComplementCacheIsolation) {
    // Verify that set_zskip on ~f does not corrupt f's cache
    const int N = 12;
    std::vector<bddvar> vars(N);
    for (int i = 0; i < N; ++i) {
        vars[i] = bddnewvar();
    }
    // F = power_set - {∅}: does NOT contain ∅
    ZDD F = ZDD::power_set(N) - ZDD(1);
    ZDD Fc = ~F;  // contains ∅

    // Reference values before any skip caching
    ZDD ref_f = F.zlev(5, 0);
    ZDD ref_fc = Fc.zlev(5, 0);

    // F does not contain ∅, Fc does
    EXPECT_FALSE(ref_f.has_empty());
    EXPECT_TRUE(ref_fc.has_empty());

    // set_zskip on ~F, then check F
    Fc.set_zskip();
    EXPECT_EQ(F.zlev(5, 0), ref_f);
    EXPECT_FALSE(F.zlev(5, 0).has_empty());

    // set_zskip on F, then check ~F
    F.set_zskip();
    EXPECT_EQ(Fc.zlev(5, 0), ref_fc);
    EXPECT_TRUE(Fc.zlev(5, 0).has_empty());
}

TEST_F(BDDTest, ZDD_Join) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    // join == operator*
    EXPECT_EQ(z_v1.join(z_v2), z_v1 * z_v2);
    // join with empty
    EXPECT_EQ(z_v1.join(ZDD(0)), ZDD(0));
    // join with single (unit family {∅})
    EXPECT_EQ(z_v1.join(ZDD(1)), z_v1);
}

