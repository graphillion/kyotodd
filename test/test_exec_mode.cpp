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
#include <stdexcept>
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
