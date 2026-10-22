#include "permutation.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdlib>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

using namespace kyotodd;

/* ---- construction ---- */
TEST(PermutationTest, ConstructFromVector) {
    Permutation p({2, 3, 1});
    EXPECT_EQ(p.size(), 3);
    EXPECT_EQ(p.to_vector(), (std::vector<int>{2, 3, 1}));
}

TEST(PermutationTest, DefaultIsEmptyIdentity) {
    Permutation p;
    EXPECT_EQ(p.size(), 0);
    EXPECT_TRUE(p.to_vector().empty());
    EXPECT_EQ(p, Permutation::identity(0));
    EXPECT_EQ(p, Permutation::identity(5));
}

TEST(PermutationTest, Identity) {
    Permutation id = Permutation::identity(4);
    EXPECT_EQ(id.size(), 4);
    EXPECT_EQ(id.to_vector(), (std::vector<int>{1, 2, 3, 4}));
    EXPECT_THROW(Permutation::identity(-1), std::invalid_argument);
}

TEST(PermutationTest, ConstructRejectsNonPermutations) {
    EXPECT_THROW(Permutation({0, 1}), std::invalid_argument);
    EXPECT_THROW(Permutation({1, 3}), std::invalid_argument);
    EXPECT_THROW(Permutation({1, 1}), std::invalid_argument);
    EXPECT_THROW(Permutation({2, 2, 1}), std::invalid_argument);
    EXPECT_THROW(Permutation({-1, 2}), std::invalid_argument);
    EXPECT_NO_THROW(Permutation({1}));
    EXPECT_NO_THROW(Permutation(std::vector<int>()));
}

/* ---- image ---- */
TEST(PermutationTest, CallReturnsImage) {
    Permutation p({2, 3, 1});
    EXPECT_EQ(p(1), 2);
    EXPECT_EQ(p(2), 3);
    EXPECT_EQ(p(3), 1);
}

TEST(PermutationTest, CallBeyondSizeIsFixedPoint) {
    Permutation p({2, 1});
    EXPECT_EQ(p(3), 3);
    EXPECT_EQ(p(100), 100);
    EXPECT_EQ(Permutation()(7), 7);
}

TEST(PermutationTest, CallRejectsNonPositivePosition) {
    Permutation p({2, 1});
    EXPECT_THROW(p(0), std::out_of_range);
    EXPECT_THROW(p(-3), std::out_of_range);
}

/* ---- equality ignores trailing fixed points ---- */
TEST(PermutationTest, EqualityIgnoresTrailingFixedPoints) {
    EXPECT_EQ(Permutation({2, 1}), Permutation({2, 1, 3}));
    EXPECT_EQ(Permutation({2, 1, 3}), Permutation({2, 1}));
    EXPECT_EQ(Permutation({2, 1}), Permutation({2, 1, 3, 4, 5}));
    EXPECT_NE(Permutation({2, 1}), Permutation({2, 1, 4, 3}));
    EXPECT_NE(Permutation({2, 1}), Permutation({1, 2}));
    EXPECT_NE(Permutation({2, 1}), Permutation({2, 3, 1}));
    EXPECT_EQ(Permutation::identity(3), Permutation::identity(0));
}

TEST(PermutationTest, HashConsistentWithEquality) {
    EXPECT_EQ(Permutation({2, 1}).hash(), Permutation({2, 1, 3}).hash());
    EXPECT_EQ(Permutation({2, 1}).hash(), Permutation({2, 1, 3, 4}).hash());
    EXPECT_EQ(Permutation().hash(), Permutation::identity(6).hash());
    EXPECT_NE(Permutation({2, 1}).hash(), Permutation({1, 2}).hash());
    EXPECT_NE(Permutation({2, 3, 1}).hash(), Permutation({3, 1, 2}).hash());
}

/* ---- ordering ---- */
TEST(PermutationTest, LessIsLexicographic) {
    EXPECT_LT(Permutation({1, 2, 3}), Permutation({1, 3, 2}));
    EXPECT_LT(Permutation({1, 3, 2}), Permutation({2, 1, 3}));
    EXPECT_LT(Permutation({2, 1, 3}), Permutation({3, 2, 1}));
    EXPECT_FALSE(Permutation({2, 1}) < Permutation({2, 1, 3}));
    EXPECT_FALSE(Permutation({2, 1, 3}) < Permutation({2, 1}));
    /* extension with fixed points: [2 1] vs [2 1 4 3] */
    EXPECT_LT(Permutation({2, 1}), Permutation({2, 1, 4, 3}));
    EXPECT_FALSE(Permutation({2, 1, 4, 3}) < Permutation({2, 1}));
}

TEST(PermutationTest, UsableInStdSet) {
    std::set<Permutation> s;
    s.insert(Permutation({2, 1, 3}));
    s.insert(Permutation({2, 1}));        // equal to the first one
    s.insert(Permutation({1, 2, 3}));
    s.insert(Permutation({3, 2, 1}));
    EXPECT_EQ(s.size(), 3u);
    EXPECT_EQ(s.begin()->to_vector(), (std::vector<int>{1, 2, 3}));
}

/* ---- inverse ---- */
TEST(PermutationTest, Inverse) {
    Permutation p({2, 3, 1});
    EXPECT_EQ(p.inverse().to_vector(), (std::vector<int>{3, 1, 2}));
    EXPECT_EQ(p.inverse().inverse(), p);
    EXPECT_EQ(p * p.inverse(), Permutation::identity(3));
    EXPECT_EQ(p.inverse() * p, Permutation::identity(3));
    EXPECT_EQ(Permutation().inverse(), Permutation());
    EXPECT_EQ(Permutation({3, 1, 4, 2}).inverse().to_vector(),
              (std::vector<int>{2, 4, 1, 3}));
}

/* ---- composition: left operand first ---- */
TEST(PermutationTest, CompositionAppliesLeftOperandFirst) {
    Permutation p({2, 3, 1});
    Permutation q({1, 3, 2});
    /* (p * q)(i) = q(p(i)) */
    Permutation pq = p * q;
    EXPECT_EQ(pq.to_vector(), (std::vector<int>{3, 2, 1}));
    for (int i = 1; i <= 3; ++i) EXPECT_EQ(pq(i), q(p(i)));
    /* (q * p)(i) = p(q(i)) */
    Permutation qp = q * p;
    EXPECT_EQ(qp.to_vector(), (std::vector<int>{2, 1, 3}));
    for (int i = 1; i <= 3; ++i) EXPECT_EQ(qp(i), p(q(i)));
}

TEST(PermutationTest, CompositionWithIdentity) {
    Permutation p({3, 1, 2});
    EXPECT_EQ(p * Permutation::identity(3), p);
    EXPECT_EQ(Permutation::identity(3) * p, p);
    EXPECT_EQ(p * Permutation(), p);
    EXPECT_EQ(Permutation() * p, p);
}

TEST(PermutationTest, CompositionExtendsShorterOperand) {
    Permutation p({2, 1});
    Permutation q({1, 2, 4, 3});
    Permutation pq = p * q;
    EXPECT_EQ(pq.size(), 4);
    EXPECT_EQ(pq.to_vector(), (std::vector<int>{2, 1, 4, 3}));
    EXPECT_EQ(q * p, pq);   /* disjoint supports commute */
    Permutation r({3, 1, 2});
    EXPECT_EQ((p * r).size(), 3);
    EXPECT_EQ((p * r).to_vector(), (std::vector<int>{1, 3, 2}));
    EXPECT_EQ((r * p).to_vector(), (std::vector<int>{3, 2, 1}));
}

TEST(PermutationTest, CompositionIsAssociative) {
    std::srand(2024);
    for (int trial = 0; trial < 50; ++trial) {
        std::vector<int> a(6), b(6), c(6);
        for (int i = 0; i < 6; ++i) a[i] = b[i] = c[i] = i + 1;
        std::random_shuffle(a.begin(), a.end());
        std::random_shuffle(b.begin(), b.end());
        std::random_shuffle(c.begin(), c.end());
        Permutation pa(a), pb(b), pc(c);
        EXPECT_EQ((pa * pb) * pc, pa * (pb * pc));
        EXPECT_EQ((pa * pb).inverse(), pb.inverse() * pa.inverse());
    }
}

TEST(PermutationTest, InPlaceComposition) {
    Permutation p({2, 3, 1});
    Permutation q({1, 3, 2});
    Permutation r = p;
    r *= q;
    EXPECT_EQ(r, p * q);
}

/* ---- parity ---- */
TEST(PermutationTest, Parity) {
    EXPECT_TRUE(Permutation().is_even());
    EXPECT_TRUE(Permutation::identity(5).is_even());
    EXPECT_TRUE(Permutation({2, 1}).is_odd());
    EXPECT_FALSE(Permutation({2, 1}).is_even());
    EXPECT_TRUE(Permutation({2, 3, 1}).is_even());        /* 3-cycle */
    EXPECT_TRUE(Permutation({2, 1, 4, 3}).is_even());     /* two transpositions */
    EXPECT_TRUE(Permutation({4, 3, 2, 1}).is_even());     /* (1 4)(2 3) */
    EXPECT_TRUE(Permutation({2, 3, 4, 1}).is_odd());      /* 4-cycle */
}

TEST(PermutationTest, ParityIsMultiplicative) {
    std::srand(7);
    for (int trial = 0; trial < 50; ++trial) {
        std::vector<int> a(7), b(7);
        for (int i = 0; i < 7; ++i) a[i] = b[i] = i + 1;
        std::random_shuffle(a.begin(), a.end());
        std::random_shuffle(b.begin(), b.end());
        Permutation pa(a), pb(b);
        EXPECT_EQ((pa * pb).is_even(), pa.is_even() == pb.is_even());
        EXPECT_EQ(pa.inverse().is_even(), pa.is_even());
    }
}

/* ---- text ---- */
TEST(PermutationTest, ToStringAndStream) {
    EXPECT_EQ(Permutation({2, 3, 1}).to_string(), "[2 3 1]");
    EXPECT_EQ(Permutation().to_string(), "[]");
    EXPECT_EQ(Permutation({1}).to_string(), "[1]");
    std::ostringstream os;
    os << Permutation({3, 1, 2});
    EXPECT_EQ(os.str(), "[3 1 2]");
}
