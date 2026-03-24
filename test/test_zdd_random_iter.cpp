#include <gtest/gtest.h>
#include "bdd.h"
#include <vector>
#include <algorithm>
#include <set>
#include <random>

class ZddRandomIterTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD::init(100, 1 << 20);
    }
    void TearDown() override {
        bddfinal();
    }
};

// ---- Basic tests ----

TEST_F(ZddRandomIterTest, EmptyFamily) {
    ZDD f(0); // bddempty
    std::mt19937_64 rng(42);
    ZddRandomRange<std::mt19937_64> range(f, rng);
    EXPECT_EQ(range.begin(), range.end());
}

TEST_F(ZddRandomIterTest, NullZdd) {
    ZDD f(-1); // bddnull
    std::mt19937_64 rng(42);
    EXPECT_THROW(
        (ZddRandomRange<std::mt19937_64>(f, rng).begin()),
        std::invalid_argument);
}

TEST_F(ZddRandomIterTest, SingleEmptySet) {
    // ZDD(1) = {∅}
    ZDD f(1);
    std::mt19937_64 rng(42);
    ZddRandomRange<std::mt19937_64> range(f, rng);
    auto it = range.begin();
    ASSERT_NE(it, range.end());
    EXPECT_TRUE(it->empty());
    ++it;
    EXPECT_EQ(it, range.end());
}

TEST_F(ZddRandomIterTest, SingleSingleton) {
    BDD::new_var();
    ZDD f = ZDD::singleton(1); // {{1}}
    std::mt19937_64 rng(42);
    ZddRandomRange<std::mt19937_64> range(f, rng);
    auto it = range.begin();
    ASSERT_NE(it, range.end());
    std::vector<bddvar> expected = {1};
    EXPECT_EQ(*it, expected);
    ++it;
    EXPECT_EQ(it, range.end());
}

// ---- Exhaustive enumeration: all sets, no duplicates ----

TEST_F(ZddRandomIterTest, PowerSet3NoDuplicates) {
    BDD::new_var(3);
    ZDD f = ZDD::power_set(3);
    uint64_t total = f.Card();

    std::mt19937_64 rng(123);
    std::set<std::vector<bddvar> > seen;
    for (auto& s : ZddRandomRange<std::mt19937_64>(f, rng)) {
        EXPECT_TRUE(seen.find(s) == seen.end())
            << "Duplicate set found";
        seen.insert(s);
    }
    EXPECT_EQ(seen.size(), total);
}

TEST_F(ZddRandomIterTest, PowerSet5NoDuplicates) {
    BDD::new_var(5);
    ZDD f = ZDD::power_set(5);
    uint64_t total = f.Card();

    std::mt19937_64 rng(456);
    std::set<std::vector<bddvar> > seen;
    for (auto& s : ZddRandomRange<std::mt19937_64>(f, rng)) {
        EXPECT_TRUE(seen.find(s) == seen.end())
            << "Duplicate set found";
        seen.insert(s);
    }
    EXPECT_EQ(seen.size(), total);
}

TEST_F(ZddRandomIterTest, Combination5_2NoDuplicates) {
    BDD::new_var(5);
    ZDD f = ZDD::combination(5, 2);
    uint64_t total = f.Card();

    std::mt19937_64 rng(789);
    std::set<std::vector<bddvar> > seen;
    for (auto& s : ZddRandomRange<std::mt19937_64>(f, rng)) {
        EXPECT_TRUE(seen.find(s) == seen.end())
            << "Duplicate set found";
        seen.insert(s);
    }
    EXPECT_EQ(seen.size(), total);
}

// ---- Sets match the original family ----

TEST_F(ZddRandomIterTest, AllSetsAreMembers) {
    BDD::new_var(4);
    std::vector<std::vector<bddvar> > sets = {
        {1, 3}, {2, 4}, {1, 2, 3, 4}, {}
    };
    ZDD f = ZDD::from_sets(sets);

    std::mt19937_64 rng(101);
    std::set<std::vector<bddvar> > seen;
    for (auto& s : ZddRandomRange<std::mt19937_64>(f, rng)) {
        // Every returned set must be in the original family.
        EXPECT_NE(f.rank(s), -1) << "Set not in family";
        seen.insert(s);
    }
    EXPECT_EQ(seen.size(), static_cast<size_t>(f.Card()));
}

// ---- Early break ----

TEST_F(ZddRandomIterTest, EarlyBreak) {
    BDD::new_var(5);
    ZDD f = ZDD::power_set(5); // 32 sets

    std::mt19937_64 rng(202);
    int count = 0;
    std::set<std::vector<bddvar> > seen;
    for (auto it = ZddRandomIterator<std::mt19937_64>(f, rng);
         it != ZddRandomIterator<std::mt19937_64>(); ++it) {
        seen.insert(*it);
        if (++count >= 5) break;
    }
    EXPECT_EQ(count, 5);
    EXPECT_EQ(seen.size(), 5u); // All unique.
}

// ---- Postfix increment ----

TEST_F(ZddRandomIterTest, PostfixIncrement) {
    BDD::new_var(3);
    ZDD f = ZDD::power_set(3);

    std::mt19937_64 rng(303);
    ZddRandomIterator<std::mt19937_64> it(f, rng);
    auto first = *it;
    auto old = it++;
    EXPECT_EQ(*old, first);
    EXPECT_NE(*it, first); // Different (very likely with 8 sets).
}

// ---- Range-based for ----

TEST_F(ZddRandomIterTest, RangeBasedFor) {
    BDD::new_var(3);
    ZDD f = ZDD::combination(3, 1); // {{1}, {2}, {3}}

    std::mt19937_64 rng(404);
    std::set<std::vector<bddvar> > seen;
    for (const auto& s : ZddRandomRange<std::mt19937_64>(f, rng)) {
        seen.insert(s);
    }
    EXPECT_EQ(seen.size(), 3u);
}

// ---- Different seeds produce different orderings ----

TEST_F(ZddRandomIterTest, DifferentSeeds) {
    BDD::new_var(5);
    ZDD f = ZDD::power_set(5);

    auto collect = [&](uint64_t seed) {
        std::mt19937_64 rng(seed);
        std::vector<std::vector<bddvar> > result;
        for (auto& s : ZddRandomRange<std::mt19937_64>(f, rng)) {
            result.push_back(s);
        }
        return result;
    };

    auto order1 = collect(1);
    auto order2 = collect(2);

    ASSERT_EQ(order1.size(), order2.size());
    // Very unlikely that two different seeds produce identical orderings
    // for 32 sets.
    EXPECT_NE(order1, order2);
}

// ---- Hybrid mode transition (indirect) ----

TEST_F(ZddRandomIterTest, HybridModeTransition) {
    // power_set(3) has 8 sets. After 4 samples, should switch to
    // direct mode. Verify all 8 sets are returned.
    BDD::new_var(3);
    ZDD f = ZDD::power_set(3);

    std::mt19937_64 rng(505);
    std::set<std::vector<bddvar> > seen;
    for (auto& s : ZddRandomRange<std::mt19937_64>(f, rng)) {
        seen.insert(s);
    }
    EXPECT_EQ(seen.size(), 8u);
}

// ---- Family with empty set included ----

TEST_F(ZddRandomIterTest, FamilyWithEmptySet) {
    BDD::new_var(2);
    ZDD f = ZDD::power_set(2); // {∅, {1}, {2}, {1,2}}

    std::mt19937_64 rng(606);
    std::set<std::vector<bddvar> > seen;
    bool found_empty = false;
    for (auto& s : ZddRandomRange<std::mt19937_64>(f, rng)) {
        if (s.empty()) found_empty = true;
        seen.insert(s);
    }
    EXPECT_EQ(seen.size(), 4u);
    EXPECT_TRUE(found_empty);
}
