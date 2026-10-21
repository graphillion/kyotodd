// Execution-mode dispatch tests for operations that gained BddExecMode
// overloads late: each must agree with its Auto form in every mode, and
// on DDs deeper than BDD_RecurLimit the forced Recursive mode must hit the
// recursion limit while Iterative (and Auto) succeed.

#include <gtest/gtest.h>
#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <stdexcept>
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
