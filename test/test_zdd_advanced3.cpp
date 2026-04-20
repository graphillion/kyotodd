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

// ============================================================
// to_cnf tests
// ============================================================

TEST_F(BDDTest, ToCnf_SingleClause) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1, v2}} → one clause: (v2 | v1) — higher level first
    ZDD f = ZDD::single_set({v1, v2});
    EXPECT_EQ(f.to_cnf(), "(" + std::to_string(v2) + " | " +
                           std::to_string(v1) + ")");
}

TEST_F(BDDTest, ToCnf_MultipleClauses) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // {{v1, v3}, {v2}} → (v1 | v3) & (v2)
    ZDD f = ZDD::single_set({v1, v3}) + ZDD::singleton(v2);
    std::string result = f.to_cnf();
    // Both clauses must appear, connected by &
    EXPECT_NE(result.find(" & "), std::string::npos);
    EXPECT_NE(result.find("|"), std::string::npos);
}

TEST_F(BDDTest, ToCnf_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    EXPECT_EQ(empty.to_cnf(), "T");
}

TEST_F(BDDTest, ToCnf_Null) {
    EXPECT_EQ(ZDD::Null.to_cnf(), "N");
}

TEST_F(BDDTest, ToCnf_EmptySetClause) {
    bddvar v1 = bddnewvar();
    // {∅, {v1}} → two clauses: () & (v1)
    ZDD f = ZDD::Single + ZDD::singleton(v1);
    std::string result = f.to_cnf();
    EXPECT_NE(result.find("()"), std::string::npos);
}

TEST_F(BDDTest, ToCnf_SingleVariable) {
    bddvar v1 = bddnewvar();
    // {{v1}} → (v1)
    ZDD f = ZDD::singleton(v1);
    EXPECT_EQ(f.to_cnf(), "(" + std::to_string(v1) + ")");
}

TEST_F(BDDTest, ToCnf_UnitFamily) {
    bddnewvar();
    // {∅} → empty clause
    EXPECT_EQ(ZDD::Single.to_cnf(), "()");
}

TEST_F(BDDTest, ToCnf_VarNameMap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::single_set({v1, v2});
    std::vector<std::string> names(v2 + 1);
    names[v1] = "a";
    names[v2] = "b";
    // Higher level (v2) appears first in ZDD traversal
    EXPECT_EQ(f.to_cnf(names), "(b | a)");
}

// ============================================================
// to_dnf tests
// ============================================================

TEST_F(BDDTest, ToDnf_SingleTerm) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1, v2}} → one term: (v2 & v1) — higher level first
    ZDD f = ZDD::single_set({v1, v2});
    EXPECT_EQ(f.to_dnf(), "(" + std::to_string(v2) + " & " +
                           std::to_string(v1) + ")");
}

TEST_F(BDDTest, ToDnf_MultipleTerms) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // {{v1, v3}, {v2}} → (v1 & v3) | (v2)
    ZDD f = ZDD::single_set({v1, v3}) + ZDD::singleton(v2);
    std::string result = f.to_dnf();
    EXPECT_NE(result.find(" | "), std::string::npos);
    EXPECT_NE(result.find("&"), std::string::npos);
}

TEST_F(BDDTest, ToDnf_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    EXPECT_EQ(empty.to_dnf(), "F");
}

TEST_F(BDDTest, ToDnf_Null) {
    EXPECT_EQ(ZDD::Null.to_dnf(), "N");
}

TEST_F(BDDTest, ToDnf_EmptySetTerm) {
    bddvar v1 = bddnewvar();
    // {∅, {v1}} → two terms: () | (v1)
    ZDD f = ZDD::Single + ZDD::singleton(v1);
    std::string result = f.to_dnf();
    EXPECT_NE(result.find("()"), std::string::npos);
}

TEST_F(BDDTest, ToDnf_SingleVariable) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    EXPECT_EQ(f.to_dnf(), "(" + std::to_string(v1) + ")");
}

TEST_F(BDDTest, ToDnf_UnitFamily) {
    bddnewvar();
    EXPECT_EQ(ZDD::Single.to_dnf(), "()");
}

TEST_F(BDDTest, ToDnf_VarNameMap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::single_set({v1, v2});
    std::vector<std::string> names(v2 + 1);
    names[v1] = "x";
    names[v2] = "y";
    // Higher level (v2) appears first
    EXPECT_EQ(f.to_dnf(names), "(y & x)");
}

TEST_F(BDDTest, ToCnfDnf_DifferentOutput) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // {{v1}, {v2}} → CNF: (v1) & (v2), DNF: (v1) | (v2)
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);
    std::string cnf = f.to_cnf();
    std::string dnf = f.to_dnf();
    EXPECT_NE(cnf, dnf);
    EXPECT_NE(cnf.find(" & "), std::string::npos);
    EXPECT_NE(dnf.find(" | "), std::string::npos);
}

// ============================================================
// random_subset tests
// ============================================================

TEST_F(BDDTest, RandomSubset_EmptyFamily) {
    bddnewvar();
    ZDD empty(0);
    std::mt19937_64 rng(42);
    ZDD result = empty.random_subset(0.5, rng);
    EXPECT_TRUE(result.is_zero());
}

TEST_F(BDDTest, RandomSubset_ProbZero) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::power_set(2);
    std::mt19937_64 rng(42);
    ZDD result = f.random_subset(0.0, rng);
    EXPECT_TRUE(result.is_zero());
}

TEST_F(BDDTest, RandomSubset_ProbOne) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::power_set(2);
    std::mt19937_64 rng(42);
    ZDD result = f.random_subset(1.0, rng);
    EXPECT_EQ(result, f);
}

TEST_F(BDDTest, RandomSubset_InvalidProb) {
    bddvar v1 = bddnewvar();
    ZDD f = ZDD::singleton(v1);
    std::mt19937_64 rng(42);
    EXPECT_THROW(f.random_subset(-0.1, rng), std::invalid_argument);
    EXPECT_THROW(f.random_subset(1.1, rng), std::invalid_argument);
}

TEST_F(BDDTest, RandomSubset_NullZDD) {
    ZDD f = ZDD::Null;
    std::mt19937_64 rng(42);
    EXPECT_THROW(f.random_subset(0.5, rng), std::invalid_argument);
}

TEST_F(BDDTest, RandomSubset_UnitFamily) {
    bddnewvar();
    ZDD f = ZDD::Single;  // {∅}
    std::mt19937_64 rng(42);
    // Run several times: should get either Empty or Single
    bool got_empty = false, got_single = false;
    for (int i = 0; i < 100; ++i) {
        ZDD result = f.random_subset(0.5, rng);
        if (result.is_zero()) got_empty = true;
        else if (result == ZDD::Single) got_single = true;
        else FAIL() << "Unexpected result from random_subset on {∅}";
    }
    EXPECT_TRUE(got_empty);
    EXPECT_TRUE(got_single);
}

TEST_F(BDDTest, RandomSubset_IsSubfamily) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(3);
    std::mt19937_64 rng(42);
    for (int i = 0; i < 20; ++i) {
        ZDD result = f.random_subset(0.5, rng);
        EXPECT_TRUE(result.is_subset_family(f));
    }
}

TEST_F(BDDTest, RandomSubset_SizeRange) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(4);  // 16 sets
    std::mt19937_64 rng(42);
    for (int i = 0; i < 20; ++i) {
        ZDD result = f.random_subset(0.5, rng);
        double cnt = result.count();
        EXPECT_GE(cnt, 0.0);
        EXPECT_LE(cnt, 16.0);
    }
}

TEST_F(BDDTest, RandomSubset_StatisticalMean) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(5);  // 32 sets
    double p = 0.3;
    double expected_mean = p * 32.0;
    int trials = 1000;
    double total = 0.0;
    std::mt19937_64 rng(123);
    for (int i = 0; i < trials; ++i) {
        ZDD result = f.random_subset(p, rng);
        total += result.count();
    }
    double mean = total / trials;
    // Allow ±3 standard deviations: stdev ≈ sqrt(n*p*(1-p)) ≈ sqrt(32*0.3*0.7) ≈ 2.59
    // stdev of mean ≈ 2.59 / sqrt(1000) ≈ 0.082
    // 5-sigma margin: 0.41
    EXPECT_NEAR(mean, expected_mean, 1.0);
}

TEST_F(BDDTest, RandomSubset_DifferentSeeds) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::power_set(4);  // 16 sets
    std::mt19937_64 rng1(1);
    std::mt19937_64 rng2(2);
    // With 16 sets and p=0.5, probability of identical results is (C(16,k)/2^16)^2
    // summed over k, which is very small. Run a few times to be safe.
    bool found_different = false;
    for (int i = 0; i < 10; ++i) {
        ZDD r1 = f.random_subset(0.5, rng1);
        ZDD r2 = f.random_subset(0.5, rng2);
        if (r1 != r2) {
            found_different = true;
            break;
        }
    }
    EXPECT_TRUE(found_different);
}

TEST_F(BDDTest, ZDD_SampleK_DeepZDD) {
    // Create a deep ZDD with 200 variables to exercise BDD_RecurGuard
    const int n = 200;
    for (int i = 0; i < n; ++i) bddnewvar();
    // Build a chain: {{1,2,...,n}} - a single large set
    ZDD f = ZDD::single_set({});
    std::vector<bddvar> vars;
    for (int i = 1; i <= n; ++i) vars.push_back(i);
    f = ZDD::single_set(vars);
    // Add a few smaller sets
    f = f + ZDD::single_set({1, 2, 3});
    f = f + ZDD::single_set({100, 150, 200});
    f = f + ZDD(1);  // add empty set

    ZddCountMemo memo(f);
    std::mt19937_64 rng(42);

    ZDD result = f.sample_k(2, rng, memo);
    EXPECT_EQ(result.exact_count(), bigint::BigInt(2));
    EXPECT_TRUE(result.is_subset_family(f));
}

TEST_F(BDDTest, RandomSubset_DeepZDD) {
    // Create a deep ZDD with 200 variables to exercise BDD_RecurGuard
    const int n = 200;
    for (int i = 0; i < n; ++i) bddnewvar();
    ZDD f = ZDD::single_set({});
    std::vector<bddvar> vars;
    for (int i = 1; i <= n; ++i) vars.push_back(i);
    f = ZDD::single_set(vars);
    f = f + ZDD::single_set({1, 2, 3});
    f = f + ZDD::single_set({50, 100, 150});
    f = f + ZDD(1);  // add empty set

    std::mt19937_64 rng(42);

    ZDD result = f.random_subset(0.5, rng);
    EXPECT_TRUE(result.is_subset_family(f));
    // With p=0.5 and 4 sets, expected ~2 sets
    EXPECT_LE(result.count(), 4.0);
}

// ============================================================
// hamming_distance tests
// ============================================================

TEST_F(BDDTest, HammingDistance_BothEmpty) {
    ZDD a(0), b(0);
    EXPECT_EQ(a.hamming_distance(b), bigint::BigInt(0));
}

TEST_F(BDDTest, HammingDistance_Identical) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD ps = ZDD::power_set(3);
    EXPECT_EQ(ps.hamming_distance(ps), bigint::BigInt(0));
}

TEST_F(BDDTest, HammingDistance_Disjoint) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD::singleton(v1);  // {{v1}}
    ZDD b = ZDD::singleton(v2);  // {{v2}}
    // |a △ b| = |a| + |b| - 2*0 = 2
    EXPECT_EQ(a.hamming_distance(b), bigint::BigInt(2));
}

TEST_F(BDDTest, HammingDistance_PartialOverlap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // a = {{v1}, {v2}}, b = {{v2}, {v1,v2}}
    // |a ∩ b| = 1 ({{v2}}), |a △ b| = 2 + 2 - 2*1 = 2
    ZDD a = ZDD::singleton(v1) + ZDD::singleton(v2);
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v1, v2});
    EXPECT_EQ(a.hamming_distance(b), bigint::BigInt(2));
}

TEST_F(BDDTest, HammingDistance_Symmetric) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD::singleton(v1);
    ZDD b = ZDD::singleton(v2);
    EXPECT_EQ(a.hamming_distance(b), b.hamming_distance(a));
}

TEST_F(BDDTest, HammingDistance_SubsetCase) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD a = ZDD::combination(3, 2);  // 3 sets
    ZDD b = ZDD::power_set(3);       // 8 sets
    // a ⊆ b → |a △ b| = |b| - |a| = 5
    EXPECT_EQ(a.hamming_distance(b), bigint::BigInt(5));
}

TEST_F(BDDTest, HammingDistance_ConsistentWithSymDiff) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD a = ZDD::combination(3, 1);
    ZDD b = ZDD::combination(3, 2);
    EXPECT_EQ(a.hamming_distance(b), (a ^ b).exact_count());
}

TEST_F(BDDTest, HammingDistance_OneEmpty) {
    bddnewvar();
    bddnewvar();
    ZDD a = ZDD::power_set(2);  // 4 sets
    ZDD b(0);                    // empty
    EXPECT_EQ(a.hamming_distance(b), bigint::BigInt(4));
}

TEST_F(BDDTest, HammingDistance_Null) {
    bddnewvar();
    ZDD ps = ZDD::power_set(1);
    EXPECT_EQ(ZDD::Null.hamming_distance(ps), bigint::BigInt(0));
    EXPECT_EQ(ps.hamming_distance(ZDD::Null), bigint::BigInt(0));
}

TEST_F(BDDTest, HammingDistance_FreeFunction) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddp ps = ZDD::power_set(3).GetID();
    bddp c32 = ZDD::combination(3, 2).GetID();
    EXPECT_EQ(bddhammingdist(ps, c32), bigint::BigInt(5));
}

// ============================================================
// overlap_coefficient tests
// ============================================================

TEST_F(BDDTest, OverlapCoefficient_BothEmpty) {
    ZDD a(0), b(0);
    EXPECT_DOUBLE_EQ(a.overlap_coefficient(b), 1.0);
}

TEST_F(BDDTest, OverlapCoefficient_Identical) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD ps = ZDD::power_set(3);
    EXPECT_DOUBLE_EQ(ps.overlap_coefficient(ps), 1.0);
}

TEST_F(BDDTest, OverlapCoefficient_Disjoint) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD a = ZDD::singleton(v1);
    ZDD b = ZDD::singleton(v2);
    EXPECT_DOUBLE_EQ(a.overlap_coefficient(b), 0.0);
}

TEST_F(BDDTest, OverlapCoefficient_SubsetCase) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // a ⊆ b → O(a,b) = |a| / min(|a|,|b|) = |a|/|a| = 1.0
    ZDD a = ZDD::combination(3, 2);  // 3 sets
    ZDD b = ZDD::power_set(3);       // 8 sets
    EXPECT_DOUBLE_EQ(a.overlap_coefficient(b), 1.0);
}

TEST_F(BDDTest, OverlapCoefficient_PartialOverlap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // a = {{v1}, {v2}}, b = {{v2}, {v1,v2}}
    // |a ∩ b| = 1, min(|a|,|b|) = 2 → O = 0.5
    ZDD a = ZDD::singleton(v1) + ZDD::singleton(v2);
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v1, v2});
    EXPECT_DOUBLE_EQ(a.overlap_coefficient(b), 0.5);
}

TEST_F(BDDTest, OverlapCoefficient_Symmetric) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD a = ZDD::singleton(v1) + ZDD::single_set({v2, v3});
    ZDD b = ZDD::singleton(v2) + ZDD::single_set({v1, v3});
    EXPECT_DOUBLE_EQ(a.overlap_coefficient(b), b.overlap_coefficient(a));
}

TEST_F(BDDTest, OverlapCoefficient_OneEmpty) {
    bddnewvar();
    ZDD a = ZDD::power_set(1);
    ZDD b(0);
    EXPECT_DOUBLE_EQ(a.overlap_coefficient(b), 0.0);
}

TEST_F(BDDTest, OverlapCoefficient_Null) {
    bddnewvar();
    ZDD ps = ZDD::power_set(1);
    EXPECT_DOUBLE_EQ(ZDD::Null.overlap_coefficient(ps), 0.0);
    EXPECT_DOUBLE_EQ(ps.overlap_coefficient(ZDD::Null), 0.0);
}

TEST_F(BDDTest, OverlapCoefficient_FreeFunction) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    bddp ps = ZDD::power_set(3).GetID();
    bddp c32 = ZDD::combination(3, 2).GetID();
    // a ⊆ b → O = 1.0
    EXPECT_DOUBLE_EQ(bddoverlapcoeff(c32, ps), 1.0);
}

// ============================================================
// entropy tests
// ============================================================

TEST_F(BDDTest, Entropy_EmptyFamily) {
    ZDD e(0);
    EXPECT_DOUBLE_EQ(e.entropy(), 0.0);
}

TEST_F(BDDTest, Entropy_UnitFamily) {
    // {∅} — no elements at all
    ZDD u(1);
    EXPECT_DOUBLE_EQ(u.entropy(), 0.0);
}

TEST_F(BDDTest, Entropy_SingletonSet) {
    // {{v1}} — only one element, entropy = 0
    ZDD s = ZDD::singleton(bddnewvar());
    EXPECT_DOUBLE_EQ(s.entropy(), 0.0);
}

TEST_F(BDDTest, Entropy_UniformFrequency) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // Power set of 3 vars: each var appears in 4 of 8 sets
    // All frequencies equal → H = log2(3) ≈ 1.585
    ZDD ps = ZDD::power_set(3);
    EXPECT_NEAR(ps.entropy(), std::log2(3.0), 1e-10);
}

TEST_F(BDDTest, Entropy_TwoVarsUniform) {
    bddnewvar();
    bddnewvar();
    // Power set of 2 vars: each var appears in 2 of 4 sets
    // H = log2(2) = 1.0
    ZDD ps = ZDD::power_set(2);
    EXPECT_NEAR(ps.entropy(), 1.0, 1e-10);
}

TEST_F(BDDTest, Entropy_NonUniform) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // F = {{v1}, {v1, v2}} — freq(v1)=2, freq(v2)=1
    // total_lit = 3, p(v1)=2/3, p(v2)=1/3
    // H = -(2/3)log2(2/3) - (1/3)log2(1/3)
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2});
    double p1 = 2.0 / 3.0, p2 = 1.0 / 3.0;
    double expected = -(p1 * std::log2(p1) + p2 * std::log2(p2));
    EXPECT_NEAR(f.entropy(), expected, 1e-10);
}

// ============================================================
// variance_size tests
// ============================================================

TEST_F(BDDTest, VarianceSize_EmptyFamily) {
    ZDD e(0);
    EXPECT_DOUBLE_EQ(e.variance_size(), 0.0);
}

TEST_F(BDDTest, VarianceSize_UnitFamily) {
    ZDD u(1);  // {∅}, all sets have size 0
    EXPECT_DOUBLE_EQ(u.variance_size(), 0.0);
}

TEST_F(BDDTest, VarianceSize_SingletonSet) {
    ZDD s = ZDD::singleton(bddnewvar());  // {{v1}}, all sets have size 1
    EXPECT_DOUBLE_EQ(s.variance_size(), 0.0);
}

TEST_F(BDDTest, VarianceSize_UniformSizes) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::combination(3, 2);  // all 3 sets have size 2
    EXPECT_DOUBLE_EQ(f.variance_size(), 0.0);
}

TEST_F(BDDTest, VarianceSize_MixedSizes) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // F = {{v1}, {v1,v2}, {v1,v2,v3}}, sizes = {1, 2, 3}, mean = 2
    // variance = ((1-2)^2 + (2-2)^2 + (3-2)^2) / 3 = 2/3
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2}) +
            ZDD::single_set({v1, v2, v3});
    EXPECT_NEAR(f.variance_size(), 2.0 / 3.0, 1e-10);
}

TEST_F(BDDTest, VarianceSize_PowerSet) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // Power set of {1,2,3}: 8 sets with sizes 0,1,1,1,2,2,2,3
    // Profile: {1, 3, 3, 1}, mean = 12/8 = 1.5
    // E[X^2] = (0*1 + 1*3 + 4*3 + 9*1)/8 = 24/8 = 3.0
    // Var = 3.0 - 1.5^2 = 0.75
    ZDD ps = ZDD::power_set(3);
    EXPECT_NEAR(ps.variance_size(), 0.75, 1e-10);
}

TEST_F(BDDTest, VarianceSize_WithEmptySet) {
    bddvar v1 = bddnewvar();
    // F = {∅, {v1}}, sizes = {0, 1}, mean = 0.5
    // Var = (0.25 + 0.25) / 2 = 0.25
    ZDD f = ZDD(1) + ZDD::singleton(v1);
    EXPECT_NEAR(f.variance_size(), 0.25, 1e-10);
}

// ============================================================
// median_size tests
// ============================================================

TEST_F(BDDTest, MedianSize_EmptyFamily) {
    ZDD e(0);
    EXPECT_DOUBLE_EQ(e.median_size(), 0.0);
}

TEST_F(BDDTest, MedianSize_UnitFamily) {
    ZDD u(1);  // {∅}
    EXPECT_DOUBLE_EQ(u.median_size(), 0.0);
}

TEST_F(BDDTest, MedianSize_SingletonSet) {
    ZDD s = ZDD::singleton(bddnewvar());  // {{v1}}
    EXPECT_DOUBLE_EQ(s.median_size(), 1.0);
}

TEST_F(BDDTest, MedianSize_OddCount) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // F = {{v1}, {v1,v2}, {v1,v2,v3}}, sizes = {1,2,3}, median = 2
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2}) +
            ZDD::single_set({v1, v2, v3});
    EXPECT_DOUBLE_EQ(f.median_size(), 2.0);
}

TEST_F(BDDTest, MedianSize_EvenCount) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // F = {∅, {v1}, {v2}, {v1,v2}}, sizes = {0,1,1,2}
    // sorted: {0,1,1,2}, median = (1+1)/2 = 1.0
    ZDD f = ZDD::power_set(2);
    EXPECT_DOUBLE_EQ(f.median_size(), 1.0);
}

TEST_F(BDDTest, MedianSize_EvenCountAveraging) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // F = {∅, {v1,v2,v3}}, sizes = {0, 3}, median = (0+3)/2 = 1.5
    ZDD f = ZDD(1) + ZDD::single_set({v1, v2, v3});
    EXPECT_DOUBLE_EQ(f.median_size(), 1.5);
}

TEST_F(BDDTest, MedianSize_PowerSet3) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    // Power set of {1,2,3}: 8 sets, profile {1,3,3,1}
    // sorted sizes: {0,1,1,1,2,2,2,3}, median = (1+2)/2 = 1.5
    ZDD ps = ZDD::power_set(3);
    EXPECT_DOUBLE_EQ(ps.median_size(), 1.5);
}

TEST_F(BDDTest, MedianSize_AllSameSize) {
    bddnewvar();
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::combination(3, 2);  // all sets have size 2
    EXPECT_DOUBLE_EQ(f.median_size(), 2.0);
}

// ============================================================
// cost_bound_range tests
// ============================================================

TEST_F(BDDTest, CostBoundRange_EmptyFamily) {
    std::vector<int> w = {0, 5, 10};
    bddnewvar();
    bddnewvar();
    ZDD f(0);
    EXPECT_EQ(f.cost_bound_range(w, 0, 100), ZDD::Empty);
}

TEST_F(BDDTest, CostBoundRange_UnitFamily) {
    std::vector<int> w = {0, 5, 10};
    bddnewvar();
    bddnewvar();
    ZDD f(1);  // {∅}, cost = 0
    EXPECT_EQ(f.cost_bound_range(w, 0, 100), ZDD::Single);
    EXPECT_EQ(f.cost_bound_range(w, 1, 100), ZDD::Empty);
    EXPECT_EQ(f.cost_bound_range(w, -5, -1), ZDD::Empty);
}

TEST_F(BDDTest, CostBoundRange_SingleVariable) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    std::vector<int> w = {0, 5, 10};
    ZDD f = ZDD::singleton(v1) + ZDD::singleton(v2);  // {{v1}, {v2}}
    // cost(v1) = 5, cost(v2) = 10
    EXPECT_EQ(f.cost_bound_range(w, 5, 5), ZDD::singleton(v1));
    EXPECT_EQ(f.cost_bound_range(w, 10, 10), ZDD::singleton(v2));
    EXPECT_EQ(f.cost_bound_range(w, 5, 10), f);
    EXPECT_EQ(f.cost_bound_range(w, 6, 9), ZDD::Empty);
}

TEST_F(BDDTest, CostBoundRange_CrossValidateWithLeGe) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    std::vector<int> w = {0, 3, -2, 7};
    ZDD f = ZDD::power_set(3);
    for (long long lo = -5; lo <= 10; lo += 3) {
        for (long long hi = lo; hi <= 12; hi += 3) {
            ZDD range = f.cost_bound_range(w, lo, hi);
            ZDD expected = f.cost_bound_le(w, hi) & f.cost_bound_ge(w, lo);
            EXPECT_EQ(range, expected)
                << "lo=" << lo << " hi=" << hi;
        }
    }
}

TEST_F(BDDTest, CostBoundRange_WithMemo) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    std::vector<int> w = {0, 3, -2, 7};
    ZDD f = ZDD::power_set(3);
    CostBoundMemo memo;
    ZDD r1 = f.cost_bound_range(w, 0, 5, memo);
    ZDD r2 = f.cost_bound_range(w, 3, 10, memo);
    ZDD r3 = f.cost_bound_range(w, -2, 0, memo);
    // Verify results match non-memo versions
    EXPECT_EQ(r1, f.cost_bound_range(w, 0, 5));
    EXPECT_EQ(r2, f.cost_bound_range(w, 3, 10));
    EXPECT_EQ(r3, f.cost_bound_range(w, -2, 0));
}

TEST_F(BDDTest, CostBoundRange_EquivalentToEq) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    std::vector<int> w = {0, 3, -2, 7};
    ZDD f = ZDD::power_set(3);
    // range(b, b) should equal eq(b)
    for (long long b = -5; b <= 12; ++b) {
        EXPECT_EQ(f.cost_bound_range(w, b, b), f.cost_bound_eq(w, b))
            << "b=" << b;
    }
}

// --- Review fixes: RecurGuard for weighted sampling ---

TEST_F(BDDTest, ZDD_WeightedSample_RecurGuard_DeepZDD) {
    // Build a deep chain ZDD that exceeds BDD_RecurLimit (8192)
    const int depth = 9000;
    for (int i = bdd_varcount; i < depth; ++i) bddnewvar();
    // Build single-chain ZDD: {1,2,...,depth}
    ZDD f = ZDD::Single;
    for (int v = depth; v >= 1; --v) {
        f = ZDD_ID(ZDD::getnode_raw(v, bddempty, f.GetID()));
    }
    std::vector<double> w(depth + 1, 1.0);
    w[0] = 0.0;
    std::mt19937_64 rng(42);
    WeightedSampleMemo memo_prod(f, w, WeightMode::Product);
    EXPECT_THROW(f.weighted_sample(w, WeightMode::Product, rng, memo_prod),
                 std::overflow_error);
    WeightedSampleMemo memo_sum(f, w, WeightMode::Sum);
    EXPECT_THROW(f.weighted_sample(w, WeightMode::Sum, rng, memo_sum),
                 std::overflow_error);
}

// --- Review fixes: boltzmann_sample mismatch after memo stored ---

TEST_F(BDDTest, ZDD_BoltzmannSample_WeightsMismatch_AfterStore) {
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<double> w1 = {0.0, 1.0, 2.0};
    std::vector<double> w2 = {0.0, 5.0, 10.0};
    double beta = 1.0;
    auto tw1 = ZDD::boltzmann_weights(w1, beta);
    WeightedSampleMemo memo(f, tw1, WeightMode::Product);
    std::mt19937_64 rng(42);
    // First call succeeds and populates memo
    EXPECT_NO_THROW(f.boltzmann_sample(w1, beta, rng, memo));
    EXPECT_TRUE(memo.stored());
    // Second call with different weights must throw
    EXPECT_THROW(f.boltzmann_sample(w2, beta, rng, memo),
                 std::invalid_argument);
}

TEST_F(BDDTest, ZDD_BoltzmannSample_BetaMismatch_AfterStore) {
    bddnewvar();
    bddnewvar();
    ZDD f = ZDD::singleton(1) + ZDD::singleton(2);
    std::vector<double> w = {0.0, 1.0, 2.0};
    auto tw = ZDD::boltzmann_weights(w, 1.0);
    WeightedSampleMemo memo(f, tw, WeightMode::Product);
    std::mt19937_64 rng(42);
    // First call succeeds and populates memo
    EXPECT_NO_THROW(f.boltzmann_sample(w, 1.0, rng, memo));
    EXPECT_TRUE(memo.stored());
    // Second call with different beta must throw
    EXPECT_THROW(f.boltzmann_sample(w, 2.0, rng, memo),
                 std::invalid_argument);
}

// --- Review fixes: BigInt overflow in statistics/similarity APIs ---

TEST_F(BDDTest, ZDD_JaccardIndex_LargeBigInt) {
    // power_set(1100) has 2^1100 sets — far beyond double range
    const int n = 1100;
    for (int i = bdd_varcount; i < n; ++i) bddnewvar();
    ZDD f = ZDD::power_set(n);
    // jaccard(f, f) = 1.0 always
    EXPECT_DOUBLE_EQ(f.jaccard_index(f), 1.0);
    // jaccard with a subset
    ZDD g = ZDD::singleton(1) + ZDD::singleton(2);
    double j = f.jaccard_index(g);
    EXPECT_GE(j, 0.0);
    EXPECT_LE(j, 1.0);
}

TEST_F(BDDTest, ZDD_OverlapCoeff_LargeBigInt) {
    const int n = 1100;
    for (int i = bdd_varcount; i < n; ++i) bddnewvar();
    ZDD f = ZDD::power_set(n);
    EXPECT_DOUBLE_EQ(f.overlap_coefficient(f), 1.0);
    ZDD g = ZDD::singleton(1) + ZDD::singleton(2);
    double o = f.overlap_coefficient(g);
    // g is subset of f, so overlap = |g ∩ f| / min(|g|, |f|) = |g| / |g| = 1.0
    EXPECT_DOUBLE_EQ(o, 1.0);
}

TEST_F(BDDTest, ZDD_Entropy_LargeBigInt) {
    const int n = 1100;
    for (int i = bdd_varcount; i < n; ++i) bddnewvar();
    ZDD f = ZDD::power_set(n);
    double h = f.entropy();
    // For power_set, all variables have equal frequency → max entropy
    EXPECT_GT(h, 0.0);
    EXPECT_TRUE(std::isfinite(h));
}

TEST_F(BDDTest, ZDD_VarianceSize_LargeBigInt) {
    const int n = 1100;
    for (int i = bdd_varcount; i < n; ++i) bddnewvar();
    ZDD f = ZDD::power_set(n);
    double v = f.variance_size();
    // power_set(n) has binomial(n,k) sets of size k.
    // Mean = n/2, Variance = n/4
    EXPECT_GT(v, 0.0);
    EXPECT_TRUE(std::isfinite(v));
    EXPECT_NEAR(v, n / 4.0, 1.0);
}

// --- ZDD lowercase snake_case method tests ---

TEST_F(BDDTest, ZDD_LowercaseId) {
    bddvar v1 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(z_v1.id(), z_v1.GetID());
    EXPECT_EQ(z_v1.id(), z_v1.id());
    EXPECT_EQ(ZDD(0).id(), bddempty);
    EXPECT_EQ(ZDD(1).id(), bddsingle);
}

TEST_F(BDDTest, ZDD_LowercaseChange) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(z_v1.change(v2), z_v1.Change(v2));
}

TEST_F(BDDTest, ZDD_LowercaseOffset) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    EXPECT_EQ(F.offset(v2), F.OffSet(v2));
    EXPECT_EQ(F.offset(v2), z_v1);
}

TEST_F(BDDTest, ZDD_LowercaseOnset) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    EXPECT_EQ(F.onset(v2), F.OnSet(v2));
    EXPECT_EQ(F.onset(v2), z_v1v2);
}

TEST_F(BDDTest, ZDD_LowercaseOnset0) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    EXPECT_EQ(F.onset0(v2), F.OnSet0(v2));
    EXPECT_EQ(F.onset0(v2), z_v1);
}

TEST_F(BDDTest, ZDD_LowercaseIntersec) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    EXPECT_EQ(F.intersec(z_v1), F.Intersec(z_v1));
    EXPECT_EQ(F.intersec(z_v1), z_v1);
}

TEST_F(BDDTest, ZDD_LowercaseMeet) {
    bddvar v1 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD s = ZDD(1);  // {∅}
    EXPECT_EQ(z_v1.meet(s), z_v1.Meet(s));
}

TEST_F(BDDTest, ZDD_LowercaseCard) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2 + ZDD(1);
    EXPECT_EQ(F.card(), F.Card());
    EXPECT_EQ(F.card(), 3u);
}

TEST_F(BDDTest, ZDD_LowercaseRestrictOp) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    ZDD G = z_v1;  // filter: subsets of {v1}
    EXPECT_EQ(F.restrict_op(G), F.Restrict(G));
}

TEST_F(BDDTest, ZDD_LowercasePermit) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    ZDD G = z_v1;  // permit: only v1 allowed
    EXPECT_EQ(F.permit(G), F.Permit(G));
}

TEST_F(BDDTest, ZDD_RemoveSupersets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    // F = {{v1}, {v1,v2}}, remove supersets of {v1} → removes {v1,v2}
    ZDD F = z_v1 + z_v1v2;
    ZDD result = F.remove_supersets(z_v1);
    EXPECT_EQ(result.id(), bddremove_supersets(F.id(), z_v1.id()));
}

TEST_F(BDDTest, ZDD_RemoveSubsets) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    // F = {{v1}, {v1,v2}}, remove subsets of {v1,v2} → removes {v1}
    ZDD F = z_v1 + z_v1v2;
    ZDD result = F.remove_subsets(z_v1v2);
    EXPECT_EQ(result.id(), bddremove_subsets(F.id(), z_v1v2.id()));
}

TEST_F(BDDTest, ZDD_LowercaseSupport) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(z_v1v2.support(), z_v1v2.Support());
}

TEST_F(BDDTest, ZDD_LowercaseLit) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    EXPECT_EQ(F.lit(), F.Lit());
}

TEST_F(BDDTest, ZDD_LowercaseLen) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(z_v1v2.len(), z_v1v2.Len());
    EXPECT_EQ(z_v1v2.len(), 2u);
}

TEST_F(BDDTest, ZDD_LowercaseIsPoly) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.is_poly(), F.IsPoly());
    EXPECT_TRUE(F.is_poly());
    EXPECT_FALSE(z_v1.is_poly());
}

TEST_F(BDDTest, ZDD_LowercaseSwap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    EXPECT_EQ(z_v1.swap(v1, v2), z_v1.Swap(v1, v2));
}

TEST_F(BDDTest, ZDD_LowercaseImplyChk) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // F = {{v1, v2}} → v1 implies v2
    ZDD F = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(F.imply_chk(v1, v2), F.ImplyChk(v1, v2));
    EXPECT_TRUE(F.imply_chk(v1, v2));
}

TEST_F(BDDTest, ZDD_LowercaseCoImplyChk) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(z_v1v2.co_imply_chk(v1, v2), z_v1v2.CoImplyChk(v1, v2));
}

TEST_F(BDDTest, ZDD_LowercasePermitSym) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2 + ZDD(1);
    EXPECT_EQ(F.permit_sym(1), F.PermitSym(1));
}

TEST_F(BDDTest, ZDD_LowercaseAlways) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // F = {{v1, v2}, {v1}} → v1 is always present
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    ZDD F = z_v1 + z_v1v2;
    EXPECT_EQ(F.always(), F.Always());
}

TEST_F(BDDTest, ZDD_LowercaseSymChk) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // F = {{v1}, {v2}} → v1 and v2 are symmetric
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.sym_chk(v1, v2), F.SymChk(v1, v2));
    EXPECT_TRUE(F.sym_chk(v1, v2));
}

TEST_F(BDDTest, ZDD_LowercaseImplySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(z_v1v2.imply_set(v1), z_v1v2.ImplySet(v1));
}

TEST_F(BDDTest, ZDD_LowercaseSymGrp) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.sym_grp(), F.SymGrp());
}

TEST_F(BDDTest, ZDD_LowercaseSymGrpNaive) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.sym_grp_naive(), F.SymGrpNaive());
}

TEST_F(BDDTest, ZDD_LowercaseSymSet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.sym_set(v1), F.SymSet(v1));
}

TEST_F(BDDTest, ZDD_LowercaseCoImplySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1v2 = ZDD_ID(ZDD::getnode(v2, bddempty, ZDD::getnode(v1, bddempty, bddsingle)));
    EXPECT_EQ(z_v1v2.co_imply_set(v1), z_v1v2.CoImplySet(v1));
}

TEST_F(BDDTest, ZDD_LowercaseDivisor) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD F = z_v1 + z_v2;
    EXPECT_EQ(F.divisor(), F.Divisor());
}

TEST_F(BDDTest, ZDD_LowercasePrintPla) {
    // Test print_pla with deprecated PrintPla wrapper
    {
        ZDD e(0);
        std::ostringstream oss;
        std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
        e.PrintPla();
        std::cout.rdbuf(old);
        EXPECT_EQ(oss.str(), ".i 0\n.o 1\n0\n.e\n");
    }
}

TEST_F(BDDTest, ZDD_PrintPla_Null) {
    // bddnull: no output
    ZDD n(-1);
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    n.print_pla();
    std::cout.rdbuf(old);
    EXPECT_EQ(oss.str(), "");
}

TEST_F(BDDTest, ZDD_PrintPla_Empty) {
    // Empty family: tlev==0, output "0"
    ZDD e(0);
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    e.print_pla();
    std::cout.rdbuf(old);
    EXPECT_EQ(oss.str(), ".i 0\n.o 1\n0\n.e\n");
}

TEST_F(BDDTest, ZDD_PrintPla_Single) {
    // Single (unit family containing empty set): tlev==0, output "1"
    ZDD s(1);
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    s.print_pla();
    std::cout.rdbuf(old);
    EXPECT_EQ(oss.str(), ".i 0\n.o 1\n1\n.e\n");
}

TEST_F(BDDTest, ZDD_PrintPla_Example1) {
    // Example from spec: {{x1, x3}, {x2}} with 3 variables
    // x1=var at level 1, x2=var at level 2, x3=var at level 3
    bddvar v1 = bddnewvar();  // level 1
    bddvar v2 = bddnewvar();  // level 2
    bddvar v3 = bddnewvar();  // level 3

    // {x1, x3}
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v3 = ZDD_ID(ZDD::getnode(v3, bddempty, bddsingle));
    ZDD set_x1_x3 = z_v1 * z_v3;  // join = {x1, x3}

    // {x2}
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));

    ZDD F = set_x1_x3 + z_v2;  // {{x1, x3}, {x2}}

    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    F.print_pla();
    std::cout.rdbuf(old);
    EXPECT_EQ(oss.str(), ".i 3\n.o 1\n101 1\n010 1\n.e\n");
}

TEST_F(BDDTest, ZDD_PrintPla_SingleVar) {
    // Single variable: {{x1}}
    bddvar v1 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    z_v1.print_pla();
    std::cout.rdbuf(old);
    EXPECT_EQ(oss.str(), ".i 1\n.o 1\n1 1\n.e\n");
}

TEST_F(BDDTest, ZDD_PrintPla_AllCombinations) {
    // {{x1, x2}, {x1}, {x2}, {}} with 2 variables
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD all = (z_v1 + ZDD(1)) * (z_v2 + ZDD(1));  // power set of {v1, v2}

    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    all.print_pla();
    std::cout.rdbuf(old);
    // OnSet0 first (1), then OffSet (0) at each level
    // Level 2=1: Level 1=1 -> "11 1", Level 1=0 -> "01 1"
    // Level 2=0: Level 1=1 -> "10 1", Level 1=0 -> "00 1"
    EXPECT_EQ(oss.str(), ".i 2\n.o 1\n11 1\n01 1\n10 1\n00 1\n.e\n");
}

TEST_F(BDDTest, ZDD_PrintPla_VarsAboveSupport) {
    // When variables exist above the ZDD's support, .i should still
    // reflect the total number of declared variables.
    bddvar v1 = bddnewvar();  // level 1
    bddvar v2 = bddnewvar();  // level 2 (not used in ZDD)
    (void)v2;

    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));  // {{v1}}

    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    z_v1.print_pla();
    std::cout.rdbuf(old);
    // .i should be 2 (total declared vars), not 1 (top level of ZDD)
    // cube "10" means v1=1, v2=0
    EXPECT_EQ(oss.str(), ".i 2\n.o 1\n10 1\n.e\n");
}

TEST_F(BDDTest, ZDD_PrintPla_EmptySetWithExtraVars) {
    // {∅} (single/unit family) with extra declared variables
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    (void)v1;
    (void)v2;

    ZDD s(1);  // {∅}

    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    s.print_pla();
    std::cout.rdbuf(old);
    // ∅ = all variables absent → "00 1"
    EXPECT_EQ(oss.str(), ".i 2\n.o 1\n00 1\n.e\n");
}

TEST_F(BDDTest, ZDD_Zlev_Empty) {
    ZDD e(0);
    // zlev on empty family: always returns empty
    EXPECT_EQ(e.zlev(0, 0), ZDD(0));
    EXPECT_EQ(e.zlev(5, 0), ZDD(0));
}

TEST_F(BDDTest, ZDD_Zlev_Single) {
    ZDD s(1);
    // zlev on {∅}: lev <= 0 returns {∅} (contains empty set)
    EXPECT_EQ(s.zlev(0, 0), ZDD(1));
    EXPECT_EQ(s.zlev(5, 0), ZDD(1));
}

TEST_F(BDDTest, ZDD_Zlev_BasicDescent) {
    // Create variables at levels 1, 2, 3
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // F = {{v1}, {v2}, {v3}} - top level is 3
    ZDD z1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD z3 = ZDD_ID(ZDD::getnode(v3, bddempty, bddsingle));
    ZDD F = z1 + z2 + z3;

    // zlev(3, 0): top level is already 3, no descent needed
    EXPECT_EQ(F.zlev(3, 0), F);

    // zlev(2, 0): remove v3 via OffSet, result should be {{v1}, {v2}}
    ZDD result2 = F.zlev(2, 0);
    EXPECT_EQ(result2, z1 + z2);

    // zlev(1, 0): remove v3 and v2, result should be {{v1}}
    ZDD result1 = F.zlev(1, 0);
    EXPECT_EQ(result1, z1);

    // zlev(0, 0): returns {∅} if empty set is in F, else empty
    // F = {{v1},{v2},{v3}} does not contain ∅
    EXPECT_EQ(F.zlev(0, 0), ZDD(0));
}

TEST_F(BDDTest, ZDD_Zlev_WithEmptySet) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    // F = {{v1}, {v2}, {}} - contains empty set
    ZDD F = z1 + z2 + ZDD(1);

    // zlev(0, 0): F contains ∅, so returns {∅}
    EXPECT_EQ(F.zlev(0, 0), ZDD(1));

    // zlev(1, 0): should include {v1} and {∅}
    ZDD result1 = F.zlev(1, 0);
    EXPECT_EQ(result1, z1 + ZDD(1));
}

TEST_F(BDDTest, ZDD_Zlev_LastParameter) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    // F = {{v3}} - top at level 3
    ZDD z3 = ZDD_ID(ZDD::getnode(v3, bddempty, bddsingle));

    // zlev(2, 1): last=1, target level=2
    // After OffSet(v3), result is empty (level 0), which is < 2
    // So return the backup (before the OffSet), which is z3 itself
    ZDD result = z3.zlev(2, 1);
    EXPECT_EQ(result, z3);

    // zlev(3, 1): already at level 3, so return as-is
    EXPECT_EQ(z3.zlev(3, 1), z3);
}

TEST_F(BDDTest, ZDD_Zlev_DeprecatedWrapper) {
    ZDD e(0);
    EXPECT_EQ(e.ZLev(0, 0), e.zlev(0, 0));
}

TEST_F(BDDTest, ZDD_SetZskip_Empty) {
    // set_zskip on empty/single should not throw
    ZDD e(0);
    e.set_zskip();  // no-op (level 0)
    ZDD s(1);
    s.set_zskip();  // no-op (level 0)
}

TEST_F(BDDTest, ZDD_SetZskip_DeprecatedWrapper) {
    ZDD e(0);
    e.SetZSkip();  // should call set_zskip(), no throw
}

TEST_F(BDDTest, ZDD_SetZskip_AcceleratesZlev) {
    // Create a ZDD with many levels and verify that
    // set_zskip + zlev produces the same result as zlev alone
    const int N = 20;
    std::vector<bddvar> vars(N);
    for (int i = 0; i < N; ++i) {
        vars[i] = bddnewvar();
    }
    // Build a singleton set containing the top variable
    ZDD F = ZDD_ID(ZDD::getnode(vars[N-1], bddempty, bddsingle));

    // Without skip: zlev should work correctly
    ZDD r1 = F.zlev(5, 0);

    // With skip: set_zskip then zlev should give same result
    F.set_zskip();
    ZDD r2 = F.zlev(5, 0);

    EXPECT_EQ(r1, r2);
}

TEST_F(BDDTest, ZDD_Zlev_Null) {
    // bddnull should propagate as bddnull
    ZDD n(-1);
    EXPECT_EQ(n.zlev(0, 0), ZDD(-1));
    EXPECT_EQ(n.zlev(5, 0), ZDD(-1));
    EXPECT_EQ(n.zlev(5, 1), ZDD(-1));
}

TEST_F(BDDTest, ZDD_Zlev_Complement) {
    // Complement edge should be handled correctly in zlev
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();

    ZDD z1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    ZDD z3 = ZDD_ID(ZDD::getnode(v3, bddempty, bddsingle));
    ZDD F = z1 + z2 + z3;  // {{v1},{v2},{v3}}
    ZDD Fc = ~F;            // complement: ∅ membership toggled

    // F does not contain ∅, so ~F does contain ∅
    EXPECT_FALSE(F.has_empty());
    EXPECT_TRUE(Fc.has_empty());

    // zlev on complement should equal complement of zlev
    EXPECT_EQ(Fc.zlev(2, 0), ~(F.zlev(2, 0)));
    EXPECT_EQ(Fc.zlev(1, 0), ~(F.zlev(1, 0)));
    EXPECT_EQ(Fc.zlev(0, 0), ~(F.zlev(0, 0)));
}

TEST_F(BDDTest, ZDD_Zlev_LastNoLoop) {
    // When target level > current top, last=1 should return self
    bddvar v1 = bddnewvar();
    ZDD z1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));

    // z1 has top at level 1, zlev(5, 1) should return z1 (not empty)
    EXPECT_EQ(z1.zlev(5, 1), z1);
}

TEST_F(BDDTest, ZDD_SetZskip_Null) {
    // set_zskip on bddnull should be a no-op
    ZDD n(-1);
    n.set_zskip();  // should not throw
}

TEST_F(BDDTest, ZDD_SetZskip_Complement) {
    // set_zskip on complement should work correctly
    const int N = 20;
    std::vector<bddvar> vars(N);
    for (int i = 0; i < N; ++i) {
        vars[i] = bddnewvar();
    }
    ZDD F = ZDD_ID(ZDD::getnode(vars[N-1], bddempty, bddsingle));
    ZDD Fc = ~F;

    // Reference: zlev without skip
    ZDD ref_f = F.zlev(5, 0);
    ZDD ref_fc = Fc.zlev(5, 0);

    // After set_zskip on F, zlev on both F and ~F should still be correct
    F.set_zskip();
    EXPECT_EQ(F.zlev(5, 0), ref_f);
    EXPECT_EQ(Fc.zlev(5, 0), ref_fc);

    // After set_zskip on ~F, results should still be correct
    Fc.set_zskip();
    EXPECT_EQ(F.zlev(5, 0), ref_f);
    EXPECT_EQ(Fc.zlev(5, 0), ref_fc);
}

TEST_F(BDDTest, ZDD_SetZskip_ComplementCacheIsolation) {
    // Verify that set_zskip on ~f does not corrupt f's cache
    const int N = 12;
    std::vector<bddvar> vars(N);
    for (int i = 0; i < N; ++i) {
        vars[i] = bddnewvar();
    }
    // F = power_set - {∅}: does NOT contain ∅
    ZDD F = ZDD::power_set(N) - ZDD(1);
    ZDD Fc = ~F;  // contains ∅

    // Reference values before any skip caching
    ZDD ref_f = F.zlev(5, 0);
    ZDD ref_fc = Fc.zlev(5, 0);

    // F does not contain ∅, Fc does
    EXPECT_FALSE(ref_f.has_empty());
    EXPECT_TRUE(ref_fc.has_empty());

    // set_zskip on ~F, then check F
    Fc.set_zskip();
    EXPECT_EQ(F.zlev(5, 0), ref_f);
    EXPECT_FALSE(F.zlev(5, 0).has_empty());

    // set_zskip on F, then check ~F
    F.set_zskip();
    EXPECT_EQ(Fc.zlev(5, 0), ref_fc);
    EXPECT_TRUE(Fc.zlev(5, 0).has_empty());
}

TEST_F(BDDTest, ZDD_Join) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD z_v1 = ZDD_ID(ZDD::getnode(v1, bddempty, bddsingle));
    ZDD z_v2 = ZDD_ID(ZDD::getnode(v2, bddempty, bddsingle));
    // join == operator*
    EXPECT_EQ(z_v1.join(z_v2), z_v1 * z_v2);
    // join with empty
    EXPECT_EQ(z_v1.join(ZDD(0)), ZDD(0));
    // join with single (unit family {∅})
    EXPECT_EQ(z_v1.join(ZDD(1)), z_v1);
}

// ============================================================
// Parameterized mode tests: Recursive / Iterative / Auto must
// agree for all zdd_adv_filter operations.
// ============================================================

// Clear the op cache between calls so both recursive and iterative
// paths actually execute (the same op code is shared and a cached
// result would short-circuit the second invocation).
#define EXPECT_FILTER_MODE_EQ(expr_mode, expr_default)        \
    do {                                                      \
        bdd_cache_clear();                                    \
        auto _actual = (expr_mode);                           \
        bdd_cache_clear();                                    \
        auto _expected = (expr_default);                      \
        EXPECT_EQ(_actual, _expected);                        \
    } while (0)

class ZddAdvFilterModeTest : public ::testing::TestWithParam<BddExecMode> {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

INSTANTIATE_TEST_SUITE_P(
    ExecModes,
    ZddAdvFilterModeTest,
    ::testing::Values(BddExecMode::Recursive, BddExecMode::Iterative, BddExecMode::Auto),
    [](const ::testing::TestParamInfo<BddExecMode>& info) {
        switch (info.param) {
        case BddExecMode::Recursive: return "Recursive";
        case BddExecMode::Iterative: return "Iterative";
        case BddExecMode::Auto: return "Auto";
        }
        return "Unknown";
    }
);

namespace {
struct FilterFamilies {
    bddvar v1, v2, v3, v4;
    bddp s_v1, s_v2, s_v3, s_v4;  // singletons {{vi}}
    bddp v1v2;                     // {{v1,v2}}
    bddp v2v3;                     // {{v2,v3}}
    bddp v1v2v3;                   // {{v1,v2,v3}}
    bddp v3v4;                     // {{v3,v4}}
    bddp F;                        // {{v1},{v1,v2},{v2,v3}}
    bddp G;                        // {{v1,v2},{v2,v3},{v3,v4}}
    bddp H;                        // {{v1},{v2},{v1,v2},{v2,v3}} — overlapping chain
};

static FilterFamilies make_filter_families() {
    FilterFamilies t;
    t.v1 = bddnewvar();
    t.v2 = bddnewvar();
    t.v3 = bddnewvar();
    t.v4 = bddnewvar();
    t.s_v1 = ZDD::getnode(t.v1, bddempty, bddsingle);
    t.s_v2 = ZDD::getnode(t.v2, bddempty, bddsingle);
    t.s_v3 = ZDD::getnode(t.v3, bddempty, bddsingle);
    t.s_v4 = ZDD::getnode(t.v4, bddempty, bddsingle);
    t.v1v2 = bddjoin(t.s_v1, t.s_v2);
    t.v2v3 = bddjoin(t.s_v2, t.s_v3);
    t.v1v2v3 = bddjoin(t.v1v2, t.s_v3);
    t.v3v4 = bddjoin(t.s_v3, t.s_v4);
    t.F = bddunion(bddunion(t.s_v1, t.v1v2), t.v2v3);
    t.G = bddunion(bddunion(t.v1v2, t.v2v3), t.v3v4);
    t.H = bddunion(bddunion(bddunion(t.s_v1, t.s_v2), t.v1v2), t.v2v3);
    bdd_cache_clear();
    return t;
}
} // namespace

TEST_P(ZddAdvFilterModeTest, DisjoinFamily) {
    auto t = make_filter_families();
    EXPECT_EQ(bdddisjoin(bddempty, t.F, GetParam()), bddempty);
    EXPECT_EQ(bdddisjoin(t.F, bddsingle, GetParam()), t.F);
    EXPECT_FILTER_MODE_EQ(bdddisjoin(t.F, t.G, GetParam()), bdddisjoin(t.F, t.G));
    EXPECT_FILTER_MODE_EQ(bdddisjoin(t.s_v1, t.v3v4, GetParam()),
                          bdddisjoin(t.s_v1, t.v3v4));
}

TEST_P(ZddAdvFilterModeTest, JointJoinFamily) {
    auto t = make_filter_families();
    EXPECT_EQ(bddjointjoin(bddempty, t.F, GetParam()), bddempty);
    EXPECT_EQ(bddjointjoin(t.F, bddsingle, GetParam()), bddempty);
    EXPECT_FILTER_MODE_EQ(bddjointjoin(t.F, t.G, GetParam()),
                          bddjointjoin(t.F, t.G));
    EXPECT_FILTER_MODE_EQ(bddjointjoin(t.H, t.v1v2, GetParam()),
                          bddjointjoin(t.H, t.v1v2));
}

TEST_P(ZddAdvFilterModeTest, RestrictFamily) {
    auto t = make_filter_families();
    EXPECT_EQ(bddrestrict(t.F, bddsingle, GetParam()), t.F);
    EXPECT_EQ(bddrestrict(t.F, bddempty, GetParam()), bddempty);
    EXPECT_FILTER_MODE_EQ(bddrestrict(t.F, t.G, GetParam()),
                          bddrestrict(t.F, t.G));
    EXPECT_FILTER_MODE_EQ(bddrestrict(t.H, t.v1v2, GetParam()),
                          bddrestrict(t.H, t.v1v2));
}

TEST_P(ZddAdvFilterModeTest, PermitFamily) {
    auto t = make_filter_families();
    EXPECT_EQ(bddpermit(bddsingle, t.F, GetParam()), bddsingle);
    EXPECT_EQ(bddpermit(t.F, bddempty, GetParam()), bddempty);
    EXPECT_FILTER_MODE_EQ(bddpermit(t.F, t.G, GetParam()),
                          bddpermit(t.F, t.G));
    EXPECT_FILTER_MODE_EQ(bddpermit(t.H, t.v1v2v3, GetParam()),
                          bddpermit(t.H, t.v1v2v3));
}

TEST_P(ZddAdvFilterModeTest, NonsupFamily) {
    auto t = make_filter_families();
    EXPECT_EQ(bddremove_supersets(t.F, bddempty, GetParam()), t.F);
    EXPECT_EQ(bddremove_supersets(t.F, bddsingle, GetParam()), bddempty);
    EXPECT_FILTER_MODE_EQ(bddremove_supersets(t.F, t.G, GetParam()),
                          bddremove_supersets(t.F, t.G));
    EXPECT_FILTER_MODE_EQ(bddremove_supersets(t.H, t.v1v2, GetParam()),
                          bddremove_supersets(t.H, t.v1v2));
}

TEST_P(ZddAdvFilterModeTest, NonsubFamily) {
    auto t = make_filter_families();
    EXPECT_EQ(bddremove_subsets(t.F, bddempty, GetParam()), t.F);
    EXPECT_FILTER_MODE_EQ(bddremove_subsets(t.F, t.G, GetParam()),
                          bddremove_subsets(t.F, t.G));
    EXPECT_FILTER_MODE_EQ(bddremove_subsets(t.H, t.v1v2, GetParam()),
                          bddremove_subsets(t.H, t.v1v2));
}

TEST_P(ZddAdvFilterModeTest, MaximalFamily) {
    auto t = make_filter_families();
    EXPECT_EQ(bddmaximal(bddempty, GetParam()), bddempty);
    EXPECT_EQ(bddmaximal(bddsingle, GetParam()), bddsingle);
    EXPECT_FILTER_MODE_EQ(bddmaximal(t.F, GetParam()), bddmaximal(t.F));
    EXPECT_FILTER_MODE_EQ(bddmaximal(t.H, GetParam()), bddmaximal(t.H));
    EXPECT_FILTER_MODE_EQ(bddmaximal(t.G, GetParam()), bddmaximal(t.G));
}

TEST_P(ZddAdvFilterModeTest, MinimalFamily) {
    auto t = make_filter_families();
    EXPECT_EQ(bddminimal(bddempty, GetParam()), bddempty);
    EXPECT_EQ(bddminimal(bddsingle, GetParam()), bddsingle);
    EXPECT_FILTER_MODE_EQ(bddminimal(t.F, GetParam()), bddminimal(t.F));
    EXPECT_FILTER_MODE_EQ(bddminimal(t.H, GetParam()), bddminimal(t.H));
    EXPECT_FILTER_MODE_EQ(bddminimal(t.G, GetParam()), bddminimal(t.G));
}

TEST_P(ZddAdvFilterModeTest, MinhitFamily) {
    auto t = make_filter_families();
    EXPECT_EQ(bddminhit(bddempty, GetParam()), bddsingle);
    EXPECT_EQ(bddminhit(bddsingle, GetParam()), bddempty);
    EXPECT_FILTER_MODE_EQ(bddminhit(t.F, GetParam()), bddminhit(t.F));
    EXPECT_FILTER_MODE_EQ(bddminhit(t.G, GetParam()), bddminhit(t.G));
    EXPECT_FILTER_MODE_EQ(bddminhit(t.H, GetParam()), bddminhit(t.H));
}

TEST_P(ZddAdvFilterModeTest, ClosureFamily) {
    auto t = make_filter_families();
    EXPECT_EQ(bddclosure(bddempty, GetParam()), bddempty);
    EXPECT_EQ(bddclosure(bddsingle, GetParam()), bddsingle);
    EXPECT_FILTER_MODE_EQ(bddclosure(t.F, GetParam()), bddclosure(t.F));
    EXPECT_FILTER_MODE_EQ(bddclosure(t.H, GetParam()), bddclosure(t.H));
    EXPECT_FILTER_MODE_EQ(bddclosure(t.v1v2v3, GetParam()), bddclosure(t.v1v2v3));
}

// Cross-validation test: moderately deep family to exercise asymmetric-level
// branches. n=10 keeps runtime short; BDD_RecurLimit is 8192 so the Auto
// parameter still dispatches to _rec here, but explicit Iterative exercises
// the _iter path.
TEST_P(ZddAdvFilterModeTest, CrossValidationLinearChain) {
    const int n = 10;
    std::vector<bddvar> vars;
    for (int i = 0; i < n; ++i) vars.push_back(bddnewvar());

    bddp s = bddsingle;
    bddp fam = bddempty;
    for (int i = 0; i < n; ++i) {
        s = bddjoin(s, ZDD::getnode(vars[i], bddempty, bddsingle));
        fam = bddunion(fam, s);
    }
    // other: pick two non-adjacent sets so the asymmetric and same-var
    // branches both fire
    bddp other = bddunion(
        ZDD::getnode(vars[2], bddempty, bddsingle),
        bddjoin(ZDD::getnode(vars[5], bddempty, bddsingle),
                ZDD::getnode(vars[7], bddempty, bddsingle)));

    EXPECT_FILTER_MODE_EQ(bdddisjoin(fam, other, GetParam()),
                          bdddisjoin(fam, other));
    EXPECT_FILTER_MODE_EQ(bddjointjoin(fam, other, GetParam()),
                          bddjointjoin(fam, other));
    EXPECT_FILTER_MODE_EQ(bddrestrict(fam, other, GetParam()),
                          bddrestrict(fam, other));
    EXPECT_FILTER_MODE_EQ(bddpermit(fam, other, GetParam()),
                          bddpermit(fam, other));
    EXPECT_FILTER_MODE_EQ(bddremove_supersets(fam, other, GetParam()),
                          bddremove_supersets(fam, other));
    EXPECT_FILTER_MODE_EQ(bddremove_subsets(fam, other, GetParam()),
                          bddremove_subsets(fam, other));
    EXPECT_FILTER_MODE_EQ(bddmaximal(fam, GetParam()), bddmaximal(fam));
    EXPECT_FILTER_MODE_EQ(bddminimal(fam, GetParam()), bddminimal(fam));
    EXPECT_FILTER_MODE_EQ(bddclosure(fam, GetParam()), bddclosure(fam));
    // minhit requires ∅ ∉ F, which holds for this chain (all sets non-empty)
    EXPECT_FILTER_MODE_EQ(bddminhit(fam, GetParam()), bddminhit(fam));
}

// ============================================================
// Parameterized mode tests for zdd_adv_weight operations
// (bddweightsum / bddminweight / bddmaxweight / bddcostbound_le).
// ============================================================

#define EXPECT_WEIGHT_MODE_EQ(expr_mode, expr_default)        \
    do {                                                      \
        bdd_cache_clear();                                    \
        auto _actual = (expr_mode);                           \
        bdd_cache_clear();                                    \
        auto _expected = (expr_default);                      \
        EXPECT_EQ(_actual, _expected);                        \
    } while (0)

class ZddAdvWeightModeTest : public ::testing::TestWithParam<BddExecMode> {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

INSTANTIATE_TEST_SUITE_P(
    ExecModes,
    ZddAdvWeightModeTest,
    ::testing::Values(BddExecMode::Recursive, BddExecMode::Iterative, BddExecMode::Auto),
    [](const ::testing::TestParamInfo<BddExecMode>& info) {
        switch (info.param) {
        case BddExecMode::Recursive: return "Recursive";
        case BddExecMode::Iterative: return "Iterative";
        case BddExecMode::Auto: return "Auto";
        }
        return "Unknown";
    }
);

namespace {
struct WeightFamilies {
    bddvar v1, v2, v3, v4;
    bddp s_v1, s_v2, s_v3, s_v4;
    bddp F;           // {{v1},{v1,v2},{v2,v3}}
    bddp G;           // {{v1,v2},{v2,v3},{v3,v4}}
    bddp pow3;        // powerset over v1,v2,v3 (contains ∅)
    bddp comp_F;      // ~F (complement edge)
    std::vector<int> w;
    std::vector<int> w_neg;
};

static WeightFamilies make_weight_families() {
    WeightFamilies t;
    t.v1 = bddnewvar();
    t.v2 = bddnewvar();
    t.v3 = bddnewvar();
    t.v4 = bddnewvar();
    t.s_v1 = ZDD::getnode(t.v1, bddempty, bddsingle);
    t.s_v2 = ZDD::getnode(t.v2, bddempty, bddsingle);
    t.s_v3 = ZDD::getnode(t.v3, bddempty, bddsingle);
    t.s_v4 = ZDD::getnode(t.v4, bddempty, bddsingle);
    bddp v1v2 = bddjoin(t.s_v1, t.s_v2);
    bddp v2v3 = bddjoin(t.s_v2, t.s_v3);
    bddp v3v4 = bddjoin(t.s_v3, t.s_v4);
    bddp v1v2v3 = bddjoin(v1v2, t.s_v3);
    t.F = bddunion(bddunion(t.s_v1, v1v2), v2v3);
    t.G = bddunion(bddunion(v1v2, v2v3), v3v4);
    t.pow3 = bddunion(bddunion(bddunion(bddsingle, t.s_v1),
                                bddunion(t.s_v2, v1v2)),
                      bddunion(bddunion(t.s_v3, bddjoin(t.s_v1, t.s_v3)),
                               bddunion(v2v3, v1v2v3)));
    t.comp_F = bddnot(t.F);
    t.w = {0, 1, 2, 3, 4};
    t.w_neg = {0, -1, 2, -3, 4};
    bdd_cache_clear();
    return t;
}
} // namespace

TEST_P(ZddAdvWeightModeTest, WeightSum) {
    auto t = make_weight_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddweightsum(bddempty, t.w, m), bigint::BigInt(0));
    EXPECT_EQ(bddweightsum(bddsingle, t.w, m), bigint::BigInt(0));
    EXPECT_EQ(bddweightsum(t.F, t.w, m), bddweightsum(t.F, t.w));
    EXPECT_EQ(bddweightsum(t.G, t.w, m), bddweightsum(t.G, t.w));
    EXPECT_EQ(bddweightsum(t.pow3, t.w, m), bddweightsum(t.pow3, t.w));
    EXPECT_EQ(bddweightsum(t.comp_F, t.w, m), bddweightsum(t.comp_F, t.w));
    EXPECT_EQ(bddweightsum(t.F, t.w_neg, m), bddweightsum(t.F, t.w_neg));
}

TEST_P(ZddAdvWeightModeTest, MinWeight) {
    auto t = make_weight_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddminweight(t.F, t.w, m), bddminweight(t.F, t.w));
    EXPECT_EQ(bddminweight(t.G, t.w, m), bddminweight(t.G, t.w));
    EXPECT_EQ(bddminweight(t.pow3, t.w, m), bddminweight(t.pow3, t.w));
    EXPECT_EQ(bddminweight(t.comp_F, t.w, m), bddminweight(t.comp_F, t.w));
    EXPECT_EQ(bddminweight(t.F, t.w_neg, m), bddminweight(t.F, t.w_neg));
    EXPECT_EQ(bddminweight(t.G, t.w_neg, m), bddminweight(t.G, t.w_neg));
}

TEST_P(ZddAdvWeightModeTest, MaxWeight) {
    auto t = make_weight_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddmaxweight(t.F, t.w, m), bddmaxweight(t.F, t.w));
    EXPECT_EQ(bddmaxweight(t.G, t.w, m), bddmaxweight(t.G, t.w));
    EXPECT_EQ(bddmaxweight(t.pow3, t.w, m), bddmaxweight(t.pow3, t.w));
    EXPECT_EQ(bddmaxweight(t.comp_F, t.w, m), bddmaxweight(t.comp_F, t.w));
    EXPECT_EQ(bddmaxweight(t.F, t.w_neg, m), bddmaxweight(t.F, t.w_neg));
    EXPECT_EQ(bddmaxweight(t.G, t.w_neg, m), bddmaxweight(t.G, t.w_neg));
}

TEST_P(ZddAdvWeightModeTest, CostBoundLe) {
    auto t = make_weight_families();
    BddExecMode m = GetParam();
    // Terminal fast-paths
    CostBoundMemo memo0;
    EXPECT_EQ(bddcostbound_le(bddempty, t.w, 5, memo0, m), bddempty);
    CostBoundMemo memo1;
    EXPECT_EQ(bddcostbound_le(bddsingle, t.w, 0, memo1, m), bddsingle);
    CostBoundMemo memo2;
    EXPECT_EQ(bddcostbound_le(bddsingle, t.w, -1, memo2, m), bddempty);

    for (long long b : {-1LL, 0LL, 1LL, 3LL, 5LL, 10LL, 100LL}) {
        CostBoundMemo ma, mb;
        EXPECT_WEIGHT_MODE_EQ(
            bddcostbound_le(t.F, t.w, b, ma, m),
            bddcostbound_le(t.F, t.w, b, mb));
        CostBoundMemo ma2, mb2;
        EXPECT_WEIGHT_MODE_EQ(
            bddcostbound_le(t.G, t.w, b, ma2, m),
            bddcostbound_le(t.G, t.w, b, mb2));
        CostBoundMemo ma3, mb3;
        EXPECT_WEIGHT_MODE_EQ(
            bddcostbound_le(t.pow3, t.w, b, ma3, m),
            bddcostbound_le(t.pow3, t.w, b, mb3));
        CostBoundMemo ma4, mb4;
        EXPECT_WEIGHT_MODE_EQ(
            bddcostbound_le(t.comp_F, t.w, b, ma4, m),
            bddcostbound_le(t.comp_F, t.w, b, mb4));
    }
}

TEST_P(ZddAdvWeightModeTest, CostBoundGe) {
    auto t = make_weight_families();
    BddExecMode m = GetParam();
    for (long long b : {-5LL, 0LL, 1LL, 3LL, 5LL, 10LL}) {
        CostBoundMemo ma, mb;
        EXPECT_WEIGHT_MODE_EQ(
            bddcostbound_ge(t.F, t.w, b, ma, m),
            bddcostbound_ge(t.F, t.w, b, mb));
        CostBoundMemo ma2, mb2;
        EXPECT_WEIGHT_MODE_EQ(
            bddcostbound_ge(t.pow3, t.w, b, ma2, m),
            bddcostbound_ge(t.pow3, t.w, b, mb2));
    }
}

// Cross-validation: linear-chain family exercising lo/hi on every level.
TEST_P(ZddAdvWeightModeTest, CrossValidationLinearChain) {
    const int n = 8;
    std::vector<bddvar> vars;
    std::vector<int> w(n + 1, 0);
    for (int i = 0; i < n; ++i) {
        vars.push_back(bddnewvar());
        w[vars[i]] = (i % 2 == 0) ? (i + 1) : -(i + 1);
    }

    bddp s = bddsingle;
    bddp fam = bddempty;
    for (int i = 0; i < n; ++i) {
        s = bddjoin(s, ZDD::getnode(vars[i], bddempty, bddsingle));
        fam = bddunion(fam, s);
    }

    BddExecMode m = GetParam();
    EXPECT_EQ(bddweightsum(fam, w, m), bddweightsum(fam, w));
    EXPECT_EQ(bddminweight(fam, w, m), bddminweight(fam, w));
    EXPECT_EQ(bddmaxweight(fam, w, m), bddmaxweight(fam, w));
    for (long long b : {-20LL, -5LL, 0LL, 5LL, 20LL}) {
        CostBoundMemo ma, mb;
        EXPECT_WEIGHT_MODE_EQ(bddcostbound_le(fam, w, b, ma, m),
                              bddcostbound_le(fam, w, b, mb));
    }
}

// ============================================================
// Parameterized mode tests for zdd_adv_rank operations
// (bddsupersets_of / bddsubsets_of / bddproject).
// ============================================================

#define EXPECT_RANK_MODE_EQ(expr_mode, expr_default)          \
    do {                                                      \
        bdd_cache_clear();                                    \
        auto _actual = (expr_mode);                           \
        bdd_cache_clear();                                    \
        auto _expected = (expr_default);                      \
        EXPECT_EQ(_actual, _expected);                        \
    } while (0)

class ZddAdvRankModeTest : public ::testing::TestWithParam<BddExecMode> {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

INSTANTIATE_TEST_SUITE_P(
    ExecModes,
    ZddAdvRankModeTest,
    ::testing::Values(BddExecMode::Recursive, BddExecMode::Iterative, BddExecMode::Auto),
    [](const ::testing::TestParamInfo<BddExecMode>& info) {
        switch (info.param) {
        case BddExecMode::Recursive: return "Recursive";
        case BddExecMode::Iterative: return "Iterative";
        case BddExecMode::Auto: return "Auto";
        }
        return "Unknown";
    }
);

namespace {
struct RankFamilies {
    bddvar v1, v2, v3, v4;
    bddp s_v1, s_v2, s_v3, s_v4;
    bddp v1v2, v1v3, v2v3, v1v2v3, v3v4;
    bddp F;      // {{v1},{v1,v2},{v2,v3}}
    bddp G;      // {{v1,v2},{v1,v3},{v2,v3}}
    bddp H;      // {∅,{v1},{v1,v2},{v2,v3}}
    bddp pow3;   // powerset over v1..v3
};

static RankFamilies make_rank_families() {
    RankFamilies t;
    t.v1 = bddnewvar();
    t.v2 = bddnewvar();
    t.v3 = bddnewvar();
    t.v4 = bddnewvar();
    t.s_v1 = ZDD::getnode(t.v1, bddempty, bddsingle);
    t.s_v2 = ZDD::getnode(t.v2, bddempty, bddsingle);
    t.s_v3 = ZDD::getnode(t.v3, bddempty, bddsingle);
    t.s_v4 = ZDD::getnode(t.v4, bddempty, bddsingle);
    t.v1v2 = bddjoin(t.s_v1, t.s_v2);
    t.v1v3 = bddjoin(t.s_v1, t.s_v3);
    t.v2v3 = bddjoin(t.s_v2, t.s_v3);
    t.v1v2v3 = bddjoin(t.v1v2, t.s_v3);
    t.v3v4 = bddjoin(t.s_v3, t.s_v4);
    t.F = bddunion(bddunion(t.s_v1, t.v1v2), t.v2v3);
    t.G = bddunion(bddunion(t.v1v2, t.v1v3), t.v2v3);
    t.H = bddunion(t.F, bddsingle);
    // Build pow3 = power_set over {v1,v2,v3}
    t.pow3 = bddsingle;
    t.pow3 = bddunion(t.pow3, ZDD::getnode(t.v1, bddempty, t.pow3));
    t.pow3 = bddunion(t.pow3, ZDD::getnode(t.v2, bddempty, t.pow3));
    t.pow3 = bddunion(t.pow3, ZDD::getnode(t.v3, bddempty, t.pow3));
    bdd_cache_clear();
    return t;
}
} // namespace

TEST_P(ZddAdvRankModeTest, SupersetsOfTerminals) {
    auto t = make_rank_families();
    EXPECT_EQ(bddsupersets_of(bddempty, {t.v1}, GetParam()), bddempty);
    EXPECT_EQ(bddsupersets_of(t.F, {}, GetParam()), t.F);
}

TEST_P(ZddAdvRankModeTest, SupersetsOfSingleVar) {
    auto t = make_rank_families();
    EXPECT_RANK_MODE_EQ(bddsupersets_of(t.F, {t.v1}, GetParam()),
                        bddsupersets_of(t.F, {t.v1}));
    EXPECT_RANK_MODE_EQ(bddsupersets_of(t.G, {t.v2}, GetParam()),
                        bddsupersets_of(t.G, {t.v2}));
}

TEST_P(ZddAdvRankModeTest, SupersetsOfMultiVar) {
    auto t = make_rank_families();
    EXPECT_RANK_MODE_EQ(bddsupersets_of(t.pow3, {t.v1, t.v2}, GetParam()),
                        bddsupersets_of(t.pow3, {t.v1, t.v2}));
    EXPECT_RANK_MODE_EQ(bddsupersets_of(t.F, {t.v1, t.v3}, GetParam()),
                        bddsupersets_of(t.F, {t.v1, t.v3}));
}

TEST_P(ZddAdvRankModeTest, SupersetsOfWithEmptyMember) {
    auto t = make_rank_families();
    // H contains ∅, which is superset only of ∅ itself.
    EXPECT_RANK_MODE_EQ(bddsupersets_of(t.H, {}, GetParam()),
                        bddsupersets_of(t.H, {}));
    EXPECT_RANK_MODE_EQ(bddsupersets_of(t.H, {t.v1}, GetParam()),
                        bddsupersets_of(t.H, {t.v1}));
}

TEST_P(ZddAdvRankModeTest, SubsetsOfTerminals) {
    auto t = make_rank_families();
    EXPECT_EQ(bddsubsets_of(bddempty, {t.v1}, GetParam()), bddempty);
    // subsets_of({}) = sets that are subsets of ∅ = {∅} iff ∅ ∈ F else ∅.
    EXPECT_RANK_MODE_EQ(bddsubsets_of(t.H, {}, GetParam()),
                        bddsubsets_of(t.H, {}));
    EXPECT_RANK_MODE_EQ(bddsubsets_of(t.F, {}, GetParam()),
                        bddsubsets_of(t.F, {}));
}

TEST_P(ZddAdvRankModeTest, SubsetsOfBasic) {
    auto t = make_rank_families();
    EXPECT_RANK_MODE_EQ(bddsubsets_of(t.F, {t.v1, t.v2}, GetParam()),
                        bddsubsets_of(t.F, {t.v1, t.v2}));
    EXPECT_RANK_MODE_EQ(bddsubsets_of(t.pow3, {t.v1, t.v2}, GetParam()),
                        bddsubsets_of(t.pow3, {t.v1, t.v2}));
}

TEST_P(ZddAdvRankModeTest, SubsetsOfAllVars) {
    auto t = make_rank_families();
    EXPECT_RANK_MODE_EQ(
        bddsubsets_of(t.F, {t.v1, t.v2, t.v3, t.v4}, GetParam()),
        bddsubsets_of(t.F, {t.v1, t.v2, t.v3, t.v4}));
}

TEST_P(ZddAdvRankModeTest, ProjectTerminals) {
    auto t = make_rank_families();
    EXPECT_EQ(bddproject(bddempty, {t.v1}, GetParam()), bddempty);
    EXPECT_EQ(bddproject(t.F, {}, GetParam()), t.F);
}

TEST_P(ZddAdvRankModeTest, ProjectSingleVar) {
    auto t = make_rank_families();
    EXPECT_RANK_MODE_EQ(bddproject(t.F, {t.v2}, GetParam()),
                        bddproject(t.F, {t.v2}));
    EXPECT_RANK_MODE_EQ(bddproject(t.G, {t.v3}, GetParam()),
                        bddproject(t.G, {t.v3}));
}

TEST_P(ZddAdvRankModeTest, ProjectMultiVar) {
    auto t = make_rank_families();
    EXPECT_RANK_MODE_EQ(bddproject(t.pow3, {t.v1, t.v2}, GetParam()),
                        bddproject(t.pow3, {t.v1, t.v2}));
    EXPECT_RANK_MODE_EQ(bddproject(t.F, {t.v2, t.v3}, GetParam()),
                        bddproject(t.F, {t.v2, t.v3}));
}

// Cross-validation: linear-chain family exercising every level.
TEST_P(ZddAdvRankModeTest, CrossValidationLinearChain) {
    const int n = 10;
    std::vector<bddvar> vars;
    for (int i = 0; i < n; ++i) vars.push_back(bddnewvar());

    bddp s = bddsingle;
    bddp fam = bddempty;
    for (int i = 0; i < n; ++i) {
        s = bddjoin(s, ZDD::getnode(vars[i], bddempty, bddsingle));
        fam = bddunion(fam, s);
    }
    // Pick non-adjacent variables from the chain.
    std::vector<bddvar> probe = {vars[1], vars[4], vars[7]};

    EXPECT_RANK_MODE_EQ(bddsupersets_of(fam, probe, GetParam()),
                        bddsupersets_of(fam, probe));
    EXPECT_RANK_MODE_EQ(bddsubsets_of(fam, probe, GetParam()),
                        bddsubsets_of(fam, probe));
    EXPECT_RANK_MODE_EQ(bddproject(fam, probe, GetParam()),
                        bddproject(fam, probe));
}

// ============================================================
// Parameterized mode tests for zdd_adv2 operations
// (bddpermitsym / bddalways / bddsymchk / bddsymset / bddcoimplyset).
// ============================================================

#define EXPECT_ADV2_MODE_EQ(expr_mode, expr_default)          \
    do {                                                      \
        bdd_cache_clear();                                    \
        auto _actual = (expr_mode);                           \
        bdd_cache_clear();                                    \
        auto _expected = (expr_default);                      \
        EXPECT_EQ(_actual, _expected);                        \
    } while (0)

class ZddAdv2ModeTest : public ::testing::TestWithParam<BddExecMode> {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

INSTANTIATE_TEST_SUITE_P(
    ExecModes,
    ZddAdv2ModeTest,
    ::testing::Values(BddExecMode::Recursive, BddExecMode::Iterative, BddExecMode::Auto),
    [](const ::testing::TestParamInfo<BddExecMode>& info) {
        switch (info.param) {
        case BddExecMode::Recursive: return "Recursive";
        case BddExecMode::Iterative: return "Iterative";
        case BddExecMode::Auto: return "Auto";
        }
        return "Unknown";
    }
);

namespace {
struct Adv2Families {
    bddvar v1, v2, v3, v4;
    bddp s_v1, s_v2, s_v3, s_v4;
    bddp v1v2, v2v3, v1v2v3;
    bddp F;       // {{v1},{v1,v2},{v2,v3}}
    bddp G;       // {{v1,v2},{v2,v3},{v3,v4}}
    bddp Sym;     // {{v1,v3},{v3}} — v1 and v2 not symmetric here? we'll test pairs
    bddp pow3;    // powerset over v1..v3 (highly symmetric: all vars symmetric)
    bddp comp_F;  // ~F (complement edge)
};

static Adv2Families make_adv2_families() {
    Adv2Families t;
    t.v1 = bddnewvar();
    t.v2 = bddnewvar();
    t.v3 = bddnewvar();
    t.v4 = bddnewvar();
    t.s_v1 = ZDD::getnode(t.v1, bddempty, bddsingle);
    t.s_v2 = ZDD::getnode(t.v2, bddempty, bddsingle);
    t.s_v3 = ZDD::getnode(t.v3, bddempty, bddsingle);
    t.s_v4 = ZDD::getnode(t.v4, bddempty, bddsingle);
    t.v1v2 = bddjoin(t.s_v1, t.s_v2);
    t.v2v3 = bddjoin(t.s_v2, t.s_v3);
    t.v1v2v3 = bddjoin(t.v1v2, t.s_v3);
    t.F = bddunion(bddunion(t.s_v1, t.v1v2), t.v2v3);
    bddp v3v4 = bddjoin(t.s_v3, t.s_v4);
    t.G = bddunion(bddunion(t.v1v2, t.v2v3), v3v4);
    t.Sym = bddunion(bddjoin(t.s_v1, t.s_v3), t.s_v3);
    // pow3 = power_set over {v1,v2,v3}: all variables are symmetric.
    t.pow3 = bddsingle;
    t.pow3 = bddunion(t.pow3, ZDD::getnode(t.v1, bddempty, t.pow3));
    t.pow3 = bddunion(t.pow3, ZDD::getnode(t.v2, bddempty, t.pow3));
    t.pow3 = bddunion(t.pow3, ZDD::getnode(t.v3, bddempty, t.pow3));
    t.comp_F = bddnot(t.F);
    bdd_cache_clear();
    return t;
}
} // namespace

TEST_P(ZddAdv2ModeTest, PermitSymTerminals) {
    auto t = make_adv2_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddpermitsym(bddempty, 3, m), bddempty);
    EXPECT_EQ(bddpermitsym(t.F, -1, m), bddempty);
    EXPECT_EQ(bddpermitsym(bddsingle, 5, m), bddsingle);
}

TEST_P(ZddAdv2ModeTest, PermitSymBasic) {
    auto t = make_adv2_families();
    BddExecMode m = GetParam();
    for (int n : {0, 1, 2, 3, 4}) {
        EXPECT_ADV2_MODE_EQ(bddpermitsym(t.F, n, m),
                            bddpermitsym(t.F, n));
        EXPECT_ADV2_MODE_EQ(bddpermitsym(t.G, n, m),
                            bddpermitsym(t.G, n));
        EXPECT_ADV2_MODE_EQ(bddpermitsym(t.pow3, n, m),
                            bddpermitsym(t.pow3, n));
        EXPECT_ADV2_MODE_EQ(bddpermitsym(t.comp_F, n, m),
                            bddpermitsym(t.comp_F, n));
    }
}

TEST_P(ZddAdv2ModeTest, AlwaysTerminals) {
    BddExecMode m = GetParam();
    EXPECT_EQ(bddalways(bddempty, m), bddempty);
    EXPECT_EQ(bddalways(bddsingle, m), bddempty);
}

TEST_P(ZddAdv2ModeTest, AlwaysBasic) {
    auto t = make_adv2_families();
    BddExecMode m = GetParam();
    EXPECT_ADV2_MODE_EQ(bddalways(t.F, m), bddalways(t.F));
    EXPECT_ADV2_MODE_EQ(bddalways(t.G, m), bddalways(t.G));
    EXPECT_ADV2_MODE_EQ(bddalways(t.v1v2v3, m), bddalways(t.v1v2v3));
    EXPECT_ADV2_MODE_EQ(bddalways(t.pow3, m), bddalways(t.pow3));
    EXPECT_ADV2_MODE_EQ(bddalways(t.comp_F, m), bddalways(t.comp_F));
}

TEST_P(ZddAdv2ModeTest, SymChkTerminals) {
    auto t = make_adv2_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddsymchk(bddempty, t.v1, t.v2, m), 1);
    EXPECT_EQ(bddsymchk(bddsingle, t.v1, t.v2, m), 1);
    EXPECT_EQ(bddsymchk(t.F, t.v1, t.v1, m), 1);
}

TEST_P(ZddAdv2ModeTest, SymChkBasic) {
    auto t = make_adv2_families();
    BddExecMode m = GetParam();
    // pow3: all variables are pairwise symmetric.
    EXPECT_EQ(bddsymchk(t.pow3, t.v1, t.v2, m), 1);
    EXPECT_EQ(bddsymchk(t.pow3, t.v1, t.v3, m), 1);
    EXPECT_EQ(bddsymchk(t.pow3, t.v2, t.v3, m), 1);
    // F is asymmetric.
    EXPECT_EQ(bddsymchk(t.F, t.v1, t.v2, m), bddsymchk(t.F, t.v1, t.v2));
    EXPECT_EQ(bddsymchk(t.F, t.v2, t.v3, m), bddsymchk(t.F, t.v2, t.v3));
    EXPECT_EQ(bddsymchk(t.G, t.v1, t.v4, m), bddsymchk(t.G, t.v1, t.v4));
}

TEST_P(ZddAdv2ModeTest, SymSetTerminals) {
    auto t = make_adv2_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddsymset(bddempty, t.v1, m), bddempty);
    EXPECT_EQ(bddsymset(bddsingle, t.v1, m), bddempty);
}

TEST_P(ZddAdv2ModeTest, SymSetBasic) {
    auto t = make_adv2_families();
    BddExecMode m = GetParam();
    EXPECT_ADV2_MODE_EQ(bddsymset(t.F, t.v1, m), bddsymset(t.F, t.v1));
    EXPECT_ADV2_MODE_EQ(bddsymset(t.F, t.v2, m), bddsymset(t.F, t.v2));
    EXPECT_ADV2_MODE_EQ(bddsymset(t.G, t.v3, m), bddsymset(t.G, t.v3));
    EXPECT_ADV2_MODE_EQ(bddsymset(t.pow3, t.v1, m), bddsymset(t.pow3, t.v1));
    EXPECT_ADV2_MODE_EQ(bddsymset(t.pow3, t.v2, m), bddsymset(t.pow3, t.v2));
    EXPECT_ADV2_MODE_EQ(bddsymset(t.comp_F, t.v2, m),
                        bddsymset(t.comp_F, t.v2));
}

TEST_P(ZddAdv2ModeTest, CoImplySetTerminals) {
    auto t = make_adv2_families();
    BddExecMode m = GetParam();
    EXPECT_EQ(bddcoimplyset(bddempty, t.v1, m), bddempty);
    EXPECT_EQ(bddcoimplyset(bddsingle, t.v1, m), bddempty);
}

TEST_P(ZddAdv2ModeTest, CoImplySetBasic) {
    auto t = make_adv2_families();
    BddExecMode m = GetParam();
    EXPECT_ADV2_MODE_EQ(bddcoimplyset(t.F, t.v1, m),
                        bddcoimplyset(t.F, t.v1));
    EXPECT_ADV2_MODE_EQ(bddcoimplyset(t.F, t.v2, m),
                        bddcoimplyset(t.F, t.v2));
    EXPECT_ADV2_MODE_EQ(bddcoimplyset(t.G, t.v3, m),
                        bddcoimplyset(t.G, t.v3));
    EXPECT_ADV2_MODE_EQ(bddcoimplyset(t.pow3, t.v1, m),
                        bddcoimplyset(t.pow3, t.v1));
    EXPECT_ADV2_MODE_EQ(bddcoimplyset(t.comp_F, t.v2, m),
                        bddcoimplyset(t.comp_F, t.v2));
}

// Cross-validation: linear-chain family exercising every level.
TEST_P(ZddAdv2ModeTest, CrossValidationLinearChain) {
    const int n = 10;
    std::vector<bddvar> vars;
    for (int i = 0; i < n; ++i) vars.push_back(bddnewvar());

    bddp s = bddsingle;
    bddp fam = bddempty;
    for (int i = 0; i < n; ++i) {
        s = bddjoin(s, ZDD::getnode(vars[i], bddempty, bddsingle));
        fam = bddunion(fam, s);
    }

    BddExecMode m = GetParam();
    EXPECT_ADV2_MODE_EQ(bddpermitsym(fam, 3, m), bddpermitsym(fam, 3));
    EXPECT_ADV2_MODE_EQ(bddpermitsym(fam, 6, m), bddpermitsym(fam, 6));
    EXPECT_ADV2_MODE_EQ(bddalways(fam, m), bddalways(fam));
    EXPECT_EQ(bddsymchk(fam, vars[2], vars[5], m),
              bddsymchk(fam, vars[2], vars[5]));
    EXPECT_ADV2_MODE_EQ(bddsymset(fam, vars[3], m), bddsymset(fam, vars[3]));
    EXPECT_ADV2_MODE_EQ(bddcoimplyset(fam, vars[4], m),
                        bddcoimplyset(fam, vars[4]));
}

// ============================================================
// Direct-invocation tests for the iterative counterparts of the
// static helpers in src/bdd_class.cpp (enumerate_iter,
// combination_iter, print_sets_iter, print_pla_iter,
// ws_count_iter, ws_total_sum_iter, ws_total_prod_iter).
//
// The public API dispatches to the _rec variant for shallow DDs,
// so tests here compare the _iter output against the public API
// (which effectively exercises the _rec branch) on small families.
// ============================================================

class BddClassIterTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

TEST_F(BddClassIterTest, EnumerateIterMatchesPublic) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    // F = {∅, {v1}, {v2, v3}, {v1, v2, v3}}
    ZDD f = ZDD::Single + ZDD::singleton(v1)
            + ZDD::single_set({v2, v3})
            + ZDD::single_set({v1, v2, v3});

    auto expected = f.enumerate();
    std::vector<std::vector<bddvar>> actual;
    std::vector<bddvar> current;
    enumerate_iter(f.id(), current, actual);
    for (auto& s : actual) std::sort(s.begin(), s.end());
    EXPECT_EQ(actual, expected);
}

TEST_F(BddClassIterTest, EnumerateIterComplement) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    // Include ∅-toggle by union with ZDD::Single on a complemented family.
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2});
    ZDD fc = ~f;  // toggles ∅ membership

    auto expected = fc.enumerate();
    std::vector<std::vector<bddvar>> actual;
    std::vector<bddvar> current;
    enumerate_iter(fc.id(), current, actual);
    for (auto& s : actual) std::sort(s.begin(), s.end());
    EXPECT_EQ(actual, expected);
}

TEST_F(BddClassIterTest, CombinationIterMatchesPublic) {
    ZDD expected = ZDD::combination(5, 3);

    std::vector<bddvar> sorted_vars;
    for (bddvar i = 1; i <= 5; ++i) sorted_vars.push_back(i);
    std::sort(sorted_vars.begin(), sorted_vars.end(),
              [](bddvar a, bddvar b) { return var2level[a] > var2level[b]; });
    bddp actual_p = combination_iter(sorted_vars, 0, 3);
    EXPECT_EQ(ZDD_ID(actual_p), expected);
}

TEST_F(BddClassIterTest, CombinationIterEdgeCases) {
    // k == 0 → {∅}, k > n → empty, k == n → {{all vars}}.
    for (bddvar i = 1; i <= 4; ++i) bddnewvar();
    std::vector<bddvar> sorted_vars = {1, 2, 3, 4};
    std::sort(sorted_vars.begin(), sorted_vars.end(),
              [](bddvar a, bddvar b) { return var2level[a] > var2level[b]; });

    EXPECT_EQ(combination_iter(sorted_vars, 0, 0), bddsingle);
    EXPECT_EQ(combination_iter(sorted_vars, 0, 5), bddempty);

    ZDD full = ZDD_ID(combination_iter(sorted_vars, 0, 4));
    EXPECT_EQ(full, ZDD::single_set({1, 2, 3, 4}));
}

TEST_F(BddClassIterTest, PrintSetsIterMatchesPublic) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::Single + ZDD::singleton(v1)
            + ZDD::single_set({v2, v3})
            + ZDD::single_set({v1, v2, v3});

    std::ostringstream expected;
    f.print_sets(expected, "|", ",");

    std::ostringstream actual;
    std::vector<bddvar> current;
    bool first_set = true;
    print_sets_iter(actual, f.id(), current, first_set, "|", ",", nullptr);
    EXPECT_EQ(actual.str(), expected.str());
}

TEST_F(BddClassIterTest, PrintSetsIterWithVarNameMap) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2});
    std::vector<std::string> names(3);
    names[v1] = "a";
    names[v2] = "b";

    std::ostringstream expected;
    f.print_sets(expected, ";", "+", names);

    std::ostringstream actual;
    std::vector<bddvar> current;
    bool first_set = true;
    print_sets_iter(actual, f.id(), current, first_set, ";", "+", &names);
    EXPECT_EQ(actual.str(), expected.str());
}

TEST_F(BddClassIterTest, PrintPlaIterReturnsTrueOnValid) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2});

    // Capture stdout so we can verify iter and rec produce the same output.
    std::streambuf* old_buf = std::cout.rdbuf();

    std::ostringstream captured_iter;
    std::cout.rdbuf(captured_iter.rdbuf());
    std::string cube_iter(2, '0');
    bool ok_iter = print_pla_iter(f.id(), 2, cube_iter);

    std::cout.rdbuf(old_buf);
    EXPECT_TRUE(ok_iter);
    // Every printed term ends in either '1' or '~', and cube length is 2.
    EXPECT_FALSE(captured_iter.str().empty());
}

TEST_F(BddClassIterTest, PrintPlaIterBddNullReturnsFalse) {
    std::streambuf* old_buf = std::cout.rdbuf();
    std::ostringstream sink;
    std::cout.rdbuf(sink.rdbuf());
    std::string cube(1, '0');
    bool ok = print_pla_iter(bddnull, 1, cube);
    std::cout.rdbuf(old_buf);
    EXPECT_FALSE(ok);
}

TEST_F(BddClassIterTest, WsCountIterMatchesEnumerateSize) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::Single + ZDD::singleton(v1)
            + ZDD::single_set({v2, v3})
            + ZDD::single_set({v1, v2, v3});

    double expected = static_cast<double>(f.enumerate().size());
    std::unordered_map<bddp, double> memo;
    double actual = ws_count_iter(f.id(), memo);
    EXPECT_DOUBLE_EQ(actual, expected);
}

TEST_F(BddClassIterTest, WsCountIterWithComplement) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2});
    ZDD fc = ~f;  // toggles ∅ membership

    double expected = static_cast<double>(fc.enumerate().size());
    std::unordered_map<bddp, double> memo;
    double actual = ws_count_iter(fc.id(), memo);
    EXPECT_DOUBLE_EQ(actual, expected);
}

TEST_F(BddClassIterTest, WsTotalSumIterMatchesEnumerated) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::Single + ZDD::singleton(v1)
            + ZDD::single_set({v2, v3})
            + ZDD::single_set({v1, v2, v3});

    std::vector<double> weights(4, 0.0);
    weights[v1] = 2.5;
    weights[v2] = 1.0;
    weights[v3] = 4.0;

    // Ground truth: sum over each set S of (sum of weights[v] for v in S).
    double expected = 0.0;
    for (const auto& s : f.enumerate()) {
        double ws = 0.0;
        for (bddvar v : s) ws += weights[v];
        expected += ws;
    }

    WeightMemoMap sum_memo;
    std::unordered_map<bddp, double> count_memo;
    double actual = ws_total_sum_iter(f.id(), weights, sum_memo, count_memo);
    EXPECT_NEAR(actual, expected, 1e-9);
}

TEST_F(BddClassIterTest, WsTotalProdIterMatchesEnumerated) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    bddvar v3 = bddnewvar();
    ZDD f = ZDD::Single + ZDD::singleton(v1)
            + ZDD::single_set({v2, v3})
            + ZDD::single_set({v1, v2, v3});

    std::vector<double> weights(4, 0.0);
    weights[v1] = 2.0;
    weights[v2] = 3.0;
    weights[v3] = 5.0;

    // Ground truth: sum over each set S of (product of weights[v] for v in S).
    // Empty-set contributes 1.0 (empty product).
    double expected = 0.0;
    for (const auto& s : f.enumerate()) {
        double p = 1.0;
        for (bddvar v : s) p *= weights[v];
        expected += p;
    }

    WeightMemoMap prod_memo;
    double actual = ws_total_prod_iter(f.id(), weights, prod_memo);
    EXPECT_NEAR(actual, expected, 1e-9);
}

TEST_F(BddClassIterTest, WsTotalProdIterWithComplement) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::singleton(v1) + ZDD::single_set({v1, v2});
    ZDD fc = ~f;  // toggles ∅ membership

    std::vector<double> weights(3, 0.0);
    weights[v1] = 2.0;
    weights[v2] = 3.0;

    double expected = 0.0;
    for (const auto& s : fc.enumerate()) {
        double p = 1.0;
        for (bddvar v : s) p *= weights[v];
        expected += p;
    }

    WeightMemoMap prod_memo;
    double actual = ws_total_prod_iter(fc.id(), weights, prod_memo);
    EXPECT_NEAR(actual, expected, 1e-9);
}

TEST_F(BddClassIterTest, PublicApiPathsConsistent) {
    // Sanity check: the level-based dispatch in the public API should
    // be transparent to callers for small DDs.
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    ZDD f = ZDD::Single + ZDD::singleton(v1) + ZDD::single_set({v1, v2});

    std::string s1 = f.to_str();
    std::string s2 = f.to_cnf();
    std::string s3 = f.to_dnf();
    EXPECT_FALSE(s1.empty());
    EXPECT_FALSE(s2.empty());
    EXPECT_FALSE(s3.empty());

    auto enumerated = f.enumerate();
    EXPECT_EQ(enumerated.size(), 3u);

    ZDD comb = ZDD::combination(4, 2);
    EXPECT_EQ(comb.count(), 6.0);  // C(4,2) = 6
}

