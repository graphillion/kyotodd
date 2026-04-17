#include <gtest/gtest.h>
#include "bdd.h"
#include "bdd_internal.h"
#include "bigint.hpp"
#include <algorithm>
#include <cstring>
#include <random>
#include <sstream>
#include <unordered_set>
#include <climits>
#include <limits>

using namespace kyotodd;

class BDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

// --- bddlit ---

TEST_F(BDDTest, Bddlit_Null) {
    EXPECT_EQ(bddlit(bddnull), 0u);
}

TEST_F(BDDTest, Bddlit_Empty) {
    EXPECT_EQ(bddlit(bddempty), 0u);
}

TEST_F(BDDTest, Bddlit_Single) {
    // {∅} — empty set has 0 literals
    EXPECT_EQ(bddlit(bddsingle), 0u);
}

TEST_F(BDDTest, Bddlit_SingleVar) {
    // ZDD for {{v1}}: onset of v1 from bddsingle
    bddvar v1 = bddnewvar();
    bddp f = bddchange(bddsingle, v1);  // {{v1}}
    EXPECT_EQ(bddcard(f), 1u);
    EXPECT_EQ(bddlit(f), 1u);  // 1 set, 1 element
}

TEST_F(BDDTest, Bddlit_TwoElementSet) {
    // {{v1, v2}}: single set with 2 elements
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddchange(bddsingle, v1);  // {{v1}}
    f = bddchange(f, v2);               // {{v1, v2}}
    EXPECT_EQ(bddcard(f), 1u);
    EXPECT_EQ(bddlit(f), 2u);
}

TEST_F(BDDTest, Bddlit_TwoSets) {
    // {{v1}, {v2}}: two singleton sets
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp s1 = bddchange(bddsingle, v1);  // {{v1}}
    bddp s2 = bddchange(bddsingle, v2);  // {{v2}}
    bddp f = bddunion(s1, s2);           // {{v1}, {v2}}
    EXPECT_EQ(bddcard(f), 2u);
    EXPECT_EQ(bddlit(f), 2u);  // 1 + 1
}

TEST_F(BDDTest, Bddlit_Example) {
    // {{a,b}, {a}, {b,c,d}} -> 2 + 1 + 3 = 6
    bddvar a = bddnewvar();
    bddvar b = bddnewvar();
    bddvar c = bddnewvar();
    bddvar d = bddnewvar();

    // Build {a,b}
    bddp ab = bddchange(bddsingle, a);
    ab = bddchange(ab, b);

    // Build {a}
    bddp sa = bddchange(bddsingle, a);

    // Build {b,c,d}
    bddp bcd = bddchange(bddsingle, b);
    bcd = bddchange(bcd, c);
    bcd = bddchange(bcd, d);

    bddp f = bddunion(ab, sa);
    f = bddunion(f, bcd);

    EXPECT_EQ(bddcard(f), 3u);
    EXPECT_EQ(bddlit(f), 6u);
}

TEST_F(BDDTest, Bddlit_WithEmptySet) {
    // {{}, {v1}}: empty set contributes 0 literals
    bddvar v1 = bddnewvar();
    bddp s1 = bddchange(bddsingle, v1);  // {{v1}}
    bddp f = bddunion(bddsingle, s1);    // {{}, {v1}}
    EXPECT_EQ(bddcard(f), 2u);
    EXPECT_EQ(bddlit(f), 1u);  // 0 + 1
}

TEST_F(BDDTest, Bddlit_Complement) {
    // Complement toggles ∅ membership, lit should be unchanged
    bddvar v1 = bddnewvar();
    bddp s1 = bddchange(bddsingle, v1);  // {{v1}}
    uint64_t lit_orig = bddlit(s1);
    uint64_t lit_comp = bddlit(bddnot(s1));  // {{}, {v1}} or {{v1}} minus ∅
    EXPECT_EQ(lit_orig, lit_comp);
}

// --- bddlen ---

TEST_F(BDDTest, Bddlen_Null) {
    EXPECT_EQ(bddlen(bddnull), 0u);
}

TEST_F(BDDTest, Bddlen_Empty) {
    EXPECT_EQ(bddlen(bddempty), 0u);
}

TEST_F(BDDTest, Bddlen_Single) {
    // {∅} — max size is 0
    EXPECT_EQ(bddlen(bddsingle), 0u);
}

TEST_F(BDDTest, Bddlen_SingleVar) {
    bddvar v1 = bddnewvar();
    bddp f = bddchange(bddsingle, v1);  // {{v1}}
    EXPECT_EQ(bddlen(f), 1u);
}

TEST_F(BDDTest, Bddlen_TwoElementSet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddchange(bddsingle, v1);
    f = bddchange(f, v2);  // {{v1, v2}}
    EXPECT_EQ(bddlen(f), 2u);
}

TEST_F(BDDTest, Bddlen_Example) {
    // {{a,b}, {a}, {b,c,d}} -> max(2, 1, 3) = 3
    bddvar a = bddnewvar();
    bddvar b = bddnewvar();
    bddvar c = bddnewvar();
    bddvar d = bddnewvar();

    bddp ab = bddchange(bddsingle, a);
    ab = bddchange(ab, b);

    bddp sa = bddchange(bddsingle, a);

    bddp bcd = bddchange(bddsingle, b);
    bcd = bddchange(bcd, c);
    bcd = bddchange(bcd, d);

    bddp f = bddunion(ab, sa);
    f = bddunion(f, bcd);

    EXPECT_EQ(bddlen(f), 3u);
}

TEST_F(BDDTest, Bddlen_WithEmptySet) {
    // {{}, {v1, v2}}: max(0, 2) = 2
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddchange(bddsingle, v1);
    f = bddchange(f, v2);
    f = bddunion(f, bddsingle);
    EXPECT_EQ(bddlen(f), 2u);
}

TEST_F(BDDTest, Bddlen_Complement) {
    // Complement toggles ∅ membership; max set size unchanged
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddchange(bddsingle, v1);
    f = bddchange(f, v2);  // {{v1, v2}}
    EXPECT_EQ(bddlen(f), bddlen(bddnot(f)));
}

// --- bddcount tests ---

TEST_F(BDDTest, BddCount_Terminals) {
    EXPECT_DOUBLE_EQ(bddcount(bddempty), 0.0);
    EXPECT_DOUBLE_EQ(bddcount(bddsingle), 1.0);
    EXPECT_DOUBLE_EQ(bddcount(bddnull), 0.0);
}

TEST_F(BDDTest, BddCount_SingleVariable) {
    bddnewvar();
    bddp x1 = bddprime(1);
    EXPECT_DOUBLE_EQ(bddcount(x1), 1.0);
}

TEST_F(BDDTest, BddCount_Union) {
    bddnewvar(); bddnewvar();
    bddp x1 = bddprime(1);
    bddp x2 = bddprime(2);
    bddp f = bddunion(x1, x2);
    EXPECT_DOUBLE_EQ(bddcount(f), 2.0);
}

TEST_F(BDDTest, BddCount_PowerSet) {
    // Power set of 3 variables = 2^3 = 8 sets
    bddnewvar(); bddnewvar(); bddnewvar();
    bddp x1 = bddprime(1);
    bddp x2 = bddprime(2);
    bddp x3 = bddprime(3);
    bddp f = bddunion(x1, x2);
    f = bddunion(f, x3);
    f = bddunion(f, bddchange(x1, 2));
    f = bddunion(f, bddchange(x1, 3));
    f = bddunion(f, bddchange(x2, 3));
    f = bddunion(f, bddchange(bddchange(x1, 2), 3));
    f = bddunion(f, bddsingle);
    EXPECT_DOUBLE_EQ(bddcount(f), 8.0);
}

TEST_F(BDDTest, BddCount_Complement) {
    bddnewvar(); bddnewvar(); bddnewvar();
    bddp x1 = bddprime(1);
    EXPECT_DOUBLE_EQ(bddcount(x1), 1.0);
    EXPECT_DOUBLE_EQ(bddcount(bddnot(x1)), 2.0);

    EXPECT_DOUBLE_EQ(bddcount(bddsingle), 1.0);
    EXPECT_DOUBLE_EQ(bddcount(bddnot(bddsingle)), 0.0);

    EXPECT_DOUBLE_EQ(bddcount(bddempty), 0.0);
    EXPECT_DOUBLE_EQ(bddcount(bddnot(bddempty)), 1.0);
}

TEST_F(BDDTest, BddCount_MatchesCard) {
    bddnewvar(); bddnewvar(); bddnewvar();
    bddp x1 = bddprime(1);
    bddp x2 = bddprime(2);
    bddp x3 = bddprime(3);
    bddp f = bddunion(x1, bddunion(x2, x3));
    f = bddunion(f, bddchange(x1, 2));
    EXPECT_DOUBLE_EQ(bddcount(f), static_cast<double>(bddcard(f)));
    EXPECT_DOUBLE_EQ(bddcount(bddnot(f)), static_cast<double>(bddcard(bddnot(f))));
}

TEST_F(BDDTest, BddCount_ZDDClassMethod) {
    bddnewvar(); bddnewvar();
    bddp x1 = bddprime(1);
    bddp x2 = bddprime(2);
    bddp f = bddunion(x1, bddunion(x2, bddsingle));

    ZDD zf = ZDD_ID(f);
    EXPECT_DOUBLE_EQ(zf.count(), 3.0);
    EXPECT_DOUBLE_EQ(ZDD::Empty.count(), 0.0);
    EXPECT_DOUBLE_EQ(ZDD::Single.count(), 1.0);
}

// --- bddexactcount tests ---

TEST_F(BDDTest, BddExactCount_Terminals) {
    EXPECT_EQ(bddexactcount(bddempty), bigint::BigInt(0));
    EXPECT_EQ(bddexactcount(bddsingle), bigint::BigInt(1));
    EXPECT_EQ(bddexactcount(bddnull), bigint::BigInt(0));
}

TEST_F(BDDTest, BddExactCount_SingleVariable) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddexactcount(x1), bigint::BigInt(1));

    bddp x2 = ZDD::getnode(v2, bddempty, bddsingle);
    EXPECT_EQ(bddexactcount(x2), bigint::BigInt(1));
}

TEST_F(BDDTest, BddExactCount_Union) {
    bddvar v1 = bddnewvar();

    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp f = bddunion(x1, bddsingle);  // {{}, {v1}}
    EXPECT_EQ(bddexactcount(f), bigint::BigInt(2));
}

TEST_F(BDDTest, BddExactCount_PowerSet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp x2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp x12 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp f = bddunion(bddunion(bddunion(bddsingle, x1), x2), x12);
    EXPECT_EQ(bddexactcount(f), bigint::BigInt(4));
}

TEST_F(BDDTest, BddExactCount_Complement) {
    bddvar v1 = bddnewvar();

    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    EXPECT_EQ(bddexactcount(x1), bigint::BigInt(1));
    EXPECT_EQ(bddexactcount(bddnot(x1)), bigint::BigInt(2));

    EXPECT_EQ(bddexactcount(bddsingle), bigint::BigInt(1));
    EXPECT_EQ(bddexactcount(bddnot(bddsingle)), bigint::BigInt(0));

    EXPECT_EQ(bddexactcount(bddempty), bigint::BigInt(0));
    EXPECT_EQ(bddexactcount(bddnot(bddempty)), bigint::BigInt(1));
}

TEST_F(BDDTest, BddExactCount_ComplementWithEmptySet) {
    bddvar v1 = bddnewvar();

    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp f = bddunion(x1, bddsingle);  // {{}, {v1}}
    EXPECT_EQ(bddexactcount(f), bigint::BigInt(2));
    EXPECT_EQ(bddexactcount(bddnot(f)), bigint::BigInt(1));
}

TEST_F(BDDTest, BddExactCount_LargerFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp x2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp x3 = ZDD::getnode(v3, bddempty, bddsingle);
    bddp x12 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp x13 = ZDD::getnode(v3, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp x23 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, bddsingle));
    bddp x123 = ZDD::getnode(v3, bddempty, ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));

    bddp f = bddunion(bddunion(bddunion(x1, x2), bddunion(x3, x12)),
                       bddunion(bddunion(x13, x23), x123));
    EXPECT_EQ(bddexactcount(f), bigint::BigInt(7));

    bddp g = bddunion(f, bddsingle);
    EXPECT_EQ(bddexactcount(g), bigint::BigInt(8));
}

TEST_F(BDDTest, BddExactCount_MatchesBddCard) {
    // For small families, bddexactcount should match bddcard
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddvar v4 = bddnewvar();

    bddp s1 = bddchange(bddsingle, v1);
    bddp s2 = bddchange(bddsingle, v2);
    bddp s3 = bddchange(bddsingle, v3);
    bddp s4 = bddchange(bddsingle, v4);
    bddp s12 = bddchange(s1, v2);
    bddp s34 = bddchange(s3, v4);

    bddp f = bddunion(bddunion(bddunion(s1, s2), bddunion(s3, s4)),
                       bddunion(s12, s34));
    EXPECT_EQ(bddexactcount(f).to_string(),
              std::to_string(bddcard(f)));

    // With complement
    bddp cf = bddnot(f);
    EXPECT_EQ(bddexactcount(cf).to_string(),
              std::to_string(bddcard(cf)));
}

TEST_F(BDDTest, BddExactCount_ZDDClassMethod) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z1 = bddchange(bddsingle, v1);
    bddp z2 = bddchange(bddsingle, v2);
    bddp f = bddunion(bddunion(bddsingle, z1), z2);

    ZDD zf = ZDD_ID(f);
    EXPECT_EQ(zf.exact_count(), bigint::BigInt(3));
    EXPECT_EQ(ZDD::Empty.exact_count(), bigint::BigInt(0));
    EXPECT_EQ(ZDD::Single.exact_count(), bigint::BigInt(1));
}

TEST_F(BDDTest, BddExactCount_PowerSet65_Exceeds2pow64) {
    // Power set of 65 variables has 2^65 elements, which exceeds uint64_t max.
    // The ZDD for a power set of {v1,...,vn} needs only n internal nodes:
    //   node_i = ZDD::getnode(v_i, node_{i-1}, node_{i-1})
    // where node_0 = bddsingle.
    const int N = 65;
    bddvar vars[N];
    for (int i = 0; i < N; i++) {
        vars[i] = bddnewvar();
    }

    bddp f = bddsingle;
    bddgc_protect(&f);
    for (int i = 0; i < N; i++) {
        f = ZDD::getnode(vars[i], f, f);
    }

    bigint::BigInt result = bddexactcount(f);
    // 2^65 = 36893488147419103232
    EXPECT_EQ(result.to_string(), "36893488147419103232");

    bddgc_unprotect(&f);
}

TEST_F(BDDTest, BddExactCount_PowerSet100) {
    // Power set of 100 variables has 2^100 elements.
    // 2^100 = 1267650600228229401496703205376
    const int N = 100;
    bddvar vars[N];
    for (int i = 0; i < N; i++) {
        vars[i] = bddnewvar();
    }

    bddp f = bddsingle;
    bddgc_protect(&f);
    for (int i = 0; i < N; i++) {
        f = ZDD::getnode(vars[i], f, f);
    }

    bigint::BigInt result = bddexactcount(f);
    EXPECT_EQ(result.to_string(), "1267650600228229401496703205376");

    bddgc_unprotect(&f);
}

TEST_F(BDDTest, BddExactCount_PowerSet200) {
    // Power set of 200 variables has 2^200 elements.
    // 2^200 = 1606938044258990275541962092341162602522202993782792835301376
    const int N = 200;
    bddvar vars[N];
    for (int i = 0; i < N; i++) {
        vars[i] = bddnewvar();
    }

    bddp f = bddsingle;
    bddgc_protect(&f);
    for (int i = 0; i < N; i++) {
        f = ZDD::getnode(vars[i], f, f);
    }

    bigint::BigInt result = bddexactcount(f);
    EXPECT_EQ(result.to_string(),
              "1606938044258990275541962092341162602522202993782792835301376");

    bddgc_unprotect(&f);
}

TEST_F(BDDTest, BddExactCount_PowerSet200_Complement) {
    // Complement of power set: 2^200 - 1 (∅ removed) or 2^200 + 1 (∅ added)
    // Power set already contains ∅, so complement removes it: 2^200 - 1
    const int N = 200;
    bddvar vars[N];
    for (int i = 0; i < N; i++) {
        vars[i] = bddnewvar();
    }

    bddp f = bddsingle;
    bddgc_protect(&f);
    for (int i = 0; i < N; i++) {
        f = ZDD::getnode(vars[i], f, f);
    }

    bigint::BigInt result = bddexactcount(bddnot(f));
    // 2^200 - 1
    EXPECT_EQ(result.to_string(),
              "1606938044258990275541962092341162602522202993782792835301375");

    bddgc_unprotect(&f);
}

// --- ZddCountMemo tests ---

TEST_F(BDDTest, ExactCount_ZddCountMemo) {
    bddnewvar(); bddnewvar(); bddnewvar();
    bddp x1 = bddprime(1);
    bddp x2 = bddprime(2);
    bddp x3 = bddprime(3);
    bddp f = bddunion(x1, bddunion(x2, x3));

    ZDD zf = ZDD_ID(f);
    ZddCountMemo memo(zf);
    EXPECT_FALSE(memo.stored());

    bigint::BigInt c1 = zf.exact_count(memo);
    EXPECT_EQ(c1, bigint::BigInt(3));
    EXPECT_TRUE(memo.stored());
    EXPECT_FALSE(memo.map().empty());

    // Second call reuses memo
    bigint::BigInt c2 = zf.exact_count(memo);
    EXPECT_EQ(c2, bigint::BigInt(3));
}

TEST_F(BDDTest, ZddCountMemo_MismatchThrows) {
    bddnewvar(); bddnewvar();
    bddp x1 = bddprime(1);
    bddp x2 = bddprime(2);

    ZDD zf = ZDD_ID(x1);
    ZDD zg = ZDD_ID(bddunion(x1, x2));
    ZddCountMemo memo(zf);
    EXPECT_THROW(zg.exact_count(memo), std::invalid_argument);
}

TEST_F(BDDTest, ZddCountMemo_FromBddp) {
    bddnewvar();
    bddp x1 = bddprime(1);
    ZddCountMemo memo(x1);
    ZDD zf = ZDD_ID(x1);
    EXPECT_EQ(zf.exact_count(memo), bigint::BigInt(1));
    EXPECT_TRUE(memo.stored());
}

TEST_F(BDDTest, ExactCount_NoMemo) {
    bddnewvar();
    bddp x1 = bddprime(1);

    ZDD zf = ZDD_ID(x1);
    EXPECT_EQ(zf.exact_count(), bigint::BigInt(1));
}

TEST_F(BDDTest, ExactCount_ConstZDDWorks) {
    bddnewvar();
    bddp x1 = bddprime(1);

    const ZDD zf = ZDD_ID(x1);
    EXPECT_EQ(zf.exact_count(), bigint::BigInt(1));
}

// --- BddCountMemo tests ---

TEST_F(BDDTest, BddCountMemo_Basic) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    BddCountMemo memo(f, 2);
    EXPECT_FALSE(memo.stored());
    EXPECT_EQ(f.exact_count(2, memo), bigint::BigInt(1));
    EXPECT_TRUE(memo.stored());
    // Second call reuses memo
    EXPECT_EQ(f.exact_count(2, memo), bigint::BigInt(1));
}

TEST_F(BDDTest, BddCountMemo_MismatchThrows) {
    bddvar v1 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = ~a;
    BddCountMemo memo(a, 1);
    EXPECT_THROW(b.exact_count(1, memo), std::invalid_argument);
}

TEST_F(BDDTest, BddCountMemo_NMismatchThrows) {
    bddvar v1 = bddnewvar();
    bddnewvar();
    BDD a = BDDvar(v1);
    BddCountMemo memo(a, 1);
    EXPECT_THROW(a.exact_count(2, memo), std::invalid_argument);
}

// --- uniform_sample tests ---

TEST_F(BDDTest, UniformSample_SingleSet) {
    // Family = {{}} (just the empty set)
    ZDD zf = ZDD::Single;
    ZddCountMemo memo(zf);
    std::mt19937_64 rng(42);
    auto s = zf.uniform_sample(rng, memo);
    EXPECT_TRUE(s.empty());
}

TEST_F(BDDTest, UniformSample_SingletonSet) {
    // Family = {{1}}
    bddnewvar();
    bddp x1 = bddprime(1);
    ZDD zf = ZDD_ID(x1);
    ZddCountMemo memo(zf);
    std::mt19937_64 rng(42);
    auto s = zf.uniform_sample(rng, memo);
    ASSERT_EQ(s.size(), 1u);
    EXPECT_EQ(s[0], 1u);
}

TEST_F(BDDTest, UniformSample_EmptyFamilyThrows) {
    ZDD zf = ZDD::Empty;
    ZddCountMemo memo(zf);
    std::mt19937_64 rng(42);
    EXPECT_THROW(zf.uniform_sample(rng, memo), std::invalid_argument);
}

TEST_F(BDDTest, UniformSample_AllSetsReachable) {
    // Family = {{1}, {2}, {1,2}}  (3 sets)
    bddnewvar(); bddnewvar();
    bddp x1 = bddprime(1);
    bddp x2 = bddprime(2);
    bddp x12 = bddchange(x1, 2);
    bddp f = bddunion(x1, bddunion(x2, x12));
    ZDD zf = ZDD_ID(f);
    ZddCountMemo memo(zf);

    std::mt19937_64 rng(123);
    std::set<std::vector<bddvar>> seen;
    for (int i = 0; i < 300; i++) {
        auto s = zf.uniform_sample(rng, memo);
        std::sort(s.begin(), s.end());
        seen.insert(s);
    }
    EXPECT_EQ(seen.size(), 3u);
    EXPECT_TRUE(seen.count({1}) > 0);
    EXPECT_TRUE(seen.count({2}) > 0);
    std::vector<bddvar> v12 = {1, 2};
    EXPECT_TRUE(seen.count(v12) > 0);
}

TEST_F(BDDTest, UniformSample_Uniformity) {
    // Family = {{1}, {2}} — each should appear ~50%
    bddnewvar(); bddnewvar();
    bddp x1 = bddprime(1);
    bddp x2 = bddprime(2);
    bddp f = bddunion(x1, x2);
    ZDD zf = ZDD_ID(f);
    ZddCountMemo memo(zf);

    std::mt19937_64 rng(456);
    int count1 = 0;
    const int N = 10000;
    for (int i = 0; i < N; i++) {
        auto s = zf.uniform_sample(rng, memo);
        ASSERT_EQ(s.size(), 1u);
        if (s[0] == 1) count1++;
    }
    // Should be roughly 50%, allow generous margin
    double ratio = static_cast<double>(count1) / N;
    EXPECT_GT(ratio, 0.45);
    EXPECT_LT(ratio, 0.55);
}

TEST_F(BDDTest, UniformSample_WithComplement) {
    // bddnot({{1}}) toggles empty set membership: {{1}} → {{}, {1}}
    bddnewvar(); bddnewvar();
    bddp x1 = bddprime(1);
    bddp f = bddnot(x1);  // complement
    ZDD zf = ZDD_ID(f);
    ZddCountMemo memo(zf);

    EXPECT_EQ(zf.exact_count(), bigint::BigInt(2));

    std::mt19937_64 rng(789);
    std::set<std::vector<bddvar>> seen;
    for (int i = 0; i < 200; i++) {
        auto s = zf.uniform_sample(rng, memo);
        seen.insert(s);
    }
    EXPECT_EQ(seen.size(), 2u);
    // Should contain {} and {1}
    EXPECT_TRUE(seen.count({}) > 0);
    EXPECT_TRUE(seen.count({1}) > 0);
}

TEST_F(BDDTest, UniformSample_RequiresMemo) {
    bddnewvar();
    bddp x1 = bddprime(1);
    ZDD zf = ZDD_ID(x1);
    ZddCountMemo memo(zf);
    EXPECT_FALSE(memo.stored());

    std::mt19937_64 rng(42);
    zf.uniform_sample(rng, memo);
    EXPECT_TRUE(memo.stored());
}

// --- BDD::uniform_sample tests ---

TEST_F(BDDTest, BDD_UniformSample_Basic) {
    // f = x1 OR x2, n=2: satisfying assignments = {01, 10, 11}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a | b;  // x1 OR x2
    BddCountMemo memo(f, 2);

    std::mt19937_64 rng(42);
    std::set<std::vector<bddvar>> seen;
    for (int i = 0; i < 300; i++) {
        auto s = f.uniform_sample(rng, 2, memo);
        // Each sample must be a valid satisfying assignment
        // At least one of v1, v2 must be in the result
        std::set<bddvar> vars(s.begin(), s.end());
        EXPECT_TRUE(vars.count(v1) > 0 || vars.count(v2) > 0);
        std::vector<bddvar> sorted_s = s;
        std::sort(sorted_s.begin(), sorted_s.end());
        seen.insert(sorted_s);
    }
    // All 3 satisfying assignments should be reachable
    EXPECT_EQ(seen.size(), 3u);
}

TEST_F(BDDTest, BDD_UniformSample_SingleSolution) {
    // f = x1 AND x2, n=2: only satisfying assignment is {x1=1, x2=1}
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    BddCountMemo memo(f, 2);

    std::mt19937_64 rng(42);
    for (int i = 0; i < 50; i++) {
        auto s = f.uniform_sample(rng, 2, memo);
        ASSERT_EQ(s.size(), 2u);
        std::set<bddvar> vars(s.begin(), s.end());
        EXPECT_TRUE(vars.count(v1) > 0);
        EXPECT_TRUE(vars.count(v2) > 0);
    }
}

TEST_F(BDDTest, BDD_UniformSample_AllTrue) {
    // f = bddtrue, n=3: all 8 assignments satisfy
    bddnewvar(); bddnewvar(); bddnewvar();
    BDD f = BDD::True;
    BddCountMemo memo(f, 3);

    std::mt19937_64 rng(42);
    // Count how many times each variable appears in 1000 samples
    int counts[4] = {};  // counts[1], counts[2], counts[3]
    const int N = 3000;
    for (int i = 0; i < N; i++) {
        auto s = f.uniform_sample(rng, 3, memo);
        for (bddvar v : s) {
            ASSERT_GE(v, 1u);
            ASSERT_LE(v, 3u);
            counts[v]++;
        }
    }
    // Each variable should appear ~50% of the time
    for (int v = 1; v <= 3; v++) {
        double ratio = static_cast<double>(counts[v]) / N;
        EXPECT_GT(ratio, 0.45);
        EXPECT_LT(ratio, 0.55);
    }
}

TEST_F(BDDTest, BDD_UniformSample_SkippedVariables) {
    // f depends only on x1, n=3: x2 and x3 are skipped (don't care)
    // f = x1: satisfying = {x1=1, x2=?, x3=?} → 4 assignments
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    BDD f = BDDvar(v1);
    BddCountMemo memo(f, 3);

    std::mt19937_64 rng(42);
    bool v2_seen = false, v3_seen = false;
    for (int i = 0; i < 200; i++) {
        auto s = f.uniform_sample(rng, 3, memo);
        // v1 must always be present
        std::set<bddvar> vars(s.begin(), s.end());
        EXPECT_TRUE(vars.count(v1) > 0);
        if (vars.count(v2) > 0) v2_seen = true;
        if (vars.count(v3) > 0) v3_seen = true;
    }
    // Skipped variables should appear in some samples
    EXPECT_TRUE(v2_seen);
    EXPECT_TRUE(v3_seen);
}

TEST_F(BDDTest, BDD_UniformSample_ThrowsOnFalse) {
    BDD f = BDD::False;
    BddCountMemo memo(f, 2);
    std::mt19937_64 rng(42);
    EXPECT_THROW(f.uniform_sample(rng, 2, memo), std::invalid_argument);
}

TEST_F(BDDTest, BDD_UniformSample_ThrowsOnNull) {
    BDD f = BDD::Null;
    BddCountMemo memo(f, 2);
    std::mt19937_64 rng(42);
    EXPECT_THROW(f.uniform_sample(rng, 2, memo), std::invalid_argument);
}

TEST_F(BDDTest, BDD_UniformSample_MemoMismatch) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD f1 = BDDvar(v1);
    BDD f2 = BDDvar(v2);
    BddCountMemo memo(f1, 2);
    std::mt19937_64 rng(42);
    EXPECT_THROW(f2.uniform_sample(rng, 2, memo), std::invalid_argument);
}

TEST_F(BDDTest, BDD_UniformSample_NMismatch) {
    bddvar v1 = bddnewvar();
    bddnewvar();
    BDD f = BDDvar(v1);
    BddCountMemo memo(f, 2);
    std::mt19937_64 rng(42);
    // memo was created with n=2, but calling with n=3
    EXPECT_THROW(f.uniform_sample(rng, 3, memo), std::invalid_argument);
}

TEST_F(BDDTest, BDD_UniformSample_PopulatesMemo) {
    bddvar v1 = bddnewvar();
    BDD f = BDDvar(v1);
    BddCountMemo memo(f, 1);
    EXPECT_FALSE(memo.stored());

    std::mt19937_64 rng(42);
    f.uniform_sample(rng, 1, memo);
    EXPECT_TRUE(memo.stored());
}

TEST_F(BDDTest, BDD_UniformSample_LevelDescending) {
    // Verify result is in decreasing level order
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    BDD f = BDD::True;
    BddCountMemo memo(f, 3);

    std::mt19937_64 rng(42);
    for (int i = 0; i < 100; i++) {
        auto s = f.uniform_sample(rng, 3, memo);
        // Check that levels are strictly decreasing
        for (std::size_t j = 1; j < s.size(); j++) {
            EXPECT_GT(bddlevofvar(s[j-1]), bddlevofvar(s[j]));
        }
    }
}

// --- bddcardmp16 tests ---

TEST_F(BDDTest, Bddcardmp16_Terminals) {
    // bddempty → 0
    char *r = bddcardmp16(bddempty, NULL);
    EXPECT_STREQ(r, "0");
    std::free(r);

    // bddsingle → 1
    r = bddcardmp16(bddsingle, NULL);
    EXPECT_STREQ(r, "1");
    std::free(r);

    // bddnull → 0
    r = bddcardmp16(bddnull, NULL);
    EXPECT_STREQ(r, "0");
    std::free(r);
}

TEST_F(BDDTest, Bddcardmp16_SmallFamily) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();

    // Power set of 2 vars → 4 = 0x4
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp x2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp x12 = ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle));
    bddp f = bddunion(bddunion(bddunion(bddsingle, x1), x2), x12);

    char *r = bddcardmp16(f, NULL);
    EXPECT_STREQ(r, "4");
    std::free(r);
}

TEST_F(BDDTest, Bddcardmp16_UserBuffer) {
    bddvar v1 = bddnewvar();
    bddp x1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp f = bddunion(x1, bddsingle);  // 2 = 0x2

    char buf[64];
    char *r = bddcardmp16(f, buf);
    EXPECT_EQ(r, buf);
    EXPECT_STREQ(buf, "2");
}

TEST_F(BDDTest, Bddcardmp16_PowerSet64_Hex) {
    // 2^64 = 0x10000000000000000 (17 hex digits)
    const int N = 64;
    bddvar vars[N];
    for (int i = 0; i < N; i++) {
        vars[i] = bddnewvar();
    }
    bddp f = bddsingle;
    bddgc_protect(&f);
    for (int i = 0; i < N; i++) {
        f = ZDD::getnode(vars[i], f, f);
    }

    char *r = bddcardmp16(f, NULL);
    EXPECT_STREQ(r, "10000000000000000");
    std::free(r);
    bddgc_unprotect(&f);
}

TEST_F(BDDTest, Bddcardmp16_PowerSet200_Hex) {
    // 2^200 in hex = 1 followed by 50 zeros
    const int N = 200;
    bddvar vars[N];
    for (int i = 0; i < N; i++) {
        vars[i] = bddnewvar();
    }
    bddp f = bddsingle;
    bddgc_protect(&f);
    for (int i = 0; i < N; i++) {
        f = ZDD::getnode(vars[i], f, f);
    }

    char *r = bddcardmp16(f, NULL);
    // 2^200 = 0x100000000000000000000000000000000000000000000000000 (1 + 50 zeros)
    std::string expected = "1";
    for (int i = 0; i < 50; i++) expected += "0";
    EXPECT_STREQ(r, expected.c_str());
    std::free(r);
    bddgc_unprotect(&f);
}

TEST_F(BDDTest, Bddcardmp16_UppercaseLetters) {
    // Build a family with cardinality 255 = 0xFF to check uppercase
    // 2^8 - 1 = 255. Power set of 8 vars minus ∅.
    const int N = 8;
    bddvar vars[N];
    for (int i = 0; i < N; i++) {
        vars[i] = bddnewvar();
    }
    bddp f = bddsingle;
    bddgc_protect(&f);
    for (int i = 0; i < N; i++) {
        f = ZDD::getnode(vars[i], f, f);
    }
    // f = power set (256 elements), complement removes ∅ → 255
    char *r = bddcardmp16(bddnot(f), NULL);
    EXPECT_STREQ(r, "FF");
    std::free(r);
    bddgc_unprotect(&f);
}

// --- bddpush ---

TEST_F(BDDTest, BddpushTerminals) {
    bddvar v1 = bddnewvar();
    // push on empty family → empty (zero-suppression: hi=bddempty)
    EXPECT_EQ(bddpush(bddempty, v1), bddempty);
    // push on null → null
    EXPECT_EQ(bddpush(bddnull, v1), bddnull);
}

TEST_F(BDDTest, BddpushSingle) {
    bddvar v1 = bddnewvar();
    // push v1 onto {∅} → {{v1}}
    bddp result = bddpush(bddsingle, v1);
    // This should be ZDD::getnode(v1, bddempty, bddsingle) = node(v1, 0-edge=∅, 1-edge={∅})
    // which represents {{v1}}
    EXPECT_EQ(bddcard(result), 1u);
    // The top variable should be v1
    EXPECT_EQ(bddtop(result), v1);
    // onset0 of v1 should give {∅}
    EXPECT_EQ(bddonset0(result, v1), bddsingle);
    // offset of v1 should give empty
    EXPECT_EQ(bddoffset(result, v1), bddempty);
}

TEST_F(BDDTest, BddpushOnZDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // Build {{v2}} = ZDD::getnode(v2, bddempty, bddsingle)
    bddp f = ZDD::getnode(v2, bddempty, bddsingle);
    // push v1 onto {{v2}} → node(v1, bddempty, {{v2}})
    // This represents sets where v1 is always present, and v2 is present
    // So the family is {{v1, v2}}
    bddp result = bddpush(f, v1);
    EXPECT_EQ(bddcard(result), 1u);
    EXPECT_EQ(bddtop(result), v1);
}

TEST_F(BDDTest, BddpushNoLevelOrderCheck) {
    bddvar v1 = bddnewvar();  // level 1
    bddvar v2 = bddnewvar();  // level 2
    // Build a ZDD with top variable v1 (level 1)
    bddp f = ZDD::getnode(v1, bddempty, bddsingle);  // {{v1}}
    // Push v2 (level 2) on top — this would violate normal ZDD ordering
    // but bddpush should allow it for Sequence BDD support
    bddp result = bddpush(f, v2);
    EXPECT_NE(result, bddnull);
    EXPECT_EQ(bddtop(result), v2);
}

TEST_F(BDDTest, BddpushSameVar) {
    bddvar v1 = bddnewvar();
    // Build {{v1}}
    bddp f = ZDD::getnode(v1, bddempty, bddsingle);
    // Push v1 again — for Sequence BDD, same variable can appear multiple times
    bddp result = bddpush(f, v1);
    EXPECT_NE(result, bddnull);
    EXPECT_EQ(bddtop(result), v1);
}

TEST_F(BDDTest, BddpushVarOutOfRange) {
    bddnewvar();
    EXPECT_THROW(bddpush(bddsingle, 0), std::invalid_argument);
    EXPECT_THROW(bddpush(bddsingle, bdd_varcount + 1), std::invalid_argument);
}

// --- ZDD_Meet ---

TEST_F(BDDTest, ZDD_MeetTerminalCases) {
    ZDD e(0);  // empty
    ZDD s(1);  // single

    EXPECT_EQ(ZDD_Meet(e, e), e);
    EXPECT_EQ(ZDD_Meet(e, s), e);
    EXPECT_EQ(ZDD_Meet(s, e), e);
    EXPECT_EQ(ZDD_Meet(s, s), s);
}

TEST_F(BDDTest, ZDD_MeetWithSingle) {
    bddvar v1 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));  // {{v1}}
    ZDD s(1);

    EXPECT_EQ(ZDD_Meet(s, z_v1), s);
    EXPECT_EQ(ZDD_Meet(z_v1, s), s);
}

TEST_F(BDDTest, ZDD_MeetSameFamily) {
    bddvar v1 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    EXPECT_EQ(ZDD_Meet(z_v1, z_v1), z_v1);
}

TEST_F(BDDTest, ZDD_MeetDisjointSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD s(1);

    // {{v1}} ⊓ {{v2}} = {∅}
    EXPECT_EQ(ZDD_Meet(z_v1, z_v2), s);
}

TEST_F(BDDTest, ZDD_MeetOverlapping) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));

    // {{v1,v2}} ⊓ {{v1}} = {{v1}}
    EXPECT_EQ(ZDD_Meet(z_v1v2, z_v1), z_v1);
}

TEST_F(BDDTest, ZDD_MeetCommutativity) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));

    ZDD F = z_v1 + z_v1v2;

    EXPECT_EQ(ZDD_Meet(F, z_v2), ZDD_Meet(z_v2, F));
}

// --- ZDD_Import ---

TEST_F(BDDTest, ZDD_ImportIstreamSingle) {
    bddvar v1 = bddnewvar();
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    bddp p[] = { z };
    std::ostringstream oss;
    bddexport(oss, p, 1);
    std::istringstream iss(oss.str());
    ZDD result = ZDD_Import(iss);
    EXPECT_EQ(result.GetID(), z);
}

TEST_F(BDDTest, ZDD_ImportIstreamVector) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    std::vector<bddp> orig = { z_v1, z_v2 };
    std::ostringstream oss;
    bddexport(oss, orig);
    std::istringstream iss(oss.str());
    std::vector<ZDD> v;
    int ret = ZDD_Import(iss, v);
    EXPECT_EQ(ret, 2);
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0].GetID(), z_v1);
    EXPECT_EQ(v[1].GetID(), z_v2);
}

TEST_F(BDDTest, ZDD_ImportFilePtrSingle) {
    bddvar v1 = bddnewvar();
    bddp z = ZDD::getnode(v1, bddempty, bddsingle);
    bddp p[] = { z };
    FILE* tmp = std::tmpfile();
    ASSERT_NE(tmp, nullptr);
    bddexport(tmp, p, 1);
    std::rewind(tmp);
    ZDD result = ZDD_Import(tmp);
    std::fclose(tmp);
    EXPECT_EQ(result.GetID(), z);
}

TEST_F(BDDTest, ZDD_ImportFilePtrVector) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    std::vector<bddp> orig = { z_v1, z_v2 };
    FILE* tmp = std::tmpfile();
    ASSERT_NE(tmp, nullptr);
    bddexport(tmp, orig);
    std::rewind(tmp);
    std::vector<ZDD> v;
    int ret = ZDD_Import(tmp, v);
    std::fclose(tmp);
    EXPECT_EQ(ret, 2);
    ASSERT_EQ(v.size(), 2u);
    EXPECT_EQ(v[0].GetID(), z_v1);
    EXPECT_EQ(v[1].GetID(), z_v2);
}

// --- BDD_CacheZDD ---

TEST_F(BDDTest, BDD_CacheZDD_Miss) {
    bddvar v1 = bddnewvar();
    bddp p1 = ZDD::getnode(v1, bddempty, bddsingle);
    // No prior write: should return ZDD::Null
    EXPECT_EQ(BDD_CacheZDD(BDD_OP_UNION, p1, p1), ZDD::Null);
}

TEST_F(BDDTest, BDD_CacheZDD_Hit) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp result = bddunion(z_v1, z_v2);

    bddwcache(BDD_OP_UNION, z_v1, z_v2, result);
    ZDD cached = BDD_CacheZDD(BDD_OP_UNION, z_v1, z_v2);
    EXPECT_EQ(cached.GetID(), result);
}

TEST_F(BDDTest, BDD_CacheZDD_DifferentOpMiss) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp z_v1 = ZDD::getnode(v1, bddempty, bddsingle);
    bddp z_v2 = ZDD::getnode(v2, bddempty, bddsingle);
    bddp result = bddunion(z_v1, z_v2);

    bddwcache(BDD_OP_UNION, z_v1, z_v2, result);
    // Different op should miss
    EXPECT_EQ(BDD_CacheZDD(BDD_OP_INTERSEC, z_v1, z_v2), ZDD::Null);
}

// --- ZDD_Random ---

TEST_F(BDDTest, ZDD_Random_Lev0_Density0) {
    // density=0: always empty
    ZDD z = ZDD_Random(0, 0);
    EXPECT_EQ(z, ZDD::Empty);
}

TEST_F(BDDTest, ZDD_Random_Lev0_Density100) {
    // density=100: always single
    ZDD z = ZDD_Random(0, 100);
    EXPECT_EQ(z, ZDD::Single);
}

TEST_F(BDDTest, ZDD_Random_ReturnsValidZDD) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    std::srand(42);
    ZDD z = ZDD_Random(3, 50);
    // Result must be a valid ZDD (not null)
    EXPECT_NE(z, ZDD::Null);
}

TEST_F(BDDTest, ZDD_Random_Density0_AlwaysEmpty) {
    bddnewvar();
    bddnewvar();
    // density=0: all terminals are 0, result is empty family
    ZDD z = ZDD_Random(2, 0);
    EXPECT_EQ(z, ZDD::Empty);
}

TEST_F(BDDTest, ZDD_Random_DifferentSeedsGiveDifferentResults) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    std::srand(1);
    ZDD z1 = ZDD_Random(3, 50);
    std::srand(999);
    ZDD z2 = ZDD_Random(3, 50);
    // Very unlikely to be equal with different seeds
    // (not guaranteed, but extremely improbable for 3 vars)
    EXPECT_NE(z1.GetID(), z2.GetID());
}

// --- ZDD operator<< / operator<<= / operator>> / operator>>= ---

TEST_F(BDDTest, ZDD_OperatorLshift) {
    bddvar v1 = bddnewvar();
    bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));  // {{v1}}
    ZDD shifted = z << 1;
    EXPECT_EQ(shifted.GetID(), bddlshiftz(z.GetID(), 1));
}

TEST_F(BDDTest, ZDD_OperatorLshiftAssign) {
    bddvar v1 = bddnewvar();
    bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    bddp expected = bddlshiftz(z.GetID(), 1);
    z <<= 1;
    EXPECT_EQ(z.GetID(), expected);
}

TEST_F(BDDTest, ZDD_OperatorRshift) {
    bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));  // {{v2}}
    ZDD shifted = z >> 1;
    EXPECT_EQ(shifted.GetID(), bddrshiftz(z.GetID(), 1));
}

TEST_F(BDDTest, ZDD_OperatorRshiftAssign) {
    bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    bddp expected = bddrshiftz(z.GetID(), 1);
    z >>= 1;
    EXPECT_EQ(z.GetID(), expected);
}

TEST_F(BDDTest, ZDD_LshiftRshiftRoundtrip) {
    bddvar v1 = bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD roundtrip = (z << 2) >> 2;
    EXPECT_EQ(roundtrip, z);
}

TEST_F(BDDTest, ZDD_LshiftMultipleSetFamily) {
    // Regression: bddlshift dropped sets from multi-set families
    for (int i = 0; i < 10; i++) bddnewvar();

    // Input: {{1}, {1,2,4,5}, {1,2,5}}
    ZDD a(1); a = a.Change(1);
    ZDD b(1); b = b.Change(1).Change(2).Change(4).Change(5);
    ZDD c(1); c = c.Change(1).Change(2).Change(5);
    ZDD f = a + b + c;
    ASSERT_EQ(f.Card(), 3);

    ZDD result = f << 1;
    // Expected: {{2}, {2,3,5,6}, {2,3,6}}
    EXPECT_EQ(result.Card(), 3);

    // Verify roundtrip
    ZDD roundtrip = (f << 1) >> 1;
    EXPECT_EQ(roundtrip, f);
}

TEST_F(BDDTest, ZDD_RshiftMultipleSetFamily) {
    // Regression: bddrshift had the same getnode/getznode bug
    for (int i = 0; i < 10; i++) bddnewvar();

    // Input: {{2}, {2,3,5,6}, {2,3,6}}
    ZDD a(1); a = a.Change(2);
    ZDD b(1); b = b.Change(2).Change(3).Change(5).Change(6);
    ZDD c(1); c = c.Change(2).Change(3).Change(6);
    ZDD f = a + b + c;
    ASSERT_EQ(f.Card(), 3);

    ZDD result = f >> 1;
    // Expected: {{1}, {1,2,4,5}, {1,2,5}}
    EXPECT_EQ(result.Card(), 3);
}

// --- ZDD::Intersec ---

TEST_F(BDDTest, ZDD_Intersec_MatchesOperatorAnd) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;  // {{v1}, {v2}}

    EXPECT_EQ(F.Intersec(z_v1), F & z_v1);
}

TEST_F(BDDTest, ZDD_Intersec_WithSelf) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    EXPECT_EQ(z.Intersec(z), z);
}

TEST_F(BDDTest, ZDD_Intersec_WithEmpty) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD e(0);

    EXPECT_EQ(z.Intersec(e), e);
}

// --- BDD::Top ---

TEST_F(BDDTest, BDD_Top) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    BDD f = BDDvar(v1) & BDDvar(v2);
    EXPECT_EQ(f.Top(), bddtop(f.GetID()));
}

TEST_F(BDDTest, BDD_TopTerminal) {
    BDD f(0);
    EXPECT_EQ(f.Top(), 0u);
}

// --- ZDD::Top ---

TEST_F(BDDTest, ZDD_Top) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(z.Top(), bddtop(z.GetID()));
}

TEST_F(BDDTest, ZDD_TopTerminal) {
    ZDD e(0);
    EXPECT_EQ(e.Top(), bddtop(bddempty));
}

// --- ZDD::Support ---

TEST_F(BDDTest, ZDD_Support) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD sup = z.Support();
    EXPECT_EQ(sup.GetID(), bddsupport(z.GetID()));
}

TEST_F(BDDTest, ZDD_SupportTerminal) {
    ZDD e(0);
    ZDD sup = e.Support();
    EXPECT_EQ(sup.GetID(), bddsupport(bddempty));
}

// --- ZDD::XPrint ---

TEST_F(BDDTest, ZDD_XPrint) {
    ZDD z = ZDD_ID(ZDD::getnode(bddnewvar(), bddempty, bddsingle));
    EXPECT_THROW(z.XPrint(), std::logic_error);
}

// --- ZDD::Size ---

TEST_F(BDDTest, ZDD_Size) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.Size(), bddsize(F.GetID()));
}

// --- ZDD::Lit ---

TEST_F(BDDTest, ZDD_Lit) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;  // {{v1}, {v2}} -> 2 literals total
    EXPECT_EQ(F.Lit(), bddlit(F.GetID()));
}

// --- ZDD::Len ---

TEST_F(BDDTest, ZDD_Len) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = ZDD(1) + z_v1v2;  // {∅, {v1,v2}} -> lengths 0 + 2 = 2
    EXPECT_EQ(F.Len(), bddlen(F.GetID()));
}

// --- ZDD::CardMP16 ---

TEST_F(BDDTest, ZDD_CardMP16) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;  // {{v1}, {v2}} -> card = 2
    char* result = F.CardMP16(nullptr);
    char* expected = bddcardmp16(F.GetID(), nullptr);
    EXPECT_STREQ(result, expected);
    std::free(result);
    std::free(expected);
}

