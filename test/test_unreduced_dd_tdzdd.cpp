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

// Level-skipping spec on 3 levels (stateless):
//   level 3: lo -> level 1 (skips level 2), hi -> level 2
//   level 2: lo, hi -> level 1
//   level 1: lo -> 0, hi -> 1
class Skip3 : public tdzdd::StatelessDdSpec<Skip3, 2> {
public:
    int getRoot() const { return 3; }
    int getChild(int level, int value) const {
        if (level == 3) return value ? 2 : 1;
        if (level == 2) return 1;
        return value ? -1 : 0;
    }
};

// Level-skipping spec on 4 levels where two parents skip to the same child:
//   level 4: lo -> level 1, hi -> level 3
//   level 3: lo -> level 1, hi -> level 2
//   level 2: lo, hi -> level 1
//   level 1: lo -> 0, hi -> 1
class Skip4 : public tdzdd::StatelessDdSpec<Skip4, 2> {
public:
    int getRoot() const { return 4; }
    int getChild(int level, int value) const {
        if (level == 4) return value ? 3 : 1;
        if (level == 3) return value ? 2 : 1;
        if (level == 2) return 1;
        return value ? -1 : 0;
    }
};

// Edge to the 1-terminal that skips level 1:
//   level 2: lo -> 0, hi -> 1-terminal
class SkipToOne : public tdzdd::StatelessDdSpec<SkipToOne, 2> {
public:
    int getRoot() const { return 2; }
    int getChild(int /*level*/, int value) const { return value ? -1 : 0; }
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

// True iff every edge goes to a terminal or to a node exactly one
// KyotoDD level below (quasi-reduced shape).
static bool is_quasi_reduced(const UnreducedDD& dd) {
    std::unordered_set<bddp> seen;
    std::vector<bddp> stack;
    stack.push_back(dd.id() & ~BDD_COMP_FLAG);
    while (!stack.empty()) {
        bddp f = stack.back(); stack.pop_back();
        if (f & BDD_CONST_FLAG) continue;
        if (!seen.insert(f).second) continue;
        bddvar flev = bddlevofvar(node_var(f));
        for (int b = 0; b < 2; ++b) {
            bddp c = DDBase::raw_child(f, b) & ~BDD_COMP_FLAG;
            if (c & BDD_CONST_FLAG) continue;
            bddvar clev = bddlevofvar(node_var(c));
            if (clev + 1 != flev) return false;
            stack.push_back(c);
        }
    }
    return true;
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

// =============================================================
// Level-skipping edges and SkipPadding
// =============================================================

TEST_F(UnreducedDDTdZddTest, Skip_None_KeepsSkippingEdge) {
    UnreducedDD u = UnreducedDD::construct(Skip3());
    EXPECT_EQ(u.raw_size(), 3u);
    EXPECT_FALSE(is_quasi_reduced(u));
    // root.lo is the level-1 node directly
    EXPECT_EQ(bddlevofvar(u.raw_child0().top()), 1u);
    EXPECT_EQ(bddlevofvar(u.raw_child1().top()), 2u);

    bddvar v1 = bddvaroflev(1);
    EXPECT_EQ(u.reduce_as_bdd().id(), BDDvar(v1).id());
    // ZDD: {1} | {1,3} | {1,2,3}
    EXPECT_EQ(u.reduce_as_zdd().card(), 3u);
}

TEST_F(UnreducedDDTdZddTest, Skip_BddPadding) {
    UnreducedDD u = UnreducedDD::construct(Skip3(), SkipPadding::Bdd);
    EXPECT_EQ(u.raw_size(), 4u);
    EXPECT_TRUE(is_quasi_reduced(u));
    UnreducedDD pad = u.raw_child0();
    EXPECT_EQ(bddlevofvar(pad.top()), 2u);
    EXPECT_EQ(pad.raw_child0().id(), pad.raw_child1().id());

    bddvar v1 = bddvaroflev(1);
    EXPECT_EQ(u.reduce_as_bdd().id(), BDDvar(v1).id());
    // ZDD: {1} | {1,2} | {1,3} | {1,2,3}
    EXPECT_EQ(u.reduce_as_zdd().card(), 4u);
}

TEST_F(UnreducedDDTdZddTest, Skip_ZddPadding) {
    UnreducedDD u = UnreducedDD::construct(Skip3(), SkipPadding::Zdd);
    EXPECT_EQ(u.raw_size(), 4u);
    EXPECT_TRUE(is_quasi_reduced(u));
    UnreducedDD pad = u.raw_child0();
    EXPECT_EQ(bddlevofvar(pad.top()), 2u);
    EXPECT_EQ(pad.raw_child1().id(), bddfalse);

    bddvar v1 = bddvaroflev(1), v2 = bddvaroflev(2), v3 = bddvaroflev(3);
    BDD x1 = BDDvar(v1), x2 = BDDvar(v2), x3 = BDDvar(v3);
    BDD expected = (~x3 & ~x2 & x1) | (x3 & x1);
    EXPECT_EQ(u.reduce_as_bdd().id(), expected.id());
    // Zero-suppression removes the pad node: same ZDD as without padding.
    EXPECT_EQ(u.reduce_as_zdd().id(),
              UnreducedDD::construct(Skip3()).reduce_as_zdd().id());
    EXPECT_EQ(u.reduce_as_zdd().card(), 3u);
}

TEST_F(UnreducedDDTdZddTest, Skip_PadNodesAreShared) {
    UnreducedDD u = UnreducedDD::construct(Skip4(), SkipPadding::Bdd);
    // root, n3, n2, n1 + pad at level 3 and pad at level 2
    EXPECT_EQ(u.raw_size(), 6u);
    EXPECT_TRUE(is_quasi_reduced(u));
    UnreducedDD pad3 = u.raw_child0();
    UnreducedDD n3 = u.raw_child1();
    EXPECT_EQ(bddlevofvar(pad3.top()), 3u);
    // Both the level-3 pad node and n3 skip to n1 via the same level-2 pad.
    EXPECT_EQ(pad3.raw_child0().id(), n3.raw_child0().id());
    EXPECT_EQ(bddlevofvar(n3.raw_child0().top()), 2u);
}

TEST_F(UnreducedDDTdZddTest, Skip_EdgeToOneIsPadded) {
    UnreducedDD none = UnreducedDD::construct(SkipToOne());
    EXPECT_EQ(none.raw_size(), 1u);
    EXPECT_EQ(none.raw_child1().id(), bddtrue);

    UnreducedDD zpad = UnreducedDD::construct(SkipToOne(), SkipPadding::Zdd);
    EXPECT_EQ(zpad.raw_size(), 2u);
    EXPECT_TRUE(is_quasi_reduced(zpad));
    EXPECT_EQ(zpad.raw_child1().raw_child0().id(), bddtrue);
    EXPECT_EQ(zpad.raw_child1().raw_child1().id(), bddfalse);

    UnreducedDD bpad = UnreducedDD::construct(SkipToOne(), SkipPadding::Bdd);
    EXPECT_EQ(bpad.raw_size(), 2u);
    EXPECT_TRUE(is_quasi_reduced(bpad));
    EXPECT_EQ(bpad.raw_child1().raw_child0().id(), bddtrue);
    EXPECT_EQ(bpad.raw_child1().raw_child1().id(), bddtrue);

    bddvar v1 = bddvaroflev(1), v2 = bddvaroflev(2);
    BDD x1 = BDDvar(v1), x2 = BDDvar(v2);
    EXPECT_EQ(bpad.reduce_as_bdd().id(), x2.id());
    EXPECT_EQ(zpad.reduce_as_bdd().id(), (x2 & ~x1).id());
    EXPECT_EQ(bpad.reduce_as_zdd().card(), 2u);   // {2}, {1,2}
    EXPECT_EQ(zpad.reduce_as_zdd().card(), 1u);   // {2}
}

TEST_F(UnreducedDDTdZddTest, Skip_EdgeToZeroIsNotPadded) {
    // Skip3 level 1 lo -> 0: with padding there is still no node below.
    UnreducedDD u = UnreducedDD::construct(SkipToOne(), SkipPadding::Bdd);
    EXPECT_EQ(u.raw_child0().id(), bddfalse);
}
