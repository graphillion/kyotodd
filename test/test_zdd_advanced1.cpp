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

