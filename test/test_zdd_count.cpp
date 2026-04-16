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

// --- ZDD::Export ---

TEST_F(BDDTest, ZDD_ExportOstream) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    std::ostringstream oss1;
    z.Export(oss1);

    std::ostringstream oss2;
    bddp p = z.GetID();
    bddexport(oss2, &p, 1);

    EXPECT_EQ(oss1.str(), oss2.str());
}

TEST_F(BDDTest, ZDD_ExportFilePtr) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    FILE* tmp = std::tmpfile();
    ASSERT_NE(tmp, nullptr);
    z.Export(tmp);
    long len1 = std::ftell(tmp);

    std::rewind(tmp);
    // Verify roundtrip via ZDD_Import
    ZDD imported = ZDD_Import(tmp);
    std::fclose(tmp);
    EXPECT_GT(len1, 0);
    EXPECT_EQ(imported, z);
}

// --- bddispoly / ZDD::IsPoly ---

TEST(ZDD_IsPolyTest, EmptyAndSingle) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // Empty family: 0 sets -> not poly
    EXPECT_EQ(bddispoly(bddempty), 0);
    // Single family {∅}: 1 set -> not poly
    EXPECT_EQ(bddispoly(bddsingle), 0);
    // Null -> error
    EXPECT_EQ(bddispoly(bddnull), -1);
}

TEST(ZDD_IsPolyTest, SingleSet) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {v1} = single set containing only variable 1 -> not poly
    bddp s1 = bddchange(bddsingle, 1);
    EXPECT_EQ(bddispoly(s1), 0);

    // {v1, v2} = single set -> not poly
    bddp s12 = bddchange(bddchange(bddsingle, 1), 2);
    EXPECT_EQ(bddispoly(s12), 0);
}

TEST(ZDD_IsPolyTest, MultipleSets) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}} = 2 sets -> poly
    bddp s1 = bddchange(bddsingle, 1);
    bddp s2 = bddchange(bddsingle, 2);
    bddp f = bddunion(s1, s2);
    EXPECT_EQ(bddispoly(f), 1);

    // {{∅}, {v1}} = 2 sets -> poly
    bddp g = bddunion(bddsingle, s1);
    EXPECT_EQ(bddispoly(g), 1);
}

TEST(ZDD_IsPolyTest, ClassWrapper) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    ZDD empty = ZDD::Empty;
    EXPECT_EQ(empty.IsPoly(), 0);

    ZDD single = ZDD::Single;
    EXPECT_EQ(single.IsPoly(), 0);

    ZDD s1 = ZDD::Single.Change(1);
    ZDD s2 = ZDD::Single.Change(2);
    ZDD f = s1 + s2;
    EXPECT_EQ(f.IsPoly(), 1);
}

// --- bddswapz / ZDD::Swap ---

TEST(ZDD_SwapTest, SameVariable) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    ZDD s1 = ZDD::Single.Change(1);
    EXPECT_EQ(s1.Swap(1, 1), s1);
}

TEST(ZDD_SwapTest, SwapSingletons) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {v1} swapped(1,2) = {v2}
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s2 = ZDD::Single.Change(2);
    EXPECT_EQ(s1.Swap(1, 2), s2);
    EXPECT_EQ(s2.Swap(1, 2), s1);
}

TEST(ZDD_SwapTest, BothPresent) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2}} swapped(1,2) = {{v1, v2}} (unchanged)
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    EXPECT_EQ(s12.Swap(1, 2), s12);
}

TEST(ZDD_SwapTest, MixedFamily) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2, v3}} swapped(1,2) = {{v2}, {v1, v3}}
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s23 = ZDD::Single.Change(2).Change(3);
    ZDD f = s1 + s23;

    ZDD s2 = ZDD::Single.Change(2);
    ZDD s13 = ZDD::Single.Change(1).Change(3);
    ZDD expected = s2 + s13;

    EXPECT_EQ(f.Swap(1, 2), expected);
}

TEST(ZDD_SwapTest, TerminalCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // Empty family: swap is still empty
    EXPECT_EQ(bddswapz(bddempty, 1, 2), bddempty);
    // Single family {∅}: no variables, swap is identity
    EXPECT_EQ(bddswapz(bddsingle, 1, 2), bddsingle);
    // Null -> null
    EXPECT_EQ(bddswapz(bddnull, 1, 2), bddnull);
}

TEST(ZDD_SwapTest, Involution) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // Swap is an involution: swap(swap(f, v1, v2), v1, v2) == f
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s23 = ZDD::Single.Change(2).Change(3);
    ZDD f = s1 + s23;

    EXPECT_EQ(f.Swap(1, 2).Swap(1, 2), f);
}

// --- bddimplychk / ZDD::ImplyChk ---

TEST(ZDD_ImplyChkTest, TrivialCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // v1 == v2: always true
    ZDD f = ZDD::Single.Change(1);
    EXPECT_EQ(f.ImplyChk(1, 1), 1);
    // Empty family: trivially true
    EXPECT_EQ(ZDD::Empty.ImplyChk(1, 2), 1);
    // Single family {∅}: trivially true (no set contains v1)
    EXPECT_EQ(ZDD::Single.ImplyChk(1, 2), 1);
    // Null -> error
    EXPECT_EQ(bddimplychk(bddnull, 1, 2), -1);
}

TEST(ZDD_ImplyChkTest, ImplicationHolds) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2}, {v3}}: every set with v1 also has v2 -> v1 implies v2
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    ZDD s3 = ZDD::Single.Change(3);
    ZDD f = s12 + s3;
    EXPECT_EQ(f.ImplyChk(1, 2), 1);
}

TEST(ZDD_ImplyChkTest, ImplicationFails) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}}: v1 does not imply v2 ({v1} has v1 but not v2)
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s2 = ZDD::Single.Change(2);
    ZDD f = s1 + s2;
    EXPECT_EQ(f.ImplyChk(1, 2), 0);
}

// --- bddcoimplychk / ZDD::CoImplyChk ---

TEST(ZDD_CoImplyChkTest, TrivialCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(ZDD::Empty.CoImplyChk(1, 2), 1);
    EXPECT_EQ(ZDD::Single.CoImplyChk(1, 2), 1);
    EXPECT_EQ(bddcoimplychk(bddnull, 1, 2), -1);
}

TEST(ZDD_CoImplyChkTest, ImplicationSubsumesCoImplication) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // If v1 implies v2, co-implication also holds
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    ZDD s3 = ZDD::Single.Change(3);
    ZDD f = s12 + s3;
    EXPECT_EQ(f.ImplyChk(1, 2), 1);
    EXPECT_EQ(f.CoImplyChk(1, 2), 1);
}

TEST(ZDD_CoImplyChkTest, CoImplicationHoldsButNotImplication) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v3}, {v2}}: v1 does not imply v2 (set {v1,v3} has v1 but not v2)
    // f10 = {v3} (sets with v1 but not v2, v1 removed)
    // f01 = {∅}  (sets with v2 but not v1, v2 removed) -- wait, {v2} has v2 but not v1, so f01 = OnSet0(OffSet(f,v1),v2) = {∅}
    // f10 - f01 = {v3} - {∅} = {v3} != empty -> co-implication fails
    ZDD s13 = ZDD::Single.Change(1).Change(3);
    ZDD s2 = ZDD::Single.Change(2);
    ZDD f = s13 + s2;
    EXPECT_EQ(f.ImplyChk(1, 2), 0);
    EXPECT_EQ(f.CoImplyChk(1, 2), 0);

    // {{v1}, {v2}}: f10 = {∅} (v1 only, v1 removed), f01 = {∅} (v2 only, v2 removed)
    // f10 - f01 = {∅} - {∅} = empty -> co-implication holds
    ZDD s1 = ZDD::Single.Change(1);
    ZDD g = s1 + s2;
    EXPECT_EQ(g.ImplyChk(1, 2), 0);
    EXPECT_EQ(g.CoImplyChk(1, 2), 1);
}

// --- bddpermitsym / ZDD::PermitSym ---

TEST(ZDD_PermitSymTest, TerminalCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(bddpermitsym(bddnull, 3), bddnull);
    EXPECT_EQ(bddpermitsym(bddempty, 3), bddempty);
    EXPECT_EQ(bddpermitsym(bddsingle, 3), bddsingle);
}

TEST(ZDD_PermitSymTest, NZero) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // n=0: only keep ∅ if present
    ZDD s1 = ZDD::Single.Change(1);
    ZDD f = ZDD::Single + s1;  // {∅, {v1}}
    EXPECT_EQ(f.PermitSym(0), ZDD::Single);  // only ∅

    // n=0 on family without ∅
    EXPECT_EQ(s1.PermitSym(0), ZDD::Empty);
}

TEST(ZDD_PermitSymTest, FilterBySize) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v1,v2}, {v1,v2,v3}}
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    ZDD s123 = ZDD::Single.Change(1).Change(2).Change(3);
    ZDD f = s1 + s12 + s123;

    // n=1: only {v1}
    EXPECT_EQ(f.PermitSym(1), s1);
    // n=2: {v1} and {v1,v2}
    EXPECT_EQ(f.PermitSym(2), s1 + s12);
    // n=3: all
    EXPECT_EQ(f.PermitSym(3), f);
    // n=10: all (n >= max size)
    EXPECT_EQ(f.PermitSym(10), f);
}

// --- bddalways / ZDD::Always ---

TEST(ZDD_AlwaysTest, TerminalCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(bddnull, bddalways(bddnull));
    // Empty family: no sets -> no common elements
    EXPECT_EQ(bddempty, bddalways(bddempty));
    // {∅}: empty set has no elements -> Always = ∅
    EXPECT_EQ(bddempty, bddalways(bddsingle));
}

TEST(ZDD_AlwaysTest, AllSetsContainVariable) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v1, v2}}: v1 is in all sets -> Always = {v1}
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    ZDD f = s1 + s12;
    ZDD expected = ZDD::Single.Change(1);  // {v1}
    EXPECT_EQ(f.Always(), expected);
}

TEST(ZDD_AlwaysTest, NoCommonVariable) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}}: no common variable -> Always = ∅
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s2 = ZDD::Single.Change(2);
    ZDD f = s1 + s2;
    EXPECT_EQ(f.Always(), ZDD::Empty);
}

TEST(ZDD_AlwaysTest, MultipleCommonVariables) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2, v3}, {v1, v2, v4}}: v1 and v2 are common
    // Always returns family of singletons: {{v1}, {v2}}
    ZDD s123 = ZDD::Single.Change(1).Change(2).Change(3);
    ZDD s124 = ZDD::Single.Change(1).Change(2).Change(4);
    ZDD f = s123 + s124;
    ZDD expected = ZDD::Single.Change(1) + ZDD::Single.Change(2);
    EXPECT_EQ(f.Always(), expected);
}

TEST(ZDD_AlwaysTest, WithEmptySet) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {∅, {v1}}: ∅ has no elements -> Always = ∅
    ZDD f = ZDD::Single + ZDD::Single.Change(1);
    EXPECT_EQ(f.Always(), ZDD::Empty);
}

// --- bddsymchk / ZDD::SymChk ---

TEST(ZDD_SymChkTest, TrivialCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(bddsymchk(bddnull, 1, 2), -1);
    EXPECT_EQ(ZDD::Empty.SymChk(1, 2), 1);
    EXPECT_EQ(ZDD::Single.SymChk(1, 2), 1);
    // Same variable -> trivially symmetric
    ZDD s1 = ZDD::Single.Change(1);
    EXPECT_EQ(s1.SymChk(1, 1), 1);
}

TEST(ZDD_SymChkTest, SymmetricFamily) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}}: swapping v1 and v2 gives the same family -> symmetric
    ZDD f = ZDD::Single.Change(1) + ZDD::Single.Change(2);
    EXPECT_EQ(f.SymChk(1, 2), 1);

    // {{v1, v2}}: swapping gives {{v1, v2}} -> symmetric
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    EXPECT_EQ(s12.SymChk(1, 2), 1);
}

TEST(ZDD_SymChkTest, NotSymmetric) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}}: swapping v1,v2 gives {{v2}} != {{v1}} -> not symmetric
    ZDD s1 = ZDD::Single.Change(1);
    EXPECT_EQ(s1.SymChk(1, 2), 0);
}

TEST(ZDD_SymChkTest, ConsistentWithSwap) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // Build a variety of families and verify: SymChk(v1,v2)==1 iff Swap(v1,v2)==f
    ZDD s1 = ZDD::Single.Change(1);
    ZDD s2 = ZDD::Single.Change(2);
    ZDD s3 = ZDD::Single.Change(3);
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    ZDD s13 = ZDD::Single.Change(1).Change(3);
    ZDD s23 = ZDD::Single.Change(2).Change(3);

    ZDD families[] = {
        s1 + s2,          // {{v1}, {v2}}
        s1 + s3,          // {{v1}, {v3}}
        s12 + s3,         // {{v1,v2}, {v3}}
        s13 + s23,        // {{v1,v3}, {v2,v3}}
        s1 + s2 + s12,    // {{v1}, {v2}, {v1,v2}}
    };

    for (const auto& f : families) {
        for (int v1 = 1; v1 <= 3; v1++) {
            for (int v2 = v1 + 1; v2 <= 3; v2++) {
                bool swap_eq = (f.Swap(v1, v2) == f);
                bool sym = (f.SymChk(v1, v2) == 1);
                EXPECT_EQ(swap_eq, sym)
                    << "v1=" << v1 << " v2=" << v2
                    << " swap_eq=" << swap_eq << " sym=" << sym;
            }
        }
    }
}

TEST(ZDD_SymChkTest, IntermediateVariables) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v3}, {v2, v3}}: v1,v2 symmetric (v3 is intermediate between v1,v2 and both branches have it)
    ZDD f = ZDD::Single.Change(1).Change(3) + ZDD::Single.Change(2).Change(3);
    EXPECT_EQ(f.SymChk(1, 2), 1);

    // {{v1, v3}, {v2}}: v1,v2 not symmetric
    ZDD g = ZDD::Single.Change(1).Change(3) + ZDD::Single.Change(2);
    EXPECT_EQ(g.SymChk(1, 2), 0);
}

// --- bddimplyset / ZDD::ImplySet ---

TEST(ZDD_ImplySetTest, Basic) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2}, {v1, v3}}: v1 implies nothing else (v2 sometimes, v3 sometimes)
    ZDD f = ZDD::Single.Change(1).Change(2) + ZDD::Single.Change(1).Change(3);
    EXPECT_EQ(f.ImplySet(1), ZDD::Empty);

    // {{v1, v2}, {v3}}: v1 implies v2 (every set with v1 also has v2)
    ZDD g = ZDD::Single.Change(1).Change(2) + ZDD::Single.Change(3);
    ZDD expected = ZDD::Single.Change(2);  // {v2} as singleton family
    EXPECT_EQ(g.ImplySet(1), expected);
}

TEST(ZDD_ImplySetTest, ConsistentWithImplyChk) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2, v3}, {v1, v2}}: v1 implies v2
    ZDD f = ZDD::Single.Change(1).Change(2).Change(3) + ZDD::Single.Change(1).Change(2);
    ZDD iset = f.ImplySet(1);

    // Verify: for each variable v != 1, ImplyChk should agree with ImplySet
    for (int v = 2; v <= 5; v++) {
        ZDD sv = ZDD::Single.Change(v);
        bool in_iset = ((iset & sv) != ZDD::Empty);
        bool chk = (f.ImplyChk(1, v) == 1);
        EXPECT_EQ(in_iset, chk) << "v=" << v;
    }
}

// --- bddsymgrp / ZDD::SymGrp ---

TEST(ZDD_SymGrpTest, NoGroups) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}}: no variable pair is symmetric -> empty result
    ZDD f = ZDD::Single.Change(1);
    EXPECT_EQ(f.SymGrp(), ZDD::Empty);
}

TEST(ZDD_SymGrpTest, OneGroup) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}}: v1 and v2 are symmetric -> group {v1, v2}
    ZDD f = ZDD::Single.Change(1) + ZDD::Single.Change(2);
    ZDD expected = ZDD::Single.Change(1).Change(2);  // {{v1, v2}}
    EXPECT_EQ(f.SymGrp(), expected);
}

TEST(ZDD_SymGrpTest, MultipleGroups) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v3}, {v1, v4}, {v2, v3}, {v2, v4}}: v1↔v2 symmetric, v3↔v4 symmetric
    ZDD f = ZDD::Single.Change(1).Change(3) + ZDD::Single.Change(1).Change(4)
          + ZDD::Single.Change(2).Change(3) + ZDD::Single.Change(2).Change(4);

    ZDD grp = f.SymGrp();
    // Should contain {v1,v2} and {v3,v4}
    ZDD g12 = ZDD::Single.Change(1).Change(2);
    ZDD g34 = ZDD::Single.Change(3).Change(4);
    EXPECT_EQ(grp, g12 + g34);
}

// --- bddsymgrpnaive / ZDD::SymGrpNaive ---

TEST(ZDD_SymGrpNaiveTest, IncludesSingletons) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}}: SymGrpNaive includes size-1 groups
    ZDD f = ZDD::Single.Change(1);
    ZDD result = f.SymGrpNaive();
    // Should contain {v1} as a singleton group
    ZDD expected = ZDD::Single.Change(1);
    EXPECT_EQ(result, expected);
}

TEST(ZDD_SymGrpNaiveTest, ConsistentWithSymGrp) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v3}, {v1, v4}, {v2, v3}, {v2, v4}}
    ZDD f = ZDD::Single.Change(1).Change(3) + ZDD::Single.Change(1).Change(4)
          + ZDD::Single.Change(2).Change(3) + ZDD::Single.Change(2).Change(4);

    ZDD grp = f.SymGrp();
    ZDD naive = f.SymGrpNaive();

    // SymGrp (size>=2) should be subset of SymGrpNaive (all sizes)
    // For this case, naive should also have {v1,v2} and {v3,v4} (no singletons since all are in groups)
    EXPECT_EQ(grp, naive);
}

// --- bddsymset / ZDD::SymSet ---

TEST(ZDD_SymSetTest, Basic) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}}: v1 and v2 are symmetric -> SymSet(v1) = {{v2}}
    ZDD f = ZDD::Single.Change(1) + ZDD::Single.Change(2);
    ZDD expected = ZDD::Single.Change(2);
    EXPECT_EQ(f.SymSet(1), expected);
    EXPECT_EQ(f.SymSet(2), ZDD::Single.Change(1));
}

TEST(ZDD_SymSetTest, ConsistentWithSymChk) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v3}, {v1, v4}, {v2, v3}, {v2, v4}}
    ZDD f = ZDD::Single.Change(1).Change(3) + ZDD::Single.Change(1).Change(4)
          + ZDD::Single.Change(2).Change(3) + ZDD::Single.Change(2).Change(4);

    // For each variable v, SymSet(v) should contain exactly those w where SymChk(v, w)==1
    for (int v = 1; v <= 4; v++) {
        ZDD sset = f.SymSet(v);
        for (int w = 1; w <= 4; w++) {
            if (w == v) continue;
            ZDD sw = ZDD::Single.Change(w);
            bool in_sset = ((sset & sw) != ZDD::Empty);
            bool chk = (f.SymChk(v, w) == 1);
            EXPECT_EQ(in_sset, chk) << "v=" << v << " w=" << w;
        }
    }
}

TEST(ZDD_SymSetTest, IntermediateVariables) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v3}, {v2, v3}}: v1 and v2 are symmetric
    ZDD f = ZDD::Single.Change(1).Change(3) + ZDD::Single.Change(2).Change(3);
    EXPECT_EQ(f.SymSet(1), ZDD::Single.Change(2));
    EXPECT_EQ(f.SymSet(2), ZDD::Single.Change(1));
}

// --- bddcoimplyset / ZDD::CoImplySet ---

TEST(ZDD_CoImplySetTest, Basic) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2}, {v3}}: v1 co-implies v2 (ImplyChk is true, so CoImply is also true)
    ZDD f = ZDD::Single.Change(1).Change(2) + ZDD::Single.Change(3);
    ZDD cset = f.CoImplySet(1);
    // v2 should be in co-imply set
    EXPECT_NE(cset & ZDD::Single.Change(2), ZDD::Empty);
}

TEST(ZDD_CoImplySetTest, ConsistentWithCoImplyChk) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1}, {v2}}: v1 co-implies v2 (f10={∅}, f01={∅}, f10-f01=empty)
    ZDD f = ZDD::Single.Change(1) + ZDD::Single.Change(2);
    ZDD cset = f.CoImplySet(1);

    for (int w = 2; w <= 5; w++) {
        ZDD sw = ZDD::Single.Change(w);
        bool in_cset = ((cset & sw) != ZDD::Empty);
        bool chk = (f.CoImplyChk(1, w) == 1);
        EXPECT_EQ(in_cset, chk) << "w=" << w;
    }
}

TEST(ZDD_CoImplySetTest, TerminalCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(bddcoimplyset(bddnull, 1), bddnull);
    EXPECT_EQ(bddcoimplyset(bddempty, 1), bddempty);
    EXPECT_EQ(bddcoimplyset(bddsingle, 1), bddempty);
}

TEST(ZDD_CoImplySetTest, ExhaustiveConsistencyWithChk) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // Test several families where the recursive decomposition hits
    // the f11==bddempty / f10==bddempty branches
    std::vector<ZDD> families;

    // f = {{v1,v3}, {v2,v3}, {v2,v4}}
    families.push_back(
        ZDD::Single.Change(1).Change(3) +
        ZDD::Single.Change(2).Change(3) +
        ZDD::Single.Change(2).Change(4));

    // f = {{v1,v2,v3}, {v3,v4}}
    families.push_back(
        ZDD::Single.Change(1).Change(2).Change(3) +
        ZDD::Single.Change(3).Change(4));

    // f = {{v1}, {v2}, {v3,v4,v5}}
    families.push_back(
        ZDD::Single.Change(1) +
        ZDD::Single.Change(2) +
        ZDD::Single.Change(3).Change(4).Change(5));

    // f = {{v1,v2}, {v1,v3}, {v2,v4}, {v3,v5}}
    families.push_back(
        ZDD::Single.Change(1).Change(2) +
        ZDD::Single.Change(1).Change(3) +
        ZDD::Single.Change(2).Change(4) +
        ZDD::Single.Change(3).Change(5));

    for (const ZDD& f : families) {
        for (bddvar v = 1; v <= 5; v++) {
            ZDD cset = f.CoImplySet(v);
            for (bddvar w = 1; w <= 5; w++) {
                if (w == v) continue;
                ZDD sw = ZDD::Single.Change(w);
                bool in_cset = ((cset & sw) != ZDD::Empty);
                bool chk = (f.CoImplyChk(v, w) == 1);
                EXPECT_EQ(in_cset, chk)
                    << "family=" << f.id()
                    << " v=" << v << " w=" << w;
            }
        }
    }
}

// --- bdddivisor / ZDD::Divisor ---

TEST(ZDD_DivisorTest, TerminalCases) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(bdddivisor(bddnull), bddnull);
    EXPECT_EQ(bdddivisor(bddempty), bddempty);
    // Single set (monomial) -> returns single (unit element)
    ZDD s1 = ZDD::Single.Change(1);
    EXPECT_EQ(s1.Divisor(), ZDD::Single);
}

TEST(ZDD_DivisorTest, SimplePolynomial) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2}, {v1, v3}}: restricting by v1 gives {{v2}, {v3}} which is still polynomial
    // So divisor should be {{v2}, {v3}} or a further restriction
    ZDD f = ZDD::Single.Change(1).Change(2) + ZDD::Single.Change(1).Change(3);
    ZDD d = f.Divisor();
    // Divisor should have ≥ 2 sets (still polynomial) or be the end result
    EXPECT_EQ(d.IsPoly(), 1);
}

TEST(ZDD_DivisorTest, DivisorDivides) {
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    // {{v1, v2}, {v1, v3}}: the divisor should divide the original
    ZDD f = ZDD::Single.Change(1).Change(2) + ZDD::Single.Change(1).Change(3);
    ZDD d = f.Divisor();
    // f / d should not be empty (d is a factor)
    ZDD q = f / d;
    EXPECT_NE(q, ZDD::Empty);
    // q * d should give back f (or a subset of f)
    EXPECT_EQ(q * d, f);
}

TEST(ZDD_DivisorTest, CounterexampleThreeSets) {
    // Counterexample from review: f = {{v1}, {v2}, {v1,v2}}
    // Divisor should either return d where q*d == f, or bddsingle
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    ZDD s1 = ZDD::Single.Change(1);
    ZDD s2 = ZDD::Single.Change(2);
    ZDD s12 = ZDD::Single.Change(1).Change(2);
    ZDD f = s1 + s2 + s12;

    ZDD d = f.Divisor();
    ZDD q = f / d;
    EXPECT_EQ(q * d, f);
}

TEST(ZDD_PermitSymTest, NegativeN) {
    // n < 0: no set has negative cardinality, should return empty
    BDD_Init(256, 256);
    for (int i = 0; i < 5; i++) BDD_NewVar();

    EXPECT_EQ(ZDD::Single.PermitSym(-1), ZDD::Empty);

    ZDD s1 = ZDD::Single.Change(1);
    ZDD f = ZDD::Single + s1;  // {∅, {v1}}
    EXPECT_EQ(f.PermitSym(-1), ZDD::Empty);
    EXPECT_EQ(f.PermitSym(-100), ZDD::Empty);
}

// --- ZDD::Meet ---

TEST_F(BDDTest, ZDD_Meet_MatchesFreeFunction) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;  // {{v1}, {v2}}

    EXPECT_EQ(F.Meet(z_v1), ZDD_Meet(F, z_v1));
}

TEST_F(BDDTest, ZDD_Meet_WithSelf) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    EXPECT_EQ(z.Meet(z), z);
}

TEST_F(BDDTest, ZDD_Meet_WithEmpty) {
    bddvar v1 = bddnewvar();
    ZDD z = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD e(0);

    EXPECT_EQ(z.Meet(e), e);
}

TEST_F(BDDTest, ZDD_Meet_WithSingle) {
    bddvar v1 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD s(1);  // {{}}

    EXPECT_EQ(z_v1.Meet(s), s);
}

// --- bddimport invalid input tests ---

TEST_F(BDDTest, ImportEmptyStream) {
    std::istringstream empty("");
    bddp p = bddnull;
    int ret = bddimport(empty, &p, 1);
    EXPECT_LT(ret, 0);
}

TEST_F(BDDTest, ImportMalformedHeader) {
    std::istringstream bad("garbage data\n");
    bddp p = bddnull;
    int ret = bddimport(bad, &p, 1);
    EXPECT_LT(ret, 0);
}

TEST_F(BDDTest, ImportMissingNodeSection) {
    std::istringstream bad("_i 2\n_o 1\n_n 1\n");
    bddp p = bddnull;
    int ret = bddimport(bad, &p, 1);
    EXPECT_LT(ret, 0);
}

TEST_F(BDDTest, ImportzEmptyStream) {
    std::istringstream empty("");
    bddp p = bddnull;
    int ret = bddimportz(empty, &p, 1);
    EXPECT_LT(ret, 0);
}

TEST_F(BDDTest, ImportRejectsTrailingGarbageInTerminal) {
    // "FZ" should not be accepted as bddfalse
    std::istringstream ss("_i 1\n_o 1\n_n 0\nFZ\n");
    bddp p = bddnull;
    int ret = bddimport(ss, &p, 1);
    EXPECT_LT(ret, 0);
}

TEST_F(BDDTest, ImportRejectsTrailingGarbageInNodeId) {
    // "2abc" should not be accepted as node ID 2
    bddvar v1 = bddnewvar();
    (void)v1;
    std::istringstream ss("_i 1\n_o 1\n_n 1\n2 1 F T\n2abc\n");
    bddp p = bddnull;
    int ret = bddimport(ss, &p, 1);
    EXPECT_LT(ret, 0);
}

TEST_F(BDDTest, ImportRejectsDuplicateNodeId) {
    bddvar v1 = bddnewvar();
    (void)v1;
    // Two nodes with the same old_id = 2
    std::istringstream ss("_i 1\n_o 1\n_n 2\n2 1 F T\n2 1 T F\nT\n");
    bddp p = bddnull;
    int ret = bddimport(ss, &p, 1);
    EXPECT_LT(ret, 0);
}

// --- Garbage Collection ---

TEST_F(BDDTest, GCManualCallReducesNodes) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // Create some nodes without protecting them
    {
        BDD a = BDDvar(v1) & BDDvar(v2);
        BDD b = BDDvar(v1) | BDDvar(v2);
        // a, b are protected via BDD class
    }
    // a, b destructors ran; nodes are now unreachable
    uint64_t before = bddlive();
    bddgc();
    uint64_t after = bddlive();
    EXPECT_LE(after, before);
}

TEST_F(BDDTest, GCProtectUnprotect) {
    bddvar v1 = bddnewvar();
    bddp node = bddand(BDDvar(v1).GetID(), bddtrue);
    bddgc_protect(&node);
    EXPECT_EQ(bddgc_rootcount(), 1u);
    bddgc();
    // node should still be valid after GC
    EXPECT_EQ(bddtop(node), v1);
    bddgc_unprotect(&node);
    EXPECT_EQ(bddgc_rootcount(), 0u);
}

TEST_F(BDDTest, GCThreshold) {
    double orig = bddgc_getthreshold();
    EXPECT_DOUBLE_EQ(orig, 0.9);
    bddgc_setthreshold(0.5);
    EXPECT_DOUBLE_EQ(bddgc_getthreshold(), 0.5);
    bddgc_setthreshold(0.9);
}

// --- bddfinal ---

TEST_F(BDDTest, BddFinalReleasesResources) {
    bddvar v1 = bddnewvar();
    (void)v1;
    bddfinal();
    // After bddfinal, re-init should work
    bddinit(256, UINT64_MAX);
    bddvar v2 = bddnewvar();
    EXPECT_EQ(v2, 1u);
}

TEST_F(BDDTest, BddFinalDoubleCallSafe) {
    bddfinal();
    bddfinal();  // should not crash
    bddinit(256, UINT64_MAX);
}

TEST_F(BDDTest, BddFinalResetState) {
    bddnewvar();
    bddgc_setthreshold(0.5);
    bddfinal();
    bddinit(256, UINT64_MAX);
    // Threshold should be reset to default
    EXPECT_DOUBLE_EQ(bddgc_getthreshold(), 0.9);
    // Variable count should be 0
    EXPECT_EQ(bddvarused(), 0u);
}

// --- Memory pressure ---

TEST_F(BDDTest, NodeMaxLimitTriggersGC) {
    // Re-init with small node_max to force GC
    bddfinal();
    bddinit(16, 64);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // Create many nodes in a loop; GC should keep things running
    for (int i = 0; i < 100; i++) {
        BDD a = BDDvar(v1) & BDDvar(v2);
        BDD b = a | BDDvar(v3);
        (void)b;
    }
    // If we got here without overflow, GC is working
}

TEST_F(BDDTest, NodeMaxExhaustion) {
    bddfinal();
    bddinit(4, 4);
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddvar v4 = bddnewvar();
    // With only 4 slots and many variables, eventually nodes exhaust
    EXPECT_THROW({
        BDD a = BDDvar(v1) & BDDvar(v2) & BDDvar(v3) & BDDvar(v4);
        BDD b = BDDvar(v1) | BDDvar(v2) | BDDvar(v3) | BDDvar(v4);
        BDD c = a ^ b;
        (void)c;
    }, std::overflow_error);
}

TEST_F(BDDTest, BddfinalWithLiveObjectsThrows) {
    bddvar v = bddnewvar();
    {
        BDD x = BDDvar(v);
        EXPECT_THROW(bddfinal(), std::runtime_error);
        // x is still valid
        EXPECT_EQ(x.Top(), v);
    }
    // After x is destroyed, bddfinal succeeds
    bddfinal();
}

TEST_F(BDDTest, BddinitWithLiveObjectsThrows) {
    bddvar v = bddnewvar();
    {
        BDD x = BDDvar(v);  // non-terminal node
        EXPECT_THROW(bddinit(256, UINT64_MAX), std::runtime_error);
        (void)x;
    }
}

// --- bddplainsize / plain_size ---

TEST_F(BDDTest, PlainSize_Terminals) {
    EXPECT_EQ(bddplainsize(bddfalse, false), 0u);
    EXPECT_EQ(bddplainsize(bddtrue, false), 0u);
    EXPECT_EQ(bddplainsize(bddnull, false), 0u);
    EXPECT_EQ(bddplainsize(bddempty, true), 0u);
    EXPECT_EQ(bddplainsize(bddsingle, true), 0u);
}

TEST_F(BDDTest, PlainSize_BDD_NoComplement) {
    // A single variable BDD (no complement edges) should have same plain_size and Size
    bddvar v = bddnewvar();
    BDD x = BDDvar(v);
    EXPECT_EQ(x.plain_size(), x.Size());
}

TEST_F(BDDTest, PlainSize_BDD_WithComplement) {
    // ~x uses a complement edge, so plain_size should be >= Size
    bddvar v = bddnewvar();
    BDD x = BDDvar(v);
    BDD nx = ~x;
    // x and ~x share the same physical node, so Size is 1 for both
    EXPECT_EQ(x.Size(), 1u);
    EXPECT_EQ(nx.Size(), 1u);
    // plain_size of ~x: the root is complemented, so both children are
    // flipped. This may create a distinct logical node.
    EXPECT_GE(nx.plain_size(), nx.Size());
}

TEST_F(BDDTest, PlainSize_BDD_Complex) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;     // AND: no complement edges needed
    BDD g = ~(a & b);  // NAND: complement at root
    // NAND has same physical nodes as AND, but plain_size may differ
    EXPECT_EQ(f.Size(), g.Size());
    EXPECT_GE(g.plain_size(), g.Size());
}

TEST_F(BDDTest, PlainSize_ZDD_NoComplement) {
    bddvar v = bddnewvar();
    bddp z = bddchange(bddsingle, v);  // {{v}}
    EXPECT_EQ(bddplainsize(z, true), bddsize(z));
}

TEST_F(BDDTest, PlainSize_ZDD_WithComplement) {
    bddvar v = bddnewvar();
    bddp z = bddchange(bddsingle, v);  // {{v}}
    bddp nz = z ^ BDD_COMP_FLAG;       // complement edge
    // ZDD complement only affects lo child
    EXPECT_GE(bddplainsize(nz, true), bddsize(nz));
}

TEST_F(BDDTest, PlainSize_ZDD_MemberFunction) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD_ID(bddchange(bddsingle, v1));
    ZDD b = ZDD_ID(bddchange(bddsingle, v2));
    ZDD u = a + b;  // {{v1}, {v2}}
    EXPECT_GE(u.plain_size(), 1u);
    // plain_size >= Size always
    EXPECT_GE(u.plain_size(), u.Size());
}

// --- bddrawsize / bddplainsize (vector) ---

TEST_F(BDDTest, RawSize_Vector_BDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    BDD g = a | b;
    // Shared count should be <= sum of individual counts
    std::vector<BDD> vec = {f, g};
    EXPECT_LE(BDD::raw_size(vec), f.raw_size() + g.raw_size());
    // Shared count >= max of individual counts
    EXPECT_GE(BDD::raw_size(vec),
              std::max(f.raw_size(), g.raw_size()));
}

TEST_F(BDDTest, RawSize_Vector_ZDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD_ID(bddchange(bddsingle, v1));
    ZDD b = ZDD_ID(bddchange(bddsingle, v2));
    ZDD u = a + b;
    ZDD c = a * b;
    std::vector<ZDD> vec = {u, c};
    EXPECT_LE(ZDD::raw_size(vec), u.raw_size() + c.raw_size());
    EXPECT_GE(ZDD::raw_size(vec),
              std::max(u.raw_size(), c.raw_size()));
}

TEST_F(BDDTest, PlainSize_Vector_BDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    BDD g = ~(a & b);
    std::vector<BDD> vec = {f, g};
    EXPECT_GE(BDD::plain_size(vec), BDD::raw_size(vec));
}

TEST_F(BDDTest, PlainSize_Vector_ZDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD_ID(bddchange(bddsingle, v1));
    ZDD b = ZDD_ID(bddchange(bddsingle, v2));
    ZDD u = a + b;
    ZDD c = a * b;
    std::vector<ZDD> vec = {u, c};
    EXPECT_GE(ZDD::plain_size(vec), ZDD::raw_size(vec));
}

TEST_F(BDDTest, RawSize_Vector_Empty) {
    std::vector<BDD> empty_bdd;
    EXPECT_EQ(BDD::raw_size(empty_bdd), 0u);
    std::vector<ZDD> empty_zdd;
    EXPECT_EQ(ZDD::raw_size(empty_zdd), 0u);
}

TEST_F(BDDTest, RawSize_Vector_bddp) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v2));
    bddp g = bddor(bddprime(v1), bddprime(v2));
    std::vector<bddp> vec = {f, g};
    EXPECT_LE(bddrawsize(vec), bddsize(f) + bddsize(g));
}

// --- BDD satisfiability counting (double) ---

TEST_F(BDDTest, BDDCount_False) {
    EXPECT_EQ(bddcount(bddfalse, (bddvar)0), 0.0);
    EXPECT_EQ(bddcount(bddfalse, (bddvar)3), 0.0);
}

TEST_F(BDDTest, BDDCount_True) {
    EXPECT_EQ(bddcount(bddtrue, (bddvar)0), 1.0);
    EXPECT_EQ(bddcount(bddtrue, (bddvar)3), 8.0);
    EXPECT_EQ(bddcount(bddtrue, (bddvar)10), 1024.0);
}

TEST_F(BDDTest, BDDCount_SingleVar) {
    bddvar v1 = bddnewvar();
    bddp x1 = bddprime(v1);
    EXPECT_EQ(bddcount(x1, (bddvar)1), 1.0);
    bddnewvar(); bddnewvar();
    EXPECT_EQ(bddcount(x1, (bddvar)3), 4.0);
}

TEST_F(BDDTest, BDDCount_NotVar) {
    bddvar v1 = bddnewvar();
    bddp x1 = bddprime(v1);
    bddp not_x1 = bddnot(x1);
    EXPECT_EQ(bddcount(not_x1, (bddvar)1), 1.0);
    bddnewvar(); bddnewvar();
    EXPECT_EQ(bddcount(not_x1, (bddvar)3), 4.0);
}

TEST_F(BDDTest, BDDCount_And) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v2));
    EXPECT_EQ(bddcount(f, (bddvar)2), 1.0);
}

TEST_F(BDDTest, BDDCount_Or) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddor(bddprime(v1), bddprime(v2));
    EXPECT_EQ(bddcount(f, (bddvar)2), 3.0);
}

TEST_F(BDDTest, BDDCount_Xor) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddxor(bddprime(v1), bddprime(v2));
    EXPECT_EQ(bddcount(f, (bddvar)2), 2.0);
}

TEST_F(BDDTest, BDDCount_DontCareGap) {
    bddvar v1 = bddnewvar();
    bddnewvar();
    bddvar v3 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v3));
    EXPECT_EQ(bddcount(f, (bddvar)3), 2.0);
}

TEST_F(BDDTest, BDDCount_VarGtN_Error) {
    bddnewvar();
    bddvar v2 = bddnewvar();
    bddp x2 = bddprime(v2);
    EXPECT_THROW(bddcount(x2, (bddvar)1), std::invalid_argument);
}

TEST_F(BDDTest, BDDCount_Null) {
    EXPECT_EQ(bddcount(bddnull, (bddvar)3), 0.0);
}

TEST_F(BDDTest, BDDCount_ComplementSymmetry) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddor(bddprime(v2), bddprime(v3)));
    bddp nf = bddnot(f);
    EXPECT_EQ(bddcount(f, (bddvar)3) + bddcount(nf, (bddvar)3), 8.0);
}

// --- BDD satisfiability counting (exact) ---

TEST_F(BDDTest, BDDExactCount_False) {
    EXPECT_EQ(bddexactcount(bddfalse, (bddvar)0), bigint::BigInt(0));
    EXPECT_EQ(bddexactcount(bddfalse, (bddvar)3), bigint::BigInt(0));
}

TEST_F(BDDTest, BDDExactCount_True) {
    EXPECT_EQ(bddexactcount(bddtrue, (bddvar)0), bigint::BigInt(1));
    EXPECT_EQ(bddexactcount(bddtrue, (bddvar)3), bigint::BigInt(8));
}

TEST_F(BDDTest, BDDExactCount_And) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v2));
    EXPECT_EQ(bddexactcount(f, (bddvar)2), bigint::BigInt(1));
}

TEST_F(BDDTest, BDDExactCount_Or) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddor(bddprime(v1), bddprime(v2));
    EXPECT_EQ(bddexactcount(f, (bddvar)2), bigint::BigInt(3));
}

TEST_F(BDDTest, BDDExactCount_NotOr) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddp f = bddnot(bddor(bddprime(v1), bddprime(v2)));
    EXPECT_EQ(bddexactcount(f, (bddvar)2), bigint::BigInt(1));
}

TEST_F(BDDTest, BDDExactCount_ComplementSymmetry) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v2));
    bddp nf = bddnot(f);
    bigint::BigInt cf = bddexactcount(f, (bddvar)3);
    bigint::BigInt cnf = bddexactcount(nf, (bddvar)3);
    EXPECT_EQ(cf + cnf, bigint::BigInt(8));
}

TEST_F(BDDTest, BDDExactCount_DontCareGap) {
    bddvar v1 = bddnewvar();
    bddnewvar();
    bddvar v3 = bddnewvar();
    bddp f = bddand(bddprime(v1), bddprime(v3));
    EXPECT_EQ(bddexactcount(f, (bddvar)3), bigint::BigInt(2));
}

TEST_F(BDDTest, BDDExactCount_VarGtN_Error) {
    bddnewvar();
    bddvar v2 = bddnewvar();
    bddp x2 = bddprime(v2);
    EXPECT_THROW(bddexactcount(x2, (bddvar)1), std::invalid_argument);
}

// --- BDD/ZDD child accessor functions ---

// BDD raw_child0/1: basic node
TEST_F(BDDTest, BDD_RawChild_BasicNode) {
    bddvar v1 = bddnewvar();
    BDD f = BDDvar(v1);
    // BDDvar(v1) creates node with lo=bddfalse, hi=bddtrue
    EXPECT_EQ(f.raw_child0().GetID(), bddfalse);
    EXPECT_EQ(f.raw_child1().GetID(), bddtrue);
    // raw_child(int) should match
    EXPECT_EQ(f.raw_child(0).GetID(), bddfalse);
    EXPECT_EQ(f.raw_child(1).GetID(), bddtrue);
}

// BDD raw_child: complement node returns stored values (not resolved)
TEST_F(BDDTest, BDD_RawChild_ComplementNode) {
    bddvar v1 = bddnewvar();
    BDD f = BDDvar(v1);
    BDD nf = ~f;
    // raw_child ignores complement bit, returns stored lo/hi
    EXPECT_EQ(nf.raw_child0().GetID(), bddfalse);
    EXPECT_EQ(nf.raw_child1().GetID(), bddtrue);
}

// BDD child0/1: basic node (no complement)
TEST_F(BDDTest, BDD_Child_BasicNode) {
    bddvar v1 = bddnewvar();
    BDD f = BDDvar(v1);
    // No complement on root, so child == raw_child
    EXPECT_EQ(f.child0().GetID(), bddfalse);
    EXPECT_EQ(f.child1().GetID(), bddtrue);
    EXPECT_EQ(f.child(0).GetID(), bddfalse);
    EXPECT_EQ(f.child(1).GetID(), bddtrue);
}

// BDD child0/1: complement node (both children flipped)
TEST_F(BDDTest, BDD_Child_ComplementNode) {
    bddvar v1 = bddnewvar();
    BDD f = BDDvar(v1);
    BDD nf = ~f;
    // BDD complement: both lo and hi get flipped
    EXPECT_EQ(nf.child0().GetID(), bddtrue);
    EXPECT_EQ(nf.child1().GetID(), bddfalse);
    EXPECT_EQ(nf.child(0).GetID(), bddtrue);
    EXPECT_EQ(nf.child(1).GetID(), bddfalse);
}

// BDD child: multi-variable node
TEST_F(BDDTest, BDD_Child_MultiVariable) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;  // v1 AND v2
    // Top variable is v2 (higher level = closer to root).
    // lo = restrict v2=0: v1 AND 0 = false
    // hi = restrict v2=1: v1 AND 1 = v1
    EXPECT_EQ(f.child0(), BDD::False);
    EXPECT_EQ(f.child1(), a);
}

// BDD child: complement of multi-variable node
TEST_F(BDDTest, BDD_Child_ComplementMultiVariable) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    BDD nf = ~f;
    // ~(v1 AND v2): complement flips both children
    EXPECT_EQ(nf.child0(), BDD::True);
    EXPECT_EQ(nf.child1(), ~a);
}

// BDD static versions match member versions
TEST_F(BDDTest, BDD_StaticChild_MatchesMember) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD f = BDDvar(v1) | BDDvar(v2);
    bddp fid = f.GetID();
    EXPECT_EQ(BDD::raw_child0(fid), f.raw_child0().GetID());
    EXPECT_EQ(BDD::raw_child1(fid), f.raw_child1().GetID());
    EXPECT_EQ(BDD::raw_child(fid, 0), f.raw_child(0).GetID());
    EXPECT_EQ(BDD::raw_child(fid, 1), f.raw_child(1).GetID());
    EXPECT_EQ(BDD::child0(fid), f.child0().GetID());
    EXPECT_EQ(BDD::child1(fid), f.child1().GetID());
    EXPECT_EQ(BDD::child(fid, 0), f.child(0).GetID());
    EXPECT_EQ(BDD::child(fid, 1), f.child(1).GetID());
}

// BDD child: terminal node throws
TEST_F(BDDTest, BDD_Child_TerminalThrows) {
    EXPECT_THROW(BDD::raw_child0(bddfalse), std::invalid_argument);
    EXPECT_THROW(BDD::raw_child1(bddtrue), std::invalid_argument);
    EXPECT_THROW(BDD::child0(bddfalse), std::invalid_argument);
    EXPECT_THROW(BDD::child1(bddtrue), std::invalid_argument);
    EXPECT_THROW(BDD::child(bddfalse, 0), std::invalid_argument);
    EXPECT_THROW(BDD::raw_child(bddtrue, 1), std::invalid_argument);
    // Member versions on terminal BDD
    BDD f(0);  // false
    EXPECT_THROW(f.child0(), std::invalid_argument);
    EXPECT_THROW(f.raw_child1(), std::invalid_argument);
}

// BDD child: bddnull throws
TEST_F(BDDTest, BDD_Child_NullThrows) {
    EXPECT_THROW(BDD::raw_child0(bddnull), std::invalid_argument);
    EXPECT_THROW(BDD::raw_child1(bddnull), std::invalid_argument);
    EXPECT_THROW(BDD::child0(bddnull), std::invalid_argument);
    EXPECT_THROW(BDD::child1(bddnull), std::invalid_argument);
    // Member version
    BDD n(-1);  // null
    EXPECT_THROW(n.child0(), std::invalid_argument);
    EXPECT_THROW(n.raw_child0(), std::invalid_argument);
}

// BDD child(int): invalid argument throws
TEST_F(BDDTest, BDD_Child_InvalidArgThrows) {
    bddvar v1 = bddnewvar();
    BDD f = BDDvar(v1);
    bddp fid = f.GetID();
    EXPECT_THROW(BDD::child(fid, 2), std::invalid_argument);
    EXPECT_THROW(BDD::child(fid, -1), std::invalid_argument);
    EXPECT_THROW(BDD::raw_child(fid, 2), std::invalid_argument);
    EXPECT_THROW(BDD::raw_child(fid, -1), std::invalid_argument);
    EXPECT_THROW(f.child(2), std::invalid_argument);
    EXPECT_THROW(f.raw_child(-1), std::invalid_argument);
}

// ZDD raw_child0/1: basic node
TEST_F(BDDTest, ZDD_RawChild_BasicNode) {
    bddvar v1 = bddnewvar();
    // {{v1}} = ZDD::getnode(v1, bddempty, bddsingle)
    ZDD f = ZDD::Single.Change(v1);
    // Change creates a node with lo=bddempty, hi=bddsingle
    EXPECT_EQ(f.raw_child0().GetID(), bddempty);
    EXPECT_EQ(f.raw_child1().GetID(), bddsingle);
    EXPECT_EQ(f.raw_child(0).GetID(), bddempty);
    EXPECT_EQ(f.raw_child(1).GetID(), bddsingle);
}

// ZDD raw_child: complement node returns stored values
TEST_F(BDDTest, ZDD_RawChild_ComplementNode) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::Single.Change(v1);
    ZDD nf = ~f;
    // raw_child ignores complement, returns stored lo/hi
    EXPECT_EQ(nf.raw_child0().GetID(), bddempty);
    EXPECT_EQ(nf.raw_child1().GetID(), bddsingle);
}

// ZDD child0/1: basic node (no complement)
TEST_F(BDDTest, ZDD_Child_BasicNode) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::Single.Change(v1);
    EXPECT_EQ(f.child0().GetID(), bddempty);
    EXPECT_EQ(f.child1().GetID(), bddsingle);
    EXPECT_EQ(f.child(0).GetID(), bddempty);
    EXPECT_EQ(f.child(1).GetID(), bddsingle);
}

// ZDD child0/1: complement node (ZDD semantics: only lo flipped)
TEST_F(BDDTest, ZDD_Child_ComplementNode) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::Single.Change(v1);
    ZDD nf = ~f;
    // ZDD complement: only lo is flipped, hi stays
    EXPECT_EQ(nf.child0().GetID(), bddnot(bddempty));  // lo flipped
    EXPECT_EQ(nf.child1().GetID(), bddsingle);          // hi unchanged
}

// ZDD child: multi-variable node
TEST_F(BDDTest, ZDD_Child_MultiVariable) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1, v2}} = Single.Change(v1).Change(v2)
    ZDD f = ZDD::Single.Change(v1).Change(v2);
    // Top variable is v2 (higher level = closer to root)
    // child0 = empty, child1 = {{v1}} = Single.Change(v1)
    EXPECT_EQ(f.child0(), ZDD::Empty);
    EXPECT_EQ(f.child1(), ZDD::Single.Change(v1));
}

// ZDD child: complement semantics differ from BDD
TEST_F(BDDTest, ZDD_Child_ComplementSemanticsDifferFromBDD) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::Single.Change(v1);
    ZDD nf = ~f;
    // ZDD: hi is NOT flipped (unlike BDD)
    // raw hi = bddsingle, child1 should also be bddsingle
    EXPECT_EQ(nf.child1().GetID(), nf.raw_child1().GetID());
    // But lo IS flipped
    EXPECT_NE(nf.child0().GetID(), nf.raw_child0().GetID());
    EXPECT_EQ(nf.child0().GetID(), bddnot(nf.raw_child0().GetID()));
}

// ZDD static versions match member versions
TEST_F(BDDTest, ZDD_StaticChild_MatchesMember) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::Single.Change(v1) + ZDD::Single.Change(v2);
    bddp fid = f.GetID();
    EXPECT_EQ(ZDD::raw_child0(fid), f.raw_child0().GetID());
    EXPECT_EQ(ZDD::raw_child1(fid), f.raw_child1().GetID());
    EXPECT_EQ(ZDD::raw_child(fid, 0), f.raw_child(0).GetID());
    EXPECT_EQ(ZDD::raw_child(fid, 1), f.raw_child(1).GetID());
    EXPECT_EQ(ZDD::child0(fid), f.child0().GetID());
    EXPECT_EQ(ZDD::child1(fid), f.child1().GetID());
    EXPECT_EQ(ZDD::child(fid, 0), f.child(0).GetID());
    EXPECT_EQ(ZDD::child(fid, 1), f.child(1).GetID());
}

// ZDD child: terminal node throws
TEST_F(BDDTest, ZDD_Child_TerminalThrows) {
    EXPECT_THROW(ZDD::raw_child0(bddempty), std::invalid_argument);
    EXPECT_THROW(ZDD::raw_child1(bddsingle), std::invalid_argument);
    EXPECT_THROW(ZDD::child0(bddempty), std::invalid_argument);
    EXPECT_THROW(ZDD::child1(bddsingle), std::invalid_argument);
    ZDD e(0);  // empty
    EXPECT_THROW(e.child0(), std::invalid_argument);
    EXPECT_THROW(e.raw_child1(), std::invalid_argument);
}

// ZDD child: bddnull throws
TEST_F(BDDTest, ZDD_Child_NullThrows) {
    EXPECT_THROW(ZDD::raw_child0(bddnull), std::invalid_argument);
    EXPECT_THROW(ZDD::child0(bddnull), std::invalid_argument);
    ZDD n(-1);  // null
    EXPECT_THROW(n.child0(), std::invalid_argument);
}

// ZDD child(int): invalid argument throws
TEST_F(BDDTest, ZDD_Child_InvalidArgThrows) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::Single.Change(v1);
    bddp fid = f.GetID();
    EXPECT_THROW(ZDD::child(fid, 2), std::invalid_argument);
    EXPECT_THROW(ZDD::raw_child(fid, -1), std::invalid_argument);
    EXPECT_THROW(f.child(3), std::invalid_argument);
    EXPECT_THROW(f.raw_child(-1), std::invalid_argument);
}

// --- BDD class count/exact_count ---

TEST_F(BDDTest, BDDCount_ClassMethod) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD a = BDDvar(v1);
    BDD b = BDDvar(v2);
    BDD f = a & b;
    EXPECT_EQ(f.count(2), 1.0);
    EXPECT_EQ(f.exact_count(2), bigint::BigInt(1));
    BDD nf = ~f;
    EXPECT_EQ(nf.count(2), 3.0);
    EXPECT_EQ(nf.exact_count(2), bigint::BigInt(3));
}

// --- bddhasempty / ZDD::has_empty ---

TEST_F(BDDTest, ZDD_HasEmpty_EmptyFamily) {
    // Empty family {} contains no sets at all
    EXPECT_FALSE(bddhasempty(bddempty));
    ZDD f(0);
    EXPECT_FALSE(f.has_empty());
}

TEST_F(BDDTest, ZDD_HasEmpty_SingleFamily) {
    // Unit family {∅} contains the empty set
    EXPECT_TRUE(bddhasempty(bddsingle));
    ZDD f(1);
    EXPECT_TRUE(f.has_empty());
}

TEST_F(BDDTest, ZDD_HasEmpty_SingletonSet) {
    // Family {{a}} does not contain the empty set
    bddp a = bddchange(bddsingle, 1);
    EXPECT_FALSE(bddhasempty(a));
    ZDD f = ZDD_ID(a);
    EXPECT_FALSE(f.has_empty());
}

TEST_F(BDDTest, ZDD_HasEmpty_UnionWithEmpty) {
    // Family {{a}, ∅} contains the empty set
    bddp a = bddchange(bddsingle, 1);
    bddp u = bddunion(a, bddsingle);
    EXPECT_TRUE(bddhasempty(u));
    ZDD z = ZDD_ID(u);
    EXPECT_TRUE(z.has_empty());
}

TEST_F(BDDTest, ZDD_HasEmpty_ComplementEdge) {
    // Complement toggles ∅ membership
    bddp a = bddchange(bddsingle, 1);  // {{a}}, no ∅
    EXPECT_FALSE(bddhasempty(a));
    EXPECT_TRUE(bddhasempty(bddnot(a)));  // complement adds ∅

    bddp u = bddunion(a, bddsingle);  // {{a}, ∅}, has ∅
    EXPECT_TRUE(bddhasempty(u));
    EXPECT_FALSE(bddhasempty(bddnot(u)));  // complement removes ∅
}

// --- bddcontains / ZDD::contains ---

TEST_F(BDDTest, ZDD_Contains_EmptyFamily) {
    EXPECT_FALSE(bddcontains(bddempty, {}));
    EXPECT_FALSE(bddcontains(bddempty, {1}));
}

TEST_F(BDDTest, ZDD_Contains_SingleFamily) {
    // {∅} contains ∅ but not {1}
    EXPECT_TRUE(bddcontains(bddsingle, {}));
    EXPECT_FALSE(bddcontains(bddsingle, {1}));
}

TEST_F(BDDTest, ZDD_Contains_SingletonSet) {
    bddp a = bddchange(bddsingle, 1);  // {{1}}
    EXPECT_TRUE(bddcontains(a, {1}));
    EXPECT_FALSE(bddcontains(a, {}));
    EXPECT_FALSE(bddcontains(a, {2}));
    EXPECT_FALSE(bddcontains(a, {1, 2}));
}

TEST_F(BDDTest, ZDD_Contains_MultipleVars) {
    // {{1,2}}
    bddp s = bddchange(bddchange(bddsingle, 1), 2);
    EXPECT_TRUE(bddcontains(s, {1, 2}));
    EXPECT_FALSE(bddcontains(s, {1}));
    EXPECT_FALSE(bddcontains(s, {2}));
    EXPECT_FALSE(bddcontains(s, {}));
}

TEST_F(BDDTest, ZDD_Contains_FamilyWithEmpty) {
    // {{1}, ∅}
    bddp a = bddchange(bddsingle, 1);
    bddp f = bddunion(a, bddsingle);
    EXPECT_TRUE(bddcontains(f, {1}));
    EXPECT_TRUE(bddcontains(f, {}));
    EXPECT_FALSE(bddcontains(f, {2}));
}

TEST_F(BDDTest, ZDD_Contains_DuplicateInput) {
    bddp a = bddchange(bddsingle, 1);  // {{1}}
    EXPECT_TRUE(bddcontains(a, {1, 1}));
}

TEST_F(BDDTest, ZDD_Contains_UnsortedInput) {
    bddp s = bddchange(bddchange(bddsingle, 1), 2);  // {{1,2}}
    EXPECT_TRUE(bddcontains(s, {2, 1}));
}

TEST_F(BDDTest, ZDD_Contains_PowerSet) {
    ZDD ps = ZDD::power_set(3);
    // All 8 subsets should be contained
    EXPECT_TRUE(bddcontains(ps.GetID(), {}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {1}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {2}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {3}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {1, 2}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {1, 3}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {2, 3}));
    EXPECT_TRUE(bddcontains(ps.GetID(), {1, 2, 3}));
    // {4} should not be contained (variable 4 exists but not in power_set(3))
}

TEST_F(BDDTest, ZDD_Contains_ComplementEdge) {
    bddp a = bddchange(bddsingle, 1);  // {{1}}, no ∅
    // ~{{1}} toggles ∅ membership → {{1}, ∅} → has ∅
    EXPECT_TRUE(bddcontains(bddnot(a), {}));
    EXPECT_TRUE(bddcontains(bddnot(a), {1}));

    bddp u = bddunion(a, bddsingle);  // {{1}, ∅}
    // ~{{1}, ∅} removes ∅ → {{1}}
    EXPECT_FALSE(bddcontains(bddnot(u), {}));
    EXPECT_TRUE(bddcontains(bddnot(u), {1}));
}

TEST_F(BDDTest, ZDD_Contains_ConsistencyWithEnumerate) {
    ZDD ps = ZDD::power_set(3);
    auto all_sets = ps.enumerate();
    for (auto& s : all_sets) {
        EXPECT_TRUE(ps.contains(s));
    }
    // A non-member
    EXPECT_FALSE(ps.contains({4}));
}

TEST_F(BDDTest, ZDD_Contains_Null) {
    EXPECT_FALSE(bddcontains(bddnull, {}));
    EXPECT_FALSE(bddcontains(bddnull, {1}));
}

TEST_F(BDDTest, ZDD_Contains_MemberFunction) {
    ZDD ps = ZDD::power_set(3);
    EXPECT_TRUE(ps.contains({1, 2}));
    EXPECT_FALSE(ps.contains({4}));
}

// --- bddissubset / ZDD::is_subset_family ---

TEST_F(BDDTest, ZDD_IsSubsetFamily_EmptyIsSubsetOfAll) {
    ZDD e(0);
    ZDD u(1);
    ZDD ps = ZDD::power_set(3);
    EXPECT_TRUE(e.is_subset_family(e));
    EXPECT_TRUE(e.is_subset_family(u));
    EXPECT_TRUE(e.is_subset_family(ps));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_SelfSubset) {
    ZDD ps = ZDD::power_set(3);
    EXPECT_TRUE(ps.is_subset_family(ps));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_UnitFamily) {
    // {∅} ⊆ power_set(3)  → true
    ZDD u(1);
    ZDD ps = ZDD::power_set(3);
    EXPECT_TRUE(u.is_subset_family(ps));
    // power_set(3) ⊆ {∅}  → false
    EXPECT_FALSE(ps.is_subset_family(u));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_SingletonSubset) {
    // {{1}} ⊆ {{1}, {2}} → true
    ZDD s1 = ZDD::singleton(1);
    ZDD s2 = ZDD::singleton(2);
    ZDD f = s1 + s2;
    EXPECT_TRUE(s1.is_subset_family(f));
    EXPECT_TRUE(s2.is_subset_family(f));
    // {{1}, {2}} ⊆ {{1}} → false
    EXPECT_FALSE(f.is_subset_family(s1));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_NotSubset) {
    // {{1}} ⊆ {{2}} → false
    ZDD s1 = ZDD::singleton(1);
    ZDD s2 = ZDD::singleton(2);
    EXPECT_FALSE(s1.is_subset_family(s2));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_ChooseSubsetOfPowerSet) {
    // C(3,2) ⊆ power_set(3) → true
    ZDD ps = ZDD::power_set(3);
    ZDD c32 = ZDD::combination(3, 2);
    EXPECT_TRUE(c32.is_subset_family(ps));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_ComplementEdge) {
    // {{1}} complemented → {{1}, ∅}
    // {{1}, ∅} ⊆ power_set(3) → true
    ZDD s1 = ZDD::singleton(1);
    ZDD comp = ~s1;
    ZDD ps = ZDD::power_set(3);
    EXPECT_TRUE(comp.is_subset_family(ps));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_ConsistencyWithSubtract) {
    // F ⊆ G iff (F - G) == ∅
    ZDD ps = ZDD::power_set(3);
    ZDD c32 = ZDD::combination(3, 2);
    ZDD s1 = ZDD::singleton(1);

    EXPECT_EQ(c32.is_subset_family(ps), (c32 - ps).is_zero());
    EXPECT_EQ(ps.is_subset_family(c32), (ps - c32).is_zero());
    EXPECT_EQ(s1.is_subset_family(c32), (s1 - c32).is_zero());
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_Null) {
    ZDD n(-1);
    ZDD ps = ZDD::power_set(3);
    EXPECT_FALSE(n.is_subset_family(ps));
    EXPECT_FALSE(ps.is_subset_family(n));
}

TEST_F(BDDTest, ZDD_IsSubsetFamily_FreeFunction) {
    bddp ps = ZDD::power_set(3).GetID();
    bddp c32 = ZDD::combination(3, 2).GetID();
    EXPECT_TRUE(bddissubset(c32, ps));
    EXPECT_FALSE(bddissubset(ps, c32));
}

// --- bddflatten / ZDD::flatten ---

TEST_F(BDDTest, ZDD_Flatten_EmptyFamily) {
    ZDD e(0);
    ZDD result = e.flatten();
    EXPECT_TRUE(result.is_zero());
}

TEST_F(BDDTest, ZDD_Flatten_Null) {
    EXPECT_EQ(bddflatten(bddnull), bddnull);
}

TEST_F(BDDTest, ZDD_Flatten_UnitFamily) {
    // {∅} → {∅}
    ZDD u(1);
    ZDD result = u.flatten();
    EXPECT_TRUE(result.is_one());
}

TEST_F(BDDTest, ZDD_Flatten_SingletonSet) {
    // {{1}} → {{1}}
    ZDD s1 = ZDD::singleton(1);
    ZDD result = s1.flatten();
    EXPECT_EQ(result.GetID(), s1.GetID());
}

TEST_F(BDDTest, ZDD_Flatten_TwoDisjointSets) {
    // {{1}, {2}} → {{1,2}}
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    ZDD result = f.flatten();
    ZDD expected = ZDD::single_set({1, 2});
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Flatten_OverlappingSets) {
    // {{1,2}, {2,3}} → {{1,2,3}}
    ZDD f = ZDD::single_set({1, 2}) + ZDD::single_set({2, 3});
    ZDD result = f.flatten();
    ZDD expected = ZDD::single_set({1, 2, 3});
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Flatten_PowerSet) {
    // power_set(3) → {{1,2,3}}
    ZDD ps = ZDD::power_set(3);
    ZDD result = ps.flatten();
    ZDD expected = ZDD::single_set({1, 2, 3});
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Flatten_WithEmptySet) {
    // {{}, {1,2}} → {{1,2}} (∅ contributes nothing)
    ZDD f = ZDD(1) + ZDD::single_set({1, 2});
    ZDD result = f.flatten();
    ZDD expected = ZDD::single_set({1, 2});
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Flatten_ComplementEdge) {
    // ~{{1}} = {{1}, ∅} → flatten = {{1}}
    ZDD s1 = ZDD::singleton(1);
    ZDD comp = ~s1;
    ZDD result = comp.flatten();
    EXPECT_EQ(result.GetID(), s1.GetID());
}

TEST_F(BDDTest, ZDD_Flatten_ResultIsSingleSet) {
    // flatten always returns 0 or 1 sets
    ZDD ps = ZDD::power_set(4);
    ZDD result = ps.flatten();
    EXPECT_EQ(result.exact_count(), bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Flatten_FreeFunction) {
    bddp f = bddunion(bddchange(bddsingle, 1), bddchange(bddsingle, 2));
    bddp result = bddflatten(f);
    bddp expected = bddchange(bddchange(bddsingle, 1), 2);
    EXPECT_EQ(result, expected);
}

// --- bddcoalesce / ZDD::coalesce ---

TEST_F(BDDTest, ZDD_Coalesce_EmptyFamily) {
    ZDD e(0);
    EXPECT_TRUE(e.coalesce(1, 2).is_zero());
}

TEST_F(BDDTest, ZDD_Coalesce_UnitFamily) {
    // {∅} → {∅} (no variables to merge)
    ZDD u(1);
    EXPECT_TRUE(u.coalesce(1, 2).is_one());
}

TEST_F(BDDTest, ZDD_Coalesce_SameVariable) {
    // coalesce(v, v) = identity
    ZDD f = ZDD::power_set(3);
    EXPECT_EQ(f.coalesce(1, 1).GetID(), f.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_V2NotPresent) {
    // {{1}} coalesce(1,2): v2=2 allocated but not in any set → unchanged
    ZDD::new_var();  // var 1
    ZDD::new_var();  // var 2
    ZDD s1 = ZDD::singleton(1);
    ZDD result = s1.coalesce(1, 2);
    EXPECT_EQ(result.GetID(), s1.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_V1OutOfRange) {
    ZDD::new_var();
    ZDD s1 = ZDD::singleton(1);
    EXPECT_THROW(s1.coalesce(99, 1), std::invalid_argument);
}

TEST_F(BDDTest, ZDD_Coalesce_V2OutOfRange) {
    ZDD::new_var();
    ZDD s1 = ZDD::singleton(1);
    EXPECT_THROW(s1.coalesce(1, 99), std::invalid_argument);
}

TEST_F(BDDTest, ZDD_Coalesce_V1NotPresent) {
    // {{2}} coalesce(1,2): v2=2 replaced by v1=1 → {{1}}
    ZDD s2 = ZDD::singleton(2);
    ZDD result = s2.coalesce(1, 2);
    ZDD expected = ZDD::singleton(1);
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_BothPresent) {
    // {{1,2}} coalesce(1,2): both present → v2 removed, v1 stays → {{1}}
    ZDD s12 = ZDD::single_set({1, 2});
    ZDD result = s12.coalesce(1, 2);
    ZDD expected = ZDD::singleton(1);
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_MergesDuplicates) {
    // {{1}, {2}} coalesce(1,2): both become {{1}} → {{1}} (deduplicated)
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    ZDD result = f.coalesce(1, 2);
    ZDD expected = ZDD::singleton(1);
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_MixedSets) {
    // {{1,3}, {2,3}} coalesce(1,2): → {{1,3}} (deduplicated)
    ZDD f = ZDD::single_set({1, 3}) + ZDD::single_set({2, 3});
    ZDD result = f.coalesce(1, 2);
    ZDD expected = ZDD::single_set({1, 3});
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_PreservesOtherVariables) {
    // {{2,3}} coalesce(1,2): → {{1,3}}
    ZDD f = ZDD::single_set({2, 3});
    ZDD result = f.coalesce(1, 2);
    auto sets = result.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    std::vector<bddvar> expected = {1, 3};
    EXPECT_EQ(sets[0], expected);
}

TEST_F(BDDTest, ZDD_Coalesce_WithEmptySet) {
    // {{}, {1}, {2}} coalesce(1,2): ∅→∅, {1}→{1}, {2}→{1} → {{}, {1}}
    ZDD f = ZDD(1) + ZDD::singleton(1) + ZDD::singleton(2);
    ZDD result = f.coalesce(1, 2);
    ZDD expected = ZDD(1) + ZDD::singleton(1);
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_ConsistencyWithEnumerate) {
    // Verify by brute-force: enumerate, coalesce each set, compare
    // F = {{1,2}, {2,3}, {3}}
    ZDD f = ZDD::single_set({1, 2}) + ZDD::single_set({2, 3}) + ZDD::singleton(3);
    ZDD result = f.coalesce(1, 2);
    // {1,2} → v2 removed, v1 stays → {1}
    // {2,3} → v2 replaced by v1 → {1,3}
    // {3}   → no v1 or v2 → {3}
    ZDD expected = ZDD::singleton(1) + ZDD::single_set({1, 3}) + ZDD::singleton(3);
    EXPECT_EQ(result.GetID(), expected.GetID());
}

TEST_F(BDDTest, ZDD_Coalesce_FreeFunction) {
    bddp f = bddunion(bddchange(bddsingle, 1), bddchange(bddsingle, 2));
    bddp result = bddcoalesce(f, 1, 2);
    bddp expected = bddchange(bddsingle, 1);
    EXPECT_EQ(result, expected);
}

// --- bddchoose / ZDD::choose ---

TEST_F(BDDTest, ZDD_Choose_TerminalCases) {
    EXPECT_EQ(bddchoose(bddnull, 0), bddnull);
    EXPECT_EQ(bddchoose(bddempty, 0), bddempty);
    EXPECT_EQ(bddchoose(bddempty, 1), bddempty);
    EXPECT_EQ(bddchoose(bddsingle, 0), bddsingle);
    EXPECT_EQ(bddchoose(bddsingle, 1), bddempty);
}

TEST_F(BDDTest, ZDD_Choose_NegativeK) {
    EXPECT_EQ(bddchoose(bddsingle, -1), bddempty);
    bddp a = bddchange(bddsingle, 1);
    EXPECT_EQ(bddchoose(a, -1), bddempty);
}

TEST_F(BDDTest, ZDD_Choose_ZeroK_WithEmpty) {
    // {{1}, ∅} → choose(0) = {∅}
    bddp a = bddchange(bddsingle, 1);
    bddp f = bddunion(a, bddsingle);
    EXPECT_EQ(bddchoose(f, 0), bddsingle);
}

TEST_F(BDDTest, ZDD_Choose_ZeroK_WithoutEmpty) {
    // {{1}} → choose(0) = {}
    bddp a = bddchange(bddsingle, 1);
    EXPECT_EQ(bddchoose(a, 0), bddempty);
}

TEST_F(BDDTest, ZDD_Choose_PowerSet) {
    ZDD ps = ZDD::power_set(3);
    // C(3,0)=1, C(3,1)=3, C(3,2)=3, C(3,3)=1
    ZDD c0 = ps.choose(0);
    ZDD c1 = ps.choose(1);
    ZDD c2 = ps.choose(2);
    ZDD c3 = ps.choose(3);

    EXPECT_EQ(c0.exact_count(), bigint::BigInt(1));
    EXPECT_EQ(c1.exact_count(), bigint::BigInt(3));
    EXPECT_EQ(c2.exact_count(), bigint::BigInt(3));
    EXPECT_EQ(c3.exact_count(), bigint::BigInt(1));

    // choose(k) should equal combination(3, k) for power_set(3)
    EXPECT_EQ(c1.GetID(), ZDD::combination(3, 1).GetID());
    EXPECT_EQ(c2.GetID(), ZDD::combination(3, 2).GetID());
    EXPECT_EQ(c3.GetID(), ZDD::combination(3, 3).GetID());
}

TEST_F(BDDTest, ZDD_Choose_KExceedsMax) {
    ZDD ps = ZDD::power_set(3);
    EXPECT_EQ(ps.choose(4), ZDD::Empty);
    EXPECT_EQ(ps.choose(100), ZDD::Empty);
}

TEST_F(BDDTest, ZDD_Choose_ComplementEdge) {
    bddp a = bddchange(bddsingle, 1);  // {{1}}
    // ~{{1}} = {{1}, ∅}
    bddp ca = bddnot(a);
    EXPECT_EQ(bddchoose(ca, 0), bddsingle);  // {∅}
    EXPECT_EQ(bddchoose(ca, 1), a);          // {{1}}

    // {{1}, ∅}: complement removes ∅ → {{1}}
    bddp u = bddunion(a, bddsingle);
    bddp cu = bddnot(u);
    EXPECT_EQ(bddchoose(cu, 0), bddempty);  // no ∅
    EXPECT_EQ(bddchoose(cu, 1), a);         // {{1}}
}

TEST_F(BDDTest, ZDD_Choose_FilterCorrectness) {
    // {{1}, {1,2}, {1,2,3}}
    bddp s1 = bddchange(bddsingle, 1);
    bddp s12 = bddchange(bddchange(bddsingle, 1), 2);
    bddp s123 = bddchange(bddchange(bddchange(bddsingle, 1), 2), 3);
    bddp f = bddunion(bddunion(s1, s12), s123);

    ZDD r1 = ZDD_ID(bddchoose(f, 1));
    ZDD r2 = ZDD_ID(bddchoose(f, 2));
    ZDD r3 = ZDD_ID(bddchoose(f, 3));

    EXPECT_EQ(r1.exact_count(), bigint::BigInt(1));  // {{1}}
    EXPECT_EQ(r2.exact_count(), bigint::BigInt(1));  // {{1,2}}
    EXPECT_EQ(r3.exact_count(), bigint::BigInt(1));  // {{1,2,3}}
    EXPECT_TRUE(r1.contains({1}));
    EXPECT_TRUE(r2.contains({1, 2}));
    EXPECT_TRUE(r3.contains({1, 2, 3}));
}

// --- bddprofile / ZDD::profile ---

TEST_F(BDDTest, ZDD_Profile_EmptyFamily) {
    auto p = bddprofile(bddempty);
    EXPECT_TRUE(p.empty());
}

TEST_F(BDDTest, ZDD_Profile_Null) {
    auto p = bddprofile(bddnull);
    EXPECT_TRUE(p.empty());
}

TEST_F(BDDTest, ZDD_Profile_SingleFamily) {
    // {∅} → profile = [1]
    auto p = bddprofile(bddsingle);
    ASSERT_EQ(p.size(), 1u);
    EXPECT_EQ(p[0], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Profile_SingletonSet) {
    // {{1}} → profile = [0, 1]
    bddp a = bddchange(bddsingle, 1);
    auto p = bddprofile(a);
    ASSERT_EQ(p.size(), 2u);
    EXPECT_EQ(p[0], bigint::BigInt(0));
    EXPECT_EQ(p[1], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Profile_MixedSizes) {
    // {{1}, {1,2}, {1,2,3}} → profile = [0, 1, 1, 1]
    bddp s1 = bddchange(bddsingle, 1);
    bddp s12 = bddchange(bddchange(bddsingle, 1), 2);
    bddp s123 = bddchange(bddchange(bddchange(bddsingle, 1), 2), 3);
    bddp f = bddunion(bddunion(s1, s12), s123);
    auto p = bddprofile(f);
    ASSERT_EQ(p.size(), 4u);
    EXPECT_EQ(p[0], bigint::BigInt(0));
    EXPECT_EQ(p[1], bigint::BigInt(1));
    EXPECT_EQ(p[2], bigint::BigInt(1));
    EXPECT_EQ(p[3], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Profile_WithEmpty) {
    // {{}, {1}, {1,2}} → profile = [1, 1, 1]
    bddp s1 = bddchange(bddsingle, 1);
    bddp s12 = bddchange(bddchange(bddsingle, 1), 2);
    bddp f = bddunion(bddunion(bddsingle, s1), s12);
    auto p = bddprofile(f);
    ASSERT_EQ(p.size(), 3u);
    EXPECT_EQ(p[0], bigint::BigInt(1));
    EXPECT_EQ(p[1], bigint::BigInt(1));
    EXPECT_EQ(p[2], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Profile_PowerSet) {
    // power_set(3) → [1, 3, 3, 1] (binomial coefficients)
    ZDD ps = ZDD::power_set(3);
    auto p = bddprofile(ps.GetID());
    ASSERT_EQ(p.size(), 4u);
    EXPECT_EQ(p[0], bigint::BigInt(1));
    EXPECT_EQ(p[1], bigint::BigInt(3));
    EXPECT_EQ(p[2], bigint::BigInt(3));
    EXPECT_EQ(p[3], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Profile_SumsToCount) {
    ZDD ps = ZDD::power_set(3);
    auto p = ps.profile();
    bigint::BigInt total(0);
    for (auto& v : p) total += v;
    EXPECT_EQ(total, ps.exact_count());
}

TEST_F(BDDTest, ZDD_Profile_ChooseConsistency) {
    ZDD ps = ZDD::power_set(3);
    auto p = ps.profile();
    for (size_t k = 0; k < p.size(); k++) {
        EXPECT_EQ(p[k], ps.choose(static_cast<int>(k)).exact_count());
    }
}

TEST_F(BDDTest, ZDD_Profile_ComplementEdge) {
    bddp a = bddchange(bddsingle, 1);  // {{1}}, profile = [0, 1]
    // ~{{1}} = {{1}, ∅}, profile = [1, 1]
    auto p = bddprofile(bddnot(a));
    ASSERT_EQ(p.size(), 2u);
    EXPECT_EQ(p[0], bigint::BigInt(1));
    EXPECT_EQ(p[1], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_Profile_Double) {
    ZDD ps = ZDD::power_set(3);
    auto pd = ps.profile_double();
    ASSERT_EQ(pd.size(), 4u);
    EXPECT_DOUBLE_EQ(pd[0], 1.0);
    EXPECT_DOUBLE_EQ(pd[1], 3.0);
    EXPECT_DOUBLE_EQ(pd[2], 3.0);
    EXPECT_DOUBLE_EQ(pd[3], 1.0);
}

// --- bddelmfreq / ZDD::element_frequency ---

TEST_F(BDDTest, ZDD_ElementFrequency_EmptyFamily) {
    auto freq = bddelmfreq(bddempty);
    EXPECT_TRUE(freq.empty());
}

TEST_F(BDDTest, ZDD_ElementFrequency_Null) {
    auto freq = bddelmfreq(bddnull);
    EXPECT_TRUE(freq.empty());
}

TEST_F(BDDTest, ZDD_ElementFrequency_UnitFamily) {
    // {∅} → no variables → empty
    auto freq = bddelmfreq(bddsingle);
    EXPECT_TRUE(freq.empty());
}

TEST_F(BDDTest, ZDD_ElementFrequency_SingletonSet) {
    // {{1}} → freq[1] = 1
    bddp a = bddchange(bddsingle, 1);
    auto freq = bddelmfreq(a);
    ASSERT_GE(freq.size(), 2u);
    EXPECT_EQ(freq[0], bigint::BigInt(0));
    EXPECT_EQ(freq[1], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_ElementFrequency_TwoSets) {
    // {{1}, {1,2}} → freq[1] = 2, freq[2] = 1
    bddp s1 = bddchange(bddsingle, 1);
    bddp s12 = bddchange(bddchange(bddsingle, 1), 2);
    bddp f = bddunion(s1, s12);
    auto freq = bddelmfreq(f);
    ASSERT_GE(freq.size(), 3u);
    EXPECT_EQ(freq[1], bigint::BigInt(2));
    EXPECT_EQ(freq[2], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_ElementFrequency_PowerSet) {
    // power_set(3) = all subsets of {1,2,3}, 8 sets total
    // Each variable appears in exactly half (4) of the sets
    ZDD ps = ZDD::power_set(3);
    auto freq = ps.element_frequency();
    ASSERT_GE(freq.size(), 4u);
    EXPECT_EQ(freq[1], bigint::BigInt(4));
    EXPECT_EQ(freq[2], bigint::BigInt(4));
    EXPECT_EQ(freq[3], bigint::BigInt(4));
}

TEST_F(BDDTest, ZDD_ElementFrequency_WithEmpty) {
    // {{}, {1}, {1,2}} → freq[1] = 2, freq[2] = 1
    // Empty set doesn't affect frequency
    bddp s1 = bddchange(bddsingle, 1);
    bddp s12 = bddchange(bddchange(bddsingle, 1), 2);
    bddp f = bddunion(bddunion(bddsingle, s1), s12);
    auto freq = bddelmfreq(f);
    ASSERT_GE(freq.size(), 3u);
    EXPECT_EQ(freq[1], bigint::BigInt(2));
    EXPECT_EQ(freq[2], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_ElementFrequency_ComplementEdge) {
    // {{1}} complemented → {{1}, ∅}
    // freq[1] should still be 1 (∅ adds no variable frequency)
    bddp a = bddchange(bddsingle, 1);
    auto freq = bddelmfreq(bddnot(a));
    ASSERT_GE(freq.size(), 2u);
    EXPECT_EQ(freq[1], bigint::BigInt(1));
}

TEST_F(BDDTest, ZDD_ElementFrequency_SumsToLit) {
    // Sum of all frequencies == total literal count (Lit)
    ZDD ps = ZDD::power_set(3);
    auto freq = ps.element_frequency();
    bigint::BigInt total(0);
    for (auto& v : freq) total += v;
    // Lit() returns total number of literals across all sets
    EXPECT_EQ(total, bigint::BigInt(ps.Lit()));
}

TEST_F(BDDTest, ZDD_ElementFrequency_ConsistencyWithEnumerate) {
    // Verify against brute-force enumeration
    // {{1}, {2}, {1,2,3}}
    bddp s1 = bddchange(bddsingle, 1);
    bddp s2 = bddchange(bddsingle, 2);
    bddp s123 = bddchange(bddchange(bddchange(bddsingle, 1), 2), 3);
    bddp f = bddunion(bddunion(s1, s2), s123);
    auto freq = bddelmfreq(f);
    // freq[1] = 2 (in {1} and {1,2,3})
    // freq[2] = 2 (in {2} and {1,2,3})
    // freq[3] = 1 (in {1,2,3})
    ASSERT_GE(freq.size(), 4u);
    EXPECT_EQ(freq[1], bigint::BigInt(2));
    EXPECT_EQ(freq[2], bigint::BigInt(2));
    EXPECT_EQ(freq[3], bigint::BigInt(1));
}

// --- bddproduct / ZDD::product ---

TEST_F(BDDTest, ZDD_Product_EmptyLeft) {
    ZDD e(0);
    ZDD s = ZDD::singleton(1);
    EXPECT_TRUE(e.product(s).is_zero());
}

TEST_F(BDDTest, ZDD_Product_EmptyRight) {
    ZDD s = ZDD::singleton(1);
    ZDD e(0);
    EXPECT_TRUE(s.product(e).is_zero());
}

TEST_F(BDDTest, ZDD_Product_UnitLeft) {
    // {∅} × G = G
    ZDD u(1);
    ZDD g = ZDD::singleton(2);
    ZDD result = u.product(g);
    EXPECT_EQ(result.GetID(), g.GetID());
}

TEST_F(BDDTest, ZDD_Product_UnitRight) {
    // F × {∅} = F
    ZDD f = ZDD::singleton(1);
    ZDD u(1);
    ZDD result = f.product(u);
    EXPECT_EQ(result.GetID(), f.GetID());
}

TEST_F(BDDTest, ZDD_Product_SingletonPair) {
    // {{1}} × {{2}} = {{1,2}}
    ZDD f = ZDD::singleton(1);
    ZDD g = ZDD::singleton(2);
    ZDD result = f.product(g);
    auto sets = result.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    ASSERT_EQ(sets[0].size(), 2u);
    std::vector<bddvar> expected = {1, 2};
    EXPECT_EQ(sets[0], expected);
}

TEST_F(BDDTest, ZDD_Product_CardinalityIsMultiplicative) {
    // |F × G| = |F| * |G| for disjoint variable sets
    // F = power_set({1,2}) = 4 sets, G = power_set({3,4}) = 4 sets
    ZDD f = ZDD::power_set({1, 2});
    ZDD g = ZDD::power_set({3, 4});
    ZDD result = f.product(g);
    EXPECT_EQ(result.exact_count(), f.exact_count() * g.exact_count());
}

TEST_F(BDDTest, ZDD_Product_ConsistencyWithJoin) {
    // For disjoint variable sets, product should equal join
    ZDD f = ZDD::power_set({1, 2});
    ZDD g = ZDD::power_set({3, 4});
    ZDD prod = f.product(g);
    ZDD join = f * g;
    EXPECT_EQ(prod.GetID(), join.GetID());
}

TEST_F(BDDTest, ZDD_Product_WithEmptySet) {
    // F = {{}, {1}}, G = {{3}} → {{3}, {1,3}}
    ZDD f = ZDD(1) + ZDD::singleton(1);
    ZDD g = ZDD::singleton(3);
    ZDD result = f.product(g);
    auto sets = result.enumerate();
    ASSERT_EQ(sets.size(), 2u);
}

TEST_F(BDDTest, ZDD_Product_LargerExample) {
    // F = {{1}, {2}}, G = {{3}, {4}}
    // Result = {{1,3}, {1,4}, {2,3}, {2,4}}
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    ZDD g = ZDD::singleton(3) + ZDD::singleton(4);
    ZDD result = f.product(g);
    EXPECT_EQ(result.exact_count(), bigint::BigInt(4));
    auto sets = result.enumerate();
    ASSERT_EQ(sets.size(), 4u);
    // Each set should have exactly 2 elements
    for (auto& s : sets) {
        EXPECT_EQ(s.size(), 2u);
    }
}

TEST_F(BDDTest, ZDD_Product_Commutative) {
    ZDD f = ZDD::power_set({1, 2});
    ZDD g = ZDD::power_set({3, 4});
    EXPECT_EQ(f.product(g).GetID(), g.product(f).GetID());
}

TEST_F(BDDTest, ZDD_Product_FreeFunction) {
    bddp f = ZDD::power_set({1, 2}).GetID();
    bddp g = ZDD::power_set({3, 4}).GetID();
    bddp result = bddproduct(f, g);
    bddp join = bddjoin(f, g);
    EXPECT_EQ(result, join);
}

TEST_F(BDDTest, ZDD_Product_ComplementEdge) {
    // F = power_set({1,2}) uses complement edges internally.
    // G = {{3}} is a simple singleton.
    // Result should equal join for disjoint variables.
    ZDD f = ZDD::power_set({1, 2});
    ZDD g = ZDD::singleton(3);
    ZDD prod = f.product(g);
    ZDD join = f * g;
    EXPECT_EQ(prod.GetID(), join.GetID());
    // Verify sets: {{3}, {1,3}, {2,3}, {1,2,3}}
    EXPECT_EQ(prod.exact_count(), bigint::BigInt(4));
    auto sets = prod.enumerate();
    for (auto& s : sets) {
        bool has3 = false;
        for (bddvar v : s) {
            if (v == 3) has3 = true;
        }
        EXPECT_TRUE(has3);
    }
}

TEST_F(BDDTest, ZDD_Product_NonDisjointThrows) {
    // product() requires disjoint variable sets.
    // Overlapping variables should throw.
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    ZDD g = ZDD::singleton(2) + ZDD::singleton(3);
    EXPECT_THROW(f.product(g), std::invalid_argument);
}

// --- ZDD::average_size ---

TEST_F(BDDTest, ZDD_AverageSize_EmptyFamily) {
    ZDD e(0);
    EXPECT_DOUBLE_EQ(e.average_size(), 0.0);
}

TEST_F(BDDTest, ZDD_AverageSize_UnitFamily) {
    // {∅} → average size = 0
    ZDD u(1);
    EXPECT_DOUBLE_EQ(u.average_size(), 0.0);
}

TEST_F(BDDTest, ZDD_AverageSize_SingletonSet) {
    // {{1}} → average size = 1
    ZDD s = ZDD::singleton(1);
    EXPECT_DOUBLE_EQ(s.average_size(), 1.0);
}

TEST_F(BDDTest, ZDD_AverageSize_MixedSizes) {
    // {{1}, {1,2}, {1,2,3}} → sizes 1,2,3 → avg = 2.0
    ZDD s1 = ZDD::singleton(1);
    ZDD s12 = ZDD::single_set({1, 2});
    ZDD s123 = ZDD::single_set({1, 2, 3});
    ZDD f = s1 + s12 + s123;
    EXPECT_DOUBLE_EQ(f.average_size(), 2.0);
}

TEST_F(BDDTest, ZDD_AverageSize_PowerSet) {
    // power_set(3): 8 sets, total elements = 12, avg = 1.5
    ZDD ps = ZDD::power_set(3);
    EXPECT_DOUBLE_EQ(ps.average_size(), 1.5);
}

TEST_F(BDDTest, ZDD_AverageSize_WithEmpty) {
    // {{}, {1,2}} → sizes 0,2 → avg = 1.0
    ZDD e(1);
    ZDD s12 = ZDD::single_set({1, 2});
    ZDD f = e + s12;
    EXPECT_DOUBLE_EQ(f.average_size(), 1.0);
}

// --- ZDD::singleton ---

TEST_F(BDDTest, ZDD_Singleton) {
    ZDD s1 = ZDD::singleton(1);
    auto sets1 = s1.enumerate();
    ASSERT_EQ(sets1.size(), 1u);
    ASSERT_EQ(sets1[0].size(), 1u);
    EXPECT_EQ(sets1[0][0], 1u);

    ZDD s3 = ZDD::singleton(3);
    auto sets3 = s3.enumerate();
    ASSERT_EQ(sets3.size(), 1u);
    ASSERT_EQ(sets3[0].size(), 1u);
    EXPECT_EQ(sets3[0][0], 3u);

    // singleton should not contain the empty set
    EXPECT_FALSE(s1.has_empty());
}

// --- ZDD::single_set ---

TEST_F(BDDTest, ZDD_SingleSet_Empty) {
    // Empty vars → {{∅}} (unit family)
    ZDD f = ZDD::single_set({});
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_TRUE(sets[0].empty());
}

TEST_F(BDDTest, ZDD_SingleSet_OneVar) {
    // {1} → {{1}}, same as singleton
    ZDD f = ZDD::single_set({1});
    ZDD s = ZDD::singleton(1);
    EXPECT_EQ(f, s);
}

TEST_F(BDDTest, ZDD_SingleSet_MultipleVars) {
    // {1, 2, 3} → {{1, 2, 3}}
    ZDD f = ZDD::single_set({1, 2, 3});
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    std::vector<bddvar> expected = {1, 2, 3};
    EXPECT_EQ(sets[0], expected);
    EXPECT_FALSE(f.has_empty());
}

// --- ZDD::power_set ---

TEST_F(BDDTest, ZDD_PowerSet_Zero) {
    // power_set(0) → {{∅}}
    ZDD f = ZDD::power_set(0);
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_TRUE(sets[0].empty());
}

TEST_F(BDDTest, ZDD_PowerSet_One) {
    // power_set(1) → {{}, {1}}
    ZDD f = ZDD::power_set(1);
    auto sets = f.enumerate();
    EXPECT_EQ(sets.size(), 2u);
    EXPECT_TRUE(f.has_empty());
}

TEST_F(BDDTest, ZDD_PowerSet_Three) {
    // power_set(3) → 2^3 = 8 sets
    ZDD f = ZDD::power_set(3);
    EXPECT_EQ(f.count(), 8.0);
    EXPECT_TRUE(f.has_empty());
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 8u);
}

TEST_F(BDDTest, ZDD_PowerSet_Vec_Empty) {
    // power_set({}) → {{∅}}
    ZDD f = ZDD::power_set(std::vector<bddvar>{});
    EXPECT_EQ(f, ZDD::Single);
}

TEST_F(BDDTest, ZDD_PowerSet_Vec_SameAsN) {
    // power_set({1,2,3}) should equal power_set(3)
    ZDD a = ZDD::power_set(std::vector<bddvar>{1, 2, 3});
    ZDD b = ZDD::power_set(3);
    EXPECT_EQ(a, b);
}

TEST_F(BDDTest, ZDD_PowerSet_Vec_NonContiguous) {
    // power_set({2, 5}) → {{}, {2}, {5}, {2,5}} = 4 sets
    ZDD f = ZDD::power_set(std::vector<bddvar>{2, 5});
    EXPECT_EQ(f.count(), 4.0);
    EXPECT_TRUE(f.has_empty());
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 4u);
}

// --- ZDD::from_sets ---

TEST_F(BDDTest, ZDD_FromSets_Empty) {
    // No sets → empty family
    ZDD f = ZDD::from_sets({});
    EXPECT_EQ(f, ZDD::Empty);
}

TEST_F(BDDTest, ZDD_FromSets_SingleEmpty) {
    // {{}} → unit family
    ZDD f = ZDD::from_sets({{}});
    EXPECT_EQ(f, ZDD::Single);
}

TEST_F(BDDTest, ZDD_FromSets_Roundtrip) {
    // Build a ZDD, enumerate, reconstruct, compare
    ZDD orig = ZDD::power_set(3);
    auto sets = orig.enumerate();
    ZDD rebuilt = ZDD::from_sets(sets);
    EXPECT_EQ(rebuilt, orig);
}

TEST_F(BDDTest, ZDD_FromSets_Duplicates) {
    // Duplicate sets are merged
    ZDD f = ZDD::from_sets({{1, 2}, {1, 2}, {3}});
    EXPECT_EQ(f.count(), 2.0);  // {{1,2}, {3}}
}

// --- ZDD::combination ---

TEST_F(BDDTest, ZDD_Combination_K0) {
    // C(n, 0) = {{}} for any n
    ZDD f = ZDD::combination(5, 0);
    EXPECT_EQ(f, ZDD::Single);
}

TEST_F(BDDTest, ZDD_Combination_KGreaterThanN) {
    // C(3, 5) = empty
    ZDD f = ZDD::combination(3, 5);
    EXPECT_EQ(f, ZDD::Empty);
}

TEST_F(BDDTest, ZDD_Combination_KEqN) {
    // C(3, 3) = {{1,2,3}}
    ZDD f = ZDD::combination(3, 3);
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    ASSERT_EQ(sets[0].size(), 3u);
}

TEST_F(BDDTest, ZDD_Combination_3_2) {
    // C(3, 2) = {{1,2}, {1,3}, {2,3}} → 3 sets
    ZDD f = ZDD::combination(3, 2);
    EXPECT_EQ(f.count(), 3.0);
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 3u);
    for (const auto& s : sets) {
        EXPECT_EQ(s.size(), 2u);
    }
}

TEST_F(BDDTest, ZDD_Combination_5_3) {
    // C(5, 3) = 10 sets
    ZDD f = ZDD::combination(5, 3);
    EXPECT_EQ(f.count(), 10.0);
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 10u);
    for (const auto& s : sets) {
        EXPECT_EQ(s.size(), 3u);
    }
}

TEST_F(BDDTest, ZDD_Combination_K1) {
    // C(4, 1) = {{1}, {2}, {3}, {4}} → 4 singletons
    ZDD f = ZDD::combination(4, 1);
    EXPECT_EQ(f.count(), 4.0);
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 4u);
    for (const auto& s : sets) {
        EXPECT_EQ(s.size(), 1u);
    }
}

// --- ZDD::random_family ---

TEST_F(BDDTest, ZDD_RandomFamily_Zero) {
    // n=0: result is either {} or {{∅}}
    std::mt19937_64 rng(42);
    bool saw_empty = false, saw_single = false;
    for (int i = 0; i < 100; ++i) {
        ZDD f = ZDD::random_family(0, rng);
        if (f == ZDD::Empty) saw_empty = true;
        if (f == ZDD::Single) saw_single = true;
    }
    EXPECT_TRUE(saw_empty);
    EXPECT_TRUE(saw_single);
}

TEST_F(BDDTest, ZDD_RandomFamily_One) {
    // n=1: 2^(2^1) = 4 possible families: {}, {{1}}, {{∅}}, {{∅},{1}}
    std::mt19937_64 rng(123);
    std::set<bddp> seen;
    for (int i = 0; i < 200; ++i) {
        ZDD f = ZDD::random_family(1, rng);
        seen.insert(f.GetID());
    }
    EXPECT_EQ(seen.size(), 4u);
}

TEST_F(BDDTest, ZDD_RandomFamily_ValidSets) {
    // All sets in the family should only contain variables in {1,...,n}
    std::mt19937_64 rng(999);
    bddvar n = 3;
    for (int i = 0; i < 50; ++i) {
        ZDD f = ZDD::random_family(n, rng);
        auto sets = f.enumerate();
        for (const auto& s : sets) {
            for (bddvar v : s) {
                EXPECT_GE(v, 1u);
                EXPECT_LE(v, n);
            }
        }
    }
}

TEST_F(BDDTest, ZDD_RandomFamily_CardinalityRange) {
    // Cardinality should be between 0 and 2^n
    std::mt19937_64 rng(777);
    bddvar n = 3;
    for (int i = 0; i < 50; ++i) {
        ZDD f = ZDD::random_family(n, rng);
        double card = f.count();
        EXPECT_GE(card, 0.0);
        EXPECT_LE(card, 8.0);  // 2^3 = 8
    }
}

// --- ZDD::enumerate ---

TEST_F(BDDTest, ZDD_Enumerate_EmptyFamily) {
    ZDD f(0);  // empty family
    auto sets = f.enumerate();
    EXPECT_TRUE(sets.empty());
}

TEST_F(BDDTest, ZDD_Enumerate_SingleEmptySet) {
    ZDD f(1);  // { {} }
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_TRUE(sets[0].empty());
}

TEST_F(BDDTest, ZDD_Enumerate_SingleVariable) {
    bddvar v = bddnewvar();
    ZDD f = ZDD_ID(bddchange(bddsingle, v));  // { {v} }
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    ASSERT_EQ(sets[0].size(), 1u);
    EXPECT_EQ(sets[0][0], v);
}

TEST_F(BDDTest, ZDD_Enumerate_TwoSingletons) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD_ID(bddchange(bddsingle, v1));  // { {v1} }
    ZDD b = ZDD_ID(bddchange(bddsingle, v2));  // { {v2} }
    ZDD f = a + b;  // { {v1}, {v2} }
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 2u);
    // Sort for deterministic comparison
    std::sort(sets.begin(), sets.end());
    EXPECT_EQ(sets[0], std::vector<bddvar>({v1}));
    EXPECT_EQ(sets[1], std::vector<bddvar>({v2}));
}

TEST_F(BDDTest, ZDD_Enumerate_PowerSet) {
    // Power set of {v1, v2} = { {}, {v1}, {v2}, {v1,v2} }
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD s1 = ZDD_ID(bddchange(bddsingle, v1));
    ZDD s2 = ZDD_ID(bddchange(bddsingle, v2));
    ZDD s12 = s1 * s2;  // join: { {v1, v2} }
    ZDD f = ZDD(1) + s1 + s2 + s12;
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 4u);
    // Each inner vector is now sorted ascending; sort outer by lex order
    std::sort(sets.begin(), sets.end());
    EXPECT_EQ(sets[0], std::vector<bddvar>({}));
    EXPECT_EQ(sets[1], std::vector<bddvar>({v1}));
    EXPECT_EQ(sets[2], (std::vector<bddvar>{v1, v2}));
    EXPECT_EQ(sets[3], std::vector<bddvar>({v2}));
}

TEST_F(BDDTest, ZDD_Enumerate_ComplementEdge) {
    // ~ZDD::Empty = { {} }, ~ZDD::Single = empty
    ZDD f = ~ZDD(0);  // complement of empty = { {} }
    auto sets = f.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_TRUE(sets[0].empty());

    ZDD g = ~ZDD(1);  // complement of { {} } = empty
    auto sets2 = g.enumerate();
    EXPECT_TRUE(sets2.empty());
}

TEST_F(BDDTest, ZDD_Enumerate_ConsistentWithCard) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD s1 = ZDD_ID(bddchange(bddsingle, v1));
    ZDD s2 = ZDD_ID(bddchange(bddsingle, v2));
    ZDD s3 = ZDD_ID(bddchange(bddsingle, v3));
    ZDD f = s1 + s2 + s3 + (s1 * s2) + (s2 * s3);
    auto sets = f.enumerate();
    EXPECT_EQ(static_cast<uint64_t>(sets.size()), f.Card());
}

// --- ZDD::print_sets ---

TEST_F(BDDTest, ZDD_PrintSets_Null) {
    ZDD n(-1);
    std::ostringstream oss;
    n.print_sets(oss);
    EXPECT_EQ(oss.str(), "N");
}

TEST_F(BDDTest, ZDD_PrintSets_Empty) {
    ZDD e(0);
    std::ostringstream oss;
    e.print_sets(oss);
    EXPECT_EQ(oss.str(), "E");
}

TEST_F(BDDTest, ZDD_PrintSets_Single) {
    ZDD s(1);  // {∅}
    std::ostringstream oss;
    s.print_sets(oss);
    EXPECT_EQ(oss.str(), "{}");
}

TEST_F(BDDTest, ZDD_PrintSets_OneSingleton) {
    // {{1}}
    ZDD f = ZDD::singleton(1);
    std::ostringstream oss;
    f.print_sets(oss);
    EXPECT_EQ(oss.str(), "{1}");
}

TEST_F(BDDTest, ZDD_PrintSets_MultipleElements) {
    // {{3,2,1}} - single set with 3 elements
    ZDD f = ZDD::single_set({1, 2, 3});
    std::ostringstream oss;
    f.print_sets(oss);
    EXPECT_EQ(oss.str(), "{3,2,1}");
}

TEST_F(BDDTest, ZDD_PrintSets_PowerSet2) {
    // power_set(2) = {{},{1},{2},{2,1}} (lo-first order)
    ZDD f = ZDD::power_set(2);
    std::ostringstream oss;
    f.print_sets(oss);
    EXPECT_EQ(oss.str(), "{},{1},{2},{2,1}");
}

TEST_F(BDDTest, ZDD_PrintSets_CustomDelimiters) {
    ZDD f = ZDD::power_set(2);
    std::ostringstream oss;
    f.print_sets(oss, "};{", ",");
    EXPECT_EQ(oss.str(), "};{1};{2};{2,1");
}

TEST_F(BDDTest, ZDD_PrintSets_CustomDelimiters2) {
    ZDD f = ZDD::power_set(2);
    std::ostringstream oss;
    f.print_sets(oss, " | ", "-");
    EXPECT_EQ(oss.str(), " | 1 | 2 | 2-1");
}

TEST_F(BDDTest, ZDD_PrintSets_VarNameMap) {
    ZDD f = ZDD::single_set({1, 2, 3});
    std::vector<std::string> names = {"", "a", "b", "c"};
    std::ostringstream oss;
    f.print_sets(oss, ",", ",", names);
    EXPECT_EQ(oss.str(), "c,b,a");
}

TEST_F(BDDTest, ZDD_PrintSets_VarNameMap_Fallback) {
    ZDD f = ZDD::single_set({1, 2, 3});
    // var 3 has no mapping (out of range), falls back to number
    std::vector<std::string> names = {"", "x", "y"};
    std::ostringstream oss;
    f.print_sets(oss, ",", ",", names);
    EXPECT_EQ(oss.str(), "3,y,x");
}

TEST_F(BDDTest, ZDD_PrintSets_ComplementEdge) {
    // ~ZDD(0) = ~bddempty = {∅}
    ZDD f = ~ZDD(0);
    std::ostringstream oss;
    f.print_sets(oss);
    EXPECT_EQ(oss.str(), "{}");
}

TEST_F(BDDTest, ZDD_PrintSets_NullDelimiters) {
    ZDD n(-1);
    std::ostringstream oss;
    n.print_sets(oss, ";", ",");
    EXPECT_EQ(oss.str(), "N");
}

TEST_F(BDDTest, ZDD_PrintSets_EmptyDelimiters) {
    ZDD e(0);
    std::ostringstream oss;
    e.print_sets(oss, ";", ",");
    EXPECT_EQ(oss.str(), "E");
}

// --- ZDD::to_str ---

TEST_F(BDDTest, ZDD_ToStr_Null) {
    ZDD n(-1);
    EXPECT_EQ(n.to_str(), "N");
}

TEST_F(BDDTest, ZDD_ToStr_Empty) {
    ZDD e(0);
    EXPECT_EQ(e.to_str(), "E");
}

TEST_F(BDDTest, ZDD_ToStr_Single) {
    ZDD s(1);
    EXPECT_EQ(s.to_str(), "{}");
}

TEST_F(BDDTest, ZDD_ToStr_PowerSet2) {
    ZDD f = ZDD::power_set(2);
    EXPECT_EQ(f.to_str(), "{},{1},{2},{2,1}");
}

