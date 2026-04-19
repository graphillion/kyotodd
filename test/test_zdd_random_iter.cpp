#include <gtest/gtest.h>
#include "bdd.h"
#include <vector>
#include <algorithm>
#include <set>
#include <random>

using namespace kyotodd;

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

// ============================================================
// sample_k_iter / random_subset_iter (batch 14 of iter_plan)
//
// These verify that the iterative variants produce identical
// results to the recursive variants for the same RNG seed
// (consume RNG in the same order), exercise the public API
// dispatch, and smoke-test deep ZDDs that exceed BDD_RecurLimit.
// ============================================================

class ZddSampleIterTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

TEST_F(ZddSampleIterTest, SampleKIterMatchesRecPowerSet) {
    BDD::new_var(5);
    ZDD f = ZDD::power_set(5);  // 32 sets

    for (int64_t k : {1, 3, 7, 16, 31}) {
        ZddCountMemo memo_rec(f);
        ZddCountMemo memo_iter(f);
        std::mt19937_64 rng_rec(2026);
        std::mt19937_64 rng_iter(2026);

        bigint::BigInt bk(static_cast<int>(k));

        bddp rec_r = bdd_gc_guard([&]() -> bddp {
            f.exact_count(memo_rec);
            return detail::sample_k_rec(f.GetID(), bk, rng_rec, memo_rec.map());
        });
        bddp iter_r = bdd_gc_guard([&]() -> bddp {
            f.exact_count(memo_iter);
            return detail::sample_k_iter(f.GetID(), bk, rng_iter, memo_iter.map());
        });
        EXPECT_EQ(iter_r, rec_r) << "k=" << k;
    }
}

TEST_F(ZddSampleIterTest, RandomSubsetIterMatchesRecPowerSet) {
    BDD::new_var(5);
    ZDD f = ZDD::power_set(5);

    for (double p : {0.1, 0.3, 0.5, 0.7, 0.9}) {
        std::mt19937_64 rng_rec(4242);
        std::mt19937_64 rng_iter(4242);
        std::uniform_real_distribution<double> dist_rec(0.0, 1.0);
        std::uniform_real_distribution<double> dist_iter(0.0, 1.0);

        bddp rec_r = bdd_gc_guard([&]() -> bddp {
            return detail::random_subset_rec(f.GetID(), p, rng_rec, dist_rec);
        });
        bddp iter_r = bdd_gc_guard([&]() -> bddp {
            return detail::random_subset_iter(f.GetID(), p, rng_iter, dist_iter);
        });
        EXPECT_EQ(iter_r, rec_r) << "p=" << p;
    }
}

TEST_F(ZddSampleIterTest, SampleKPublicReturnsSubfamily) {
    BDD::new_var(6);
    ZDD f = ZDD::power_set(6);  // 64 sets
    ZddCountMemo memo(f);
    std::mt19937_64 rng(99);
    ZDD result = f.sample_k(10, rng, memo);
    EXPECT_EQ(result.exact_count(), bigint::BigInt(10));
    EXPECT_TRUE(result.is_subset_family(f));
}

TEST_F(ZddSampleIterTest, RandomSubsetPublicIsSubfamily) {
    BDD::new_var(6);
    ZDD f = ZDD::power_set(6);
    std::mt19937_64 rng(99);
    ZDD result = f.random_subset(0.5, rng);
    EXPECT_TRUE(result.is_subset_family(f));
}

TEST_F(ZddSampleIterTest, SampleKDeepZDDExceedsRecurLimit) {
    // Deep chain ZDD that exceeds BDD_RecurLimit (8192). Built bottom-up
    // (low var first) so the top level reaches `depth` and the public
    // dispatcher selects sample_k_iter.
    const int depth = 9000;
    for (int i = bdd_varcount; i < depth; ++i) bddnewvar();
    ZDD f = ZDD::Single;
    for (int v = 1; v <= depth; ++v) {
        f = ZDD_ID(ZDD::getnode_raw(v, bddempty, f.GetID()));
    }
    // f = {{1,2,...,depth}} — single large set, |F|=1.
    ZddCountMemo memo(f);
    std::mt19937_64 rng(7);
    ZDD result = f.sample_k(1, rng, memo);
    // Sampling 1 set from a singleton family must return the family.
    EXPECT_EQ(result, f);
}

TEST_F(ZddSampleIterTest, RandomSubsetDeepZDDExceedsRecurLimit) {
    // Deep chain ZDD that exceeds BDD_RecurLimit. random_subset must
    // dispatch to the iterative variant without stack overflow.
    const int depth = 9000;
    for (int i = bdd_varcount; i < depth; ++i) bddnewvar();
    ZDD f = ZDD::Single;
    for (int v = 1; v <= depth; ++v) {
        f = ZDD_ID(ZDD::getnode_raw(v, bddempty, f.GetID()));
    }
    std::mt19937_64 rng(11);
    ZDD result = f.random_subset(1.0, rng);  // p=1 → identity
    EXPECT_EQ(result, f);
}
