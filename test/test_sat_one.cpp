#include <gtest/gtest.h>
#include "bdd.h"
#include <algorithm>
#include <cstdlib>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

using namespace kyotodd;

class SatOneTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

// Check that the literals are in strictly decreasing level order.
static void expect_decreasing_level(const std::vector<int>& lits) {
    for (size_t i = 1; i < lits.size(); ++i) {
        EXPECT_GT(bddlevofvar(static_cast<bddvar>(std::abs(lits[i - 1]))),
                  bddlevofvar(static_cast<bddvar>(std::abs(lits[i]))));
    }
}

// Replay the path of sat_one(): each literal must be the current top
// variable, and a positive literal is allowed only when the 0-cofactor
// is unsatisfiable (0-edge preference). The walk must end at true.
static void expect_lo_first_path(const BDD& f, const std::vector<int>& lits) {
    BDD g = f;
    for (int lit : lits) {
        bddvar v = static_cast<bddvar>(std::abs(lit));
        ASSERT_FALSE(g.is_terminal());
        ASSERT_EQ(g.top(), v);
        if (lit > 0) {
            EXPECT_TRUE(g.at0(v).is_zero());
            g = g.at1(v);
        } else {
            g = g.at0(v);
        }
    }
    EXPECT_TRUE(g.is_one());
}

// Build a random Boolean function over vars using minterms and
// complements so that complement edges appear.
static BDD random_bdd(const std::vector<bddvar>& vars, std::mt19937& rng) {
    BDD f = BDD::False;
    size_t n = vars.size();
    for (uint32_t m = 0; m < (1u << n); ++m) {
        if (rng() % 3 != 0) continue;
        std::vector<int> lits;
        for (size_t i = 0; i < n; ++i) {
            int v = static_cast<int>(vars[i]);
            lits.push_back(((m >> i) & 1) ? v : -v);
        }
        f = f | BDD::cube(lits);
    }
    if (rng() % 2) f = ~f;
    return f;
}

// --- BDD::sat_one ---

TEST_F(SatOneTest, BddSatOneThrowsOnFalseAndNull) {
    EXPECT_THROW(BDD::False.sat_one(), std::invalid_argument);
    EXPECT_THROW(BDD::Null.sat_one(), std::invalid_argument);
}

TEST_F(SatOneTest, BddSatOneTrueIsEmpty) {
    EXPECT_TRUE(BDD::True.sat_one().empty());
}

TEST_F(SatOneTest, BddSatOneLiteral) {
    bddvar v = BDD_NewVar();
    BDD x = BDD::prime(v);
    EXPECT_EQ(x.sat_one(), std::vector<int>({static_cast<int>(v)}));
    EXPECT_EQ((~x).sat_one(), std::vector<int>({-static_cast<int>(v)}));
}

TEST_F(SatOneTest, BddSatOnePrefersZeroEdge) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    BDD x1 = BDD::prime(v1);
    BDD x2 = BDD::prime(v2);
    BDD x3 = BDD::prime(v3);
    // f = x1 & ~x3: x2 is a don't-care, x3 = 0 is taken first
    BDD f = x1 & ~x3;
    EXPECT_EQ(f.sat_one(), std::vector<int>({-3, 1}));
    // f = x1 | x2 | x3: 0-edges until x1 is forced
    EXPECT_EQ((x1 | x2 | x3).sat_one(), std::vector<int>({-3, -2, 1}));
    // f = ~(x1 & x2 & x3): complemented root, x3 = 0 suffices
    EXPECT_EQ((~(x1 & x2 & x3)).sat_one(), std::vector<int>({-3}));
}

TEST_F(SatOneTest, BddSatOneRandom) {
    std::vector<bddvar> vars;
    for (int i = 0; i < 6; ++i) vars.push_back(BDD_NewVar());
    std::mt19937 rng(12345);
    for (int trial = 0; trial < 200; ++trial) {
        BDD f = random_bdd(vars, rng);
        if (f.is_zero()) {
            EXPECT_THROW(f.sat_one(), std::invalid_argument);
            continue;
        }
        std::vector<int> lits = f.sat_one();
        EXPECT_TRUE((BDD::cube(lits) & ~f).is_zero());
        expect_decreasing_level(lits);
        expect_lo_first_path(f, lits);
    }
}

TEST_F(SatOneTest, BddSatOneNonDefaultOrder) {
    // Every new variable is inserted at level 1, so var i has level
    // 5 - i + 1 and var 1 is the top variable.
    std::vector<bddvar> vars;
    for (int i = 0; i < 5; ++i) vars.push_back(BDD_NewVarOfLev(1));
    std::mt19937 rng(777);
    for (int trial = 0; trial < 100; ++trial) {
        BDD f = random_bdd(vars, rng);
        if (f.is_zero()) continue;
        std::vector<int> lits = f.sat_one();
        EXPECT_TRUE((BDD::cube(lits) & ~f).is_zero());
        expect_decreasing_level(lits);
        expect_lo_first_path(f, lits);
    }
}

// --- BDD::sat_one_full ---

TEST_F(SatOneTest, BddSatOneFullErrors) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD x1 = BDD::prime(v1);
    BDD x2 = BDD::prime(v2);
    EXPECT_THROW(BDD::False.sat_one_full(2), std::invalid_argument);
    EXPECT_THROW(BDD::Null.sat_one_full(2), std::invalid_argument);
    EXPECT_THROW(x1.sat_one_full(3), std::invalid_argument);
    // The path of x2 contains variable 2 > n
    EXPECT_THROW(x2.sat_one_full(1), std::invalid_argument);
    // The path of ~x2 | x1 stops at x2 = 0, which is still outside 1..1
    EXPECT_THROW((~x2 | x1).sat_one_full(1), std::invalid_argument);
}

TEST_F(SatOneTest, BddSatOneFullBasic) {
    bddvar v1 = BDD_NewVar();
    BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    BDD x1 = BDD::prime(v1);
    BDD x3 = BDD::prime(v3);
    EXPECT_TRUE(BDD::True.sat_one_full(0).empty());
    EXPECT_EQ(BDD::True.sat_one_full(3), std::vector<int>({-3, -2, -1}));
    EXPECT_EQ((x1 & ~x3).sat_one_full(3), std::vector<int>({-3, -2, 1}));
    EXPECT_EQ(x3.sat_one_full(3), std::vector<int>({3, -2, -1}));
    // Variables above n but not on the path are fine
    EXPECT_EQ(x1.sat_one_full(1), std::vector<int>({1}));
}

static void check_full(const BDD& f, bddvar n) {
    std::vector<int> lits = f.sat_one_full(n);
    ASSERT_EQ(lits.size(), static_cast<size_t>(n));
    std::set<int> seen;
    for (int lit : lits) seen.insert(std::abs(lit));
    EXPECT_EQ(seen.size(), static_cast<size_t>(n));
    EXPECT_EQ(*seen.begin(), 1);
    EXPECT_EQ(*seen.rbegin(), static_cast<int>(n));
    EXPECT_TRUE((BDD::cube(lits) & ~f).is_zero());
    expect_decreasing_level(lits);
    // The literals on the sat_one() path appear unchanged
    for (int lit : f.sat_one()) {
        EXPECT_NE(std::find(lits.begin(), lits.end(), lit), lits.end());
    }
    // Everything else is set to 0
    std::vector<int> path = f.sat_one();
    for (int lit : lits) {
        if (std::find(path.begin(), path.end(), lit) == path.end()) {
            EXPECT_LT(lit, 0);
        }
    }
}

TEST_F(SatOneTest, BddSatOneFullRandom) {
    std::vector<bddvar> vars;
    for (int i = 0; i < 6; ++i) vars.push_back(BDD_NewVar());
    std::mt19937 rng(2468);
    for (int trial = 0; trial < 200; ++trial) {
        BDD f = random_bdd(vars, rng);
        if (f.is_zero()) continue;
        check_full(f, 6);
    }
}

TEST_F(SatOneTest, BddSatOneFullNonDefaultOrder) {
    std::vector<bddvar> vars;
    for (int i = 0; i < 5; ++i) vars.push_back(BDD_NewVarOfLev(1));
    std::mt19937 rng(1357);
    for (int trial = 0; trial < 100; ++trial) {
        BDD f = random_bdd(vars, rng);
        if (f.is_zero()) continue;
        check_full(f, 5);
    }
}

// --- ZDD::one_set ---

TEST_F(SatOneTest, ZddOneSetThrowsOnEmptyAndNull) {
    EXPECT_THROW(ZDD::Empty.one_set(), std::invalid_argument);
    EXPECT_THROW(ZDD::Null.one_set(), std::invalid_argument);
}

TEST_F(SatOneTest, ZddOneSetBasic) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    EXPECT_TRUE(ZDD::Single.one_set().empty());
    // {{1}, {2}}: 0-edge of var 2 leads to {{1}}
    ZDD f = ZDD::single_set({v1}) + ZDD::single_set({v2});
    EXPECT_EQ(f.one_set(), std::vector<bddvar>({v1}));
    // {{1, 3}, {2, 3}}: var 3 is forced
    ZDD g = ZDD::single_set({v1, v3}) + ZDD::single_set({v2, v3});
    EXPECT_EQ(g.one_set(), std::vector<bddvar>({v3, v1}));
    // Complement edge adds the empty set, which the 0-edges reach
    EXPECT_TRUE((~g).one_set().empty());
}

// Replay the path of one_set() with offset / onset0.
static void expect_lo_first_set(const ZDD& f, const std::vector<bddvar>& s) {
    ZDD g = f;
    size_t i = 0;
    while (!g.is_terminal()) {
        bddvar t = g.top();
        if (i < s.size() && s[i] == t) {
            EXPECT_TRUE(g.offset(t) == ZDD::Empty);
            g = g.onset0(t);
            ++i;
        } else {
            g = g.offset(t);
        }
    }
    EXPECT_EQ(i, s.size());
    EXPECT_TRUE(g == ZDD::Single);
}

static void check_one_set(const ZDD& f) {
    std::vector<bddvar> s = f.one_set();
    for (size_t i = 1; i < s.size(); ++i) {
        EXPECT_GT(bddlevofvar(s[i - 1]), bddlevofvar(s[i]));
    }
    EXPECT_FALSE((f & ZDD::single_set(s)) == ZDD::Empty);
    expect_lo_first_set(f, s);
}

static ZDD random_zdd(const std::vector<bddvar>& vars, std::mt19937& rng) {
    ZDD f = ZDD::Empty;
    size_t n = vars.size();
    for (uint32_t m = 0; m < (1u << n); ++m) {
        if (rng() % 4 != 0) continue;
        std::vector<bddvar> s;
        for (size_t i = 0; i < n; ++i) {
            if ((m >> i) & 1) s.push_back(vars[i]);
        }
        f = f + ZDD::single_set(s);
    }
    if (rng() % 2) f = ~f;
    return f;
}

TEST_F(SatOneTest, ZddOneSetRandom) {
    std::vector<bddvar> vars;
    for (int i = 0; i < 6; ++i) vars.push_back(BDD_NewVar());
    std::mt19937 rng(9876);
    for (int trial = 0; trial < 200; ++trial) {
        ZDD f = random_zdd(vars, rng);
        if (f == ZDD::Empty) {
            EXPECT_THROW(f.one_set(), std::invalid_argument);
            continue;
        }
        check_one_set(f);
    }
}

TEST_F(SatOneTest, ZddOneSetNonDefaultOrder) {
    std::vector<bddvar> vars;
    for (int i = 0; i < 5; ++i) vars.push_back(BDD_NewVarOfLev(1));
    std::mt19937 rng(5432);
    for (int trial = 0; trial < 100; ++trial) {
        ZDD f = random_zdd(vars, rng);
        if (f == ZDD::Empty) continue;
        check_one_set(f);
    }
}
