#include <gtest/gtest.h>
#include "bdd.h"
#include "unreduced_dd_tdzdd.h"
#include <tdzdd/DdStructure.hpp>

using namespace kyotodd;

class UnreducedDDTdZddTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

// =============================================================
// Specs used by the tests
// =============================================================

// Choose exactly k out of n items (TdZdd user guide example).
class Combination : public tdzdd::DdSpec<Combination, int, 2> {
    int const n;
    int const k;
public:
    Combination(int n, int k) : n(n), k(k) {}
    int getRoot(int& state) const {
        state = 0;
        return n;
    }
    int getChild(int& state, int level, int value) const {
        state += value;
        if (--level == 0) return (state == k) ? -1 : 0;
        if (state > k) return 0;
        if (state + level < k) return 0;
        return level;
    }
};

// All subsets of n items (every path leads to the 1-terminal).
class AllSubsets : public tdzdd::StatelessDdSpec<AllSubsets, 2> {
    int const n;
public:
    explicit AllSubsets(int n) : n(n) {}
    int getRoot() const { return n; }
    int getChild(int level, int /*value*/) const {
        return (level == 1) ? -1 : level - 1;
    }
};

static uint64_t binom(int n, int k) {
    if (k < 0 || k > n) return 0;
    uint64_t r = 1;
    for (int i = 1; i <= k; ++i) r = r * (n - k + i) / i;
    return r;
}

// Count reachable non-terminal nodes without any reduction interpretation.
static size_t walk_count(const UnreducedDD& dd) {
    std::unordered_set<bddp> seen;
    std::vector<bddp> stack;
    stack.push_back(dd.id() & ~BDD_COMP_FLAG);
    while (!stack.empty()) {
        bddp f = stack.back(); stack.pop_back();
        if (f & BDD_CONST_FLAG) continue;
        if (!seen.insert(f).second) continue;
        stack.push_back(DDBase::raw_child0(f) & ~BDD_COMP_FLAG);
        stack.push_back(DDBase::raw_child1(f) & ~BDD_COMP_FLAG);
    }
    return seen.size();
}

// =============================================================
// Basic construction
// =============================================================

TEST_F(UnreducedDDTdZddTest, Combination_CardMatchesBinomial) {
    for (int n = 1; n <= 8; ++n) {
        for (int k = 0; k <= n; ++k) {
            UnreducedDD u = UnreducedDD::construct(Combination(n, k));
            ZDD z = u.reduce_as_zdd();
            EXPECT_EQ(z.card(), binom(n, k)) << "n=" << n << " k=" << k;
        }
    }
}

TEST_F(UnreducedDDTdZddTest, Combination_MatchesTdZddDdStructure) {
    Combination spec(10, 4);
    UnreducedDD u = UnreducedDD::construct(spec);

    tdzdd::DdStructure<2> dd(spec);
    // Unreduced shape: same number of non-terminal nodes.
    EXPECT_EQ(u.raw_size(), dd.size());
    EXPECT_EQ(walk_count(u), dd.size());

    dd.zddReduce();
    EXPECT_EQ(u.reduce_as_zdd().exact_count().to_string(), dd.zddCardinality());
}

TEST_F(UnreducedDDTdZddTest, Combination_CreatesVariables) {
    EXPECT_EQ(bddvarused(), 0u);
    UnreducedDD u = UnreducedDD::construct(Combination(5, 2));
    EXPECT_EQ(bddvarused(), 5u);
    // Top node is at KyotoDD level 5.
    EXPECT_EQ(bddlevofvar(u.top()), 5u);
}

TEST_F(UnreducedDDTdZddTest, Combination_DoesNotAddVariablesWhenEnough) {
    bddnewvar(10);
    UnreducedDD u = UnreducedDD::construct(Combination(5, 2));
    EXPECT_EQ(bddvarused(), 10u);
    EXPECT_EQ(bddlevofvar(u.top()), 5u);
}

TEST_F(UnreducedDDTdZddTest, Stateless_AllSubsets) {
    UnreducedDD u = UnreducedDD::construct(AllSubsets(6));
    EXPECT_EQ(u.raw_size(), 6u);
    EXPECT_TRUE(u.reduce_as_bdd().is_one());
    EXPECT_EQ(u.reduce_as_zdd().card(), 64u);
}

TEST_F(UnreducedDDTdZddTest, RootTerminal_Zero) {
    // k > n: get_child rejects immediately, but the root is still level n.
    // Use a spec whose root itself is terminal.
    struct RootZero : public tdzdd::StatelessDdSpec<RootZero, 2> {
        int getRoot() const { return 0; }
        int getChild(int, int) const { return 0; }
    };
    struct RootOne : public tdzdd::StatelessDdSpec<RootOne, 2> {
        int getRoot() const { return -1; }
        int getChild(int, int) const { return 0; }
    };
    UnreducedDD z = UnreducedDD::construct(RootZero());
    UnreducedDD o = UnreducedDD::construct(RootOne());
    EXPECT_TRUE(z.is_zero());
    EXPECT_TRUE(o.is_one());
    EXPECT_EQ(bddvarused(), 0u);
}

TEST_F(UnreducedDDTdZddTest, Combination_Infeasible_GivesZero) {
    UnreducedDD u = UnreducedDD::construct(Combination(4, 6));
    EXPECT_TRUE(u.reduce_as_zdd().is_zero());
}
