#include <gtest/gtest.h>
#include "bdd.h"
#include "bdd_internal.h"
#include <random>
#include <stdexcept>
#include <vector>

using namespace kyotodd;

// --- bddandexist (relational product) ---
//
// The reference result is bddexist(bddand(f, g), cube). AND and EXIST use
// their own cache slots, so computing the reference cannot pre-populate the
// BDD_OP_AND_EXIST slots used by the operation under test.

namespace {

// Random BDD over vars as an OR of random minterm-like cubes.
BDD random_bdd(std::mt19937& rng, const std::vector<bddvar>& vars,
               int terms) {
    std::uniform_int_distribution<int> lit(0, 2);  // 0: neg, 1: pos, 2: absent
    BDD f(0);
    for (int t = 0; t < terms; ++t) {
        BDD c(1);
        for (size_t i = 0; i < vars.size(); ++i) {
            int k = lit(rng);
            if (k == 0) c &= ~BDD::prime(vars[i]);
            else if (k == 1) c &= BDD::prime(vars[i]);
        }
        f |= c;
    }
    return f;
}

// Variable set in support() encoding (disjunction of primes).
BDD var_set(const std::vector<bddvar>& vars) {
    BDD s(0);
    for (size_t i = 0; i < vars.size(); ++i) s |= BDD::prime(vars[i]);
    return s;
}

// Random subset of vars.
std::vector<bddvar> random_subset(std::mt19937& rng,
                                  const std::vector<bddvar>& vars) {
    std::uniform_int_distribution<int> coin(0, 1);
    std::vector<bddvar> sub;
    for (size_t i = 0; i < vars.size(); ++i) {
        if (coin(rng)) sub.push_back(vars[i]);
    }
    return sub;
}

BDD reference(const BDD& f, const BDD& g, const BDD& cube) {
    return BDD_ID(bddexist(bddand(f.id(), g.id()), cube.id()));
}

}  // namespace

class BddAndExistTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
        for (int i = 0; i < 4; ++i) vars.push_back(BDD_NewVar());
    }
    std::vector<bddvar> vars;
};

TEST_F(BddAndExistTest, NullOperand) {
    bddp a = bddprime(vars[0]);
    EXPECT_EQ(bddandexist(bddnull, a, a), bddnull);
    EXPECT_EQ(bddandexist(a, bddnull, a), bddnull);
    EXPECT_EQ(bddandexist(a, a, bddnull), bddnull);
}

TEST_F(BddAndExistTest, TerminalCases) {
    BDD a = BDD::prime(vars[0]);
    BDD b = BDD::prime(vars[1]);
    BDD c = BDD::prime(vars[2]);
    BDD f = (a & b) | c;
    BDD g = a | ~c;
    BDD cube = a | c;

    // False operand or complementary operands -> false
    EXPECT_EQ(bddandexist(bddfalse, g.id(), cube.id()), bddfalse);
    EXPECT_EQ(bddandexist(f.id(), bddfalse, cube.id()), bddfalse);
    EXPECT_EQ(bddandexist(f.id(), (~f).id(), cube.id()), bddfalse);
    // Empty variable set -> plain AND
    EXPECT_EQ(bddandexist(f.id(), g.id(), bddfalse), (f & g).id());
    // True operand or identical operands -> plain quantification
    EXPECT_EQ(bddandexist(bddtrue, g.id(), cube.id()), g.exist(cube).id());
    EXPECT_EQ(bddandexist(f.id(), bddtrue, cube.id()), f.exist(cube).id());
    EXPECT_EQ(bddandexist(f.id(), f.id(), cube.id()), f.exist(cube).id());
    EXPECT_EQ(bddandexist(bddtrue, bddtrue, cube.id()), bddtrue);
}

TEST_F(BddAndExistTest, SmallExample) {
    BDD a = BDD::prime(vars[0]);
    BDD b = BDD::prime(vars[1]);
    BDD c = BDD::prime(vars[2]);
    // exists b. (a & b) & (b -> c) = a & c
    BDD f = a & b;
    BDD g = ~b | c;
    EXPECT_EQ(BDD_ID(bddandexist(f.id(), g.id(), b.id())), a & c);
    // exists a, b. (a & b) & (b -> c) = c
    EXPECT_EQ(BDD_ID(bddandexist(f.id(), g.id(), (a | b).id())), c);
}

TEST_F(BddAndExistTest, CubeComplementIgnored) {
    BDD a = BDD::prime(vars[0]);
    BDD b = BDD::prime(vars[1]);
    BDD f = a | b;
    BDD g = ~a | b;
    BDD cube = a;
    EXPECT_EQ(bddandexist(f.id(), g.id(), bddnot(cube.id())),
              bddandexist(f.id(), g.id(), cube.id()));
}

TEST_F(BddAndExistTest, VectorMatchesCube) {
    BDD a = BDD::prime(vars[0]);
    BDD b = BDD::prime(vars[1]);
    BDD c = BDD::prime(vars[2]);
    BDD d = BDD::prime(vars[3]);
    BDD f = (a & b) | (c & ~d);
    BDD g = (a ^ c) | d;
    std::vector<bddvar> qv;
    qv.push_back(vars[2]);
    qv.push_back(vars[0]);
    qv.push_back(vars[2]);  // duplicates are allowed
    BDD got = BDD_ID(bddandexist(f.id(), g.id(), qv));
    EXPECT_EQ(got, reference(f, g, a | c));
    EXPECT_EQ(BDD_ID(bddandexist(f.id(), g.id(), std::vector<bddvar>())),
              f & g);
}

TEST_F(BddAndExistTest, VectorOutOfRangeThrows) {
    BDD a = BDD::prime(vars[0]);
    std::vector<bddvar> qv(1, static_cast<bddvar>(vars.size() + 1));
    EXPECT_THROW(bddandexist(a.id(), a.id(), qv), std::invalid_argument);
}

// Random cross-check against exist(and) for each execution mode. Each mode
// runs on a fresh node table so that no mode can reuse cache entries of
// another one.
TEST(BddAndExistRandomTest, MatchesExistOfAnd) {
    const BddExecMode modes[] = {BDD_EXEC_RECURSIVE, BDD_EXEC_ITERATIVE,
                                 BDD_EXEC_AUTO};
    for (size_t m = 0; m < 3; ++m) {
        BDD_Init(1024, UINT64_MAX);
        std::vector<bddvar> vars;
        for (int i = 0; i < 8; ++i) vars.push_back(BDD_NewVar());
        std::mt19937 rng(12345);
        for (int trial = 0; trial < 200; ++trial) {
            BDD f = random_bdd(rng, vars, 6);
            BDD g = random_bdd(rng, vars, 6);
            BDD cube = var_set(random_subset(rng, vars));
            BDD got = BDD_ID(bddandexist(f.id(), g.id(), cube.id(), modes[m]));
            ASSERT_EQ(got, reference(f, g, cube))
                << "mode=" << m << " trial=" << trial;
        }
    }
}

// Same check with a variable order that differs from the variable numbers,
// so that skipping cube variables must follow levels, not numbers.
TEST(BddAndExistRandomTest, MatchesExistOfAndWithPermutedOrder) {
    const BddExecMode modes[] = {BDD_EXEC_RECURSIVE, BDD_EXEC_ITERATIVE};
    for (size_t m = 0; m < 2; ++m) {
        BDD_Init(1024, UINT64_MAX);
        std::vector<bddvar> vars;
        // Each new variable is inserted at level 1, so variable i ends up
        // at level (8 - i + 1): the order is reversed.
        for (int i = 0; i < 8; ++i) vars.push_back(BDD_NewVarOfLev(1));
        ASSERT_EQ(bddlevofvar(vars[0]), 8u);
        std::mt19937 rng(777);
        for (int trial = 0; trial < 200; ++trial) {
            BDD f = random_bdd(rng, vars, 5);
            BDD g = random_bdd(rng, vars, 5);
            BDD cube = var_set(random_subset(rng, vars));
            BDD got = BDD_ID(bddandexist(f.id(), g.id(), cube.id(), modes[m]));
            ASSERT_EQ(got, reference(f, g, cube))
                << "mode=" << m << " trial=" << trial;
        }
    }
}

// --- Deep BDDs: dispatch between _rec and _iter ---
//
// Chains deeper than BDD_RecurLimit. Roots are held in BDD objects so that
// they survive the GC retry performed by bdd_gc_guard.

class BddAndExistDeepTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
        for (int i = 0; i < kDepth; ++i) BDD_NewVar();
    }

    static const int kDepth = BDD_RecurLimit + 100;

    // AND of the variables i with (i % 2 == parity), built bottom-up so the
    // root has the highest level.
    static BDD build_chain(int parity) {
        bddp f = bddtrue;
        for (int i = 1; i <= kDepth; ++i) {
            if (i % 2 == parity) {
                f = BDD::getnode_raw(static_cast<bddvar>(i), bddfalse, f);
            }
        }
        return BDD_ID(f);
    }

    // Variable set {1, 2, kDepth/2, kDepth - 1, kDepth}.
    static std::vector<bddvar> qvars() {
        std::vector<bddvar> v;
        v.push_back(1);
        v.push_back(2);
        v.push_back(kDepth / 2);
        v.push_back(static_cast<bddvar>(kDepth - 1));
        v.push_back(static_cast<bddvar>(kDepth));
        return v;
    }
};

TEST_F(BddAndExistDeepTest, RecursiveOverflowsIterativeSucceeds) {
    BDD f = build_chain(0);
    BDD g = build_chain(1);
    BDD cube = var_set(qvars());
    EXPECT_THROW(bddandexist(f.id(), g.id(), cube.id(), BDD_EXEC_RECURSIVE),
                 std::overflow_error);
    BDD it = BDD_ID(bddandexist(f.id(), g.id(), cube.id(), BDD_EXEC_ITERATIVE));
    BDD expected = reference(f, g, cube);
    EXPECT_EQ(it, expected);
    // The quantified variables are gone from the result.
    EXPECT_EQ(it.at1(1), it);
    EXPECT_EQ(it.at1(static_cast<bddvar>(kDepth)), it);
}

TEST_F(BddAndExistDeepTest, AutoDispatchesToIterative) {
    BDD f = build_chain(0);
    BDD g = build_chain(1);
    BDD cube = var_set(qvars());
    // The operation under test runs first (see the cache note above).
    BDD got(0);
    EXPECT_NO_THROW(got = BDD_ID(bddandexist(f.id(), g.id(), cube.id())));
    EXPECT_EQ(got, reference(f, g, cube));
}

TEST_F(BddAndExistDeepTest, VectorFormHonoursExecMode) {
    BDD f = build_chain(0);
    BDD g = build_chain(1);
    EXPECT_THROW(bddandexist(f.id(), g.id(), qvars(), BDD_EXEC_RECURSIVE),
                 std::overflow_error);
    BDD got(0);
    EXPECT_NO_THROW(got = BDD_ID(bddandexist(f.id(), g.id(), qvars())));
    EXPECT_EQ(got, BDD_ID(bddandexist(f.id(), g.id(), qvars(),
                                      BDD_EXEC_ITERATIVE)));
    EXPECT_EQ(got, reference(f, g, var_set(qvars())));
}
