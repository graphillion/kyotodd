#include <gtest/gtest.h>
#include "bdd.h"
#include "mvdd.h"
#include <sstream>
#include <string>
#include <random>
#include <set>

using namespace kyotodd;

class MVDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD::init(1024, 1024 * 1024);
    }
};


// ============================================================
//  MVZDD operator^  (symmetric difference)
// ============================================================

TEST_F(MVDDTest, MVZDDSymDiffBasic) {
    // k=3, 2 variables
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    // A = { (0,0), (1,0) }
    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    MVZDD A = a00 + a10;

    // B = { (0,0), (0,2) }
    auto b02 = MVZDD::singleton(base, v2, 2);
    MVZDD B = a00 + b02;

    // A ^ B = { (1,0), (0,2) }
    MVZDD result = A ^ B;
    EXPECT_EQ(result.count(), 2.0);
    EXPECT_TRUE(result.evaluate({1, 0}));
    EXPECT_TRUE(result.evaluate({0, 2}));
    EXPECT_FALSE(result.evaluate({0, 0}));  // in both, removed
}

TEST_F(MVDDTest, MVZDDSymDiffSelf) {
    MVZDD base(2);
    base.new_var();
    auto s = MVZDD::singleton(base, 1, 1);
    MVZDD result = s ^ s;
    EXPECT_TRUE(result.is_zero());
}

TEST_F(MVDDTest, MVZDDSymDiffEmpty) {
    MVZDD base(2);
    base.new_var();
    auto s = MVZDD::singleton(base, 1, 1);
    auto empty = MVZDD::zero(base.var_table());
    EXPECT_EQ(s ^ empty, s);
    EXPECT_EQ(empty ^ s, s);
}

TEST_F(MVDDTest, MVZDDSymDiffInPlace) {
    MVZDD base(3);
    bddvar v1 = base.new_var();

    auto a = MVZDD::singleton(base, v1, 1);
    auto b = MVZDD::singleton(base, v1, 2);
    MVZDD c = a + b;

    c ^= a;
    // c was {(1),(2)}, a was {(1)} => c should be {(2)}
    EXPECT_EQ(c.count(), 1.0);
    EXPECT_TRUE(c.evaluate({2}));
    EXPECT_FALSE(c.evaluate({1}));
}

// ============================================================
//  MVZDD has_empty
// ============================================================

TEST_F(MVDDTest, MVZDDHasEmptyTrue) {
    // one() = { all-zero assignment } = "empty" in ZDD terms
    MVZDD base(3);
    base.new_var();
    auto one = MVZDD::one(base.var_table());
    EXPECT_TRUE(one.has_empty());
}

TEST_F(MVDDTest, MVZDDHasEmptyFalse) {
    MVZDD base(3);
    base.new_var();
    auto s = MVZDD::singleton(base, 1, 2);
    EXPECT_FALSE(s.has_empty());
}

TEST_F(MVDDTest, MVZDDHasEmptyUnion) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    auto s = MVZDD::singleton(base, v1, 1);
    auto one = MVZDD::one(base.var_table());
    MVZDD combined = s + one;
    EXPECT_TRUE(combined.has_empty());
}

TEST_F(MVDDTest, MVZDDHasEmptyZero) {
    MVZDD base(2);
    base.new_var();
    auto empty = MVZDD::zero(base.var_table());
    EXPECT_FALSE(empty.has_empty());
}

// ============================================================
//  MVZDD contains
// ============================================================

TEST_F(MVDDTest, MVZDDContainsBasic) {
    // k=3, 2 variables
    // Family = { (1,0), (0,2) }
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();
    auto s10 = MVZDD::singleton(base, v1, 1);
    auto s02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = s10 + s02;

    EXPECT_TRUE(F.contains({1, 0}));
    EXPECT_TRUE(F.contains({0, 2}));
    EXPECT_FALSE(F.contains({0, 0}));
    EXPECT_FALSE(F.contains({1, 2}));
    EXPECT_FALSE(F.contains({2, 0}));
    EXPECT_FALSE(F.contains({0, 1}));
}

TEST_F(MVDDTest, MVZDDContainsAllZero) {
    // Family = { (0,0) }
    MVZDD base(3);
    base.new_var();
    base.new_var();
    auto one = MVZDD::one(base.var_table());
    EXPECT_TRUE(one.contains({0, 0}));
    EXPECT_FALSE(one.contains({1, 0}));
}

TEST_F(MVDDTest, MVZDDContainsEmpty) {
    MVZDD base(2);
    base.new_var();
    auto empty = MVZDD::zero(base.var_table());
    EXPECT_FALSE(empty.contains({0}));
    EXPECT_FALSE(empty.contains({1}));
}

TEST_F(MVDDTest, MVZDDContainsSizeMismatch) {
    MVZDD base(2);
    base.new_var();
    base.new_var();
    auto one = MVZDD::one(base.var_table());
    EXPECT_THROW(one.contains({0}), std::invalid_argument);
    EXPECT_THROW(one.contains({0, 0, 0}), std::invalid_argument);
}

TEST_F(MVDDTest, MVZDDContainsValueOutOfRange) {
    MVZDD base(3);
    base.new_var();
    auto one = MVZDD::one(base.var_table());
    EXPECT_THROW(one.contains({3}), std::invalid_argument);
    EXPECT_THROW(one.contains({-1}), std::invalid_argument);
}

// ============================================================
//  MVZDD support_vars
// ============================================================

TEST_F(MVDDTest, MVZDDSupportVarsBasic) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();
    bddvar v3 = base.new_var();

    // Family only uses v1 and v3
    auto s1 = MVZDD::singleton(base, v1, 1);
    auto s3 = MVZDD::singleton(base, v3, 2);
    MVZDD F = s1 + s3;

    std::vector<bddvar> vars = F.support_vars();
    EXPECT_EQ(vars.size(), 2u);
    EXPECT_EQ(vars[0], v1);
    EXPECT_EQ(vars[1], v3);
    (void)v2;
}

TEST_F(MVDDTest, MVZDDSupportVarsEmpty) {
    MVZDD base(2);
    base.new_var();
    auto empty = MVZDD::zero(base.var_table());
    std::vector<bddvar> vars = empty.support_vars();
    EXPECT_TRUE(vars.empty());
}

TEST_F(MVDDTest, MVZDDSupportVarsOne) {
    // one() = { all-zero }, represented as terminal — no DD variables
    MVZDD base(2);
    base.new_var();
    auto one = MVZDD::one(base.var_table());
    std::vector<bddvar> vars = one.support_vars();
    EXPECT_TRUE(vars.empty());
}

// ============================================================
//  MVZDD exact_count with memo
// ============================================================

TEST_F(MVDDTest, MVZDDExactCountWithMemo) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Build a family with known size
    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, 1, 1);
    auto a02 = MVZDD::singleton(base, 2, 2);
    MVZDD F = a00 + a10 + a02;

    ZddCountMemo memo(F.to_zdd());
    bigint::BigInt count1 = F.exact_count(memo);
    EXPECT_EQ(count1.to_string(), "3");

    // Calling again with same memo should give same result
    bigint::BigInt count2 = F.exact_count(memo);
    EXPECT_EQ(count2.to_string(), "3");
}

TEST_F(MVDDTest, MVZDDExactCountMemoConsistency) {
    // exact_count() and exact_count(memo) should agree
    MVZDD base(4);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto s11 = MVZDD::singleton(base, v1, 1);
    auto s12 = MVZDD::singleton(base, v1, 2);
    auto s13 = MVZDD::singleton(base, v1, 3);
    auto s21 = MVZDD::singleton(base, v2, 1);
    MVZDD F = s11 + s12 + s13 + s21;

    ZddCountMemo memo(F.to_zdd());
    bigint::BigInt with_memo = F.exact_count(memo);
    bigint::BigInt without_memo = F.exact_count();
    EXPECT_EQ(with_memo.to_string(), without_memo.to_string());
}

// ============================================================
//  MVZDD uniform_sample
// ============================================================

TEST_F(MVDDTest, MVZDDUniformSampleSingleElement) {
    // Family with exactly one assignment: { (2, 0) }
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();
    auto s = MVZDD::singleton(base, v1, 2);

    std::mt19937_64 rng(42);
    ZddCountMemo memo(s.to_zdd());
    std::vector<int> sample = s.uniform_sample(rng, memo);

    EXPECT_EQ(sample.size(), 2u);
    EXPECT_EQ(sample[0], 2);  // v1 = 2
    EXPECT_EQ(sample[1], 0);  // v2 = 0
    (void)v2;
}

TEST_F(MVDDTest, MVZDDUniformSampleAllZero) {
    // Family = { (0,0) }
    MVZDD base(3);
    base.new_var();
    base.new_var();
    auto one = MVZDD::one(base.var_table());

    std::mt19937_64 rng(123);
    ZddCountMemo memo(one.to_zdd());
    std::vector<int> sample = one.uniform_sample(rng, memo);

    EXPECT_EQ(sample.size(), 2u);
    EXPECT_EQ(sample[0], 0);
    EXPECT_EQ(sample[1], 0);
}

TEST_F(MVDDTest, MVZDDUniformSampleCoversAll) {
    // Family with 3 assignments — sample many times, check all are hit
    MVZDD base(3);
    bddvar v1 = base.new_var();

    auto s0 = MVZDD::one(base.var_table());       // (0)
    auto s1 = MVZDD::singleton(base, v1, 1);      // (1)
    auto s2 = MVZDD::singleton(base, v1, 2);      // (2)
    MVZDD F = s0 + s1 + s2;

    std::mt19937_64 rng(0);
    ZddCountMemo memo(F.to_zdd());

    std::set<int> seen;
    for (int i = 0; i < 100; ++i) {
        std::vector<int> sample = F.uniform_sample(rng, memo);
        ASSERT_EQ(sample.size(), 1u);
        int val = sample[0];
        EXPECT_GE(val, 0);
        EXPECT_LE(val, 2);
        EXPECT_TRUE(F.contains(sample));
        seen.insert(val);
    }
    // With 100 samples from 3 elements, all should appear
    EXPECT_EQ(seen.size(), 3u);
}

TEST_F(MVDDTest, MVZDDUniformSampleEmptyThrows) {
    MVZDD base(2);
    base.new_var();
    auto empty = MVZDD::zero(base.var_table());

    std::mt19937_64 rng(0);
    ZddCountMemo memo(empty.to_zdd());
    EXPECT_THROW(empty.uniform_sample(rng, memo), std::exception);
}

// ============================================================
//  MVZDD min_weight / max_weight
// ============================================================

TEST_F(MVDDTest, MVZDDMinMaxWeightBasic) {
    // k=3, 2 variables
    // Family = { (0,0), (1,0), (0,2) }
    // weights[0] = {0, 10, 20}  (var1: val0=0, val1=10, val2=20)
    // weights[1] = {0, 5, 15}   (var2: val0=0, val1=5, val2=15)
    // (0,0) weight = 0+0 = 0
    // (1,0) weight = 10+0 = 10
    // (0,2) weight = 0+15 = 15
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a02;

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    EXPECT_EQ(F.min_weight(weights), 0);
    EXPECT_EQ(F.max_weight(weights), 15);
}

TEST_F(MVDDTest, MVZDDMinMaxWeightWithVal0Cost) {
    // val=0 also has non-zero weight
    // k=2, 2 variables
    // Family = { (0,0), (1,0), (0,1), (1,1) }
    // weights[0] = {3, 7}   (var1: val0=3, val1=7)
    // weights[1] = {2, 8}   (var2: val0=2, val1=8)
    // (0,0) = 3+2 = 5
    // (1,0) = 7+2 = 9
    // (0,1) = 3+8 = 11
    // (1,1) = 7+8 = 15
    MVZDD base(2);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a01 = MVZDD::singleton(base, v2, 1);
    // Build (1,1) via ITE
    auto a11 = MVZDD::ite(base, v1, {
        MVZDD::zero(base.var_table()),  // v1=0: empty
        MVZDD::singleton(base, v2, 1)   // v1=1: {(1,1)}
    });
    MVZDD F = a00 + a10 + a01 + a11;

    std::vector<std::vector<int>> weights = {{3, 7}, {2, 8}};

    EXPECT_EQ(F.min_weight(weights), 5);
    EXPECT_EQ(F.max_weight(weights), 15);
}

TEST_F(MVDDTest, MVZDDMinMaxWeightSingleAssignment) {
    // Family = { (2) }
    // weights[0] = {1, 5, 9}
    MVZDD base(3);
    bddvar v1 = base.new_var();
    auto s = MVZDD::singleton(base, v1, 2);

    std::vector<std::vector<int>> weights = {{1, 5, 9}};

    EXPECT_EQ(s.min_weight(weights), 9);
    EXPECT_EQ(s.max_weight(weights), 9);
}

TEST_F(MVDDTest, MVZDDMinMaxWeightNegativeWeights) {
    // Family = { (0), (1), (2) }
    // weights[0] = {-5, 3, -10}
    MVZDD base(3);
    bddvar v1 = base.new_var();

    auto s0 = MVZDD::one(base.var_table());
    auto s1 = MVZDD::singleton(base, v1, 1);
    auto s2 = MVZDD::singleton(base, v1, 2);
    MVZDD F = s0 + s1 + s2;

    std::vector<std::vector<int>> weights = {{-5, 3, -10}};

    EXPECT_EQ(F.min_weight(weights), -10);
    EXPECT_EQ(F.max_weight(weights), 3);
}

// ============================================================
//  MVZDD min_weight_set / max_weight_set
// ============================================================

TEST_F(MVDDTest, MVZDDMinWeightSetBasic) {
    // k=3, 2 variables
    // Family = { (0,0), (1,0), (0,2) }
    // weights: var1 = {0, 10, 20}, var2 = {0, 5, 15}
    // min = (0,0) with weight 0
    // max = (0,2) with weight 15
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a02;

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    std::vector<int> min_set = F.min_weight_set(weights);
    EXPECT_EQ(min_set, (std::vector<int>{0, 0}));

    std::vector<int> max_set = F.max_weight_set(weights);
    EXPECT_EQ(max_set, (std::vector<int>{0, 2}));
}

TEST_F(MVDDTest, MVZDDMinWeightSetWithVal0Cost) {
    // k=2, 2 variables
    // Family = { (0,0), (1,0), (0,1), (1,1) }
    // weights: var1 = {3, 7}, var2 = {2, 8}
    // min = (0,0)=5, max = (1,1)=15
    MVZDD base(2);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a01 = MVZDD::singleton(base, v2, 1);
    auto a11 = MVZDD::ite(base, v1, {
        MVZDD::zero(base.var_table()),
        MVZDD::singleton(base, v2, 1)
    });
    MVZDD F = a00 + a10 + a01 + a11;

    std::vector<std::vector<int>> weights = {{3, 7}, {2, 8}};

    std::vector<int> min_set = F.min_weight_set(weights);
    EXPECT_EQ(min_set, (std::vector<int>{0, 0}));

    std::vector<int> max_set = F.max_weight_set(weights);
    EXPECT_EQ(max_set, (std::vector<int>{1, 1}));
}

TEST_F(MVDDTest, MVZDDMinWeightSetResultInFamily) {
    // Verify that the returned set is actually in the family
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto s12 = MVZDD::ite(base, v1, {
        MVZDD::zero(base.var_table()),
        MVZDD::singleton(base, v2, 2),
        MVZDD::zero(base.var_table())
    });
    auto s20 = MVZDD::singleton(base, v1, 2);
    MVZDD F = s12 + s20;

    std::vector<std::vector<int>> weights = {{0, 1, 100}, {0, 50, 2}};

    std::vector<int> min_set = F.min_weight_set(weights);
    EXPECT_TRUE(F.contains(min_set));

    std::vector<int> max_set = F.max_weight_set(weights);
    EXPECT_TRUE(F.contains(max_set));
}

// ============================================================
//  MVZDD weight validation errors
// ============================================================

TEST_F(MVDDTest, MVZDDWeightSizeMismatch) {
    MVZDD base(3);
    base.new_var();
    base.new_var();
    auto one = MVZDD::one(base.var_table());

    // Wrong outer size
    EXPECT_THROW(one.min_weight({{0, 1, 2}}), std::invalid_argument);
    // Wrong inner size
    EXPECT_THROW(one.min_weight({{0, 1}, {0, 1}}), std::invalid_argument);
}

TEST_F(MVDDTest, MVZDDWeightSetSizeMismatch) {
    MVZDD base(2);
    base.new_var();
    auto one = MVZDD::one(base.var_table());

    // Wrong outer size (0 instead of 1)
    EXPECT_THROW(one.min_weight_set({}), std::invalid_argument);
    EXPECT_THROW(one.max_weight_set({}), std::invalid_argument);
}

// ============================================================
//  MVZDD cost_bound_le
// ============================================================

TEST_F(MVDDTest, MVZDDCostBoundLeBasic) {
    // k=3, 2 variables
    // Family = { (0,0), (1,0), (2,0), (0,1), (0,2) }
    // weights: var1 = {0, 10, 20}, var2 = {0, 5, 15}
    // (0,0)=0, (1,0)=10, (2,0)=20, (0,1)=5, (0,2)=15
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a20 = MVZDD::singleton(base, v1, 2);
    auto a01 = MVZDD::singleton(base, v2, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a20 + a01 + a02;

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    // cost <= 10: {(0,0)=0, (1,0)=10, (0,1)=5}
    MVZDD le10 = F.cost_bound_le(weights, 10);
    EXPECT_EQ(le10.count(), 3.0);
    EXPECT_TRUE(le10.contains({0, 0}));
    EXPECT_TRUE(le10.contains({1, 0}));
    EXPECT_TRUE(le10.contains({0, 1}));
    EXPECT_FALSE(le10.contains({2, 0}));
    EXPECT_FALSE(le10.contains({0, 2}));

    // cost <= 0: only (0,0)
    MVZDD le0 = F.cost_bound_le(weights, 0);
    EXPECT_EQ(le0.count(), 1.0);
    EXPECT_TRUE(le0.contains({0, 0}));

    // cost <= 100: all
    MVZDD le100 = F.cost_bound_le(weights, 100);
    EXPECT_EQ(le100, F);
}

TEST_F(MVDDTest, MVZDDCostBoundLeWithVal0Cost) {
    // val=0 has non-zero weight
    // k=2, 1 variable
    // Family = { (0), (1) }
    // weights: var1 = {3, 7}
    // (0)=3, (1)=7
    MVZDD base(2);
    bddvar v1 = base.new_var();

    auto a0 = MVZDD::one(base.var_table());
    auto a1 = MVZDD::singleton(base, v1, 1);
    MVZDD F = a0 + a1;

    std::vector<std::vector<int>> weights = {{3, 7}};

    // cost <= 5: only (0)=3
    MVZDD le5 = F.cost_bound_le(weights, 5);
    EXPECT_EQ(le5.count(), 1.0);
    EXPECT_TRUE(le5.contains({0}));

    // cost <= 7: both
    MVZDD le7 = F.cost_bound_le(weights, 7);
    EXPECT_EQ(le7.count(), 2.0);

    // cost <= 2: empty
    MVZDD le2 = F.cost_bound_le(weights, 2);
    EXPECT_TRUE(le2.is_zero());
}

TEST_F(MVDDTest, MVZDDCostBoundLeEmpty) {
    MVZDD base(2);
    base.new_var();
    auto empty = MVZDD::zero(base.var_table());
    std::vector<std::vector<int>> weights = {{0, 1}};
    MVZDD result = empty.cost_bound_le(weights, 100);
    EXPECT_TRUE(result.is_zero());
}

// ============================================================
//  MVZDD cost_bound_ge
// ============================================================

TEST_F(MVDDTest, MVZDDCostBoundGeBasic) {
    // Same family as above
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a20 = MVZDD::singleton(base, v1, 2);
    auto a01 = MVZDD::singleton(base, v2, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a20 + a01 + a02;

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    // cost >= 15: {(2,0)=20, (0,2)=15}
    MVZDD ge15 = F.cost_bound_ge(weights, 15);
    EXPECT_EQ(ge15.count(), 2.0);
    EXPECT_TRUE(ge15.contains({2, 0}));
    EXPECT_TRUE(ge15.contains({0, 2}));

    // cost >= 0: all
    MVZDD ge0 = F.cost_bound_ge(weights, 0);
    EXPECT_EQ(ge0, F);

    // cost >= 100: empty
    MVZDD ge100 = F.cost_bound_ge(weights, 100);
    EXPECT_TRUE(ge100.is_zero());
}

TEST_F(MVDDTest, MVZDDCostBoundGeWithVal0Cost) {
    // k=2, 1 variable, val=0 has weight 3
    MVZDD base(2);
    bddvar v1 = base.new_var();

    auto a0 = MVZDD::one(base.var_table());
    auto a1 = MVZDD::singleton(base, v1, 1);
    MVZDD F = a0 + a1;

    std::vector<std::vector<int>> weights = {{3, 7}};

    // cost >= 5: only (1)=7
    MVZDD ge5 = F.cost_bound_ge(weights, 5);
    EXPECT_EQ(ge5.count(), 1.0);
    EXPECT_TRUE(ge5.contains({1}));

    // cost >= 3: both
    MVZDD ge3 = F.cost_bound_ge(weights, 3);
    EXPECT_EQ(ge3.count(), 2.0);
}

// ============================================================
//  MVZDD cost_bound_eq
// ============================================================

TEST_F(MVDDTest, MVZDDCostBoundEqBasic) {
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a20 = MVZDD::singleton(base, v1, 2);
    auto a01 = MVZDD::singleton(base, v2, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a20 + a01 + a02;

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    // cost == 10: {(1,0)=10}
    MVZDD eq10 = F.cost_bound_eq(weights, 10);
    EXPECT_EQ(eq10.count(), 1.0);
    EXPECT_TRUE(eq10.contains({1, 0}));

    // cost == 0: {(0,0)=0}
    MVZDD eq0 = F.cost_bound_eq(weights, 0);
    EXPECT_EQ(eq0.count(), 1.0);
    EXPECT_TRUE(eq0.contains({0, 0}));

    // cost == 7: empty (no assignment with cost 7)
    MVZDD eq7 = F.cost_bound_eq(weights, 7);
    EXPECT_TRUE(eq7.is_zero());
}

TEST_F(MVDDTest, MVZDDCostBoundEqWithVal0Cost) {
    // k=2, 1 variable, weights = {3, 7}
    MVZDD base(2);
    bddvar v1 = base.new_var();

    auto a0 = MVZDD::one(base.var_table());
    auto a1 = MVZDD::singleton(base, v1, 1);
    MVZDD F = a0 + a1;

    std::vector<std::vector<int>> weights = {{3, 7}};

    MVZDD eq3 = F.cost_bound_eq(weights, 3);
    EXPECT_EQ(eq3.count(), 1.0);
    EXPECT_TRUE(eq3.contains({0}));

    MVZDD eq7 = F.cost_bound_eq(weights, 7);
    EXPECT_EQ(eq7.count(), 1.0);
    EXPECT_TRUE(eq7.contains({1}));
}

// ============================================================
//  MVZDD cost_bound with memo
// ============================================================

TEST_F(MVDDTest, MVZDDCostBoundMemoReuse) {
    // Using a memo across multiple le calls should give correct results
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a20 = MVZDD::singleton(base, v1, 2);
    auto a01 = MVZDD::singleton(base, v2, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a20 + a01 + a02;

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    CostBoundMemo memo;

    MVZDD le5 = F.cost_bound_le(weights, 5, memo);
    EXPECT_EQ(le5.count(), 2.0);  // (0,0)=0, (0,1)=5

    MVZDD le10 = F.cost_bound_le(weights, 10, memo);
    EXPECT_EQ(le10.count(), 3.0);  // +  (1,0)=10

    MVZDD le20 = F.cost_bound_le(weights, 20, memo);
    EXPECT_EQ(le20.count(), 5.0);  // all

    // eq using the same memo
    MVZDD eq15 = F.cost_bound_eq(weights, 15, memo);
    EXPECT_EQ(eq15.count(), 1.0);
    EXPECT_TRUE(eq15.contains({0, 2}));
}

TEST_F(MVDDTest, MVZDDCostBoundConsistency) {
    // Verify: le(b) + ge(b+1) == F  for any b
    MVZDD base(3);
    bddvar v1 = base.new_var();
    bddvar v2 = base.new_var();

    auto a00 = MVZDD::one(base.var_table());
    auto a10 = MVZDD::singleton(base, v1, 1);
    auto a02 = MVZDD::singleton(base, v2, 2);
    MVZDD F = a00 + a10 + a02;  // costs: 0, 10, 15

    std::vector<std::vector<int>> weights = {{0, 10, 20}, {0, 5, 15}};

    for (long long b : {-1LL, 0LL, 5LL, 10LL, 15LL, 20LL}) {
        MVZDD le = F.cost_bound_le(weights, b);
        MVZDD ge = F.cost_bound_ge(weights, b + 1);
        EXPECT_EQ(le + ge, F) << "Failed for b=" << b;
    }
}

TEST_F(MVDDTest, MVZDDCostBoundNegativeWeights) {
    // Weights can be negative
    // k=3, 1 variable, weights = {-5, 3, -10}
    MVZDD base(3);
    bddvar v1 = base.new_var();

    auto s0 = MVZDD::one(base.var_table());
    auto s1 = MVZDD::singleton(base, v1, 1);
    auto s2 = MVZDD::singleton(base, v1, 2);
    MVZDD F = s0 + s1 + s2;

    std::vector<std::vector<int>> weights = {{-5, 3, -10}};

    // costs: (0)=-5, (1)=3, (2)=-10
    // le(-5): {(0)=-5, (2)=-10}
    MVZDD le_m5 = F.cost_bound_le(weights, -5);
    EXPECT_EQ(le_m5.count(), 2.0);
    EXPECT_TRUE(le_m5.contains({0}));
    EXPECT_TRUE(le_m5.contains({2}));

    // ge(0): {(1)=3}
    MVZDD ge0 = F.cost_bound_ge(weights, 0);
    EXPECT_EQ(ge0.count(), 1.0);
    EXPECT_TRUE(ge0.contains({1}));

    // eq(-10): {(2)}
    MVZDD eq_m10 = F.cost_bound_eq(weights, -10);
    EXPECT_EQ(eq_m10.count(), 1.0);
    EXPECT_TRUE(eq_m10.contains({2}));
}

// ============================================================
//  MVZDD::from_sets tests
// ============================================================

TEST_F(MVDDTest, MVZDDFromSetsEmpty) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Empty input → empty family
    MVZDD f = MVZDD::from_sets(base, {});
    EXPECT_TRUE(f.is_zero());
    EXPECT_EQ(f.count(), 0.0);
}

TEST_F(MVDDTest, MVZDDFromSetsSingleAllZero) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Single assignment: all zeros → family {(0,0)}
    MVZDD f = MVZDD::from_sets(base, {{0, 0}});
    EXPECT_EQ(f.count(), 1.0);
    EXPECT_TRUE(f.contains({0, 0}));
    EXPECT_TRUE(f.has_empty());
}

TEST_F(MVDDTest, MVZDDFromSetsSingleNonZero) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Single assignment: (1, 2)
    MVZDD f = MVZDD::from_sets(base, {{1, 2}});
    EXPECT_EQ(f.count(), 1.0);
    EXPECT_TRUE(f.contains({1, 2}));
    EXPECT_FALSE(f.contains({0, 0}));
}

TEST_F(MVDDTest, MVZDDFromSetsMultiple) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    std::vector<std::vector<int>> sets = {{0, 0}, {1, 2}, {2, 1}};
    MVZDD f = MVZDD::from_sets(base, sets);
    EXPECT_EQ(f.count(), 3.0);
    EXPECT_TRUE(f.contains({0, 0}));
    EXPECT_TRUE(f.contains({1, 2}));
    EXPECT_TRUE(f.contains({2, 1}));
    EXPECT_FALSE(f.contains({0, 1}));
}

TEST_F(MVDDTest, MVZDDFromSetsDuplicates) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Duplicate assignments should be unified
    std::vector<std::vector<int>> sets = {{1, 0}, {1, 0}, {0, 2}};
    MVZDD f = MVZDD::from_sets(base, sets);
    EXPECT_EQ(f.count(), 2.0);
    EXPECT_TRUE(f.contains({1, 0}));
    EXPECT_TRUE(f.contains({0, 2}));
}

TEST_F(MVDDTest, MVZDDFromSetsRoundTrip) {
    // from_sets(enumerate()) should produce the same MVZDD
    MVZDD base(4);
    base.new_var();
    base.new_var();
    base.new_var();

    std::vector<std::vector<int>> sets = {
        {0, 0, 0}, {1, 2, 3}, {3, 1, 0}, {0, 3, 2}
    };
    MVZDD f = MVZDD::from_sets(base, sets);

    std::vector<std::vector<int>> enumerated = f.enumerate();
    MVZDD g = MVZDD::from_sets(base, enumerated);
    EXPECT_EQ(f, g);
}

TEST_F(MVDDTest, MVZDDFromSetsInvalidNoVarTable) {
    MVZDD base;
    EXPECT_THROW(MVZDD::from_sets(base, {{0}}), std::invalid_argument);
}

TEST_F(MVDDTest, MVZDDFromSetsInvalidWrongSize) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Assignment has wrong number of elements
    EXPECT_THROW(MVZDD::from_sets(base, {{1}}), std::invalid_argument);
    EXPECT_THROW(MVZDD::from_sets(base, {{1, 2, 0}}), std::invalid_argument);
}

TEST_F(MVDDTest, MVZDDFromSetsInvalidValueOutOfRange) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // k=3, so values must be in [0, 2]
    EXPECT_THROW(MVZDD::from_sets(base, {{3, 0}}), std::invalid_argument);
    EXPECT_THROW(MVZDD::from_sets(base, {{0, -1}}), std::invalid_argument);
}

TEST_F(MVDDTest, MVZDDFromSetsBinaryK2) {
    // k=2 case (binary variables)
    MVZDD base(2);
    base.new_var();
    base.new_var();
    base.new_var();

    std::vector<std::vector<int>> sets = {
        {0, 0, 0}, {1, 0, 1}, {0, 1, 0}, {1, 1, 1}
    };
    MVZDD f = MVZDD::from_sets(base, sets);
    EXPECT_EQ(f.count(), 4.0);
    for (const auto& s : sets) {
        EXPECT_TRUE(f.contains(s));
    }
}

// ============================================================
//  MVZDD::is_subset_family / is_disjoint tests
// ============================================================

TEST_F(MVDDTest, MVZDDIsSubsetFamily) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}});
    MVZDD g = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}});
    MVZDD h = MVZDD::from_sets(base, {{0, 0}, {2, 1}});

    EXPECT_TRUE(f.is_subset_family(g));
    EXPECT_FALSE(g.is_subset_family(f));
    EXPECT_TRUE(f.is_subset_family(f));

    // Empty family is subset of anything
    MVZDD empty = MVZDD::from_sets(base, {});
    EXPECT_TRUE(empty.is_subset_family(f));
    EXPECT_TRUE(empty.is_subset_family(empty));
}

TEST_F(MVDDTest, MVZDDIsDisjoint) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}});
    MVZDD g = MVZDD::from_sets(base, {{2, 1}, {0, 1}});
    MVZDD h = MVZDD::from_sets(base, {{1, 2}, {2, 0}});

    EXPECT_TRUE(f.is_disjoint(g));
    EXPECT_FALSE(f.is_disjoint(h));  // {1,2} in common

    MVZDD empty = MVZDD::from_sets(base, {});
    EXPECT_TRUE(f.is_disjoint(empty));
    EXPECT_TRUE(empty.is_disjoint(empty));
}

// ============================================================
//  MVZDD::count_intersec / jaccard_index / hamming_distance
// ============================================================

TEST_F(MVDDTest, MVZDDCountIntersec) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}});
    MVZDD g = MVZDD::from_sets(base, {{1, 2}, {2, 1}, {0, 1}});

    bigint::BigInt ci = f.count_intersec(g);
    EXPECT_EQ(ci.to_string(), "2");  // {1,2} and {2,1}

    MVZDD empty = MVZDD::from_sets(base, {});
    EXPECT_EQ(f.count_intersec(empty).to_string(), "0");
    EXPECT_EQ(f.count_intersec(f).to_string(), "3");
}

TEST_F(MVDDTest, MVZDDJaccardIndex) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}});
    MVZDD g = MVZDD::from_sets(base, {{1, 2}, {2, 1}});

    // |F ∩ G| = 1 ({1,2}), |F ∪ G| = 3
    EXPECT_NEAR(f.jaccard_index(g), 1.0 / 3.0, 1e-10);
    EXPECT_NEAR(f.jaccard_index(f), 1.0, 1e-10);

    // Both empty → 1.0
    MVZDD empty = MVZDD::from_sets(base, {});
    EXPECT_NEAR(empty.jaccard_index(empty), 1.0, 1e-10);
}

TEST_F(MVDDTest, MVZDDHammingDistance) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}});
    MVZDD g = MVZDD::from_sets(base, {{1, 2}, {2, 1}});

    // F △ G = {{0,0}, {2,1}} → size 2
    bigint::BigInt hd = f.hamming_distance(g);
    EXPECT_EQ(hd.to_string(), "2");
    EXPECT_EQ(f.hamming_distance(f).to_string(), "0");
}

TEST_F(MVDDTest, MVZDDOverlapCoefficient) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}});
    MVZDD g = MVZDD::from_sets(base, {{1, 2}, {2, 1}, {0, 1}});

    // |F ∩ G| = 1 ({1,2}), min(|F|, |G|) = min(2, 3) = 2
    EXPECT_NEAR(f.overlap_coefficient(g), 1.0 / 2.0, 1e-10);
    EXPECT_NEAR(f.overlap_coefficient(f), 1.0, 1e-10);

    // Subset: F ⊆ G → overlap = 1.0
    MVZDD h = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}});
    EXPECT_NEAR(f.overlap_coefficient(h), 1.0, 1e-10);

    // Disjoint → 0.0
    MVZDD d = MVZDD::from_sets(base, {{2, 1}, {0, 1}});
    EXPECT_NEAR(f.overlap_coefficient(d), 0.0, 1e-10);

    // Both empty → 1.0; one empty → 0.0
    MVZDD empty = MVZDD::from_sets(base, {});
    EXPECT_NEAR(empty.overlap_coefficient(empty), 1.0, 1e-10);
    EXPECT_NEAR(f.overlap_coefficient(empty), 0.0, 1e-10);
    EXPECT_NEAR(empty.overlap_coefficient(f), 0.0, 1e-10);

    // Symmetry
    EXPECT_NEAR(f.overlap_coefficient(g), g.overlap_coefficient(f), 1e-10);
}

// ============================================================
//  MVZDD::cost_bound_range tests
// ============================================================

TEST_F(MVDDTest, MVZDDCostBoundRange) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // 3 assignments: (0,0) cost=0, (1,2) cost=5, (2,1) cost=3
    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}});
    std::vector<std::vector<int>> weights = {{0, 1, 2}, {0, 1, 2}};
    // costs: (0,0)→0, (1,2)→1+2=3, (2,1)→2+1=3

    // range [0, 0]: only (0,0)
    MVZDD r1 = f.cost_bound_range(weights, 0, 0);
    EXPECT_EQ(r1.count(), 1.0);
    EXPECT_TRUE(r1.contains({0, 0}));

    // range [1, 3]: (1,2) and (2,1) with cost 3
    MVZDD r2 = f.cost_bound_range(weights, 1, 3);
    EXPECT_EQ(r2.count(), 2.0);
    EXPECT_TRUE(r2.contains({1, 2}));
    EXPECT_TRUE(r2.contains({2, 1}));

    // range [0, 10]: everything
    MVZDD r3 = f.cost_bound_range(weights, 0, 10);
    EXPECT_EQ(r3.count(), 3.0);

    // range [4, 10]: nothing
    MVZDD r4 = f.cost_bound_range(weights, 4, 10);
    EXPECT_EQ(r4.count(), 0.0);
}

TEST_F(MVDDTest, MVZDDCostBoundRangeWithMemo) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}});
    std::vector<std::vector<int>> weights = {{0, 1, 2}, {0, 1, 2}};

    CostBoundMemo memo;
    MVZDD r1 = f.cost_bound_range(weights, 0, 0, memo);
    EXPECT_EQ(r1.count(), 1.0);

    MVZDD r2 = f.cost_bound_range(weights, 1, 3, memo);
    EXPECT_EQ(r2.count(), 2.0);
}

// ============================================================
//  MVZDD::sample_k / random_subset tests
// ============================================================

TEST_F(MVDDTest, MVZDDSampleK) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}, {0, 1}});
    ZDD z = f.to_zdd();
    ZddCountMemo memo(z);

    std::mt19937_64 rng(42);
    MVZDD sampled = f.sample_k(2, rng, memo);
    EXPECT_EQ(sampled.count(), 2.0);
    EXPECT_TRUE(sampled.is_subset_family(f));

    // k=0 → empty
    std::mt19937_64 rng2(42);
    MVZDD empty = f.sample_k(0, rng2, memo);
    EXPECT_TRUE(empty.is_zero());

    // k >= total → all
    std::mt19937_64 rng3(42);
    MVZDD all = f.sample_k(10, rng3, memo);
    EXPECT_EQ(all, f);
}

TEST_F(MVDDTest, MVZDDRandomSubset) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {2, 1}, {0, 1}});

    // p=0 → empty
    std::mt19937_64 rng1(42);
    MVZDD empty = f.random_subset(0.0, rng1);
    EXPECT_TRUE(empty.is_zero());

    // p=1 → all
    std::mt19937_64 rng2(42);
    MVZDD all = f.random_subset(1.0, rng2);
    EXPECT_EQ(all, f);

    // p=0.5 → subset
    std::mt19937_64 rng3(42);
    MVZDD sub = f.random_subset(0.5, rng3);
    EXPECT_TRUE(sub.is_subset_family(f));
    EXPECT_LE(sub.count(), 4.0);
}

// ============================================================
//  MVZDD::profile / profile_double tests
// ============================================================

TEST_F(MVDDTest, MVZDDProfileBasic) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // {(0,0), (1,2), (0,1)}
    // (0,0) → 0 non-zero vars
    // (1,2) → 2 non-zero vars
    // (0,1) → 1 non-zero var
    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {0, 1}});
    auto prof = f.profile();

    ASSERT_EQ(prof.size(), 3u);
    EXPECT_EQ(prof[0].to_string(), "1");  // (0,0)
    EXPECT_EQ(prof[1].to_string(), "1");  // (0,1)
    EXPECT_EQ(prof[2].to_string(), "1");  // (1,2)
}

TEST_F(MVDDTest, MVZDDProfileAllZero) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Only the all-zero assignment
    MVZDD f = MVZDD::from_sets(base, {{0, 0}});
    auto prof = f.profile();

    ASSERT_EQ(prof.size(), 1u);
    EXPECT_EQ(prof[0].to_string(), "1");
}

TEST_F(MVDDTest, MVZDDProfileAllNonZero) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // All vars non-zero
    MVZDD f = MVZDD::from_sets(base, {{1, 1}, {2, 2}, {1, 2}});
    auto prof = f.profile();

    ASSERT_EQ(prof.size(), 3u);
    EXPECT_EQ(prof[0].to_string(), "0");
    EXPECT_EQ(prof[1].to_string(), "0");
    EXPECT_EQ(prof[2].to_string(), "3");
}

TEST_F(MVDDTest, MVZDDProfileEmpty) {
    MVZDD base(3);
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {});
    auto prof = f.profile();
    EXPECT_TRUE(prof.empty());
}

TEST_F(MVDDTest, MVZDDProfileDouble) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {0, 1}});
    auto prof = f.profile_double();

    ASSERT_EQ(prof.size(), 3u);
    EXPECT_DOUBLE_EQ(prof[0], 1.0);
    EXPECT_DOUBLE_EQ(prof[1], 1.0);
    EXPECT_DOUBLE_EQ(prof[2], 1.0);
}

// ============================================================
//  MVZDD::element_frequency tests
// ============================================================

TEST_F(MVDDTest, MVZDDElementFrequencyBasic) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // {(0,0), (1,2), (1,0)}
    // var1: val0→1, val1→2, val2→0
    // var2: val0→2, val1→0, val2→1
    MVZDD f = MVZDD::from_sets(base, {{0, 0}, {1, 2}, {1, 0}});
    auto freq = f.element_frequency();

    ASSERT_EQ(freq.size(), 2u);
    ASSERT_EQ(freq[0].size(), 3u);
    ASSERT_EQ(freq[1].size(), 3u);

    EXPECT_EQ(freq[0][0].to_string(), "1");
    EXPECT_EQ(freq[0][1].to_string(), "2");
    EXPECT_EQ(freq[0][2].to_string(), "0");

    EXPECT_EQ(freq[1][0].to_string(), "2");
    EXPECT_EQ(freq[1][1].to_string(), "0");
    EXPECT_EQ(freq[1][2].to_string(), "1");
}

TEST_F(MVDDTest, MVZDDElementFrequencyAllZero) {
    MVZDD base(3);
    base.new_var();
    base.new_var();

    // Only {(0,0)}: each variable has value 0 once
    MVZDD f = MVZDD::from_sets(base, {{0, 0}});
    auto freq = f.element_frequency();

    ASSERT_EQ(freq.size(), 2u);
    EXPECT_EQ(freq[0][0].to_string(), "1");
    EXPECT_EQ(freq[0][1].to_string(), "0");
    EXPECT_EQ(freq[0][2].to_string(), "0");
    EXPECT_EQ(freq[1][0].to_string(), "1");
    EXPECT_EQ(freq[1][1].to_string(), "0");
    EXPECT_EQ(freq[1][2].to_string(), "0");
}

TEST_F(MVDDTest, MVZDDElementFrequencyEmpty) {
    MVZDD base(3);
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {});
    auto freq = f.element_frequency();

    ASSERT_EQ(freq.size(), 1u);
    EXPECT_EQ(freq[0][0].to_string(), "0");
    EXPECT_EQ(freq[0][1].to_string(), "0");
    EXPECT_EQ(freq[0][2].to_string(), "0");
}

TEST_F(MVDDTest, MVZDDElementFrequencySumEqualsTotal) {
    // For each variable, sum of all value frequencies == |F|
    MVZDD base(4);
    base.new_var();
    base.new_var();
    base.new_var();

    MVZDD f = MVZDD::from_sets(base, {
        {0, 0, 0}, {1, 2, 3}, {3, 1, 0}, {0, 3, 2}, {2, 0, 1}
    });
    auto freq = f.element_frequency();
    bigint::BigInt total = f.exact_count();

    ASSERT_EQ(freq.size(), 3u);
    for (size_t mv = 0; mv < freq.size(); ++mv) {
        bigint::BigInt sum;
        for (int v = 0; v < 4; ++v) {
            sum += freq[mv][v];
        }
        EXPECT_EQ(sum, total);
    }
}
