#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "bdd.h"
#include "../app/coloring_reconf/pattern_search.hpp"

using namespace kyotodd;
using namespace coloring_reconf;

namespace {

// Path 1-2-3-4, k = 3.
const char* kPath4 =
    "4 3 3\n"
    "1 2\n"
    "2 3\n"
    "3 4\n"
    "s 0 1 0 1\n"
    "t 2 0 2 0\n";

// Triangle + pendant, k = 3 (every proper coloring uses all 3 colors).
const char* kSmall =
    "4 4 3\n"
    "1 2\n"
    "2 3\n"
    "1 3\n"
    "1 4\n"
    "s 0 1 2 1\n"
    "t 0 1 2 2\n";

Instance parse(const char* text) {
    Instance inst;
    std::istringstream in(text);
    std::ostringstream err;
    EXPECT_TRUE(read_instance(in, inst, err)) << err.str();
    return inst;
}

std::vector<int> pat(int a, int b) {
    std::vector<int> p; p.push_back(a); p.push_back(b); return p;
}
std::vector<int> pat(int a, int b, int c) {
    std::vector<int> p = pat(a, b); p.push_back(c); return p;
}

}  // namespace

class PatternSearchTest : public ::testing::Test {
protected:
    void SetUp() override {
        // build_solution_space() requires a fresh DD variable pool.
        BDD::init(1024, 1024 * 1024);
    }
};

TEST_F(PatternSearchTest, ReachClosureOfFrozenTrianglePendant) {
    Instance inst = parse(kSmall);
    MVZDD space = build_solution_space(inst);
    MVZDD empty = MVZDD::zero(space.var_table());
    MVZDD seed = MVZDD::from_sets(
        empty, std::vector<std::vector<int> >(1, inst.start));

    // The triangle is frozen; only the pendant moves between 2 colors.
    MVZDD reached = reach_closure(space, seed);
    EXPECT_EQ(reached.exact_count(), bigint::BigInt(2));
    EXPECT_TRUE(reached.contains(inst.start));
    EXPECT_TRUE(reached.contains(inst.goal));

    // Closure of the whole space is the space itself.
    EXPECT_TRUE(reach_closure(space, space) == space);
}

TEST_F(PatternSearchTest, ValidatePattern) {
    EXPECT_NO_THROW(validate_pattern(pat(2, 3, 2), 3));
    EXPECT_THROW(validate_pattern(std::vector<int>(), 3),
                 std::invalid_argument);
    EXPECT_THROW(validate_pattern(pat(2, 2), 3), std::invalid_argument);
    EXPECT_THROW(validate_pattern(pat(1, 3), 3), std::invalid_argument);
    EXPECT_THROW(validate_pattern(pat(3, 4), 3), std::invalid_argument);
}

TEST_F(PatternSearchTest, CollapseCountProfile) {
    std::vector<std::vector<int> > seq;
    seq.push_back(std::vector<int>({0, 1, 0, 1}));  // 2 colors
    seq.push_back(std::vector<int>({2, 1, 0, 1}));  // 3 colors
    seq.push_back(std::vector<int>({2, 1, 2, 1}));  // 2 colors
    seq.push_back(std::vector<int>({2, 1, 2, 0}));  // 3 colors
    seq.push_back(std::vector<int>({2, 0, 2, 0}));  // 2 colors
    std::vector<int> prof = collapse_count_profile(seq);
    ASSERT_EQ(prof.size(), 5u);
    EXPECT_EQ(prof[0], 2); EXPECT_EQ(prof[1], 3); EXPECT_EQ(prof[2], 2);

    // Runs of equal counts collapse.
    seq.insert(seq.begin() + 1, std::vector<int>({0, 1, 0, 2}));  // 3 colors
    EXPECT_EQ(collapse_count_profile(seq).size(), 5u);
}

TEST_F(PatternSearchTest, PatternUpDown) {
    Instance inst = parse(kPath4);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> exact = exact_color_count_families(space);

    // 2 -> 3 -> 2: from a 2-coloring, bump one vertex and settle into a
    // different 2-coloring.
    std::vector<std::vector<MVZDD> > layers;
    int steps = pattern_reconfigure(space, exact, pat(2, 3, 2), inst.start,
                                    exact[2], &layers);
    ASSERT_EQ(steps, 2);

    // Extract the reached goal and reconstruct.
    MVZDD hits = layers.back()[2] & exact[2];
    ASSERT_FALSE(hits.is_zero());
    std::vector<int> goal = hits.unrank(0);
    std::vector<std::vector<int> > seq;
    ASSERT_TRUE(backtrack_pattern(inst.start, goal, layers, &seq));

    Instance check = inst;
    check.goal = goal;
    std::ostringstream err;
    EXPECT_TRUE(verify_pattern_sequence(check, seq, pat(2, 3, 2), &err))
        << err.str();
}

TEST_F(PatternSearchTest, PatternGoalSpecificColoring) {
    Instance inst = parse(kPath4);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> exact = exact_color_count_families(space);
    MVZDD empty = MVZDD::zero(space.var_table());

    // s = 0101 and t = 2020 are both 2-colorings; the pattern forces the
    // count to rise to 3 in between exactly once.
    MVZDD goal_f = MVZDD::from_sets(
        empty, std::vector<std::vector<int> >(1, inst.goal));
    std::vector<std::vector<MVZDD> > layers;
    int steps = pattern_reconfigure(space, exact, pat(2, 3, 2), inst.start,
                                    goal_f, &layers);
    ASSERT_GT(steps, 0);

    std::vector<std::vector<int> > seq;
    ASSERT_TRUE(backtrack_pattern(inst.start, inst.goal, layers, &seq));
    std::ostringstream err;
    EXPECT_TRUE(verify_pattern_sequence(inst, seq, pat(2, 3, 2), &err))
        << err.str();
}

TEST_F(PatternSearchTest, PatternImpossible) {
    Instance inst = parse(kSmall);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> exact = exact_color_count_families(space);

    // Every proper coloring of the triangle+pendant uses 3 colors, so a
    // profile that descends to 2 is impossible.
    std::vector<std::vector<MVZDD> > layers;
    EXPECT_EQ(pattern_reconfigure(space, exact, pat(3, 2), inst.start,
                                  exact[2], &layers), -1);

    // Start count mismatch is rejected.
    EXPECT_EQ(pattern_reconfigure(space, exact, pat(2, 3), inst.start,
                                  exact[3], &layers), -1);
}
