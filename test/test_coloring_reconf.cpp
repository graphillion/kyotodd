#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "bdd.h"
#include "../app/coloring_reconf/coloring_reconf.hpp"

using namespace kyotodd;
using namespace coloring_reconf;

namespace {

// Triangle (1,2,3) plus a pendant vertex 4 attached to 1, k = 3.
// The triangle is frozen, so only v4 can ever move.
const char* kSmall =
    "c triangle + pendant\n"
    "4 4 3\n"
    "1 2\n"
    "2 3\n"
    "1 3\n"
    "1 4\n"
    "s 0 1 2 1\n"
    "t 0 1 2 2\n";

// Path 1-2-3-4, k = 3.
const char* kPath4 =
    "4 3 3\n"
    "1 2\n"
    "2 3\n"
    "3 4\n"
    "s 0 1 0 1\n"
    "t 2 0 2 0\n";

Instance parse(const char* text) {
    Instance inst;
    std::istringstream in(text);
    std::ostringstream err;
    EXPECT_TRUE(read_instance(in, inst, err)) << err.str();
    return inst;
}

}  // namespace

class ColoringReconfTest : public ::testing::Test {
protected:
    void SetUp() override {
        // build_solution_space() requires a fresh DD variable pool.
        BDD::init(1024, 1024 * 1024);
    }
};

// ============================================================
//  Input parsing
// ============================================================

TEST_F(ColoringReconfTest, ParseValidInstance) {
    Instance inst = parse(kSmall);
    EXPECT_EQ(inst.n, 4);
    EXPECT_EQ(inst.k, 3);
    ASSERT_EQ(inst.edges.size(), 4u);
    EXPECT_EQ(inst.edges[0].first, 1);
    EXPECT_EQ(inst.edges[0].second, 2);
    EXPECT_EQ(inst.start, std::vector<int>({0, 1, 2, 1}));
    EXPECT_EQ(inst.goal, std::vector<int>({0, 1, 2, 2}));
}

TEST_F(ColoringReconfTest, ParseAcceptsGoalBeforeStart) {
    Instance inst = parse(
        "2 1 2\n"
        "1 2\n"
        "t 1 0\n"
        "s 0 1\n");
    EXPECT_EQ(inst.start, std::vector<int>({0, 1}));
    EXPECT_EQ(inst.goal, std::vector<int>({1, 0}));
}

TEST_F(ColoringReconfTest, ParseIgnoresSelfLoop) {
    Instance inst = parse(
        "2 2 2\n"
        "1 1\n"
        "1 2\n"
        "s 0 1\n"
        "t 1 0\n");
    ASSERT_EQ(inst.edges.size(), 1u);
    EXPECT_EQ(inst.edges[0].first, 1);
    EXPECT_EQ(inst.edges[0].second, 2);
}

TEST_F(ColoringReconfTest, ParseRejectsBadInput) {
    struct Case {
        const char* text;
        const char* why;
    };
    const Case cases[] = {
        {"1 0 1\ns 0\nt 0\n", "k must be at least 2"},
        {"0 0 2\ns\nt\n", "n must be positive"},
        {"2 1 2\n1 3\ns 0 1\nt 1 0\n", "vertex out of range"},
        {"2 1 2\n1 2\ns 0 1\n", "missing t line"},
        {"2 1 2\n1 2\ns 0 1\nt 1\n", "too few colors on t"},
        {"2 1 2\n1 2\ns 0 1\nt 1 0 0\n", "too many colors on t"},
        {"2 1 2\n1 2\ns 0 2\nt 1 0\n", "color out of range"},
        {"2 1 2\n1 2\ns 0 1\ns 1 0\n", "duplicate s line"},
        {"2 1 2\n1 2\nx 0 1\nt 1 0\n", "unknown marker"},
    };
    for (std::size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        Instance inst;
        std::istringstream in(cases[i].text);
        std::ostringstream err;
        EXPECT_FALSE(read_instance(in, inst, err)) << cases[i].why;
    }
}

// ============================================================
//  Proper-coloring check
// ============================================================

TEST_F(ColoringReconfTest, IsProper) {
    Instance inst = parse(kSmall);
    EXPECT_TRUE(is_proper(inst, inst.start));
    EXPECT_TRUE(is_proper(inst, inst.goal));
    // v1 and v2 are adjacent.
    EXPECT_FALSE(is_proper(inst, std::vector<int>({0, 0, 2, 1})));
    // Wrong length.
    EXPECT_FALSE(is_proper(inst, std::vector<int>({0, 1, 2})));
}

// ============================================================
//  Solution space
// ============================================================

TEST_F(ColoringReconfTest, SolutionSpaceCounts) {
    Instance inst = parse(kSmall);
    MVZDD space = build_solution_space(inst);
    // 3! ways to color the triangle, times 2 colors for the pendant.
    EXPECT_EQ(space.exact_count(), bigint::BigInt(12));
    EXPECT_TRUE(space.contains(inst.start));
    EXPECT_TRUE(space.contains(inst.goal));
    EXPECT_FALSE(space.contains(std::vector<int>({0, 0, 2, 1})));
}

TEST_F(ColoringReconfTest, SolutionSpaceMatchesChromaticPolynomial) {
    // 5-cycle with k = 3: (k-1)^n + (-1)^n (k-1) = 2^5 - 2 = 30.
    Instance inst = parse(
        "5 5 3\n"
        "1 2\n"
        "2 3\n"
        "3 4\n"
        "4 5\n"
        "5 1\n"
        "s 2 1 0 1 0\n"
        "t 2 0 2 1 0\n");
    MVZDD space = build_solution_space(inst);
    EXPECT_EQ(space.exact_count(), bigint::BigInt(30));
}

TEST_F(ColoringReconfTest, SolutionSpaceEmptyWhenNotColorable) {
    // K_4 needs 4 colors.
    Instance inst = parse(
        "4 6 3\n"
        "1 2\n"
        "1 3\n"
        "1 4\n"
        "2 3\n"
        "2 4\n"
        "3 4\n"
        "s 0 1 2 0\n"
        "t 0 1 2 0\n");
    MVZDD space = build_solution_space(inst);
    EXPECT_TRUE(space.is_zero());
}

// ============================================================
//  One-step neighborhood
// ============================================================

TEST_F(ColoringReconfTest, RecolorOneOfSingleton) {
    Instance inst = parse(kPath4);
    MVZDD space = build_solution_space(inst);
    MVZDD single = MVZDD::from_sets(
        MVZDD::zero(space.var_table()),
        std::vector<std::vector<int> >(1, inst.start));

    MVZDD nbr = recolor_one(single);
    // Every vertex can take any of the k-1 other colors, and all the
    // resulting assignments are distinct.
    EXPECT_EQ(nbr.exact_count(), bigint::BigInt(inst.n * (inst.k - 1)));
    // The source assignment itself is not a neighbor of itself.
    EXPECT_FALSE(nbr.contains(inst.start));
    // Both proper and improper neighbors are produced; filtering is the
    // caller's job.
    EXPECT_TRUE(nbr.contains(std::vector<int>({2, 1, 0, 1})));   // proper
    EXPECT_TRUE(nbr.contains(std::vector<int>({1, 1, 0, 1})));   // improper
}

TEST_F(ColoringReconfTest, RecolorOneOfEmptyFamilyIsEmpty) {
    Instance inst = parse(kPath4);
    MVZDD space = build_solution_space(inst);
    MVZDD empty = MVZDD::zero(space.var_table());
    EXPECT_TRUE(recolor_one(empty).is_zero());
}

TEST_F(ColoringReconfTest, RecolorOneIsUnionOverMembers) {
    Instance inst = parse(kPath4);
    MVZDD space = build_solution_space(inst);
    MVZDD base = MVZDD::zero(space.var_table());

    std::vector<std::vector<int> > members;
    members.push_back(std::vector<int>({0, 1, 0, 1}));
    members.push_back(std::vector<int>({2, 0, 2, 0}));
    MVZDD pair = MVZDD::from_sets(base, members);

    MVZDD expected = MVZDD::zero(space.var_table());
    for (std::size_t i = 0; i < members.size(); ++i) {
        expected = expected +
                   recolor_one(MVZDD::from_sets(
                       base, std::vector<std::vector<int> >(1, members[i])));
    }
    EXPECT_TRUE(recolor_one(pair) == expected);
}

// ============================================================
//  BFS
// ============================================================

TEST_F(ColoringReconfTest, ReconfigureReachableInOneStep) {
    Instance inst = parse(kSmall);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> layers;
    EXPECT_EQ(reconfigure(space, inst.start, inst.goal, &layers, NULL), 1);
    ASSERT_EQ(layers.size(), 2u);
    EXPECT_EQ(layers[0].exact_count(), bigint::BigInt(1));
    EXPECT_EQ(layers[1].exact_count(), bigint::BigInt(1));
}

TEST_F(ColoringReconfTest, ReconfigureZeroStepsWhenEqual) {
    Instance inst = parse(kSmall);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> layers;
    EXPECT_EQ(reconfigure(space, inst.start, inst.start, &layers, NULL), 0);
    EXPECT_EQ(layers.size(), 1u);
}

TEST_F(ColoringReconfTest, ReconfigureUnreachableFrozenTriangle) {
    // Same graph as kSmall, but the goal permutes the triangle colors.
    Instance inst = parse(
        "4 4 3\n"
        "1 2\n"
        "2 3\n"
        "1 3\n"
        "1 4\n"
        "s 0 1 2 1\n"
        "t 1 0 2 0\n");
    MVZDD space = build_solution_space(inst);
    ASSERT_TRUE(space.contains(inst.start));
    ASSERT_TRUE(space.contains(inst.goal));
    std::vector<MVZDD> layers;
    EXPECT_EQ(reconfigure(space, inst.start, inst.goal, &layers, NULL), -1);
    // F_0 = {s}, F_1 = {the single pendant move}, F_2 = empty.
    ASSERT_EQ(layers.size(), 3u);
    EXPECT_EQ(layers[1].exact_count(), bigint::BigInt(1));
    EXPECT_TRUE(layers[2].is_zero());
}

TEST_F(ColoringReconfTest, ReconfigureLayerSizesMatchBruteForce) {
    Instance inst = parse(kPath4);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> layers;
    EXPECT_EQ(reconfigure(space, inst.start, inst.goal, &layers, NULL), 4);
    // Verified against an independent brute-force BFS.
    const int expected[] = {1, 4, 5, 4, 4};
    ASSERT_EQ(layers.size(), 5u);
    for (std::size_t i = 0; i < layers.size(); ++i) {
        EXPECT_EQ(layers[i].exact_count(), bigint::BigInt(expected[i]))
            << "layer " << i;
    }
}

TEST_F(ColoringReconfTest, ReconfigureLayersAreDisjoint) {
    Instance inst = parse(kPath4);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> layers;
    ASSERT_EQ(reconfigure(space, inst.start, inst.goal, &layers, NULL), 4);
    for (std::size_t i = 0; i < layers.size(); ++i) {
        EXPECT_TRUE(layers[i].is_subset_family(space)) << "layer " << i;
        for (std::size_t j = i + 1; j < layers.size(); ++j) {
            EXPECT_TRUE(layers[i].is_disjoint(layers[j]))
                << "layers " << i << " and " << j;
        }
    }
}

TEST_F(ColoringReconfTest, ReconfigureWritesProgressLog) {
    Instance inst = parse(kSmall);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> layers;
    std::ostringstream log;
    reconfigure(space, inst.start, inst.goal, &layers, &log);
    EXPECT_NE(log.str().find("step 1"), std::string::npos);
}

// ============================================================
//  Sequence reconstruction
// ============================================================

TEST_F(ColoringReconfTest, BacktrackProducesValidSequence) {
    Instance inst = parse(kPath4);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> layers;
    int steps = reconfigure(space, inst.start, inst.goal, &layers, NULL);
    ASSERT_EQ(steps, 4);

    std::vector<std::vector<int> > sequence;
    ASSERT_TRUE(backtrack(inst.start, inst.goal, layers, &sequence));
    EXPECT_EQ(sequence.size(), static_cast<std::size_t>(steps) + 1);

    std::ostringstream err;
    EXPECT_TRUE(verify_sequence(inst, sequence, &err)) << err.str();
    // Every intermediate coloring must be a member of its own BFS layer.
    for (std::size_t i = 0; i < sequence.size(); ++i) {
        EXPECT_TRUE(layers[i].contains(sequence[i])) << "step " << i;
    }
}

TEST_F(ColoringReconfTest, BacktrackOfZeroStepSequence) {
    Instance inst = parse(kSmall);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> layers;
    ASSERT_EQ(reconfigure(space, inst.start, inst.start, &layers, NULL), 0);

    std::vector<std::vector<int> > sequence;
    ASSERT_TRUE(backtrack(inst.start, inst.start, layers, &sequence));
    ASSERT_EQ(sequence.size(), 1u);
    EXPECT_EQ(sequence[0], inst.start);
}

TEST_F(ColoringReconfTest, VerifySequenceRejectsBadSequences) {
    Instance inst = parse(kPath4);
    std::ostringstream err;

    EXPECT_FALSE(verify_sequence(inst, std::vector<std::vector<int> >(), &err));

    std::vector<std::vector<int> > seq;
    seq.push_back(std::vector<int>({0, 1, 0, 1}));  // = start
    seq.push_back(std::vector<int>({2, 0, 2, 0}));  // = goal, but 4 changes
    EXPECT_FALSE(verify_sequence(inst, seq, &err));

    seq.clear();
    seq.push_back(std::vector<int>({0, 1, 0, 1}));
    seq.push_back(std::vector<int>({1, 1, 0, 1}));  // not proper
    EXPECT_FALSE(verify_sequence(inst, seq, &err));

    seq.clear();
    seq.push_back(std::vector<int>({2, 1, 0, 1}));  // not the start
    EXPECT_FALSE(verify_sequence(inst, seq, &err));
}

TEST_F(ColoringReconfTest, ChangedVertex) {
    std::vector<int> a({0, 1, 0, 1});
    EXPECT_EQ(changed_vertex(a, std::vector<int>({0, 1, 2, 1})), 2);
    EXPECT_EQ(changed_vertex(a, a), -1);
    EXPECT_EQ(changed_vertex(a, std::vector<int>({1, 1, 2, 1})), -1);
}
