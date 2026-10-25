#include <gtest/gtest.h>
#include "bdd.h"
#include "bdd_internal.h"
#include <algorithm>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

using namespace kyotodd;

// --- bddpermute / bddpermutez (variable substitution) ---
//
// BDD results are checked against brute-force truth-table evaluation of the
// substituted function; ZDD results against a family rebuilt from the
// substituted sets. Neither reference touches the BDD_OP_PERMUTE /
// BDD_OP_PERMUTEZ cache slots, so a mode under test cannot be served from
// entries written by the other mode (each test uses fresh functions and
// runs one mode first).

namespace {

typedef std::vector<bddvar> VarVec;

// Random BDD over vars as an OR of random cubes.
BDD random_bdd(std::mt19937& rng, const VarVec& vars, int terms) {
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

// Evaluate f under the assignment value[v] (indexed by variable number).
bool eval_bdd(const BDD& f, const std::vector<int>& value) {
    BDD g = f;
    for (bddvar v = 1; v < value.size(); ++v) {
        g = value[v] ? g.at1(v) : g.at0(v);
    }
    return g == BDD(1);
}

// Image of variable v under (from, to); v itself when not listed.
bddvar image(bddvar v, const VarVec& from, const VarVec& to) {
    for (size_t i = 0; i < from.size(); ++i) {
        if (from[i] == v) return to[i];
    }
    return v;
}

// Check bddpermute(f) against brute force over all assignments of 1..n.
void check_bdd(const BDD& f, const VarVec& from, const VarVec& to,
               bddvar n, BddExecMode mode) {
    BDD r = BDD_ID(bddpermute(f.id(), from, to, mode));
    std::vector<int> value(n + 1, 0);
    for (unsigned mask = 0; mask < (1u << n); ++mask) {
        for (bddvar v = 1; v <= n; ++v) value[v] = (mask >> (v - 1)) & 1;
        // f with variable v read from x_{image(v)}
        std::vector<int> subst(n + 1, 0);
        for (bddvar v = 1; v <= n; ++v) subst[v] = value[image(v, from, to)];
        bool expected = eval_bdd(f, subst);
        bool actual = eval_bdd(r, value);
        ASSERT_EQ(expected, actual) << "mask=" << mask;
    }
}

// Random family of random subsets of vars, as a list of sets.
std::vector<std::set<bddvar> > random_sets(std::mt19937& rng,
                                           const VarVec& vars, int count) {
    std::uniform_int_distribution<int> coin(0, 1);
    std::vector<std::set<bddvar> > sets;
    for (int i = 0; i < count; ++i) {
        std::set<bddvar> s;
        for (size_t j = 0; j < vars.size(); ++j) {
            if (coin(rng)) s.insert(vars[j]);
        }
        sets.push_back(s);
    }
    return sets;
}

ZDD family_of(const std::vector<std::set<bddvar> >& sets) {
    ZDD z(0);
    for (size_t i = 0; i < sets.size(); ++i) {
        z = z + ZDD::single_set(VarVec(sets[i].begin(), sets[i].end()));
    }
    return z;
}

// Expected result: substitute inside every set (duplicates collapse).
ZDD expected_zdd(const std::vector<std::set<bddvar> >& sets,
                 const VarVec& from, const VarVec& to) {
    std::vector<std::set<bddvar> > mapped;
    for (size_t i = 0; i < sets.size(); ++i) {
        std::set<bddvar> m;
        for (std::set<bddvar>::const_iterator it = sets[i].begin();
             it != sets[i].end(); ++it) {
            m.insert(image(*it, from, to));
        }
        mapped.push_back(m);
    }
    return family_of(mapped);
}

void check_zdd(const std::vector<std::set<bddvar> >& sets, const VarVec& from,
               const VarVec& to, BddExecMode mode) {
    ZDD f = family_of(sets);
    ZDD r = ZDD_ID(bddpermutez(f.id(), from, to, mode));
    ZDD e = expected_zdd(sets, from, to);
    ASSERT_EQ(e, r);
    ASSERT_EQ(e.card(), r.card());
}

}  // namespace

// ===========================================================================
// BDD
// ===========================================================================

class BddPermuteTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
        for (int i = 0; i < 6; ++i) vars.push_back(BDD_NewVar());
    }
    VarVec vars;
    BDD x(int i) const { return BDD::prime(vars[i]); }
};

TEST_F(BddPermuteTest, NullAndTerminals) {
    VarVec from, to;
    from.push_back(vars[0]); to.push_back(vars[1]);
    EXPECT_EQ(bddpermute(bddnull, from, to), bddnull);
    EXPECT_EQ(bddpermute(bddfalse, from, to), bddfalse);
    EXPECT_EQ(bddpermute(bddtrue, from, to), bddtrue);
}

TEST_F(BddPermuteTest, IdentityReturnsSameNode) {
    BDD f = (x(0) & x(1)) | ~x(2);
    VarVec empty;
    EXPECT_EQ(bddpermute(f.id(), empty, empty), f.id());
    VarVec from, to;
    from.push_back(vars[0]); to.push_back(vars[0]);
    from.push_back(vars[3]); to.push_back(vars[3]);
    EXPECT_EQ(bddpermute(f.id(), from, to), f.id());
}

TEST_F(BddPermuteTest, Validation) {
    BDD f = x(0) & x(1);
    VarVec from, to;
    from.push_back(vars[0]);
    EXPECT_THROW(bddpermute(f.id(), from, to), std::invalid_argument);
    to.push_back(0);
    EXPECT_THROW(bddpermute(f.id(), from, to), std::invalid_argument);
    to[0] = bdd_varcount + 1;
    EXPECT_THROW(bddpermute(f.id(), from, to), std::invalid_argument);
    to[0] = vars[1];
    from.push_back(vars[0]); to.push_back(vars[2]);  // duplicate in from
    EXPECT_THROW(bddpermute(f.id(), from, to), std::invalid_argument);
    // Validation happens even for bddnull / terminals.
    EXPECT_THROW(bddpermute(bddtrue, from, to), std::invalid_argument);
}

TEST_F(BddPermuteTest, UnaffectedFunctionIsUnchanged) {
    // f depends only on variables below every changed one.
    BDD f = x(0) ^ x(1);
    VarVec from, to;
    from.push_back(vars[4]); to.push_back(vars[5]);
    from.push_back(vars[5]); to.push_back(vars[4]);
    EXPECT_EQ(bddpermute(f.id(), from, to), f.id());
}

TEST_F(BddPermuteTest, SwapMatchesBddswap) {
    std::mt19937 rng(1);
    for (int t = 0; t < 20; ++t) {
        BDD f = random_bdd(rng, vars, 5);
        for (size_t a = 0; a < vars.size(); ++a) {
            for (size_t b = a + 1; b < vars.size(); ++b) {
                VarVec from, to;
                from.push_back(vars[a]); to.push_back(vars[b]);
                from.push_back(vars[b]); to.push_back(vars[a]);
                EXPECT_EQ(bddpermute(f.id(), from, to),
                          bddswap(f.id(), vars[a], vars[b]));
            }
        }
    }
}

TEST_F(BddPermuteTest, SimpleRenames) {
    // x1 & ~x2 with 1 -> 4, 2 -> 5
    BDD f = x(0) & ~x(1);
    VarVec from, to;
    from.push_back(vars[0]); to.push_back(vars[3]);
    from.push_back(vars[1]); to.push_back(vars[4]);
    EXPECT_EQ(BDD_ID(bddpermute(f.id(), from, to)), x(3) & ~x(4));
    // The result no longer mentions the source variables.
    VarVec sup = BDD_ID(bddpermute(f.id(), from, to)).support_vec();
    std::sort(sup.begin(), sup.end());
    ASSERT_EQ(sup.size(), 2u);
    EXPECT_EQ(sup[0], vars[3]);
    EXPECT_EQ(sup[1], vars[4]);
}

TEST_F(BddPermuteTest, NonInjective) {
    VarVec from, to;
    from.push_back(vars[0]); to.push_back(vars[2]);
    from.push_back(vars[1]); to.push_back(vars[2]);
    // x1 & ~x2  ->  x3 & ~x3 = false
    EXPECT_EQ(bddpermute((x(0) & ~x(1)).id(), from, to), bddfalse);
    // x1 | x2  ->  x3
    EXPECT_EQ(BDD_ID(bddpermute((x(0) | x(1)).id(), from, to)), x(2));
    // x1 ^ x2  ->  false
    EXPECT_EQ(bddpermute((x(0) ^ x(1)).id(), from, to), bddfalse);
}

TEST_F(BddPermuteTest, TargetAlreadyInSupport) {
    // f(x1, x2) = x1 & ~x2 with 1 -> 2 gives x2 & ~x2 = false.
    BDD f = x(0) & ~x(1);
    VarVec from, to;
    from.push_back(vars[0]); to.push_back(vars[1]);
    EXPECT_EQ(bddpermute(f.id(), from, to), bddfalse);
    // (x1 | x3) & x2 with 1 -> 2 gives x2.
    BDD g = (x(0) | x(2)) & x(1);
    EXPECT_EQ(BDD_ID(bddpermute(g.id(), from, to)), x(1));
}

TEST_F(BddPermuteTest, ComplementFactorsOut) {
    std::mt19937 rng(2);
    VarVec from, to;
    from.push_back(vars[0]); to.push_back(vars[5]);
    from.push_back(vars[5]); to.push_back(vars[2]);
    from.push_back(vars[2]); to.push_back(vars[0]);
    for (int t = 0; t < 10; ++t) {
        BDD f = random_bdd(rng, vars, 4);
        EXPECT_EQ(bddpermute((~f).id(), from, to),
                  bddnot(bddpermute(f.id(), from, to)));
    }
}

TEST_F(BddPermuteTest, CycleAndReverseBruteForce) {
    std::mt19937 rng(3);
    // 3-cycle on the top three variables (crosses levels both ways)
    VarVec c_from, c_to;
    c_from.push_back(vars[3]); c_to.push_back(vars[4]);
    c_from.push_back(vars[4]); c_to.push_back(vars[5]);
    c_from.push_back(vars[5]); c_to.push_back(vars[3]);
    // Full reversal of the order
    VarVec r_from, r_to;
    for (size_t i = 0; i < vars.size(); ++i) {
        r_from.push_back(vars[i]);
        r_to.push_back(vars[vars.size() - 1 - i]);
    }
    for (int t = 0; t < 10; ++t) {
        BDD f = random_bdd(rng, vars, 5);
        check_bdd(f, c_from, c_to, 6, BddExecMode::Recursive);
        check_bdd(f, r_from, r_to, 6, BddExecMode::Recursive);
    }
}

TEST_F(BddPermuteTest, RandomMappingsRecursiveFirst) {
    std::mt19937 rng(4);
    std::uniform_int_distribution<int> pick(0, 5);
    for (int t = 0; t < 40; ++t) {
        BDD f = random_bdd(rng, vars, 5);
        // Random (possibly non-injective) mapping of a random subset.
        VarVec from, to;
        for (size_t i = 0; i < vars.size(); ++i) {
            if (pick(rng) < 3) {
                from.push_back(vars[i]);
                to.push_back(vars[pick(rng)]);
            }
        }
        check_bdd(f, from, to, 6, BddExecMode::Recursive);
        check_bdd(f, from, to, 6, BddExecMode::Iterative);
        check_bdd(f, from, to, 6, BddExecMode::Auto);
    }
}

TEST_F(BddPermuteTest, RandomMappingsIterativeFirst) {
    std::mt19937 rng(5);
    std::uniform_int_distribution<int> pick(0, 5);
    for (int t = 0; t < 40; ++t) {
        BDD f = random_bdd(rng, vars, 5);
        VarVec from, to;
        for (size_t i = 0; i < vars.size(); ++i) {
            if (pick(rng) < 3) {
                from.push_back(vars[i]);
                to.push_back(vars[pick(rng)]);
            }
        }
        check_bdd(f, from, to, 6, BddExecMode::Iterative);
        check_bdd(f, from, to, 6, BddExecMode::Recursive);
    }
}

TEST_F(BddPermuteTest, NextStateToCurrentState) {
    // Reachability-style use: current vars 1..3, next vars 4..6.
    // T(x, x') = (x1' = ~x1) & (x2' = x1) & (x3' = x2); image of
    // init = ~x1 & ~x2 & ~x3 is x1' & ~x2' & ~x3', renamed to x1 & ~x2 & ~x3.
    BDD T = (x(3) ^ x(0)) & ~(x(4) ^ x(0)) & ~(x(5) ^ x(1));
    BDD init = ~x(0) & ~x(1) & ~x(2);
    VarVec cur, nxt;
    for (int i = 0; i < 3; ++i) { cur.push_back(vars[i]); nxt.push_back(vars[i + 3]); }
    BDD img_next = init.and_exist(T, cur);
    BDD img = BDD_ID(bddpermute(img_next.id(), nxt, cur));
    EXPECT_EQ(img, x(0) & ~x(1) & ~x(2));
}

TEST_F(BddPermuteTest, SameMappingSharesCacheKey) {
    // Two calls with the same (differently ordered) mapping must agree,
    // and a different mapping must not be served from the first one's cache.
    BDD f = (x(0) & x(1)) | (x(2) & ~x(3));
    VarVec from1, to1, from2, to2, from3, to3;
    from1.push_back(vars[0]); to1.push_back(vars[4]);
    from1.push_back(vars[2]); to1.push_back(vars[5]);
    from2.push_back(vars[2]); to2.push_back(vars[5]);
    from2.push_back(vars[0]); to2.push_back(vars[4]);
    from3.push_back(vars[0]); to3.push_back(vars[5]);
    from3.push_back(vars[2]); to3.push_back(vars[4]);
    bddp r1 = bddpermute(f.id(), from1, to1);
    bddp r2 = bddpermute(f.id(), from2, to2);
    bddp r3 = bddpermute(f.id(), from3, to3);
    EXPECT_EQ(r1, r2);
    EXPECT_NE(r1, r3);
    EXPECT_EQ(BDD_ID(r1), (x(4) & x(1)) | (x(5) & ~x(3)));
    EXPECT_EQ(BDD_ID(r3), (x(5) & x(1)) | (x(4) & ~x(3)));
}

// Variables whose numbers do not follow level order.
class BddPermuteLevelTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
        for (int i = 0; i < 4; ++i) BDD_NewVar();   // vars 1..4 at levels 1..4
        BDD_NewVarOfLev(2);                           // var 5 at level 2
        BDD_NewVarOfLev(1);                           // var 6 at level 1
    }
};

TEST_F(BddPermuteLevelTest, BruteForceWithShuffledLevels) {
    std::mt19937 rng(6);
    std::uniform_int_distribution<int> pick(1, 6);
    VarVec vars;
    for (bddvar v = 1; v <= 6; ++v) vars.push_back(v);
    for (int t = 0; t < 30; ++t) {
        BDD f = random_bdd(rng, vars, 5);
        VarVec from, to;
        for (bddvar v = 1; v <= 6; ++v) {
            if (pick(rng) <= 3) { from.push_back(v); to.push_back(pick(rng)); }
        }
        check_bdd(f, from, to, 6, BddExecMode::Recursive);
        check_bdd(f, from, to, 6, BddExecMode::Iterative);
    }
}

// ===========================================================================
// ZDD
// ===========================================================================

class ZddPermuteTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
        for (int i = 0; i < 6; ++i) vars.push_back(BDD_NewVar());
    }
    VarVec vars;
    std::set<bddvar> S(int a) const { std::set<bddvar> s; s.insert(vars[a]); return s; }
    std::set<bddvar> S(int a, int b) const { std::set<bddvar> s = S(a); s.insert(vars[b]); return s; }
    std::set<bddvar> S(int a, int b, int c) const { std::set<bddvar> s = S(a, b); s.insert(vars[c]); return s; }
};

TEST_F(ZddPermuteTest, NullAndTerminals) {
    VarVec from, to;
    from.push_back(vars[0]); to.push_back(vars[1]);
    EXPECT_EQ(bddpermutez(bddnull, from, to), bddnull);
    EXPECT_EQ(bddpermutez(bddempty, from, to), bddempty);
    EXPECT_EQ(bddpermutez(bddsingle, from, to), bddsingle);
}

TEST_F(ZddPermuteTest, IdentityAndValidation) {
    ZDD f = ZDD::single_set(VarVec(1, vars[0])) + ZDD(1);
    VarVec empty;
    EXPECT_EQ(bddpermutez(f.id(), empty, empty), f.id());
    VarVec from, to;
    from.push_back(vars[0]);
    EXPECT_THROW(bddpermutez(f.id(), from, to), std::invalid_argument);
    to.push_back(bdd_varcount + 1);
    EXPECT_THROW(bddpermutez(f.id(), from, to), std::invalid_argument);
    to[0] = vars[1];
    from.push_back(vars[0]); to.push_back(vars[2]);
    EXPECT_THROW(bddpermutez(f.id(), from, to), std::invalid_argument);
}

TEST_F(ZddPermuteTest, SwapMatchesBddswapz) {
    std::mt19937 rng(11);
    for (int t = 0; t < 20; ++t) {
        ZDD f = family_of(random_sets(rng, vars, 6));
        for (size_t a = 0; a < vars.size(); ++a) {
            for (size_t b = a + 1; b < vars.size(); ++b) {
                VarVec from, to;
                from.push_back(vars[a]); to.push_back(vars[b]);
                from.push_back(vars[b]); to.push_back(vars[a]);
                EXPECT_EQ(bddpermutez(f.id(), from, to),
                          bddswapz(f.id(), vars[a], vars[b]));
            }
        }
    }
}

TEST_F(ZddPermuteTest, SimpleRename) {
    // {{1,2},{3}} with 1 -> 4, 2 -> 5  ->  {{4,5},{3}}
    std::vector<std::set<bddvar> > sets;
    sets.push_back(S(0, 1));
    sets.push_back(S(2));
    VarVec from, to;
    from.push_back(vars[0]); to.push_back(vars[3]);
    from.push_back(vars[1]); to.push_back(vars[4]);
    ZDD r = ZDD_ID(bddpermutez(family_of(sets).id(), from, to));
    std::vector<std::set<bddvar> > exp;
    exp.push_back(S(3, 4));
    exp.push_back(S(2));
    EXPECT_EQ(r, family_of(exp));
    EXPECT_EQ(r.card(), 2u);
}

TEST_F(ZddPermuteTest, NonInjectiveCollapsesElements) {
    // {{1,2},{3}} with 1 -> 4, 2 -> 4  ->  {{4},{3}}
    std::vector<std::set<bddvar> > sets;
    sets.push_back(S(0, 1));
    sets.push_back(S(2));
    VarVec from, to;
    from.push_back(vars[0]); to.push_back(vars[3]);
    from.push_back(vars[1]); to.push_back(vars[3]);
    ZDD r = ZDD_ID(bddpermutez(family_of(sets).id(), from, to));
    std::vector<std::set<bddvar> > exp;
    exp.push_back(S(3));
    exp.push_back(S(2));
    EXPECT_EQ(r, family_of(exp));
}

TEST_F(ZddPermuteTest, NonInjectiveMergesSets) {
    // {{1},{2},{1,2}} with 1 -> 2  ->  {{2}}  (three sets become one)
    std::vector<std::set<bddvar> > sets;
    sets.push_back(S(0));
    sets.push_back(S(1));
    sets.push_back(S(0, 1));
    VarVec from, to;
    from.push_back(vars[0]); to.push_back(vars[1]);
    ZDD r = ZDD_ID(bddpermutez(family_of(sets).id(), from, to));
    EXPECT_EQ(r, ZDD::singleton(vars[1]));
    EXPECT_EQ(r.card(), 1u);
}

TEST_F(ZddPermuteTest, ComplementFactorsOut) {
    std::mt19937 rng(12);
    VarVec from, to;
    from.push_back(vars[0]); to.push_back(vars[5]);
    from.push_back(vars[5]); to.push_back(vars[2]);
    from.push_back(vars[2]); to.push_back(vars[0]);
    for (int t = 0; t < 10; ++t) {
        ZDD f = family_of(random_sets(rng, vars, 5));
        EXPECT_EQ(bddpermutez((~f).id(), from, to),
                  bddnot(bddpermutez(f.id(), from, to)));
    }
}

TEST_F(ZddPermuteTest, CycleAndReverse) {
    std::mt19937 rng(13);
    VarVec c_from, c_to;
    c_from.push_back(vars[3]); c_to.push_back(vars[4]);
    c_from.push_back(vars[4]); c_to.push_back(vars[5]);
    c_from.push_back(vars[5]); c_to.push_back(vars[3]);
    VarVec r_from, r_to;
    for (size_t i = 0; i < vars.size(); ++i) {
        r_from.push_back(vars[i]);
        r_to.push_back(vars[vars.size() - 1 - i]);
    }
    for (int t = 0; t < 10; ++t) {
        std::vector<std::set<bddvar> > sets = random_sets(rng, vars, 8);
        check_zdd(sets, c_from, c_to, BddExecMode::Recursive);
        check_zdd(sets, r_from, r_to, BddExecMode::Recursive);
    }
}

TEST_F(ZddPermuteTest, RandomMappingsRecursiveFirst) {
    std::mt19937 rng(14);
    std::uniform_int_distribution<int> pick(0, 5);
    for (int t = 0; t < 40; ++t) {
        std::vector<std::set<bddvar> > sets = random_sets(rng, vars, 8);
        VarVec from, to;
        for (size_t i = 0; i < vars.size(); ++i) {
            if (pick(rng) < 3) {
                from.push_back(vars[i]);
                to.push_back(vars[pick(rng)]);
            }
        }
        check_zdd(sets, from, to, BddExecMode::Recursive);
        check_zdd(sets, from, to, BddExecMode::Iterative);
        check_zdd(sets, from, to, BddExecMode::Auto);
    }
}

TEST_F(ZddPermuteTest, RandomMappingsIterativeFirst) {
    std::mt19937 rng(15);
    std::uniform_int_distribution<int> pick(0, 5);
    for (int t = 0; t < 40; ++t) {
        std::vector<std::set<bddvar> > sets = random_sets(rng, vars, 8);
        VarVec from, to;
        for (size_t i = 0; i < vars.size(); ++i) {
            if (pick(rng) < 3) {
                from.push_back(vars[i]);
                to.push_back(vars[pick(rng)]);
            }
        }
        check_zdd(sets, from, to, BddExecMode::Iterative);
        check_zdd(sets, from, to, BddExecMode::Recursive);
    }
}

class ZddPermuteLevelTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
        for (int i = 0; i < 4; ++i) BDD_NewVar();
        BDD_NewVarOfLev(2);
        BDD_NewVarOfLev(1);
    }
};

TEST_F(ZddPermuteLevelTest, ShuffledLevels) {
    std::mt19937 rng(16);
    std::uniform_int_distribution<int> pick(1, 6);
    VarVec vars;
    for (bddvar v = 1; v <= 6; ++v) vars.push_back(v);
    for (int t = 0; t < 30; ++t) {
        std::vector<std::set<bddvar> > sets = random_sets(rng, vars, 8);
        VarVec from, to;
        for (bddvar v = 1; v <= 6; ++v) {
            if (pick(rng) <= 3) { from.push_back(v); to.push_back(pick(rng)); }
        }
        check_zdd(sets, from, to, BddExecMode::Recursive);
        check_zdd(sets, from, to, BddExecMode::Iterative);
    }
}
