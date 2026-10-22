// Execution-mode dispatch tests for operations that gained BddExecMode
// overloads late: each must agree with its Auto form in every mode, and
// on DDs deeper than BDD_RecurLimit the forced Recursive mode must hit the
// recursion limit while Iterative (and Auto) succeed.

#include <gtest/gtest.h>
#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <climits>
#include <cstdio>
#include <iostream>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

using namespace kyotodd;

namespace {

const BddExecMode kModes[] = {BDD_EXEC_RECURSIVE, BDD_EXEC_ITERATIVE, BDD_EXEC_AUTO};

// Single-set chain {{1, 2, ..., depth}} with its root at level depth.
ZDD deep_single_set(int depth) {
    for (int i = bdd_varcount; i < depth; ++i) bddnewvar();
    bddp f = bddsingle;
    for (int v = 1; v <= depth; ++v) {
        f = ZDD::getnode_raw(static_cast<bddvar>(v), bddempty, f);
    }
    return ZDD_ID(f);
}

// Power set of {1, ..., depth}: every node has two non-terminal children,
// so binary operations recurse all the way down.
ZDD deep_power_set(int depth) {
    for (int i = bdd_varcount; i < depth; ++i) bddnewvar();
    bddp f = bddsingle;
    for (int v = 1; v <= depth; ++v) {
        f = ZDD::getnode_raw(static_cast<bddvar>(v), f, f);
    }
    return ZDD_ID(f);
}

}  // namespace

class ExecModeTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
    static const int kDeep = BDD_RecurLimit + 100;
};

// --- bddremainder -----------------------------------------------------

TEST_F(ExecModeTest, RemainderModesAgree) {
    for (int i = 0; i < 4; ++i) bddnewvar();
    ZDD a = ZDD::singleton(1), b = ZDD::singleton(2);
    ZDD c = ZDD::singleton(3), d = ZDD::singleton(4);
    ZDD f = (a * b) + (a * c) + (b * c * d) + d;
    ZDD g = a + c;
    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kModes[i];
        ZDD r = ZDD_ID(bddremainder(f.id(), g.id(), m));
        EXPECT_EQ(r, f % g);
        // F = (F / G) ⊔ G + (F % G) with disjoint parts.
        EXPECT_EQ(((f / g) * g) + r, f);
        EXPECT_EQ(bddremainder(bddempty, g.id(), m), bddempty);
        EXPECT_EQ(bddremainder(f.id(), bddsingle, m), bddempty);
        EXPECT_EQ(bddremainder(f.id(), f.id(), m), bddempty);
        EXPECT_EQ(bddremainder(bddnull, g.id(), m), bddnull);
    }
}

TEST_F(ExecModeTest, RemainderDeep) {
    ZDD f = deep_power_set(kDeep);
    ZDD g = ZDD::singleton(1) + ZDD::singleton(2);
    EXPECT_THROW(bddremainder(f.id(), g.id(), BDD_EXEC_RECURSIVE), std::overflow_error);
    ZDD r = ZDD_ID(bddremainder(f.id(), g.id(), BDD_EXEC_ITERATIVE));
    EXPECT_EQ(r, ZDD_ID(bddremainder(f.id(), g.id())));
    // F = ((F / G) ⊔ G) + (F % G): the quotient is the power set of
    // {3..n}, so the remainder is every set containing both or neither of
    // {1, 2}. Check the identity and that the parts are disjoint.
    ZDD q = f / g;
    EXPECT_EQ((q * g) + r, f);
    EXPECT_EQ((q * g) & r, ZDD(0));
    EXPECT_EQ(r & ZDD::singleton(1), ZDD(0));
}

// --- similarity measures ----------------------------------------------

TEST_F(ExecModeTest, SimilarityModesAgree) {
    for (int i = 0; i < 3; ++i) bddnewvar();
    ZDD a = ZDD::singleton(1), b = ZDD::singleton(2), c = ZDD::singleton(3);
    ZDD f = a + b + (a * b);        // 3 sets
    ZDD g = b + (a * b) + c + (b * c);  // 4 sets, 2 shared
    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kModes[i];
        EXPECT_DOUBLE_EQ(bddjaccardindex(f.id(), g.id(), m), 2.0 / 5.0);
        EXPECT_EQ(bddhammingdist(f.id(), g.id(), m), bigint::BigInt(3));
        EXPECT_DOUBLE_EQ(bddoverlapcoeff(f.id(), g.id(), m), 2.0 / 3.0);
        EXPECT_DOUBLE_EQ(bddjaccardindex(f.id(), g.id(), m), bddjaccardindex(f.id(), g.id()));
        EXPECT_EQ(bddhammingdist(f.id(), g.id(), m), bddhammingdist(f.id(), g.id()));
        EXPECT_DOUBLE_EQ(bddoverlapcoeff(f.id(), g.id(), m), bddoverlapcoeff(f.id(), g.id()));
        // Terminal conventions are mode-independent.
        EXPECT_DOUBLE_EQ(bddjaccardindex(bddempty, bddempty, m), 1.0);
        EXPECT_EQ(bddhammingdist(bddempty, f.id(), m), bigint::BigInt(3));
        EXPECT_DOUBLE_EQ(bddoverlapcoeff(f.id(), f.id(), m), 1.0);
    }
}

TEST_F(ExecModeTest, SimilarityDeep) {
    ZDD f = deep_single_set(kDeep);
    ZDD g = f + ZDD::singleton(1);
    EXPECT_THROW(bddjaccardindex(f.id(), g.id(), BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(bddhammingdist(f.id(), g.id(), BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(bddoverlapcoeff(f.id(), g.id(), BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_DOUBLE_EQ(bddjaccardindex(f.id(), g.id(), BDD_EXEC_ITERATIVE), 0.5);
    EXPECT_EQ(bddhammingdist(f.id(), g.id(), BDD_EXEC_ITERATIVE), bigint::BigInt(1));
    EXPECT_DOUBLE_EQ(bddoverlapcoeff(f.id(), g.id(), BDD_EXEC_ITERATIVE), 1.0);
    EXPECT_DOUBLE_EQ(bddjaccardindex(f.id(), g.id()), 0.5);
    EXPECT_EQ(bddhammingdist(f.id(), g.id()), bigint::BigInt(1));
    EXPECT_DOUBLE_EQ(bddoverlapcoeff(f.id(), g.id()), 1.0);
}

// --- rank / unrank ----------------------------------------------------

TEST_F(ExecModeTest, RankUnrankModesAgree) {
    for (int i = 0; i < 4; ++i) bddnewvar();
    ZDD f = ZDD::power_set(4);
    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kModes[i];
        for (int64_t k = 0; k < 16; ++k) {
            std::vector<bddvar> s = bddunrank(f.id(), k, m);
            EXPECT_EQ(s, bddunrank(f.id(), k));
            EXPECT_EQ(s, bddexactunrank(f.id(), bigint::BigInt(k), m));
            CountMemoMap memo;
            EXPECT_EQ(s, bddexactunrank(f.id(), bigint::BigInt(k), memo, m));
            EXPECT_EQ(bddrank(f.id(), s, m), k);
            EXPECT_EQ(bddexactrank(f.id(), s, m), bigint::BigInt(k));
            EXPECT_EQ(bddexactrank(f.id(), s, memo, m), bigint::BigInt(k));
        }
        std::vector<bddvar> missing;
        missing.push_back(5);
        EXPECT_EQ(bddrank(f.id(), missing, m), -1);
        EXPECT_THROW(bddunrank(f.id(), 16, m), std::out_of_range);
    }
}

TEST_F(ExecModeTest, RankUnrankDeep) {
    // {{1..n}} + {{1}}: ranking {1} requires counting the deep hi-subtree
    // under the root; unranking index 1 likewise.
    ZDD f = deep_single_set(kDeep) + ZDD::singleton(1);
    std::vector<bddvar> one;
    one.push_back(1);
    EXPECT_THROW(bddrank(f.id(), one, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(bddunrank(f.id(), 1, BDD_EXEC_RECURSIVE), std::overflow_error);
    int64_t r = bddrank(f.id(), one, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(r, bddrank(f.id(), one));
    EXPECT_EQ(bddunrank(f.id(), r, BDD_EXEC_ITERATIVE), one);
    EXPECT_EQ(bddunrank(f.id(), r), one);
    std::vector<bddvar> all;
    for (int v = 1; v <= kDeep; ++v) all.push_back(static_cast<bddvar>(v));
    int64_t r_all = bddrank(f.id(), all, BDD_EXEC_ITERATIVE);
    EXPECT_NE(r, r_all);
    EXPECT_EQ(bddunrank(f.id(), r_all, BDD_EXEC_ITERATIVE), all);
}

// --- bddimply ---------------------------------------------------------

TEST_F(ExecModeTest, ImplyModesAgree) {
    for (int i = 0; i < 3; ++i) BDD_NewVar();
    BDD a = BDD::prime(1), b = BDD::prime(2), c = BDD::prime(3);
    BDD f = a & b;
    BDD g = a | c;
    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kModes[i];
        EXPECT_EQ(bddimply(f.id(), g.id(), m), 1);
        EXPECT_EQ(bddimply(g.id(), f.id(), m), 0);
        EXPECT_EQ(bddimply(f.id(), g.id(), m), bddimply(f.id(), g.id()));
        EXPECT_EQ(bddimply(bddtrue, bddfalse, m), 0);
        EXPECT_EQ(bddimply(bddnull, f.id(), m), -1);
    }
}

TEST_F(ExecModeTest, ImplyDeep) {
    // f = x1 ^ ... ^ xn (parity), g = f | x1: both cofactors stay
    // non-terminal at every level so the check recurses to the bottom.
    for (int i = bdd_varcount; i < kDeep; ++i) BDD_NewVar();
    BDD f(0);
    for (int v = 1; v <= kDeep; ++v) f ^= BDD::prime(static_cast<bddvar>(v));
    BDD g = f | BDD::prime(1);
    EXPECT_THROW(bddimply(f.id(), g.id(), BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(bddimply(f.id(), g.id(), BDD_EXEC_ITERATIVE), 1);
    EXPECT_EQ(bddimply(g.id(), f.id(), BDD_EXEC_ITERATIVE), 0);
    EXPECT_EQ(bddimply(f.id(), g.id()), 1);
}

// --- bddlit / bddlen / bddmaxsize --------------------------------------

TEST_F(ExecModeTest, LitLenModesAgree) {
    for (int i = 0; i < 4; ++i) bddnewvar();
    ZDD a = ZDD::singleton(1), b = ZDD::singleton(2), c = ZDD::singleton(3), d = ZDD::singleton(4);
    ZDD f = (a * b) + a + (b * c * d);   // literals 2+1+3, max size 3
    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kModes[i];
        EXPECT_EQ(bddlit(f.id(), m), 6u);
        EXPECT_EQ(bddlen(f.id(), m), 3u);
        EXPECT_EQ(bddmaxsize(f.id(), m), 3u);
        EXPECT_EQ(bddlit(f.id(), m), bddlit(f.id()));
        EXPECT_EQ(bddlen(f.id(), m), bddlen(f.id()));
        EXPECT_EQ(bddlit(bddempty, m), 0u);
        EXPECT_EQ(bddlen(bddsingle, m), 0u);
    }
}

TEST_F(ExecModeTest, LitLenDeep) {
    ZDD f = deep_single_set(kDeep);
    EXPECT_THROW(bddlit(f.id(), BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(bddlen(f.id(), BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(bddmaxsize(f.id(), BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(bddlit(f.id(), BDD_EXEC_ITERATIVE), static_cast<uint64_t>(kDeep));
    EXPECT_EQ(bddlen(f.id(), BDD_EXEC_ITERATIVE), static_cast<uint64_t>(kDeep));
    EXPECT_EQ(bddmaxsize(f.id(), BDD_EXEC_ITERATIVE), static_cast<uint64_t>(kDeep));
    EXPECT_EQ(bddlit(f.id()), static_cast<uint64_t>(kDeep));
    EXPECT_EQ(bddlen(f.id()), static_cast<uint64_t>(kDeep));
}

// --- bddminweightset / bddmaxweightset ---------------------------------

TEST_F(ExecModeTest, WeightSetModesAgree) {
    for (int i = 0; i < 3; ++i) bddnewvar();
    ZDD a = ZDD::singleton(1), b = ZDD::singleton(2), c = ZDD::singleton(3);
    ZDD f = a + (b * c) + (a * c);
    std::vector<int> w(bdd_varcount + 1, 0);
    w[1] = 5; w[2] = 1; w[3] = 2;   // {1}=5, {2,3}=3, {1,3}=7
    std::vector<bddvar> min_expected, max_expected;
    min_expected.push_back(2); min_expected.push_back(3);
    max_expected.push_back(1); max_expected.push_back(3);
    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kModes[i];
        EXPECT_EQ(bddminweightset(f.id(), w, m), min_expected);
        EXPECT_EQ(bddmaxweightset(f.id(), w, m), max_expected);
        EXPECT_EQ(bddminweightset(f.id(), w, m), bddminweightset(f.id(), w));
        EXPECT_EQ(bddmaxweightset(f.id(), w, m), bddmaxweightset(f.id(), w));
        EXPECT_THROW(bddminweightset(bddempty, w, m), std::invalid_argument);
    }
}

TEST_F(ExecModeTest, WeightSetDeep) {
    ZDD f = deep_single_set(kDeep) + ZDD::singleton(1);
    std::vector<int> w(bdd_varcount + 1, 1);
    std::vector<bddvar> one;
    one.push_back(1);
    std::vector<bddvar> all;
    for (int v = 1; v <= kDeep; ++v) all.push_back(static_cast<bddvar>(v));
    EXPECT_THROW(bddminweightset(f.id(), w, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(bddmaxweightset(f.id(), w, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(bddminweightset(f.id(), w, BDD_EXEC_ITERATIVE), one);
    EXPECT_EQ(bddmaxweightset(f.id(), w, BDD_EXEC_ITERATIVE), all);
    EXPECT_EQ(bddminweightset(f.id(), w), one);
    EXPECT_EQ(bddmaxweightset(f.id(), w), all);
}

// --- bdddump ----------------------------------------------------------

namespace {
// Run fn with stdout redirected to /dev/null (bdddump prints unconditionally).
template <typename F>
void with_stdout_discarded(F fn) {
    std::fflush(stdout);
    int saved = dup(fileno(stdout));
    FILE* devnull = std::freopen("/dev/null", "w", stdout);
    ASSERT_NE(devnull, nullptr);
    fn();
    std::fflush(stdout);
    dup2(saved, fileno(stdout));
    close(saved);
}
}  // namespace

TEST_F(ExecModeTest, DumpModes) {
    for (int i = 0; i < 2; ++i) BDD_NewVar();
    BDD f = BDD::prime(1) & ~BDD::prime(2);
    with_stdout_discarded([&]() {
        for (int i = 0; i < 3; ++i) {
            EXPECT_NO_THROW(bdddump(f.id(), kModes[i]));
            EXPECT_NO_THROW(bdddump(bddnull, kModes[i]));
        }
    });
}

TEST_F(ExecModeTest, DumpDeep) {
    for (int i = bdd_varcount; i < kDeep; ++i) BDD_NewVar();
    bddp chain = bddtrue;
    for (int v = 1; v <= kDeep; ++v) {
        chain = BDD::getnode_raw(static_cast<bddvar>(v), bddfalse, chain);
    }
    BDD f = BDD_ID(chain);
    with_stdout_discarded([&]() {
        EXPECT_THROW(bdddump(f.id(), BDD_EXEC_RECURSIVE), std::overflow_error);
        EXPECT_NO_THROW(bdddump(f.id(), BDD_EXEC_ITERATIVE));
        EXPECT_NO_THROW(bdddump(f.id()));
    });
}

// --- composite ZDD wrappers (group C) ----------------------------------

TEST_F(ExecModeTest, CompositeWrappersModesAgree) {
    for (int i = 0; i < 4; ++i) bddnewvar();
    ZDD a = ZDD::singleton(1), b = ZDD::singleton(2), c = ZDD::singleton(3), d = ZDD::singleton(4);
    // f is symmetric in {1, 2}; 3 implies 4 in every set containing 3.
    ZDD f = (a * c * d) + (b * c * d) + (a * b) + d;
    std::vector<int> w(bdd_varcount + 1, 0);
    w[1] = 1; w[2] = 2; w[3] = 4; w[4] = 8;
    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kModes[i];
        EXPECT_EQ(bddswapz(f.id(), 1, 2, m), bddswapz(f.id(), 1, 2));
        EXPECT_EQ(bddswapz(f.id(), 1, 2, m), f.id());   // symmetric in 1, 2
        EXPECT_EQ(bddswapz(f.id(), 1, 3, m), bddswapz(f.id(), 1, 3));
        EXPECT_NE(bddswapz(f.id(), 1, 3, m), f.id());
        EXPECT_EQ(bddimplyset(f.id(), 3, m), bddimplyset(f.id(), 3));
        EXPECT_EQ(bddimplyset(f.id(), 3, m), d.id());
        EXPECT_EQ(bddsymgrp(f.id(), m), bddsymgrp(f.id()));
        EXPECT_EQ(bddsymgrp(f.id(), m), (a * b).id());
        EXPECT_EQ(bddsymgrpnaive(f.id(), m), bddsymgrpnaive(f.id()));
        EXPECT_EQ(bdddivisor(f.id(), m), bdddivisor(f.id()));
        EXPECT_EQ(bddflatten(f.id(), m), bddflatten(f.id()));
        EXPECT_EQ(bddflatten(f.id(), m), (a * b * c * d).id());
        EXPECT_EQ(bddcoalesce(f.id(), 1, 2, m), bddcoalesce(f.id(), 1, 2));
        EXPECT_EQ(bddcoalesce(f.id(), 1, 2, m), ((a * c * d) + a + d).id());
        for (int64_t k = 0; k <= 4; ++k) {
            EXPECT_EQ(bddgetklightest(f.id(), k, w, 0, m), bddgetklightest(f.id(), k, w, 0));
            EXPECT_EQ(bddgetklightest(f.id(), bigint::BigInt(k), w, 0, m),
                      bddgetklightest(f.id(), bigint::BigInt(k), w, 0));
            EXPECT_EQ(bddgetkheaviest(f.id(), k, w, 0, m), bddgetkheaviest(f.id(), k, w, 0));
            EXPECT_EQ(bddgetkheaviest(f.id(), bigint::BigInt(k), w, 0, m),
                      bddgetkheaviest(f.id(), bigint::BigInt(k), w, 0));
        }
        EXPECT_EQ(bddgetklightest(f.id(), static_cast<int64_t>(1), w, 0, m), (a * b).id());
        EXPECT_EQ(bddgetkheaviest(f.id(), static_cast<int64_t>(1), w, 0, m), (b * c * d).id());
        // Terminal / error paths are mode-independent.
        EXPECT_EQ(bddswapz(bddnull, 1, 2, m), bddnull);
        EXPECT_EQ(bddflatten(bddempty, m), bddempty);
        EXPECT_EQ(bddcoalesce(bddsingle, 1, 2, m), bddsingle);
        EXPECT_EQ(bdddivisor(bddempty, m), bddempty);
        EXPECT_THROW(bddimplyset(f.id(), 99, m), std::invalid_argument);
        EXPECT_THROW(bddgetklightest(f.id(), static_cast<int64_t>(-1), w, 0, m), std::invalid_argument);
    }
}

TEST_F(ExecModeTest, CompositeWrappersDeep) {
    // Power set of {1..n}: offset / onset0 on variable 1 recurse to the
    // bottom, so forced Recursive mode must fail through the wrappers.
    ZDD ps = deep_power_set(kDeep);
    EXPECT_THROW(bddswapz(ps.id(), 1, 2, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(bddimplyset(ps.id(), 1, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(bddcoalesce(ps.id(), 2, 1, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(ZDD_ID(bddswapz(ps.id(), 1, 2, BDD_EXEC_ITERATIVE)), ps);
    EXPECT_EQ(ZDD_ID(bddswapz(ps.id(), 1, 2)), ps);
    EXPECT_EQ(bddimplyset(ps.id(), 1, BDD_EXEC_ITERATIVE), bddempty);
    EXPECT_EQ(bddimplyset(ps.id(), 1), bddempty);
    ZDD co = ZDD_ID(bddcoalesce(ps.id(), 2, 1, BDD_EXEC_ITERATIVE));
    EXPECT_EQ(co, ZDD_ID(bddcoalesce(ps.id(), 2, 1)));
    EXPECT_EQ(bddoffset(co.id(), 1), co.id());   // variable 1 eliminated

    // {{1..n}, {1}}: the k-lightest / k-heaviest need the exact count of
    // f, which recurses down the whole chain.
    ZDD f = deep_single_set(kDeep) + ZDD::singleton(1);
    std::vector<int> w(bdd_varcount + 1, 1);
    EXPECT_THROW(bddgetklightest(f.id(), static_cast<int64_t>(1), w, 0, BDD_EXEC_RECURSIVE),
                 std::overflow_error);
    EXPECT_THROW(bddgetkheaviest(f.id(), static_cast<int64_t>(1), w, 0, BDD_EXEC_RECURSIVE),
                 std::overflow_error);
    EXPECT_EQ(bddgetklightest(f.id(), static_cast<int64_t>(1), w, 0, BDD_EXEC_ITERATIVE),
              ZDD::singleton(1).id());
    EXPECT_EQ(bddgetklightest(f.id(), static_cast<int64_t>(1), w, 0), ZDD::singleton(1).id());
    EXPECT_EQ(ZDD_ID(bddgetkheaviest(f.id(), static_cast<int64_t>(1), w, 0, BDD_EXEC_ITERATIVE)),
              deep_single_set(kDeep));
    EXPECT_EQ(ZDD_ID(bddgetkheaviest(f.id(), static_cast<int64_t>(1), w, 0)),
              deep_single_set(kDeep));
}

TEST_F(ExecModeTest, CompositeWrappersHighLevel) {
    // bdddivisor, bddflatten, bddsymgrp and bddsymgrpnaive loop over the
    // support, so a chain with thousands of support variables costs
    // O(n^2) and is too slow for a unit test. Use a two-node family whose
    // root sits above BDD_RecurLimit instead: Auto dispatches to _iter
    // for it, and every mode must still agree.
    const bddvar top = static_cast<bddvar>(kDeep);
    for (int i = bdd_varcount; i < kDeep; ++i) bddnewvar();
    ZDD a = ZDD::singleton(1), t = ZDD::singleton(top);
    ZDD f = (a * t) + t;          // {{1, top}, {top}}
    ZDD g = (a * t) + a + t;      // symmetric in {1, top}
    EXPECT_GT(bddp_level(f.id()), static_cast<bddvar>(BDD_RecurLimit));
    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kModes[i];
        EXPECT_EQ(bdddivisor(f.id(), m), bdddivisor(f.id()));
        EXPECT_EQ(bddflatten(f.id(), m), bddflatten(f.id()));
        EXPECT_EQ(bddflatten(f.id(), m), (a * t).id());
        EXPECT_EQ(bddsymgrp(g.id(), m), bddsymgrp(g.id()));
        EXPECT_EQ(bddsymgrp(g.id(), m), (a * t).id());
        EXPECT_EQ(bddsymgrpnaive(g.id(), m), bddsymgrpnaive(g.id()));
        EXPECT_EQ(bddsymgrpnaive(g.id(), m), (a * t).id());
        EXPECT_EQ(bddsymgrp(f.id(), m), bddempty);   // 1 and top not symmetric
    }
}

// --- BDD / ZDD / QDD conversions and UnreducedDD (class members) --------

TEST_F(ExecModeTest, ConversionModesAgree) {
    for (int i = 0; i < 4; ++i) bddnewvar();
    ZDD a = ZDD::singleton(1), b = ZDD::singleton(2), c = ZDD::singleton(3);
    ZDD z = (a * c) + b + ZDD(1);
    BDD f = z.to_bdd();
    QDD q = f.to_qdd();
    UnreducedDD ub(~f), uz(z), uq(~q);
    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kModes[i];
        bddgc();  // clear the cache so each mode really recomputes
        EXPECT_EQ(z.to_bdd(0, m), f);
        EXPECT_EQ(z.to_bdd(4, m), z.to_bdd(4));
        EXPECT_EQ(z.to_qdd(m), z.to_qdd());
        EXPECT_EQ(f.to_zdd(0, m), z);
        EXPECT_EQ(f.to_zdd(4, m), f.to_zdd(4));
        EXPECT_EQ(f.to_qdd(m), q);
        EXPECT_EQ(q.to_bdd(m), f);
        EXPECT_EQ(q.to_zdd(m), q.to_zdd());
        EXPECT_EQ(UnreducedDD(~f, m).reduce_as_bdd(m), ~f);
        EXPECT_EQ(UnreducedDD(z, m).reduce_as_zdd(m), z);
        EXPECT_EQ(UnreducedDD(~q, m).reduce_as_qdd(m), ~q);
        EXPECT_EQ(ub.reduce_as_bdd(m), ~f);
        EXPECT_EQ(uz.reduce_as_zdd(m), z);
        EXPECT_EQ(uq.reduce_as_qdd(m), ~q);
        // Null inputs are mode independent.
        EXPECT_EQ(ZDD::Null.to_bdd(0, m), BDD::Null);
        EXPECT_EQ(BDD::Null.to_qdd(m), QDD::Null);
        EXPECT_EQ(QDD::Null.to_zdd(m), ZDD::Null);
        EXPECT_EQ(UnreducedDD(BDD::Null, m).id(), bddnull);
    }
}

TEST_F(ExecModeTest, ConversionDeep) {
    ZDD z = deep_single_set(kDeep);
    EXPECT_THROW(z.to_bdd(0, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(z.to_qdd(BDD_EXEC_RECURSIVE), std::overflow_error);
    BDD f = z.to_bdd(0, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(f, z.to_bdd());
    QDD qz = z.to_qdd(BDD_EXEC_ITERATIVE);
    EXPECT_EQ(qz, z.to_qdd());

    EXPECT_THROW(f.to_zdd(0, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(f.to_qdd(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(f.to_zdd(0, BDD_EXEC_ITERATIVE), z);
    EXPECT_EQ(f.to_zdd(), z);
    QDD qf = f.to_qdd(BDD_EXEC_ITERATIVE);
    EXPECT_EQ(qf, f.to_qdd());

    EXPECT_THROW(qf.to_bdd(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(qz.to_zdd(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(qf.to_bdd(BDD_EXEC_ITERATIVE), f);
    EXPECT_EQ(qf.to_bdd(), f);
    EXPECT_EQ(qz.to_zdd(BDD_EXEC_ITERATIVE), z);
    EXPECT_EQ(qz.to_zdd(), z);

    // UnreducedDD: expansion (constructor) and reduction.
    EXPECT_THROW(UnreducedDD(f, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(UnreducedDD(z, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(UnreducedDD(qf, BDD_EXEC_RECURSIVE), std::overflow_error);
    UnreducedDD ub(f, BDD_EXEC_ITERATIVE), uz(z, BDD_EXEC_ITERATIVE), uq(qf, BDD_EXEC_ITERATIVE);
    EXPECT_THROW(ub.reduce_as_bdd(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(uz.reduce_as_zdd(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(uq.reduce_as_qdd(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(ub.reduce_as_bdd(BDD_EXEC_ITERATIVE), f);
    EXPECT_EQ(ub.reduce_as_bdd(), f);
    EXPECT_EQ(uz.reduce_as_zdd(BDD_EXEC_ITERATIVE), z);
    EXPECT_EQ(uz.reduce_as_zdd(), z);
    EXPECT_EQ(uq.reduce_as_qdd(BDD_EXEC_ITERATIVE), qf);
    EXPECT_EQ(uq.reduce_as_qdd(), qf);
    EXPECT_EQ(UnreducedDD(f).reduce_as_bdd(), f);
}

// --- MTBDD / MTZDD class members -----------------------------------------

namespace {
int mt_plus(const int& p, const int& q) { return p + q; }
}  // namespace

TEST_F(ExecModeTest, MtbddModesAgree) {
    for (int i = 0; i < 4; ++i) bddnewvar();
    BDD x1 = BDD::prime(1), x2 = BDD::prime(2), x3 = BDD::prime(3);
    BDD f = (x1 & x2) | x3;
    MTBDD<int> a = MTBDD<int>::from_bdd(f, 0, 5);
    MTBDD<int> b = MTBDD<int>::from_bdd(x2, 1, 2);
    MTBDD<int> ref_sum = a + b;
    MTBDD<int> ref_ite = a.ite(b, ref_sum);
    ZDD z = (ZDD::singleton(1) * ZDD::singleton(2)) + ZDD::singleton(3) + ZDD(1);
    MTZDD<int> za = MTZDD<int>::from_zdd(z, 0, 3);
    MTZDD<int> zb = MTZDD<int>::from_zdd(ZDD::singleton(2) + ZDD::singleton(3), 0, 4);
    MTZDD<int> ref_zsum = za + zb;
    MTZDD<int> ref_zite = za.ite(zb, ref_zsum);
    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kModes[i];
        bddgc();  // clear the cache so each mode really recomputes
        EXPECT_EQ(MTBDD<int>::from_bdd(f, 0, 5, m), a);
        EXPECT_EQ(a.apply(b, mt_plus, m), ref_sum);
        EXPECT_EQ(a.ite(b, ref_sum, m), ref_ite);
        EXPECT_EQ(MTZDD<int>::from_zdd(z, 0, 3, m), za);
        EXPECT_EQ(za.apply(zb, mt_plus, m), ref_zsum);
        EXPECT_EQ(za.ite(zb, ref_zsum, m), ref_zite);
        EXPECT_EQ(ref_zsum.threshold_gt(3, m), ref_zsum.threshold_gt(3));
        EXPECT_EQ(ref_zsum.threshold_ge(3, m), ref_zsum.threshold_ge(3));
        EXPECT_EQ(ref_zsum.threshold_eq(3, m), ref_zsum.threshold_eq(3));
        EXPECT_EQ(ref_zsum.threshold_lt(3, m), ref_zsum.threshold_lt(3));
        EXPECT_EQ(ref_zsum.threshold_le(3, m), ref_zsum.threshold_le(3));
        EXPECT_EQ(ref_zsum.threshold_ne(3, m), ref_zsum.threshold_ne(3));
        EXPECT_EQ(ref_zsum.to_zdd(m), ref_zsum.to_zdd());
        EXPECT_EQ(za.to_zdd(m), z);
        EXPECT_EQ(ref_zsum.enumerate(m), ref_zsum.enumerate());
        EXPECT_EQ(ref_zsum.count(m), ref_zsum.count());
        EXPECT_EQ(ref_zsum.exact_count(m), ref_zsum.exact_count());
        EXPECT_EQ(ref_zsum.count(3, m), ref_zsum.count(3));
        EXPECT_EQ(ref_zsum.exact_count(3, m), ref_zsum.exact_count(3));
        EXPECT_EQ(za.cofactor0(2, m), za.cofactor0(2));
        EXPECT_EQ(za.cofactor1(2, m), za.cofactor1(2));
        EXPECT_EQ(za.cofactor1(2, m).to_zdd(m), z.onset0(2));
        EXPECT_EQ(za.cofactor0(2, m).to_zdd(m), z.offset(2));
        // Terminal fast paths are mode independent.
        EXPECT_EQ(MTBDD<int>::terminal(0).ite(a, b, m), b);
        EXPECT_EQ(MTZDD<int>::terminal(0).ite(za, zb, m), zb);
        EXPECT_THROW(za.cofactor0(99, m), std::invalid_argument);
    }
}

TEST_F(ExecModeTest, MtbddDeep) {
    ZDD z = deep_single_set(kDeep);
    BDD f = z.to_bdd();  // a chain of the same depth

    EXPECT_THROW(MTBDD<int>::from_bdd(f, 0, 1, BDD_EXEC_RECURSIVE), std::overflow_error);
    MTBDD<int> a = MTBDD<int>::from_bdd(f, 0, 1, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(a, MTBDD<int>::from_bdd(f));
    EXPECT_THROW(a.apply(a, mt_plus, BDD_EXEC_RECURSIVE), std::overflow_error);
    MTBDD<int> s = a.apply(a, mt_plus, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(s, a + a);
    MTBDD<int> one = MTBDD<int>::terminal(1), two = MTBDD<int>::terminal(2);
    EXPECT_THROW(a.ite(one, two, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(a.ite(one, two, BDD_EXEC_ITERATIVE), a.ite(one, two));

    EXPECT_THROW(MTZDD<int>::from_zdd(z, 0, 1, BDD_EXEC_RECURSIVE), std::overflow_error);
    MTZDD<int> za = MTZDD<int>::from_zdd(z, 0, 1, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(za, MTZDD<int>::from_zdd(z));
    EXPECT_THROW(za.apply(za, mt_plus, BDD_EXEC_RECURSIVE), std::overflow_error);
    MTZDD<int> zs = za.apply(za, mt_plus, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(zs, za + za);
    // With two terminal branches the hi cofactors collapse (g == h == 0) at
    // the first level, so use a deep then-branch to walk the whole chain.
    MTZDD<int> zone = MTZDD<int>::terminal(1);
    EXPECT_THROW(za.ite(zs, zone, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(za.ite(zs, zone, BDD_EXEC_ITERATIVE), za.ite(zs, zone));
    EXPECT_THROW(zs.threshold_gt(1, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(zs.threshold_gt(1, BDD_EXEC_ITERATIVE), z);
    EXPECT_THROW(zs.to_zdd(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(zs.to_zdd(BDD_EXEC_ITERATIVE), z);
    EXPECT_EQ(zs.to_zdd(), z);
    EXPECT_THROW(zs.enumerate(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(zs.enumerate(BDD_EXEC_ITERATIVE).size(), 1u);
    EXPECT_THROW(zs.count(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(zs.count(BDD_EXEC_ITERATIVE), 1.0);
    EXPECT_THROW(zs.exact_count(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(zs.exact_count(BDD_EXEC_ITERATIVE), bigint::BigInt(1));
    EXPECT_THROW(zs.count(2, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(zs.count(2, BDD_EXEC_ITERATIVE), 1.0);
    EXPECT_THROW(zs.exact_count(2, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(zs.exact_count(2, BDD_EXEC_ITERATIVE), bigint::BigInt(1));
    // Variable 1 sits at the bottom, so the cofactors walk the whole chain.
    EXPECT_THROW(zs.cofactor0(1, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(zs.cofactor0(1, BDD_EXEC_ITERATIVE), zs.cofactor0(1));
    EXPECT_THROW(zs.cofactor1(1, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(zs.cofactor1(1, BDD_EXEC_ITERATIVE), zs.cofactor1(1));
}

// --- ZDD class members (enumerate / print / combination / sampling) -----

namespace {

template<typename Fn>
std::string cout_capture(Fn fn) {
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    try {
        fn();
    } catch (...) {
        std::cout.rdbuf(old);
        throw;
    }
    std::cout.rdbuf(old);
    return oss.str();
}

std::string sets_str(const ZDD& f, BddExecMode m, int variant,
                     const std::vector<std::string>& names) {
    std::ostringstream oss;
    switch (variant) {
    case 0: f.print_sets(oss, m); break;
    case 1: f.print_sets(oss, ";", "+", m); break;
    default: f.print_sets(oss, ";", "+", names, m); break;
    }
    return oss.str();
}

}  // namespace

TEST_F(ExecModeTest, ZddMembersModesAgree) {
    for (int i = 0; i < 4; ++i) bddnewvar();
    ZDD a = ZDD::singleton(1), b = ZDD::singleton(2), c = ZDD::singleton(3), d = ZDD::singleton(4);
    ZDD f = (a * b) + (b * c * d) + c + ZDD(1);
    std::vector<std::string> names;
    names.push_back(""); names.push_back("p"); names.push_back("q");
    names.push_back("r"); names.push_back("s");
    std::vector<double> w(bdd_varcount + 1, 1.0);
    w[2] = 3.0; w[4] = 0.5;
    ZddCountMemo cmemo(f);

    std::vector<std::vector<bddvar>> ref_enum = f.enumerate();
    std::string ref_s0 = sets_str(f, BDD_EXEC_AUTO, 0, names);
    std::string ref_s1 = sets_str(f, BDD_EXEC_AUTO, 1, names);
    std::string ref_s2 = sets_str(f, BDD_EXEC_AUTO, 2, names);
    std::string ref_pla = cout_capture([&]() { f.print_pla(); });
    ZDD ref_comb = ZDD::combination(4, 2);
    EXPECT_EQ(ref_enum.size(), 4u);
    EXPECT_EQ(ref_comb.count(), 6.0);

    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kModes[i];
        bddgc();  // clear the cache so each mode really recomputes
        EXPECT_EQ(f.enumerate(m), ref_enum);
        EXPECT_EQ(sets_str(f, m, 0, names), ref_s0);
        EXPECT_EQ(sets_str(f, m, 1, names), ref_s1);
        EXPECT_EQ(sets_str(f, m, 2, names), ref_s2);
        EXPECT_EQ(f.to_str(m), f.to_str());
        EXPECT_EQ(f.to_str(m), ref_s0);
        EXPECT_EQ(f.to_cnf(m), f.to_cnf());
        EXPECT_EQ(f.to_cnf(names, m), f.to_cnf(names));
        EXPECT_EQ(f.to_dnf(m), f.to_dnf());
        EXPECT_EQ(f.to_dnf(names, m), f.to_dnf(names));
        EXPECT_EQ(cout_capture([&]() { f.print_pla(m); }), ref_pla);
        EXPECT_EQ(ZDD::combination(4, 2, m), ref_comb);
        EXPECT_EQ(ZDD::combination(4, 5, m), ZDD::Empty);
        EXPECT_NO_THROW(f.set_zskip(m));
        // Null / empty fast paths.
        EXPECT_EQ(ZDD::Null.to_str(m), "N");
        EXPECT_EQ(ZDD(0).to_cnf(m), "T");
        EXPECT_EQ(ZDD(0).to_dnf(m), "F");
        EXPECT_TRUE(ZDD::Null.enumerate(m).empty());
        // Sampling: every mode must return members of f.
        std::mt19937_64 rng(42);
        ZDD sk = f.sample_k(2, rng, cmemo, m);
        EXPECT_EQ(sk.count(), 2.0);
        EXPECT_EQ(sk & f, sk);
        EXPECT_EQ(f.sample_k(10, rng, cmemo, m), f);
        ZDD rs = f.random_subset(0.5, rng, m);
        EXPECT_EQ(rs & f, rs);
        EXPECT_EQ(f.random_subset(1.0, rng, m), f);
        {
            WeightedSampleMemo wm(f, w, WeightMode::Sum);
            std::vector<bddvar> s = f.weighted_sample(w, WeightMode::Sum, rng, wm, m);
            EXPECT_TRUE(f.contains(s));
            EXPECT_FALSE(s.empty());  // w(empty set) = 0 in Sum mode
        }
        {
            WeightedSampleMemo wm(f, w, WeightMode::Product);
            std::vector<bddvar> s = f.weighted_sample(w, WeightMode::Product, rng, wm, m);
            EXPECT_TRUE(f.contains(s));
        }
        {
            std::vector<double> bw = ZDD::boltzmann_weights(w, 0.5);
            WeightedSampleMemo bm(f, bw, WeightMode::Product);
            std::vector<bddvar> s = f.boltzmann_sample(w, 0.5, rng, bm, m);
            EXPECT_TRUE(f.contains(s));
        }
    }
}

TEST_F(ExecModeTest, ZddMembersDeep) {
    ZDD f = deep_single_set(kDeep);
    std::vector<std::string> names;  // empty: numeric fallback

    EXPECT_THROW(f.enumerate(BDD_EXEC_RECURSIVE), std::overflow_error);
    std::vector<std::vector<bddvar>> e = f.enumerate(BDD_EXEC_ITERATIVE);
    ASSERT_EQ(e.size(), 1u);
    EXPECT_EQ(e[0].size(), static_cast<size_t>(kDeep));
    EXPECT_EQ(e, f.enumerate());

    EXPECT_THROW(sets_str(f, BDD_EXEC_RECURSIVE, 0, names), std::overflow_error);
    EXPECT_THROW(sets_str(f, BDD_EXEC_RECURSIVE, 1, names), std::overflow_error);
    EXPECT_THROW(sets_str(f, BDD_EXEC_RECURSIVE, 2, names), std::overflow_error);
    std::string s0 = sets_str(f, BDD_EXEC_ITERATIVE, 0, names);
    EXPECT_EQ(s0, sets_str(f, BDD_EXEC_AUTO, 0, names));
    EXPECT_EQ(sets_str(f, BDD_EXEC_ITERATIVE, 1, names), sets_str(f, BDD_EXEC_AUTO, 1, names));
    EXPECT_EQ(sets_str(f, BDD_EXEC_ITERATIVE, 2, names), sets_str(f, BDD_EXEC_AUTO, 2, names));
    EXPECT_THROW(f.to_str(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(f.to_str(BDD_EXEC_ITERATIVE), s0);
    EXPECT_EQ(f.to_str(), s0);
    EXPECT_THROW(f.to_cnf(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(f.to_cnf(names, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(f.to_cnf(BDD_EXEC_ITERATIVE), f.to_cnf());
    EXPECT_EQ(f.to_cnf(names, BDD_EXEC_ITERATIVE), f.to_cnf(names));
    EXPECT_THROW(f.to_dnf(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_THROW(f.to_dnf(names, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(f.to_dnf(BDD_EXEC_ITERATIVE), f.to_dnf());
    EXPECT_EQ(f.to_dnf(names, BDD_EXEC_ITERATIVE), f.to_dnf(names));

    // print_pla recurses once per declared variable.
    EXPECT_THROW(cout_capture([&]() { f.print_pla(BDD_EXEC_RECURSIVE); }), std::overflow_error);
    std::string pla = cout_capture([&]() { f.print_pla(BDD_EXEC_ITERATIVE); });
    EXPECT_EQ(pla, cout_capture([&]() { f.print_pla(); }));
    EXPECT_NE(pla.find(".e"), std::string::npos);

    EXPECT_THROW(f.set_zskip(BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_NO_THROW(f.set_zskip(BDD_EXEC_ITERATIVE));
    EXPECT_NO_THROW(f.set_zskip());

    // combination recurses once per variable.
    EXPECT_THROW(ZDD::combination(kDeep, 2, BDD_EXEC_RECURSIVE), std::overflow_error);
    ZDD c2 = ZDD::combination(kDeep, 2, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(c2, ZDD::combination(kDeep, 2));
    EXPECT_EQ(c2.count(), static_cast<double>(kDeep) * (kDeep - 1) / 2.0);

    // Sampling over {{1..n}, {1}}.
    ZDD g = f + ZDD::singleton(1);
    std::mt19937_64 rng(7);
    {
        ZddCountMemo memo(g);
        EXPECT_THROW(g.sample_k(1, rng, memo, BDD_EXEC_RECURSIVE), std::overflow_error);
    }
    ZddCountMemo memo(g);
    ZDD sk = g.sample_k(1, rng, memo, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(sk.count(), 1.0);
    EXPECT_EQ(sk & g, sk);
    EXPECT_EQ(g.sample_k(1, rng, memo).count(), 1.0);
    EXPECT_THROW(g.random_subset(0.5, rng, BDD_EXEC_RECURSIVE), std::overflow_error);
    ZDD rs = g.random_subset(0.5, rng, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(rs & g, rs);
    std::vector<double> w(bdd_varcount + 1, 1.0);
    {
        WeightedSampleMemo wm(g, w, WeightMode::Sum);
        EXPECT_THROW(g.weighted_sample(w, WeightMode::Sum, rng, wm, BDD_EXEC_RECURSIVE),
                     std::overflow_error);
    }
    {
        WeightedSampleMemo wm(g, w, WeightMode::Sum);
        std::vector<bddvar> s = g.weighted_sample(w, WeightMode::Sum, rng, wm, BDD_EXEC_ITERATIVE);
        EXPECT_TRUE(g.contains(s));
    }
    {
        WeightedSampleMemo wm(g, w, WeightMode::Product);
        EXPECT_THROW(g.weighted_sample(w, WeightMode::Product, rng, wm, BDD_EXEC_RECURSIVE),
                     std::overflow_error);
    }
    {
        WeightedSampleMemo wm(g, w, WeightMode::Product);
        std::vector<bddvar> s = g.weighted_sample(w, WeightMode::Product, rng, wm);
        EXPECT_TRUE(g.contains(s));
    }
    std::vector<double> bw = ZDD::boltzmann_weights(w, 0.1);
    {
        WeightedSampleMemo bm(g, bw, WeightMode::Product);
        EXPECT_THROW(g.boltzmann_sample(w, 0.1, rng, bm, BDD_EXEC_RECURSIVE), std::overflow_error);
    }
    {
        WeightedSampleMemo bm(g, bw, WeightMode::Product);
        std::vector<bddvar> s = g.boltzmann_sample(w, 0.1, rng, bm, BDD_EXEC_ITERATIVE);
        EXPECT_TRUE(g.contains(s));
    }
}
