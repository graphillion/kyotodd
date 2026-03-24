#include <gtest/gtest.h>
#include "bdd.h"
#include <vector>
#include <algorithm>
#include <set>

class ZddMinWeightIterTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD::init(100, 1 << 20);
    }
    void TearDown() override {
        bddfinal();
    }
};

// ---- Basic tests ----

TEST_F(ZddMinWeightIterTest, EmptyFamily) {
    ZDD f(0); // bddempty
    std::vector<int> weights(1, 0);
    ZddMinWeightRange range(f, weights);
    EXPECT_EQ(range.begin(), range.end());
}

TEST_F(ZddMinWeightIterTest, NullZdd) {
    ZDD f(-1); // bddnull
    std::vector<int> weights(1, 0);
    EXPECT_THROW(ZddMinWeightIterator(f, weights), std::invalid_argument);
}

TEST_F(ZddMinWeightIterTest, SingleEmptySet) {
    // ZDD(1) = {empty set}
    ZDD f(1);
    std::vector<int> weights(1, 0);
    ZddMinWeightRange range(f, weights);
    ZddMinWeightIterator it = range.begin();
    ASSERT_NE(it, range.end());
    EXPECT_EQ(it->first, 0);
    EXPECT_TRUE(it->second.empty());
    ++it;
    EXPECT_EQ(it, range.end());
}

TEST_F(ZddMinWeightIterTest, SingleSingleton) {
    BDD::new_var();
    ZDD f = ZDD::singleton(1);
    std::vector<int> weights(2, 0);
    weights[1] = 5;

    ZddMinWeightRange range(f, weights);
    ZddMinWeightIterator it = range.begin();
    ASSERT_NE(it, range.end());
    EXPECT_EQ(it->first, 5);
    ASSERT_EQ(it->second.size(), 1u);
    EXPECT_EQ(it->second[0], 1u);
    ++it;
    EXPECT_EQ(it, range.end());
}

// ---- Ordering tests ----

TEST_F(ZddMinWeightIterTest, TwoSingletons) {
    BDD::new_var(2);
    // {{1}, {2}}
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<int> weights(3, 0);
    weights[1] = 10;
    weights[2] = 3;

    ZddMinWeightRange range(f, weights);
    ZddMinWeightIterator it = range.begin();

    // First: {2} with weight 3
    ASSERT_NE(it, range.end());
    EXPECT_EQ(it->first, 3);
    ASSERT_EQ(it->second.size(), 1u);
    EXPECT_EQ(it->second[0], 2u);

    // Second: {1} with weight 10
    ++it;
    ASSERT_NE(it, range.end());
    EXPECT_EQ(it->first, 10);
    ASSERT_EQ(it->second.size(), 1u);
    EXPECT_EQ(it->second[0], 1u);

    ++it;
    EXPECT_EQ(it, range.end());
}

TEST_F(ZddMinWeightIterTest, PowerSetThreeVars) {
    BDD::new_var(3);
    ZDD f = ZDD::power_set(3);
    // 8 sets: {}, {1}, {2}, {3}, {1,2}, {1,3}, {2,3}, {1,2,3}
    std::vector<int> weights(4, 0);
    weights[1] = 1;
    weights[2] = 2;
    weights[3] = 4;

    // Expected weights sorted: 0, 1, 2, 3, 4, 5, 6, 7
    ZddMinWeightRange range(f, weights);
    std::vector<long long> result_weights;
    for (ZddMinWeightIterator it = range.begin(); it != range.end(); ++it) {
        result_weights.push_back(it->first);
    }

    ASSERT_EQ(result_weights.size(), 8u);
    for (size_t i = 0; i < result_weights.size(); ++i) {
        EXPECT_EQ(result_weights[i], static_cast<long long>(i));
    }
}

TEST_F(ZddMinWeightIterTest, NegativeWeights) {
    BDD::new_var(2);
    ZDD f = ZDD::power_set(2);
    // 4 sets: {}, {1}, {2}, {1,2}
    std::vector<int> weights(3, 0);
    weights[1] = -5;
    weights[2] = 3;

    // Weights: {} -> 0, {1} -> -5, {2} -> 3, {1,2} -> -2
    // Sorted: -5, -2, 0, 3
    ZddMinWeightRange range(f, weights);
    std::vector<long long> result_weights;
    for (ZddMinWeightIterator it = range.begin(); it != range.end(); ++it) {
        result_weights.push_back(it->first);
    }

    ASSERT_EQ(result_weights.size(), 4u);
    EXPECT_EQ(result_weights[0], -5);
    EXPECT_EQ(result_weights[1], -2);
    EXPECT_EQ(result_weights[2], 0);
    EXPECT_EQ(result_weights[3], 3);
}

// ---- Cross-validation tests ----

TEST_F(ZddMinWeightIterTest, FirstMatchesMinWeight) {
    BDD::new_var(4);
    ZDD f = ZDD::power_set(4);
    std::vector<int> weights(5, 0);
    weights[1] = 7;
    weights[2] = 3;
    weights[3] = 5;
    weights[4] = 1;

    ZddMinWeightIterator it(f, weights);
    ZddMinWeightIterator end;
    ASSERT_NE(it, end);

    long long min_w = f.min_weight(weights);
    std::vector<bddvar> min_s = f.min_weight_set(weights);
    std::sort(min_s.begin(), min_s.end());

    EXPECT_EQ(it->first, min_w);
    EXPECT_EQ(it->second, min_s);
}

TEST_F(ZddMinWeightIterTest, CompleteEnumerationMatchesSort) {
    BDD::new_var(4);
    ZDD f = ZDD::power_set(4);
    std::vector<int> weights(5, 0);
    weights[1] = 7;
    weights[2] = 3;
    weights[3] = 5;
    weights[4] = 1;

    // Enumerate all sets via the standard method.
    std::vector<std::vector<bddvar> > all_sets = f.enumerate();

    // Compute weight for each set and sort.
    typedef std::pair<long long, std::vector<bddvar> > WS;
    std::vector<WS> expected;
    for (size_t i = 0; i < all_sets.size(); ++i) {
        long long w = 0;
        for (size_t j = 0; j < all_sets[i].size(); ++j) {
            w += weights[all_sets[i][j]];
        }
        std::sort(all_sets[i].begin(), all_sets[i].end());
        expected.push_back(WS(w, all_sets[i]));
    }
    std::sort(expected.begin(), expected.end());

    // Collect from iterator.
    std::vector<WS> actual;
    for (ZddMinWeightIterator it(f, weights); it != ZddMinWeightIterator(); ++it) {
        actual.push_back(*it);
    }

    ASSERT_EQ(actual.size(), expected.size());
    for (size_t i = 0; i < actual.size(); ++i) {
        EXPECT_EQ(actual[i].first, expected[i].first);
        // For same weight, sets may be in any order, so just check weight is non-decreasing.
    }

    // Verify non-decreasing weight order.
    for (size_t i = 1; i < actual.size(); ++i) {
        EXPECT_LE(actual[i - 1].first, actual[i].first);
    }

    // Verify same multiset of (weight, set) pairs.
    // Sort actual by set as secondary key for stable comparison.
    std::sort(actual.begin(), actual.end());
    EXPECT_EQ(actual, expected);
}

// ---- Edge case tests ----

TEST_F(ZddMinWeightIterTest, ComplementRoot) {
    // Build a ZDD whose root has the complement bit set.
    // ~{{v1}} = {emptyset, {v1}} (toggling empty set membership).
    BDD::new_var();
    ZDD f = ~ZDD::singleton(1);
    // f = {emptyset, {1}}

    std::vector<int> weights(2, 0);
    weights[1] = 10;

    ZddMinWeightRange range(f, weights);
    std::vector<std::pair<long long, std::vector<bddvar> > > results;
    for (ZddMinWeightIterator it = range.begin(); it != range.end(); ++it) {
        results.push_back(*it);
    }

    ASSERT_EQ(results.size(), 2u);
    // emptyset: weight 0
    EXPECT_EQ(results[0].first, 0);
    EXPECT_TRUE(results[0].second.empty());
    // {1}: weight 10
    EXPECT_EQ(results[1].first, 10);
    ASSERT_EQ(results[1].second.size(), 1u);
    EXPECT_EQ(results[1].second[0], 1u);
}

TEST_F(ZddMinWeightIterTest, EarlyBreak) {
    BDD::new_var(3);
    ZDD f = ZDD::power_set(3);
    std::vector<int> weights(4, 0);
    weights[1] = 1;
    weights[2] = 2;
    weights[3] = 4;

    // Only take the first 3 elements.
    int count = 0;
    for (ZddMinWeightIterator it(f, weights); it != ZddMinWeightIterator(); ++it) {
        ++count;
        if (count >= 3) break;
    }
    EXPECT_EQ(count, 3);
    // No crash or leak after early destruction.
}

TEST_F(ZddMinWeightIterTest, AllZeroWeights) {
    BDD::new_var(3);
    ZDD f = ZDD::power_set(3);
    std::vector<int> weights(4, 0);

    // All weights are 0, so all 8 sets have weight 0.
    ZddMinWeightRange range(f, weights);
    size_t count = 0;
    for (ZddMinWeightIterator it = range.begin(); it != range.end(); ++it) {
        EXPECT_EQ(it->first, 0);
        ++count;
    }
    EXPECT_EQ(count, 8u);
}

TEST_F(ZddMinWeightIterTest, FamilyWithEmptySetAndOthers) {
    BDD::new_var(2);
    // {emptyset, {1}, {2}}
    ZDD f = ZDD(1) + ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<int> weights(3, 0);
    weights[1] = 5;
    weights[2] = 3;

    ZddMinWeightRange range(f, weights);
    std::vector<long long> ws;
    for (ZddMinWeightIterator it = range.begin(); it != range.end(); ++it) {
        ws.push_back(it->first);
    }
    ASSERT_EQ(ws.size(), 3u);
    EXPECT_EQ(ws[0], 0);  // emptyset
    EXPECT_EQ(ws[1], 3);  // {2}
    EXPECT_EQ(ws[2], 5);  // {1}
}

TEST_F(ZddMinWeightIterTest, FiveVarsCompleteEnumeration) {
    BDD::new_var(5);
    ZDD f = ZDD::power_set(5);
    std::vector<int> weights(6, 0);
    weights[1] = 2;
    weights[2] = 3;
    weights[3] = 5;
    weights[4] = 7;
    weights[5] = 11;

    // Collect all from iterator.
    std::vector<std::pair<long long, std::vector<bddvar> > > actual;
    for (ZddMinWeightIterator it(f, weights); it != ZddMinWeightIterator(); ++it) {
        actual.push_back(*it);
    }
    EXPECT_EQ(actual.size(), 32u);

    // Verify non-decreasing.
    for (size_t i = 1; i < actual.size(); ++i) {
        EXPECT_LE(actual[i - 1].first, actual[i].first);
    }

    // Cross-validate with enumerate.
    std::vector<std::vector<bddvar> > all_sets = f.enumerate();
    EXPECT_EQ(actual.size(), all_sets.size());
}

TEST_F(ZddMinWeightIterTest, PostfixIncrementReturnsOldValue) {
    BDD::new_var(2);
    // {{1}, {2}}
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<int> weights(3, 0);
    weights[1] = 10;
    weights[2] = 3;

    ZddMinWeightIterator it(f, weights);
    // First element: {2} with weight 3
    ASSERT_NE(it, ZddMinWeightIterator());
    EXPECT_EQ(it->first, 3);

    // Postfix increment should return the old value.
    ZddMinWeightIterator old = it++;
    EXPECT_EQ(old->first, 3);    // old points to {2}
    EXPECT_EQ(it->first, 10);    // it now points to {1}
}

TEST_F(ZddMinWeightIterTest, FromSetsNonTrivial) {
    BDD::new_var(3);
    // {{1,2}, {2,3}, {1,3}, {1,2,3}}
    std::vector<std::vector<bddvar> > sets;
    std::vector<bddvar> s1; s1.push_back(1); s1.push_back(2);
    std::vector<bddvar> s2; s2.push_back(2); s2.push_back(3);
    std::vector<bddvar> s3; s3.push_back(1); s3.push_back(3);
    std::vector<bddvar> s4; s4.push_back(1); s4.push_back(2); s4.push_back(3);
    sets.push_back(s1);
    sets.push_back(s2);
    sets.push_back(s3);
    sets.push_back(s4);

    ZDD f = ZDD::from_sets(sets);
    std::vector<int> weights(4, 0);
    weights[1] = 10;
    weights[2] = 1;
    weights[3] = 5;
    // Weights: {1,2}=11, {2,3}=6, {1,3}=15, {1,2,3}=16
    // Order: {2,3}=6, {1,2}=11, {1,3}=15, {1,2,3}=16

    std::vector<std::pair<long long, std::vector<bddvar> > > actual;
    for (ZddMinWeightIterator it(f, weights); it != ZddMinWeightIterator(); ++it) {
        actual.push_back(*it);
    }

    ASSERT_EQ(actual.size(), 4u);
    EXPECT_EQ(actual[0].first, 6);
    EXPECT_EQ(actual[1].first, 11);
    EXPECT_EQ(actual[2].first, 15);
    EXPECT_EQ(actual[3].first, 16);
}

// ---- ZddMaxWeightIterator tests ----

class ZddMaxWeightIterTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD::init(100, 1 << 20);
    }
    void TearDown() override {
        bddfinal();
    }
};

TEST_F(ZddMaxWeightIterTest, EmptyFamily) {
    ZDD f(0);
    std::vector<int> weights = {0, 1};
    bddnewvar();
    ZddMaxWeightIterator it(f, weights);
    ZddMaxWeightIterator end;
    EXPECT_EQ(it, end);
}

TEST_F(ZddMaxWeightIterTest, SingleEmptySet) {
    bddnewvar();
    ZDD f(1); // {∅}
    std::vector<int> weights = {0, 5};
    ZddMaxWeightIterator it(f, weights);
    ZddMaxWeightIterator end;
    ASSERT_NE(it, end);
    EXPECT_EQ(it->first, 0);
    EXPECT_TRUE(it->second.empty());
    ++it;
    EXPECT_EQ(it, end);
}

TEST_F(ZddMaxWeightIterTest, DescendingOrder) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    std::vector<int> weights = {0, 3, 5, 10};
    ZDD f = ZDD::power_set(3);

    std::vector<std::pair<long long, std::vector<bddvar>>> results;
    for (ZddMaxWeightIterator it(f, weights); it != ZddMaxWeightIterator(); ++it) {
        results.push_back(*it);
    }

    ASSERT_EQ(results.size(), 8u);
    // Verify descending order
    for (size_t i = 1; i < results.size(); ++i) {
        EXPECT_GE(results[i - 1].first, results[i].first);
    }
    // First should be max weight, last should be min weight
    EXPECT_EQ(results.front().first, 18); // 3+5+10
    EXPECT_EQ(results.back().first, 0);   // empty set
}

TEST_F(ZddMaxWeightIterTest, CrossValidateWithMinIterator) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    std::vector<int> weights = {0, 3, 5, 10};
    ZDD f = ZDD::power_set(3);

    // Collect both orderings
    std::vector<long long> min_order, max_order;
    for (auto& p : ZddMinWeightRange(f, weights))
        min_order.push_back(p.first);
    for (auto& p : ZddMaxWeightRange(f, weights))
        max_order.push_back(p.first);

    // max_order reversed should equal min_order
    std::vector<long long> max_reversed(max_order.rbegin(), max_order.rend());
    EXPECT_EQ(min_order, max_reversed);
}

TEST_F(ZddMaxWeightIterTest, NegativeWeights) {
    bddnewvar();
    bddnewvar();
    std::vector<int> weights = {0, -3, 5};
    ZDD f = ZDD::power_set(2);

    std::vector<std::pair<long long, std::vector<bddvar>>> results;
    for (auto& p : ZddMaxWeightRange(f, weights))
        results.push_back(p);

    ASSERT_EQ(results.size(), 4u);
    EXPECT_GE(results[0].first, results[1].first);
    EXPECT_GE(results[1].first, results[2].first);
    EXPECT_GE(results[2].first, results[3].first);
    EXPECT_EQ(results.front().first, 5);   // {v2}
    EXPECT_EQ(results.back().first, -3);   // {v1}
}

TEST_F(ZddMaxWeightIterTest, FirstMatchesMaxWeight) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    std::vector<int> weights = {0, 7, -2, 4};
    ZDD f = ZDD::power_set(3);

    ZddMaxWeightIterator it(f, weights);
    ASSERT_NE(it, ZddMaxWeightIterator());
    EXPECT_EQ(it->first, f.max_weight(weights));
}

TEST_F(ZddMaxWeightIterTest, EarlyBreak) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    std::vector<int> weights = {0, 1, 2, 4};
    ZDD f = ZDD::power_set(3);

    int count = 0;
    for (auto& p : ZddMaxWeightRange(f, weights)) {
        (void)p;
        if (++count >= 3) break;
    }
    EXPECT_EQ(count, 3);
}

TEST_F(ZddMaxWeightIterTest, PostfixIncrement) {
    bddnewvar();
    bddnewvar();
    std::vector<int> weights = {0, 3, 5};
    ZDD f = ZDD::power_set(2);

    ZddMaxWeightIterator it(f, weights);
    auto old = it++;
    EXPECT_EQ(old->first, 8);  // {v1,v2} = 3+5
    EXPECT_EQ(it->first, 5);   // {v2}
}

TEST_F(ZddMaxWeightIterTest, RangeBasedFor) {
    bddnewvar();
    bddnewvar();
    std::vector<int> weights = {0, 10, 20};
    ZDD f = ZDD::power_set(2);

    std::vector<long long> ws;
    for (const auto& p : ZddMaxWeightRange(f, weights)) {
        ws.push_back(p.first);
    }
    std::vector<long long> expected = {30, 20, 10, 0};
    EXPECT_EQ(ws, expected);
}
