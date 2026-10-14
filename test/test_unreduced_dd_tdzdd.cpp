#include <gtest/gtest.h>
#include "bdd.h"
#include "unreduced_dd_tdzdd.h"
#include <tdzdd/DdStructure.hpp>
#include <unordered_map>

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

// Keep only the path with the minimum (KeepMax: maximum) number of 1s.
// All states at a level compare equal; mergeStates decides which to keep.
// Every path of length n is accepted, so without merging the result
// would be the constant-1 function.
template<bool KeepMax>
class KeepExtreme : public tdzdd::DdSpec<KeepExtreme<KeepMax>, int, 2> {
    int const n;
public:
    explicit KeepExtreme(int n) : n(n) {}
    int getRoot(int& state) const { state = 0; return n; }
    int getChild(int& state, int level, int value) const {
        state += value;
        return (level == 1) ? -1 : level - 1;
    }
    size_t hashCode(int const&) const { return 0; }
    bool equalTo(int const&, int const&) const { return true; }
    int mergeStates(int& s1, int& s2) const {
        if (s1 == s2) return 0;
        bool keep_first = KeepMax ? (s1 > s2) : (s1 < s2);
        return keep_first ? 2 : 1;
    }
};

// Combination written as a PodArrayDdSpec with a one-element array.
class PodCombination : public tdzdd::PodArrayDdSpec<PodCombination, int, 2> {
    int const n;
    int const k;
public:
    PodCombination(int n, int k) : n(n), k(k) { setArraySize(1); }
    int getRoot(int* a) const { a[0] = 0; return n; }
    int getChild(int* a, int level, int value) const {
        a[0] += value;
        if (--level == 0) return (a[0] == k) ? -1 : 0;
        if (a[0] > k || a[0] + level < k) return 0;
        return level;
    }
};

// State that counts live instances, to check construct/destruct balance.
struct Counted {
    static int live;
    int v;
    Counted() : v(0) { ++live; }
    Counted(const Counted& o) : v(o.v) { ++live; }
    ~Counted() { --live; }
    bool operator==(const Counted& o) const { return v == o.v; }
};
int Counted::live = 0;

class CountedCombination : public tdzdd::DdSpec<CountedCombination, Counted, 2> {
    int const n;
    int const k;
    int const throw_at;   // level at which getChild throws (0 = never)
public:
    CountedCombination(int n, int k, int throw_at = 0)
        : n(n), k(k), throw_at(throw_at) {}
    int getRoot(Counted& s) const { s.v = 0; return n; }
    int getChild(Counted& s, int level, int value) const {
        if (level == throw_at) throw std::runtime_error("boom");
        s.v += value;
        if (--level == 0) return (s.v == k) ? -1 : 0;
        if (s.v > k || s.v + level < k) return 0;
        return level;
    }
    size_t hashCode(Counted const& s) const { return static_cast<size_t>(s.v); }
};

// State is a std::string of the chosen bits: every path has a distinct
// state (no sharing), and the state must survive arena growth without
// being relocated.
class StringPaths : public tdzdd::DdSpec<StringPaths, std::string, 2> {
    int const n;
    int const k;
public:
    StringPaths(int n, int k) : n(n), k(k) {}
    int getRoot(std::string& s) const { s.clear(); return n; }
    int getChild(std::string& s, int level, int value) const {
        s.push_back(value ? '1' : '0');
        if (--level == 0) {
            int ones = 0;
            for (size_t i = 0; i < s.size(); ++i) ones += (s[i] == '1');
            return (ones == k) ? -1 : 0;
        }
        return level;
    }
    size_t hashCode(std::string const& s) const { return std::hash<std::string>()(s); }
};

// getChild returns the current level (not lower): must be rejected.
class BadLevel : public tdzdd::StatelessDdSpec<BadLevel, 2> {
public:
    int getRoot() const { return 3; }
    int getChild(int level, int) const { return level; }
};

// Order-dependent merging: states compare equal by `key` only, and
// mergeStates always keeps the state that was presented FIRST (return 2).
// Which state survives therefore depends on the order in which equal
// states are visited, so the result must match tdzdd::DdBuilder's order
// exactly. Transitions are a fixed pseudo-random table so that paths with
// the same key carry different costs.
struct KeyCost { int key; int cost; };
class KeepFirst : public tdzdd::DdSpec<KeepFirst, KeyCost, 2> {
    int const n;
    int const nkeys;
    unsigned seed;
    // (level, key, value) -> (next level, next key, cost added)
    int next_level(int level, int key, int value) const {
        unsigned h = mix(level, key, value);
        if (level == 1) return (h % 2) ? -1 : 0;
        if (h % 10 == 0) return (h / 10 % 2) ? -1 : 0;      // terminal
        if (level >= 3 && h % 10 < 3) return 1 + (int)(h / 10 % (level - 1)); // skip
        return level - 1;
    }
    int next_key(int level, int key, int value) const {
        return (int)(mix(level, key, value) / 100 % nkeys);
    }
    int add_cost(int level, int key, int value) const {
        return (int)(mix(level, key, value) / 1000 % 3);
    }
    unsigned mix(int level, int key, int value) const {
        unsigned h = seed ^ (unsigned)level * 2654435761u;
        h ^= (unsigned)key * 40503u + (unsigned)value * 97u;
        h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
        return h;
    }
public:
    KeepFirst(int n, int nkeys, unsigned seed) : n(n), nkeys(nkeys), seed(seed) {}
    int getRoot(KeyCost& s) const { s.key = 0; s.cost = 0; return n; }
    int getChild(KeyCost& s, int level, int value) const {
        int lv = next_level(level, s.key, value);
        s.cost += add_cost(level, s.key, value);
        s.key = next_key(level, s.key, value);
        return lv;
    }
    size_t hashCode(KeyCost const& s) const { return (size_t)s.key; }
    bool equalTo(KeyCost const& a, KeyCost const& b) const { return a.key == b.key; }
    int mergeStates(KeyCost& a, KeyCost& b) const {
        return (a.cost == b.cost) ? 0 : 2;   // keep the first one presented
    }
};

// Convert a tdzdd::DdStructure into an UnreducedDD of the same shape
// (level i -> bddvaroflev(i)), for comparing reductions. The result and
// every memoized node are UnreducedDD objects, hence GC-protected.
static UnreducedDD from_tdzdd(const tdzdd::DdStructure<2>& dd, tdzdd::NodeId f,
                              std::unordered_map<uint64_t, UnreducedDD>& memo) {
    if (f.row() == 0) return UnreducedDD(f.col() ? 1 : 0);
    auto it = memo.find(f.code());
    if (it != memo.end()) return it->second;
    UnreducedDD lo = from_tdzdd(dd, dd.child(f, 0), memo);
    UnreducedDD hi = from_tdzdd(dd, dd.child(f, 1), memo);
    UnreducedDD r = UnreducedDD::getnode(bddvaroflev(f.row()), lo, hi);
    memo.insert(std::make_pair(f.code(), r));
    return r;
}

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

TEST_F(UnreducedDDTdZddTest, LevelOffset_NegativeThrows) {
    EXPECT_THROW(UnreducedDD::construct(Combination(3, 1), SkipPadding::None, -1),
                 std::invalid_argument);
}

TEST_F(UnreducedDDTdZddTest, States_AreDestructed) {
    Counted::live = 0;
    {
        UnreducedDD u = UnreducedDD::construct(CountedCombination(8, 3));
        EXPECT_EQ(u.reduce_as_zdd().card(), binom(8, 3));
    }
    EXPECT_EQ(Counted::live, 0);
}

TEST_F(UnreducedDDTdZddTest, States_AreDestructedWhenSpecThrows) {
    Counted::live = 0;
    EXPECT_THROW(UnreducedDD::construct(CountedCombination(8, 3, 4)),
                 std::runtime_error);
    EXPECT_EQ(Counted::live, 0);
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
