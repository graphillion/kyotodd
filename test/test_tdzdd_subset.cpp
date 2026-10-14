#include <gtest/gtest.h>
#include "tdzdd_test_specs.h"
#include <random>

// Tests for UnreducedDD::zdd_subset / ZDD::subset (TdZdd's zddSubset).

class TdZddSubsetTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

// Random automaton spec over `S` keys with an accumulated cost. With
// use_merge, states compare equal by key only and mergeStates keeps the
// smaller cost (order-independent), so results are comparable with TdZdd.
struct KC { int key; int cost; };
class RandomSpec : public tdzdd::DdSpec<RandomSpec, KC, 2> {
    struct Outcome { int level; int key; };
    int n;
    int S;
    bool use_merge;
    std::vector<std::vector<std::vector<Outcome> > > tab;   // [level][key][value]
    std::vector<int> w;
public:
    RandomSpec(int n, int S, unsigned seed, bool use_merge, int skip_pct, int term_pct)
        : n(n), S(S), use_merge(use_merge) {
        std::mt19937 rng(seed);
        tab.assign(n + 1, std::vector<std::vector<Outcome> >(S, std::vector<Outcome>(2)));
        w.assign(n + 1, 0);
        for (int L = 1; L <= n; ++L) {
            w[L] = static_cast<int>(rng() % 3);
            for (int k = 0; k < S; ++k) for (int v = 0; v < 2; ++v) {
                Outcome o;
                int r = static_cast<int>(rng() % 100);
                if (L == 1 || r < term_pct) {
                    o.level = (rng() % 2) ? -1 : 0; o.key = 0;
                } else if (r < term_pct + skip_pct && L >= 3) {
                    o.level = 1 + static_cast<int>(rng() % (L - 1)); o.key = static_cast<int>(rng() % S);
                } else {
                    o.level = L - 1; o.key = static_cast<int>(rng() % S);
                }
                tab[L][k][v] = o;
            }
        }
    }
    int getRoot(KC& st) const { st.key = 0; st.cost = 0; return n; }
    int getChild(KC& st, int level, int value) const {
        const Outcome& o = tab[level][st.key][value];
        st.cost += value * w[level];
        st.key = o.key;
        return o.level;
    }
    size_t hashCode(KC const& s) const {
        return use_merge ? static_cast<size_t>(s.key)
                         : static_cast<size_t>(s.key) * 1000003u + static_cast<size_t>(s.cost);
    }
    bool equalTo(KC const& a, KC const& b) const {
        return use_merge ? a.key == b.key : (a.key == b.key && a.cost == b.cost);
    }
    int mergeStates(KC& a, KC& b) const {
        if (!use_merge || a.cost == b.cost) return 0;
        return (a.cost < b.cost) ? 2 : 1;
    }
};

// Family of all sets of {1..n} whose size is in [lo, hi].
static ZDD size_range(int n, int lo, int hi) {
    ZDD r(0);
    for (int k = lo; k <= hi; ++k) r = r + ZDD::combination(static_cast<bddvar>(n), static_cast<bddvar>(k));
    return r;
}

// =============================================================
// Basic semantics: subset == intersection with the spec's ZDD
// =============================================================

TEST_F(TdZddSubsetTest, PowerSet_EqualsConstruct) {
    for (int n = 1; n <= 7; ++n) {
        bddnewvar(n);
        ZDD all = ZDD::power_set(static_cast<bddvar>(n));
        for (int k = 0; k <= n; ++k) {
            ZDD got = all.subset(Combination(n, k));
            ZDD expected = UnreducedDD::construct(Combination(n, k)).reduce_as_zdd();
            EXPECT_EQ(got.id(), expected.id()) << "n=" << n << " k=" << k;
            EXPECT_EQ(got.card(), binom(n, k));
        }
    }
}

TEST_F(TdZddSubsetTest, Family_EqualsIntersection) {
    const int n = 7;
    bddnewvar(n);
    ZDD fam = size_range(n, 2, 4);
    for (int k = 0; k <= n; ++k) {
        ZDD got = fam.subset(Combination(n, k));
        ZDD expected = fam & UnreducedDD::construct(Combination(n, k)).reduce_as_zdd();
        EXPECT_EQ(got.id(), expected.id()) << "k=" << k;
    }
    // Disjoint: nothing survives.
    EXPECT_TRUE(size_range(n, 0, 1).subset(Combination(n, 3)).is_zero());
}

TEST_F(TdZddSubsetTest, RandomFamily_EqualsIntersection) {
    std::mt19937 rng(12345);
    for (unsigned seed = 1; seed <= 150; ++seed) {
        BDD_Init(1024, UINT64_MAX);
        int n = 2 + seed % 8;
        bddnewvar(n);
        ZDD fam = ZDD::random_family(static_cast<bddvar>(n), rng);
        RandomSpec spec(n, 1 + seed % 3, seed, false, 30, 15);
        ZDD got = fam.subset(spec);
        ZDD expected = fam & UnreducedDD::construct(spec).reduce_as_zdd();
        EXPECT_EQ(got.id(), expected.id()) << "seed=" << seed;
    }
}

TEST_F(TdZddSubsetTest, SpecThatSkipsLevels_EqualsIntersection) {
    bddnewvar(4);
    ZDD all = ZDD::power_set(4);
    // Skip3 / Skip4 contain level-skipping edges (skipped variable = 0).
    EXPECT_EQ(all.subset(Skip3()).id(), UnreducedDD::construct(Skip3()).reduce_as_zdd().id());
    EXPECT_EQ(all.subset(Skip4()).id(), UnreducedDD::construct(Skip4()).reduce_as_zdd().id());
    ZDD fam = size_range(4, 1, 2);
    EXPECT_EQ(fam.subset(Skip4()).id(),
              (fam & UnreducedDD::construct(Skip4()).reduce_as_zdd()).id());
}

// =============================================================
// Comparison with TdZdd's own zddSubset
// =============================================================

TEST_F(TdZddSubsetTest, MatchesTdZddZddSubset) {
    int nontrivial = 0;
    for (unsigned seed = 1; seed <= 200; ++seed) {
        BDD_Init(1024, UINT64_MAX);
        int n = 2 + seed % 8;
        bddnewvar(n);
        RandomSpec fam_spec(n, 1 + seed % 3, seed * 7 + 1, false, 20, 10);
        bool merge = (seed % 2 == 0);
        RandomSpec sub_spec(n, 1 + (seed / 3) % 4, seed * 13 + 5, merge, 30, 15);

        // TdZdd: build the family, reduce, subset, reduce.
        tdzdd::DdStructure<2> dd(fam_spec);
        dd.zddReduce();
        dd.zddSubset(sub_spec);
        dd.zddReduce();
        std::unordered_map<uint64_t, UnreducedDD> memo;
        ZDD expected = from_tdzdd(dd, dd.root(), memo).reduce_as_zdd();

        // KyotoDD: same pipeline on a reduced ZDD input.
        ZDD fam = UnreducedDD::construct(fam_spec).reduce_as_zdd();
        ZDD got = fam.subset(sub_spec);
        EXPECT_EQ(got.id(), expected.id()) << "seed=" << seed;
        EXPECT_EQ(got.exact_count().to_string(), dd.zddCardinality()) << "seed=" << seed;
        if (!got.is_zero()) ++nontrivial;
    }
    EXPECT_GT(nontrivial, 50);
}

TEST_F(TdZddSubsetTest, MergeStates_MatchesTdZdd) {
    // KeepExtreme merges every state at a level; on a subset the merge
    // happens per input node, so compare with TdZdd directly.
    for (int n = 1; n <= 6; ++n) {
        for (int lo = 0; lo <= n; ++lo) {
            BDD_Init(1024, UINT64_MAX);
            bddnewvar(n);
            // Family: sets of size >= lo, on both sides.
            ZDD fam = size_range(n, lo, n);
            struct SizeAtLeast : public tdzdd::DdSpec<SizeAtLeast, int, 2> {
                int n, lo;
                SizeAtLeast(int n, int lo) : n(n), lo(lo) {}
                int getRoot(int& s) const { s = 0; return n; }
                int getChild(int& s, int level, int v) const {
                    s += v;
                    if (--level == 0) return (s >= lo) ? -1 : 0;
                    return level;
                }
            };
            tdzdd::DdStructure<2> tf(SizeAtLeast(n, lo));
            tf.zddReduce();
            ASSERT_EQ(tf.zddCardinality(), fam.exact_count().to_string());

            for (int keep_max = 0; keep_max < 2; ++keep_max) {
                tdzdd::DdStructure<2> t = tf;
                ZDD got;
                if (keep_max) { t.zddSubset(KeepExtreme<true>(n));  got = fam.subset(KeepExtreme<true>(n)); }
                else          { t.zddSubset(KeepExtreme<false>(n)); got = fam.subset(KeepExtreme<false>(n)); }
                t.zddReduce();
                std::unordered_map<uint64_t, UnreducedDD> memo;
                ZDD expected = from_tdzdd(t, t.root(), memo).reduce_as_zdd();
                EXPECT_EQ(got.id(), expected.id()) << "n=" << n << " lo=" << lo << " max=" << keep_max;
            }
        }
    }
}

// =============================================================
// Variables outside the spec's range, level_offset
// =============================================================

TEST_F(TdZddSubsetTest, OutOfRangeVariables_ForcedToZero) {
    // Family over 6 variables; the spec covers KyotoDD levels 2..4
    // (root 3, offset 1). Sets containing variables at levels 1, 5, 6
    // must be dropped: result == family & construct(spec, offset 1).
    bddnewvar(6);
    ZDD all = ZDD::power_set(6);
    ZDD got = all.subset(Combination(3, 2), 1);
    ZDD expected = UnreducedDD::construct(Combination(3, 2), SkipPadding::None, 1).reduce_as_zdd();
    EXPECT_EQ(got.id(), expected.id());
    EXPECT_EQ(got.card(), 3u);
    for (bddvar lev = 1; lev <= 6; ++lev) {
        bool inside = (lev >= 2 && lev <= 4);
        EXPECT_EQ(got.onset0(bddvaroflev(lev)).is_zero(), !inside) << "lev=" << lev;
    }

    // Every set contains the top variable (level 6): nothing survives.
    ZDD top_only = all.onset(bddvaroflev(6));
    EXPECT_TRUE(top_only.subset(Combination(3, 2), 1).is_zero());
    // Every set contains the bottom variable (level 1): nothing survives.
    ZDD bot_only = all.onset(bddvaroflev(1));
    EXPECT_TRUE(bot_only.subset(Combination(3, 2), 1).is_zero());

    // A family restricted to the spec's range is unaffected by the
    // surrounding variables.
    ZDD fam = size_range(6, 1, 3);
    EXPECT_EQ(fam.subset(Combination(3, 2), 1).id(), (fam & expected).id());
}

TEST_F(TdZddSubsetTest, LevelOffset_NegativeThrows) {
    bddnewvar(3);
    EXPECT_THROW(ZDD::power_set(3).subset(Combination(3, 1), -1), std::invalid_argument);
    EXPECT_THROW(UnreducedDD::wrap_raw(ZDD::power_set(3)).zdd_subset(Combination(3, 1), SkipPadding::None, -1),
                 std::invalid_argument);
}

TEST_F(TdZddSubsetTest, SpecAboveAllVariables_NoNewVariables) {
    // Spec root level 5 but only 3 variables exist: the spec's upper
    // levels are forced to 0 by following its 0-edges; no variable is
    // created.
    bddnewvar(3);
    ZDD all = ZDD::power_set(3);
    ZDD got = all.subset(Combination(5, 1));
    EXPECT_EQ(bddvarused(), 3u);
    EXPECT_EQ(got.card(), 3u);   // {1}, {2}, {3}
    EXPECT_TRUE(all.subset(Combination(5, 4)).is_zero());
}

// =============================================================
// Terminals and degenerate inputs
// =============================================================

TEST_F(TdZddSubsetTest, Terminals) {
    bddnewvar(3);
    struct RootOne : public tdzdd::StatelessDdSpec<RootOne, 2> {
        int getRoot() const { return -1; }
        int getChild(int, int) const { return 0; }
    };
    struct RootZero : public tdzdd::StatelessDdSpec<RootZero, 2> {
        int getRoot() const { return 0; }
        int getChild(int, int) const { return 0; }
    };
    ZDD all = ZDD::power_set(3);
    ZDD empty_set_only = ZDD(1);           // {∅}
    ZDD zero = ZDD(0);

    // Spec accepting only ∅.
    EXPECT_EQ(all.subset(RootOne()).id(), empty_set_only.id());
    EXPECT_EQ(all.subset(Combination(3, 0)).id(), empty_set_only.id());
    EXPECT_TRUE(all.subset(RootZero()).is_zero());

    // {∅} input.
    EXPECT_EQ(empty_set_only.subset(Combination(3, 0)).id(), empty_set_only.id());
    EXPECT_TRUE(empty_set_only.subset(Combination(3, 1)).is_zero());
    EXPECT_EQ(empty_set_only.subset(RootOne()).id(), empty_set_only.id());

    // Empty family input.
    EXPECT_TRUE(zero.subset(Combination(3, 0)).is_zero());
    EXPECT_TRUE(zero.subset(RootOne()).is_zero());

    // Family without ∅ against a spec accepting only ∅.
    EXPECT_TRUE(size_range(3, 1, 3).subset(RootOne()).is_zero());
}

TEST_F(TdZddSubsetTest, NullInput_Throws) {
    bddnewvar(2);
    ZDD null_zdd(-1);
    EXPECT_THROW(null_zdd.subset(Combination(2, 1)), std::invalid_argument);
}

TEST_F(TdZddSubsetTest, BadChildLevel_Throws) {
    bddnewvar(3);
    EXPECT_THROW(ZDD::power_set(3).subset(BadLevel()), std::logic_error);
}

// =============================================================
// UnreducedDD input, padding, resources
// =============================================================

TEST_F(TdZddSubsetTest, UnreducedInput_WithDeadAndDuplicateNodes) {
    // construct(KeepMin) leaves dead nodes (both children 0); subset it
    // and also subset an UnreducedDD with duplicate (non-canonical) nodes.
    const int n = 5;
    bddnewvar(n);
    UnreducedDD u = UnreducedDD::construct(KeepExtreme<false>(n));
    ASSERT_EQ(u.raw_size(), static_cast<size_t>(2 * n - 1));
    ZDD got = u.zdd_subset(Combination(n, 0)).reduce_as_zdd();
    EXPECT_EQ(got.id(), (u.reduce_as_zdd() & UnreducedDD::construct(Combination(n, 0)).reduce_as_zdd()).id());
    EXPECT_EQ(got.card(), 1u);
    EXPECT_TRUE(u.zdd_subset(Combination(n, 1)).reduce_as_zdd().is_zero());

    // Two structurally identical, unshared subtrees.
    bddvar v1 = bddvaroflev(1), v2 = bddvaroflev(2);
    UnreducedDD leaf_a = UnreducedDD::getnode(v1, UnreducedDD(1), UnreducedDD(1));
    UnreducedDD leaf_b = UnreducedDD::getnode(v1, UnreducedDD(1), UnreducedDD(1));
    UnreducedDD dup = UnreducedDD::getnode(v2, leaf_a, leaf_b);   // power set of {1,2}
    for (int k = 0; k <= 2; ++k) {
        EXPECT_EQ(dup.zdd_subset(Combination(2, k)).reduce_as_zdd().card(), binom(2, k)) << "k=" << k;
    }
}

TEST_F(TdZddSubsetTest, ComplementEdgeInput) {
    // Families that contain ∅ use complement edges in KyotoDD's ZDD.
    bddnewvar(5);
    ZDD fam = size_range(5, 0, 5) - ZDD::combination(5, 2);   // all but size 2; contains ∅
    for (int k = 0; k <= 5; ++k) {
        ZDD got = fam.subset(Combination(5, k));
        EXPECT_EQ(got.card(), (k == 2) ? 0u : binom(5, k)) << "k=" << k;
    }
    // operator~ toggles membership of the empty set (complement edge on the root).
    ZDD fam2 = ~ZDD::combination(5, 1);   // singletons plus ∅
    EXPECT_EQ(fam2.card(), 6u);
    EXPECT_EQ(fam2.subset(Combination(5, 1)).card(), 5u);
    EXPECT_EQ(fam2.subset(Combination(5, 0)).card(), 1u);
    EXPECT_EQ(fam2.subset(Combination(5, 2)).card(), 0u);
}

TEST_F(TdZddSubsetTest, Padding) {
    bddnewvar(4);
    ZDD fam = size_range(4, 1, 2);
    UnreducedDD none = UnreducedDD::wrap_raw(fam).zdd_subset(Skip4());
    UnreducedDD zpad = UnreducedDD::wrap_raw(fam).zdd_subset(Skip4(), SkipPadding::Zdd);
    UnreducedDD bpad = UnreducedDD::wrap_raw(fam).zdd_subset(Skip4(), SkipPadding::Bdd);
    EXPECT_FALSE(is_quasi_reduced(none));
    EXPECT_TRUE(is_quasi_reduced(zpad));
    EXPECT_TRUE(is_quasi_reduced(bpad));
    ZDD z = none.reduce_as_zdd();
    EXPECT_EQ(zpad.reduce_as_zdd().id(), z.id());
    // Zdd padding fixes the skipped variables to 0, so the BDD
    // interpretation is the characteristic function of the family.
    EXPECT_EQ(zpad.reduce_as_bdd().id(), z.to_bdd(4).id());
    // Bdd padding: skipped variables are don't-care in the BDD.
    EXPECT_EQ(bpad.reduce_as_bdd().id(), none.reduce_as_bdd().id());
}

TEST_F(TdZddSubsetTest, States_AreDestructed) {
    bddnewvar(8);
    ZDD fam = size_range(8, 2, 5);
    Counted::live() = 0;
    {
        ZDD got = fam.subset(CountedCombination(8, 3));
        EXPECT_EQ(got.card(), binom(8, 3));
    }
    EXPECT_EQ(Counted::live(), 0);
}

TEST_F(TdZddSubsetTest, States_AreDestructedWhenSpecThrows) {
    bddnewvar(8);
    ZDD fam = ZDD::power_set(8);
    Counted::live() = 0;
    EXPECT_THROW(fam.subset(CountedCombination(8, 3, 4)), std::runtime_error);
    EXPECT_EQ(Counted::live(), 0);
}

TEST_F(TdZddSubsetTest, DeepChain_NoRecursion) {
    const int n = 20000;
    bddnewvar(n);
    ZDD all = ZDD::power_set(static_cast<bddvar>(n));
    ZDD got = all.subset(AllSubsets(n));
    EXPECT_EQ(got.id(), all.id());
}
