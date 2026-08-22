#include <gtest/gtest.h>

#include <sstream>
#include <string>
#include <vector>

#include "bdd.h"
#include "../app/coloring_reconf/color_count.hpp"

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

// Triangle + pendant, k = 3.
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

}  // namespace

class ColorCountTest : public ::testing::Test {
protected:
    void SetUp() override {
        // build_solution_space() requires a fresh DD variable pool.
        BDD::init(1024, 1024 * 1024);
    }
};

TEST_F(ColorCountTest, CountUsedColors) {
    EXPECT_EQ(count_used_colors(std::vector<int>({0, 1, 0, 1})), 2);
    EXPECT_EQ(count_used_colors(std::vector<int>({2, 2, 2})), 1);
    EXPECT_EQ(count_used_colors(std::vector<int>({0, 1, 2, 3})), 4);
    EXPECT_EQ(count_used_colors(std::vector<int>()), 0);
}

TEST_F(ColorCountTest, AvoidColorMatchesEnumeration) {
    Instance inst = parse(kPath4);
    MVZDD space = build_solution_space(inst);
    for (int c = 0; c < inst.k; ++c) {
        MVZDD avoid = avoid_color(space, c);
        // Check by explicit enumeration.
        std::vector<std::vector<int> > all = space.enumerate();
        std::size_t expected = 0;
        for (std::size_t i = 0; i < all.size(); ++i) {
            bool uses = false;
            for (int v = 0; v < inst.n; ++v) {
                if (all[i][v] == c) { uses = true; break; }
            }
            if (!uses) ++expected;
            EXPECT_EQ(!uses, avoid.contains(all[i]));
        }
        EXPECT_EQ(avoid.exact_count(), bigint::BigInt(expected));
    }
}

TEST_F(ColorCountTest, ExactFamiliesPartitionTheSpace) {
    Instance inst = parse(kSmall);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> exact = exact_color_count_families(space);
    ASSERT_EQ(exact.size(), (std::size_t)(inst.k + 1));

    // Disjoint union equals the space.
    MVZDD uni = MVZDD::zero(space.var_table());
    for (int m = 0; m <= inst.k; ++m) {
        EXPECT_TRUE(uni.is_disjoint(exact[m]));
        uni = uni + exact[m];
    }
    EXPECT_TRUE(uni == space);

    // Every member of exact[m] really uses m colors.
    for (int m = 0; m <= inst.k; ++m) {
        std::vector<std::vector<int> > members = exact[m].enumerate();
        for (std::size_t i = 0; i < members.size(); ++i) {
            EXPECT_EQ(count_used_colors(members[i]), m);
        }
    }

    // Triangle + pendant with 3 colors always uses all 3 colors.
    EXPECT_TRUE(exact[0].is_zero());
    EXPECT_TRUE(exact[1].is_zero());
    EXPECT_TRUE(exact[2].is_zero());
    EXPECT_EQ(exact[3].exact_count(), space.exact_count());
}

TEST_F(ColorCountTest, ExactFamiliesOnPath) {
    Instance inst = parse(kPath4);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> exact = exact_color_count_families(space);

    // P4 with 3 colors: chromatic polynomial gives 3*2^3 = 24 colorings.
    // 2-colorings: choose an ordered pair of distinct colors = 6.
    EXPECT_EQ(space.exact_count(), bigint::BigInt(24));
    EXPECT_TRUE(exact[1].is_zero());
    EXPECT_EQ(exact[2].exact_count(), bigint::BigInt(6));
    EXPECT_EQ(exact[3].exact_count(), bigint::BigInt(18));
}

TEST_F(ColorCountTest, AtMostColorCount) {
    Instance inst = parse(kPath4);
    MVZDD space = build_solution_space(inst);
    std::vector<MVZDD> exact = exact_color_count_families(space);

    EXPECT_EQ(at_most_color_count(exact, 2).exact_count(), bigint::BigInt(6));
    EXPECT_TRUE(at_most_color_count(exact, 3) == space);
    EXPECT_TRUE(at_most_color_count(exact, 0).is_zero());
}
