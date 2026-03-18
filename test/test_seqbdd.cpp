#include "seqbdd.h"
#include <gtest/gtest.h>
#include <sstream>

class SeqBDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
    }
};

/* ---- Constructors ---- */
TEST_F(SeqBDDTest, ConstructorDefault) {
    SeqBDD s;
    EXPECT_EQ(s, SeqBDD(0));
    EXPECT_EQ(s.card(), 0u);
}

TEST_F(SeqBDDTest, ConstructorEmpty) {
    SeqBDD s(0);
    EXPECT_EQ(s.card(), 0u);
    EXPECT_EQ(s.top(), 0);
}

TEST_F(SeqBDDTest, ConstructorEpsilon) {
    SeqBDD s(1);
    EXPECT_EQ(s.card(), 1u);
    EXPECT_EQ(s.top(), 0);
}

TEST_F(SeqBDDTest, ConstructorNull) {
    SeqBDD s(-1);
    EXPECT_NE(s, SeqBDD(0));
    EXPECT_NE(s, SeqBDD(1));
}

TEST_F(SeqBDDTest, ConstructorCopy) {
    SeqBDD a(1);
    SeqBDD b(a);
    EXPECT_EQ(a, b);
}

TEST_F(SeqBDDTest, ConstructorFromZDD) {
    BDD_NewVar();
    ZDD z = ZDD::Single;
    SeqBDD s(z);
    EXPECT_EQ(s.card(), 1u);
}

/* ---- Equality ---- */
TEST_F(SeqBDDTest, Equality) {
    SeqBDD a(0);
    SeqBDD b(0);
    SeqBDD c(1);
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a == c);
    EXPECT_TRUE(a != c);
    EXPECT_FALSE(a != b);
}

/* ---- Push ---- */
TEST_F(SeqBDDTest, PushSingleElement) {
    bddvar v1 = BDD_NewVar();
    /* Sequence {v1} = push v1 onto epsilon */
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    EXPECT_EQ(s.card(), 1u);
    EXPECT_EQ(s.top(), static_cast<int>(v1));
    EXPECT_EQ(s.len(), 1u);
}

TEST_F(SeqBDDTest, PushMultipleElements) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    /* Sequence v1 v2 v3 = push v3, then v2, then v1 onto epsilon */
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v3))
                        .push(static_cast<int>(v2))
                        .push(static_cast<int>(v1));
    EXPECT_EQ(s.card(), 1u);
    EXPECT_EQ(s.len(), 3u);
}

TEST_F(SeqBDDTest, PushRepeatedVariable) {
    bddvar v1 = BDD_NewVar();
    /* Sequence v1 v1 = push v1 twice */
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1))
                        .push(static_cast<int>(v1));
    EXPECT_EQ(s.card(), 1u);
    EXPECT_EQ(s.len(), 2u);
}

TEST_F(SeqBDDTest, PushOnEmpty) {
    bddvar v1 = BDD_NewVar();
    /* push on empty set gives empty set */
    SeqBDD s = SeqBDD(0).push(static_cast<int>(v1));
    EXPECT_EQ(s, SeqBDD(0));
}

/* ---- on_set0 ---- */
TEST_F(SeqBDDTest, OnSet0MatchingTop) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    /* Sequence v1 v2 */
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v2))
                        .push(static_cast<int>(v1));
    /* on_set0(v1) should give {v2} */
    SeqBDD r = s.on_set0(static_cast<int>(v1));
    SeqBDD expected = SeqBDD(1).push(static_cast<int>(v2));
    EXPECT_EQ(r, expected);
}

TEST_F(SeqBDDTest, OnSet0NonMatchingTop) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    /* Sequence v1 v2 */
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v2))
                        .push(static_cast<int>(v1));
    /* on_set0(v2) should give empty (v2 is not the first element) */
    SeqBDD r = s.on_set0(static_cast<int>(v2));
    EXPECT_EQ(r, SeqBDD(0));
}

TEST_F(SeqBDDTest, OnSet0MixedSet) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    /* Set = {v1, v2} (two single-element sequences) */
    SeqBDD s1 = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD s2 = SeqBDD(1).push(static_cast<int>(v2));
    SeqBDD s = s1 + s2;
    EXPECT_EQ(s.card(), 2u);
    /* on_set0(v1) should give {epsilon} */
    SeqBDD r1 = s.on_set0(static_cast<int>(v1));
    EXPECT_EQ(r1, SeqBDD(1));
    /* on_set0(v2) should give {epsilon} */
    SeqBDD r2 = s.on_set0(static_cast<int>(v2));
    EXPECT_EQ(r2, SeqBDD(1));
}

/* ---- off_set ---- */
TEST_F(SeqBDDTest, OffSetMatchingTop) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    /* Set = {v1, v2} */
    SeqBDD s1 = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD s2 = SeqBDD(1).push(static_cast<int>(v2));
    SeqBDD s = s1 + s2;
    /* off_set(v2) removes sequences starting with v2 */
    SeqBDD r = s.off_set(static_cast<int>(v2));
    EXPECT_EQ(r, s1);
}

TEST_F(SeqBDDTest, OffSetNonMatchingTop) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    /* Set = {v1 v2, v1 v3} */
    SeqBDD sa = SeqBDD(1).push(static_cast<int>(v2))
                         .push(static_cast<int>(v1));
    SeqBDD sb = SeqBDD(1).push(static_cast<int>(v3))
                         .push(static_cast<int>(v1));
    SeqBDD s = sa + sb;
    /* off_set(v1) should give empty (all sequences start with v1) */
    SeqBDD r = s.off_set(static_cast<int>(v1));
    EXPECT_EQ(r, SeqBDD(0));
}

/* ---- on_set ---- */
TEST_F(SeqBDDTest, OnSetBasic) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    /* Set = {v1, v2} */
    SeqBDD s1 = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD s2 = SeqBDD(1).push(static_cast<int>(v2));
    SeqBDD s = s1 + s2;
    /* on_set(v1) returns {v1} */
    SeqBDD r = s.on_set(static_cast<int>(v1));
    EXPECT_EQ(r, s1);
}

/* ---- Set operations ---- */
TEST_F(SeqBDDTest, Union) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    SeqBDD s1 = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD s2 = SeqBDD(1).push(static_cast<int>(v2));
    SeqBDD u = s1 + s2;
    EXPECT_EQ(u.card(), 2u);
}

TEST_F(SeqBDDTest, Intersection) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    SeqBDD s1 = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD s2 = SeqBDD(1).push(static_cast<int>(v2));
    SeqBDD u = s1 + s2;
    SeqBDD r = u & s1;
    EXPECT_EQ(r, s1);
}

TEST_F(SeqBDDTest, Difference) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    SeqBDD s1 = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD s2 = SeqBDD(1).push(static_cast<int>(v2));
    SeqBDD u = s1 + s2;
    SeqBDD r = u - s1;
    EXPECT_EQ(r, s2);
}

/* ---- Concatenation ---- */
TEST_F(SeqBDDTest, ConcatEpsilonLeft) {
    bddvar v1 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD r = SeqBDD(1) * s;
    EXPECT_EQ(r, s);
}

TEST_F(SeqBDDTest, ConcatEpsilonRight) {
    bddvar v1 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD r = s * SeqBDD(1);
    EXPECT_EQ(r, s);
}

TEST_F(SeqBDDTest, ConcatEmptyLeft) {
    bddvar v1 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD r = SeqBDD(0) * s;
    EXPECT_EQ(r, SeqBDD(0));
}

TEST_F(SeqBDDTest, ConcatEmptyRight) {
    bddvar v1 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD r = s * SeqBDD(0);
    EXPECT_EQ(r, SeqBDD(0));
}

TEST_F(SeqBDDTest, ConcatTwoSingletons) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    SeqBDD a = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD b = SeqBDD(1).push(static_cast<int>(v2));
    /* {v1} * {v2} = {v1 v2} */
    SeqBDD r = a * b;
    SeqBDD expected = SeqBDD(1).push(static_cast<int>(v2))
                               .push(static_cast<int>(v1));
    EXPECT_EQ(r, expected);
    EXPECT_EQ(r.card(), 1u);
    EXPECT_EQ(r.len(), 2u);
}

TEST_F(SeqBDDTest, ConcatMultipleSequences) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    /* {v1, v2} * {v1} = {v1 v1, v2 v1} */
    SeqBDD a = SeqBDD(1).push(static_cast<int>(v1))
             + SeqBDD(1).push(static_cast<int>(v2));
    SeqBDD b = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD r = a * b;
    EXPECT_EQ(r.card(), 2u);
}

TEST_F(SeqBDDTest, ConcatSpecExample) {
    /* Spec example: (ab + c) * (bd + 1) = {ab, abbd, c, cbd} */
    bddvar a = BDD_NewVar();  /* var 1 */
    bddvar b = BDD_NewVar();  /* var 2 */
    bddvar c = BDD_NewVar();  /* var 3 */
    bddvar d = BDD_NewVar();  /* var 4 */

    /* Build {ab, c} */
    SeqBDD ab = SeqBDD(1).push(static_cast<int>(b))
                         .push(static_cast<int>(a));
    SeqBDD sc = SeqBDD(1).push(static_cast<int>(c));
    SeqBDD lhs = ab + sc;
    EXPECT_EQ(lhs.card(), 2u);

    /* Build {bd, epsilon} */
    SeqBDD bd = SeqBDD(1).push(static_cast<int>(d))
                         .push(static_cast<int>(b));
    SeqBDD rhs = bd + SeqBDD(1);
    EXPECT_EQ(rhs.card(), 2u);

    /* Concatenation */
    SeqBDD result = lhs * rhs;
    EXPECT_EQ(result.card(), 4u);

    /* Verify individual sequences are present */
    SeqBDD abbd = SeqBDD(1).push(static_cast<int>(d))
                           .push(static_cast<int>(b))
                           .push(static_cast<int>(b))
                           .push(static_cast<int>(a));
    SeqBDD cbd = SeqBDD(1).push(static_cast<int>(d))
                          .push(static_cast<int>(b))
                          .push(static_cast<int>(c));

    SeqBDD expected = ab + abbd + sc + cbd;
    EXPECT_EQ(result, expected);
}

/* ---- Left quotient ---- */
TEST_F(SeqBDDTest, QuotientByEpsilon) {
    bddvar v1 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    /* f / {epsilon} = f */
    SeqBDD r = s / SeqBDD(1);
    EXPECT_EQ(r, s);
}

TEST_F(SeqBDDTest, QuotientBySelf) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v2))
                        .push(static_cast<int>(v1));
    /* f / f = {epsilon} */
    SeqBDD r = s / s;
    EXPECT_EQ(r, SeqBDD(1));
}

TEST_F(SeqBDDTest, QuotientByEmpty) {
    bddvar v1 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    EXPECT_THROW(s / SeqBDD(0), std::invalid_argument);
}

TEST_F(SeqBDDTest, QuotientSimple) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    /* {v1 v2 v3} / {v1} = {v2 v3} */
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v3))
                        .push(static_cast<int>(v2))
                        .push(static_cast<int>(v1));
    SeqBDD prefix = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD r = s / prefix;
    SeqBDD expected = SeqBDD(1).push(static_cast<int>(v3))
                               .push(static_cast<int>(v2));
    EXPECT_EQ(r, expected);
}

TEST_F(SeqBDDTest, QuotientNoPrefixMatch) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    /* {v1} / {v2} = empty */
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD prefix = SeqBDD(1).push(static_cast<int>(v2));
    SeqBDD r = s / prefix;
    EXPECT_EQ(r, SeqBDD(0));
}

TEST_F(SeqBDDTest, QuotientMultiplePrefix) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    /* {v1 v2 v3, v1 v3} / {v1} = {v2 v3, v3} */
    SeqBDD s1 = SeqBDD(1).push(static_cast<int>(v3))
                         .push(static_cast<int>(v2))
                         .push(static_cast<int>(v1));
    SeqBDD s2 = SeqBDD(1).push(static_cast<int>(v3))
                         .push(static_cast<int>(v1));
    SeqBDD s = s1 + s2;
    SeqBDD prefix = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD r = s / prefix;
    SeqBDD e1 = SeqBDD(1).push(static_cast<int>(v3))
                         .push(static_cast<int>(v2));
    SeqBDD e2 = SeqBDD(1).push(static_cast<int>(v3));
    EXPECT_EQ(r, e1 + e2);
}

/* ---- Left remainder ---- */
TEST_F(SeqBDDTest, RemainderByEpsilon) {
    bddvar v1 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    /* f % {epsilon} = f - {epsilon} * f = f - f = 0 */
    SeqBDD r = s % SeqBDD(1);
    EXPECT_EQ(r, SeqBDD(0));
}

TEST_F(SeqBDDTest, RemainderBySelf) {
    bddvar v1 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    /* f % f = f - f * (f / f) = f - f * {eps} = f - f = 0 */
    SeqBDD r = s % s;
    EXPECT_EQ(r, SeqBDD(0));
}

TEST_F(SeqBDDTest, RemainderIdentity) {
    /* f == p * (f / p) + (f % p) */
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    bddvar v3 = BDD_NewVar();
    SeqBDD f = SeqBDD(1).push(static_cast<int>(v3))
                        .push(static_cast<int>(v2))
                        .push(static_cast<int>(v1))
             + SeqBDD(1).push(static_cast<int>(v2));
    SeqBDD p = SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD quot = f / p;
    SeqBDD rem = f % p;
    EXPECT_EQ(f, p * quot + rem);
}

/* ---- Query methods ---- */
TEST_F(SeqBDDTest, QuerySize) {
    bddvar v1 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    EXPECT_GE(s.size(), 1u);
}

TEST_F(SeqBDDTest, QueryGetZDD) {
    bddvar v1 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    ZDD z = s.get_zdd();
    EXPECT_EQ(z.Card(), 1u);
}

TEST_F(SeqBDDTest, QueryLit) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    /* {v1 v2} has 2 literals */
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v2))
                        .push(static_cast<int>(v1));
    EXPECT_EQ(s.lit(), 2u);
}

/* ---- Compound assignment ---- */
TEST_F(SeqBDDTest, CompoundAssignUnion) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    s += SeqBDD(1).push(static_cast<int>(v2));
    EXPECT_EQ(s.card(), 2u);
}

TEST_F(SeqBDDTest, CompoundAssignConcat) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    s *= SeqBDD(1).push(static_cast<int>(v2));
    SeqBDD expected = SeqBDD(1).push(static_cast<int>(v2))
                               .push(static_cast<int>(v1));
    EXPECT_EQ(s, expected);
}

TEST_F(SeqBDDTest, CompoundAssignQuotient) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v2))
                        .push(static_cast<int>(v1));
    s /= SeqBDD(1).push(static_cast<int>(v1));
    SeqBDD expected = SeqBDD(1).push(static_cast<int>(v2));
    EXPECT_EQ(s, expected);
}

/* ---- print_seq / seq_str ---- */
TEST_F(SeqBDDTest, SeqStrUndefined) {
    SeqBDD s(-1);
    EXPECT_EQ(s.seq_str(), "(undefined)");
}

TEST_F(SeqBDDTest, SeqStrEmpty) {
    SeqBDD s(0);
    EXPECT_EQ(s.seq_str(), "(empty)");
}

TEST_F(SeqBDDTest, SeqStrEpsilon) {
    SeqBDD s(1);
    EXPECT_EQ(s.seq_str(), "e");
}

TEST_F(SeqBDDTest, SeqStrSingleElement) {
    bddvar v1 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1));
    /* Level of v1 with default ordering is 1 */
    std::string str = s.seq_str();
    EXPECT_EQ(str, std::to_string(bddlevofvar(v1)));
}

TEST_F(SeqBDDTest, SeqStrMultiElement) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v2))
                        .push(static_cast<int>(v1));
    std::string str = s.seq_str();
    std::string expected = std::to_string(bddlevofvar(v1)) + " " +
                           std::to_string(bddlevofvar(v2));
    EXPECT_EQ(str, expected);
}

TEST_F(SeqBDDTest, SeqStrMultipleSequences) {
    bddvar v1 = BDD_NewVar();
    bddvar v2 = BDD_NewVar();
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1))
             + SeqBDD(1).push(static_cast<int>(v2));
    std::string str = s.seq_str();
    /* Should contain " + " separator */
    EXPECT_NE(str.find(" + "), std::string::npos);
}

TEST_F(SeqBDDTest, SeqStrEpsilonInSet) {
    bddvar v1 = BDD_NewVar();
    /* {v1, epsilon} */
    SeqBDD s = SeqBDD(1).push(static_cast<int>(v1)) + SeqBDD(1);
    std::string str = s.seq_str();
    /* Should contain "e" for epsilon */
    EXPECT_NE(str.find("e"), std::string::npos);
}
