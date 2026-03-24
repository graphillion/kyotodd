#include <gtest/gtest.h>
#include "bdd.h"
#include <vector>
#include <algorithm>

class ZddRankIterTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD::init(100, 1 << 20);
    }
    void TearDown() override {
        bddfinal();
    }
};

// ---- Basic tests ----

TEST_F(ZddRankIterTest, EmptyFamily) {
    ZDD f(0); // bddempty
    ZddRankRange range(f);
    EXPECT_EQ(range.begin(), range.end());
}

TEST_F(ZddRankIterTest, NullZdd) {
    ZDD f(-1); // bddnull
    EXPECT_THROW(ZddRankRange(f).begin(), std::invalid_argument);
}

TEST_F(ZddRankIterTest, SingleEmptySet) {
    // ZDD(1) = {∅}
    ZDD f(1);
    ZddRankRange range(f);
    ZddRankIterator it = range.begin();
    ASSERT_NE(it, range.end());
    EXPECT_TRUE(it->empty());
    ++it;
    EXPECT_EQ(it, range.end());
}

TEST_F(ZddRankIterTest, SingleSingleton) {
    BDD::new_var();
    ZDD f = ZDD::singleton(1); // {{1}}
    ZddRankRange range(f);
    ZddRankIterator it = range.begin();
    ASSERT_NE(it, range.end());
    std::vector<bddvar> expected = {1};
    EXPECT_EQ(*it, expected);
    ++it;
    EXPECT_EQ(it, range.end());
}

TEST_F(ZddRankIterTest, TwoSingletons) {
    BDD::new_var(2);
    // {{1}, {2}}
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<std::vector<bddvar> > result;
    for (ZddRankIterator it(f); it != ZddRankIterator(); ++it) {
        result.push_back(*it);
    }
    // Verify matches unrank order.
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], f.unrank(0));
    EXPECT_EQ(result[1], f.unrank(1));
}

// ---- Cross-validation with unrank ----

TEST_F(ZddRankIterTest, PowerSet3MatchesUnrank) {
    BDD::new_var(3);
    ZDD f = ZDD::power_set(3);
    int64_t total = static_cast<int64_t>(f.Card());

    std::vector<std::vector<bddvar> > iter_result;
    for (auto& s : ZddRankRange(f)) {
        iter_result.push_back(s);
    }

    ASSERT_EQ(static_cast<int64_t>(iter_result.size()), total);
    for (int64_t i = 0; i < total; ++i) {
        EXPECT_EQ(iter_result[static_cast<size_t>(i)], f.unrank(i))
            << "Mismatch at rank " << i;
    }
}

TEST_F(ZddRankIterTest, PowerSet5MatchesUnrank) {
    BDD::new_var(5);
    ZDD f = ZDD::power_set(5);
    int64_t total = static_cast<int64_t>(f.Card());

    std::vector<std::vector<bddvar> > iter_result;
    for (auto& s : ZddRankRange(f)) {
        iter_result.push_back(s);
    }

    ASSERT_EQ(static_cast<int64_t>(iter_result.size()), total);
    for (int64_t i = 0; i < total; ++i) {
        EXPECT_EQ(iter_result[static_cast<size_t>(i)], f.unrank(i))
            << "Mismatch at rank " << i;
    }
}

TEST_F(ZddRankIterTest, Combination5_2MatchesUnrank) {
    BDD::new_var(5);
    ZDD f = ZDD::combination(5, 2);
    int64_t total = static_cast<int64_t>(f.Card());

    std::vector<std::vector<bddvar> > iter_result;
    for (auto& s : ZddRankRange(f)) {
        iter_result.push_back(s);
    }

    ASSERT_EQ(static_cast<int64_t>(iter_result.size()), total);
    for (int64_t i = 0; i < total; ++i) {
        EXPECT_EQ(iter_result[static_cast<size_t>(i)], f.unrank(i))
            << "Mismatch at rank " << i;
    }
}

TEST_F(ZddRankIterTest, Combination5_3MatchesUnrank) {
    BDD::new_var(5);
    ZDD f = ZDD::combination(5, 3);
    int64_t total = static_cast<int64_t>(f.Card());

    std::vector<std::vector<bddvar> > iter_result;
    for (auto& s : ZddRankRange(f)) {
        iter_result.push_back(s);
    }

    ASSERT_EQ(static_cast<int64_t>(iter_result.size()), total);
    for (int64_t i = 0; i < total; ++i) {
        EXPECT_EQ(iter_result[static_cast<size_t>(i)], f.unrank(i))
            << "Mismatch at rank " << i;
    }
}

// ---- Complement edge tests ----

TEST_F(ZddRankIterTest, ComplementRoot) {
    BDD::new_var(3);
    // power_set includes ∅, so its ZDD root has complement bit.
    ZDD f = ZDD::power_set(3);
    EXPECT_TRUE(f.has_empty());

    std::vector<std::vector<bddvar> > iter_result;
    for (auto& s : ZddRankRange(f)) {
        iter_result.push_back(s);
    }

    // First element should be ∅.
    ASSERT_FALSE(iter_result.empty());
    EXPECT_TRUE(iter_result[0].empty());
}

// ---- from_sets test ----

TEST_F(ZddRankIterTest, FromSets) {
    BDD::new_var(4);
    std::vector<std::vector<bddvar> > sets = {
        {1, 3}, {2, 4}, {1, 2, 3, 4}, {}
    };
    ZDD f = ZDD::from_sets(sets);
    int64_t total = static_cast<int64_t>(f.Card());

    std::vector<std::vector<bddvar> > iter_result;
    for (auto& s : ZddRankRange(f)) {
        iter_result.push_back(s);
    }

    ASSERT_EQ(static_cast<int64_t>(iter_result.size()), total);
    for (int64_t i = 0; i < total; ++i) {
        EXPECT_EQ(iter_result[static_cast<size_t>(i)], f.unrank(i))
            << "Mismatch at rank " << i;
    }
}

// ---- Early break ----

TEST_F(ZddRankIterTest, EarlyBreak) {
    BDD::new_var(5);
    ZDD f = ZDD::power_set(5);

    std::vector<std::vector<bddvar> > first3;
    int count = 0;
    for (auto it = ZddRankIterator(f); it != ZddRankIterator(); ++it) {
        first3.push_back(*it);
        if (++count >= 3) break;
    }

    ASSERT_EQ(first3.size(), 3u);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(first3[static_cast<size_t>(i)], f.unrank(i));
    }
}

// ---- Postfix increment ----

TEST_F(ZddRankIterTest, PostfixIncrement) {
    BDD::new_var(2);
    ZDD f = ZDD::power_set(2);

    ZddRankIterator it(f);
    ZddRankIterator old = it++;
    // old should have rank-0 value, it should have rank-1 value.
    EXPECT_EQ(*old, f.unrank(0));
    EXPECT_EQ(*it, f.unrank(1));
}

// ---- Range-based for ----

TEST_F(ZddRankIterTest, RangeBasedFor) {
    BDD::new_var(3);
    ZDD f = ZDD::combination(3, 1); // {{1}, {2}, {3}}

    std::vector<std::vector<bddvar> > result;
    for (const auto& s : ZddRankRange(f)) {
        result.push_back(s);
    }

    ASSERT_EQ(result.size(), 3u);
    for (size_t i = 0; i < result.size(); ++i) {
        EXPECT_EQ(result[i], f.unrank(static_cast<int64_t>(i)));
    }
}

// ---- Family without empty set ----

TEST_F(ZddRankIterTest, NoEmptySet) {
    BDD::new_var(3);
    ZDD f = ZDD::combination(3, 2); // {{1,2}, {1,3}, {2,3}}
    EXPECT_FALSE(f.has_empty());

    std::vector<std::vector<bddvar> > iter_result;
    for (auto& s : ZddRankRange(f)) {
        iter_result.push_back(s);
    }

    ASSERT_EQ(iter_result.size(), 3u);
    for (int64_t i = 0; i < 3; ++i) {
        EXPECT_EQ(iter_result[static_cast<size_t>(i)], f.unrank(i));
    }
}

// ---- Single variable power set ----

TEST_F(ZddRankIterTest, PowerSet1) {
    BDD::new_var(1);
    ZDD f = ZDD::power_set(1); // {∅, {1}}

    std::vector<std::vector<bddvar> > iter_result;
    for (auto& s : ZddRankRange(f)) {
        iter_result.push_back(s);
    }

    ASSERT_EQ(iter_result.size(), 2u);
    EXPECT_EQ(iter_result[0], f.unrank(0)); // ∅
    EXPECT_EQ(iter_result[1], f.unrank(1)); // {1}
}
