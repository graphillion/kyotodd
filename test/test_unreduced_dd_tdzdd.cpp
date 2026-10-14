#include <gtest/gtest.h>
#include "tdzdd_test_specs.h"

class UnreducedDDTdZddTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

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

// =============================================================
// merge_states (internal levels and the 1-terminal state)
// =============================================================

// Note on the order: like tdzdd::DdBuilder, equal states at a level are
// visited newest first, so the hi child (larger cost) of a node is
// presented to mergeStates BEFORE its lo child (smaller cost).

TEST_F(UnreducedDDTdZddTest, MergeStates_KeepMin_ForwardsFirstToZero) {
    // Case 1: the first state seen (hi child, larger cost) was already
    // turned into a node; it is forwarded to 0 (dead node with both
    // children 0) and the later, smaller state becomes the live node.
    const int n = 4;
    UnreducedDD u = UnreducedDD::construct(KeepExtreme<false>(n));
    // n live nodes + (n-1) dead nodes
    EXPECT_EQ(u.raw_size(), static_cast<size_t>(2 * n - 1));

    UnreducedDD dead = u.raw_child1();
    EXPECT_FALSE(dead.is_terminal());
    EXPECT_EQ(dead.raw_child0().id(), bddfalse);
    EXPECT_EQ(dead.raw_child1().id(), bddfalse);

    BDD expected = BDD(1);
    for (int lev = 1; lev <= n; ++lev) expected &= ~BDDvar(bddvaroflev(lev));
    EXPECT_EQ(u.reduce_as_bdd().id(), expected.id());
    EXPECT_EQ(u.reduce_as_zdd().card(), 1u);

    KeepExtreme<false> spec(n);
    tdzdd::DdStructure<2> dd(spec);
    dd.zddReduce();
    EXPECT_EQ(dd.zddCardinality(), "1");
}

TEST_F(UnreducedDDTdZddTest, MergeStates_KeepMax_ForwardsSecondToZero) {
    // Case 2 at internal levels and at the 1-terminal: the later state
    // (lo child, smaller cost) is sent straight to 0, so no dead nodes.
    const int n = 4;
    UnreducedDD u = UnreducedDD::construct(KeepExtreme<true>(n));
    EXPECT_EQ(u.raw_size(), static_cast<size_t>(n));   // no dead nodes
    EXPECT_EQ(u.raw_child0().id(), bddfalse);

    BDD expected = BDD(1);
    for (int lev = 1; lev <= n; ++lev) expected &= BDDvar(bddvaroflev(lev));
    EXPECT_EQ(u.reduce_as_bdd().id(), expected.id());
    EXPECT_EQ(u.reduce_as_zdd().card(), 1u);

    KeepExtreme<true> spec(n);
    tdzdd::DdStructure<2> dd(spec);
    dd.zddReduce();
    EXPECT_EQ(dd.zddCardinality(), "1");
}

TEST_F(UnreducedDDTdZddTest, MergeStates_OneTerminal_RewritesEarlierEdges) {
    // n = 1: both children reach the 1-terminal directly. KeepMax must
    // rewrite the earlier lo edge (cost 0) to 0 when hi (cost 1) arrives.
    UnreducedDD u = UnreducedDD::construct(KeepExtreme<true>(1));
    EXPECT_EQ(u.raw_child0().id(), bddfalse);
    EXPECT_EQ(u.raw_child1().id(), bddtrue);

    UnreducedDD v = UnreducedDD::construct(KeepExtreme<false>(1));
    EXPECT_EQ(v.raw_child0().id(), bddtrue);
    EXPECT_EQ(v.raw_child1().id(), bddfalse);
}

TEST_F(UnreducedDDTdZddTest, MergeStates_OrderDependent_MatchesTdZdd) {
    // The processing order of equal states follows tdzdd::DdBuilder
    // (newest first), so a "keep the first" merge yields the same
    // function as TdZdd, for both BDD and ZDD interpretations.
    int checked = 0;
    for (unsigned seed = 1; seed <= 60; ++seed) {
        BDD_Init(1024, UINT64_MAX);
        int n = 3 + seed % 7;
        bddnewvar(n);
        KeepFirst spec(n, 1 + seed % 3, seed);

        tdzdd::DdStructure<2> dd(spec);
        std::unordered_map<uint64_t, UnreducedDD> memo;
        UnreducedDD ref = from_tdzdd(dd, dd.root(), memo);
        BDD b_ref = ref.reduce_as_bdd();
        ZDD z_ref = ref.reduce_as_zdd();

        UnreducedDD u = UnreducedDD::construct(spec);
        EXPECT_EQ(u.reduce_as_bdd().id(), b_ref.id()) << "seed=" << seed;
        EXPECT_EQ(u.reduce_as_zdd().id(), z_ref.id()) << "seed=" << seed;
        if (!b_ref.is_zero() && !b_ref.is_one()) ++checked;
    }
    EXPECT_GT(checked, 10);
}

// =============================================================
// Other spec kinds, offsets, robustness
// =============================================================

TEST_F(UnreducedDDTdZddTest, PodArraySpec_MatchesTdZdd) {
    PodCombination spec(9, 4);
    UnreducedDD u = UnreducedDD::construct(spec);
    EXPECT_EQ(u.reduce_as_zdd().card(), binom(9, 4));

    tdzdd::DdStructure<2> dd(spec);
    EXPECT_EQ(u.raw_size(), dd.size());
}

TEST_F(UnreducedDDTdZddTest, LevelOffset_ShiftsVariables) {
    UnreducedDD u = UnreducedDD::construct(Combination(5, 2), SkipPadding::None, 3);
    EXPECT_EQ(bddvarused(), 8u);
    EXPECT_EQ(bddlevofvar(u.top()), 8u);
    EXPECT_EQ(u.reduce_as_zdd().card(), binom(5, 2));

    // Same spec at offset 0 lives in levels 1..5 and is a different DD.
    UnreducedDD v = UnreducedDD::construct(Combination(5, 2));
    EXPECT_EQ(bddlevofvar(v.top()), 5u);
    EXPECT_NE(u.reduce_as_zdd().id(), v.reduce_as_zdd().id());
}

TEST_F(UnreducedDDTdZddTest, LevelOffset_PaddingCoversOffsetLevels) {
    // SkipToOne at offset 1: spec levels 2,1 -> KyotoDD levels 3,2;
    // KyotoDD level 1 is below the spec's range. The hi edge to the
    // 1-terminal must be padded down to level 1, not just to level 2.
    bddnewvar(3);
    bddvar v1 = bddvaroflev(1), v2 = bddvaroflev(2), v3 = bddvaroflev(3);
    BDD x1 = BDDvar(v1), x2 = BDDvar(v2), x3 = BDDvar(v3);

    UnreducedDD zpad = UnreducedDD::construct(SkipToOne(), SkipPadding::Zdd, 1);
    EXPECT_EQ(zpad.raw_size(), 3u);
    EXPECT_TRUE(is_quasi_reduced(zpad));
    EXPECT_EQ(zpad.reduce_as_bdd().id(), (x3 & ~x2 & ~x1).id());
    EXPECT_EQ(zpad.reduce_as_zdd().card(), 1u);   // {3}

    UnreducedDD bpad = UnreducedDD::construct(SkipToOne(), SkipPadding::Bdd, 1);
    EXPECT_EQ(bpad.raw_size(), 3u);
    EXPECT_TRUE(is_quasi_reduced(bpad));
    EXPECT_EQ(bpad.reduce_as_bdd().id(), x3.id());
    EXPECT_EQ(bpad.reduce_as_zdd().card(), 4u);   // {3},{1,3},{2,3},{1,2,3}

    // Without padding the offset level is simply skipped.
    UnreducedDD none = UnreducedDD::construct(SkipToOne(), SkipPadding::None, 1);
    EXPECT_EQ(none.raw_size(), 1u);
    EXPECT_EQ(none.reduce_as_bdd().id(), x3.id());
    EXPECT_EQ(none.reduce_as_zdd().card(), 1u);

    // A larger offset on a spec whose level-1 nodes reach the 1-terminal.
    UnreducedDD c = UnreducedDD::construct(Combination(5, 2), SkipPadding::Zdd, 3);
    EXPECT_TRUE(is_quasi_reduced(c));
    EXPECT_EQ(c.reduce_as_zdd().card(), binom(5, 2));
    // Pad nodes to the 1-terminal are shared: exactly 3 of them.
    EXPECT_EQ(c.raw_size(),
              UnreducedDD::construct(Combination(5, 2), SkipPadding::None, 3).raw_size() + 3);
}

TEST_F(UnreducedDDTdZddTest, LevelOffset_NegativeThrows) {
    EXPECT_THROW(UnreducedDD::construct(Combination(3, 1), SkipPadding::None, -1),
                 std::invalid_argument);
}

TEST_F(UnreducedDDTdZddTest, States_AreDestructed) {
    Counted::live() = 0;
    {
        UnreducedDD u = UnreducedDD::construct(CountedCombination(8, 3));
        EXPECT_EQ(u.reduce_as_zdd().card(), binom(8, 3));
    }
    EXPECT_EQ(Counted::live(), 0);
}

TEST_F(UnreducedDDTdZddTest, States_AreDestructedWhenSpecThrows) {
    Counted::live() = 0;
    EXPECT_THROW(UnreducedDD::construct(CountedCombination(8, 3, 4)),
                 std::runtime_error);
    EXPECT_EQ(Counted::live(), 0);
}

TEST_F(UnreducedDDTdZddTest, StringState_SurvivesArenaGrowth) {
    const int n = 12, k = 5;
    UnreducedDD u = UnreducedDD::construct(StringPaths(n, k));
    // Every path has its own state: a full binary tree minus pruned leaves.
    EXPECT_EQ(u.raw_size(), static_cast<size_t>((1 << n) - 1));
    EXPECT_EQ(u.reduce_as_zdd().card(), binom(n, k));
}

TEST_F(UnreducedDDTdZddTest, DeepChain_NoRecursion) {
    const int n = 20000;
    UnreducedDD u = UnreducedDD::construct(AllSubsets(n));
    EXPECT_EQ(u.raw_size(), static_cast<size_t>(n));
    EXPECT_EQ(bddvarused(), static_cast<bddvar>(n));
    EXPECT_TRUE(u.reduce_as_bdd().is_one());
}

TEST_F(UnreducedDDTdZddTest, BadChildLevel_Throws) {
    EXPECT_THROW(UnreducedDD::construct(BadLevel()), std::logic_error);
}
