#include <gtest/gtest.h>
#include "bdd.h"

class DDBaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        bddinit(1024, UINT64_MAX);
    }
    void TearDown() override {
        bddfinal();
    }
};

// --- is_terminal ---

TEST_F(DDBaseTest, IsTerminal_BDDFalse) {
    EXPECT_TRUE(BDD::False.is_terminal());
}

TEST_F(DDBaseTest, IsTerminal_BDDTrue) {
    EXPECT_TRUE(BDD::True.is_terminal());
}

TEST_F(DDBaseTest, IsTerminal_ZDDEmpty) {
    EXPECT_TRUE(ZDD::Empty.is_terminal());
}

TEST_F(DDBaseTest, IsTerminal_ZDDSingle) {
    EXPECT_TRUE(ZDD::Single.is_terminal());
}

TEST_F(DDBaseTest, IsTerminal_NonTerminal) {
    bddvar v = bddnewvar();
    BDD x = BDD_ID(bddprime(v));
    EXPECT_FALSE(x.is_terminal());
}

TEST_F(DDBaseTest, IsTerminal_BDDNull) {
    EXPECT_FALSE(BDD::Null.is_terminal());
}

// --- is_one ---

TEST_F(DDBaseTest, IsOne_BDDTrue) {
    EXPECT_TRUE(BDD::True.is_one());
}

TEST_F(DDBaseTest, IsOne_BDDFalse) {
    EXPECT_FALSE(BDD::False.is_one());
}

TEST_F(DDBaseTest, IsOne_ZDDSingle) {
    EXPECT_TRUE(ZDD::Single.is_one());
}

TEST_F(DDBaseTest, IsOne_ZDDEmpty) {
    EXPECT_FALSE(ZDD::Empty.is_one());
}

TEST_F(DDBaseTest, IsOne_BDDNull) {
    EXPECT_FALSE(BDD::Null.is_one());
}

TEST_F(DDBaseTest, IsOne_NonTerminal) {
    bddvar v = bddnewvar();
    BDD x = BDD_ID(bddprime(v));
    EXPECT_FALSE(x.is_one());
}

// --- is_zero ---

TEST_F(DDBaseTest, IsZero_BDDFalse) {
    EXPECT_TRUE(BDD::False.is_zero());
}

TEST_F(DDBaseTest, IsZero_BDDTrue) {
    EXPECT_FALSE(BDD::True.is_zero());
}

TEST_F(DDBaseTest, IsZero_ZDDEmpty) {
    EXPECT_TRUE(ZDD::Empty.is_zero());
}

TEST_F(DDBaseTest, IsZero_ZDDSingle) {
    EXPECT_FALSE(ZDD::Single.is_zero());
}

TEST_F(DDBaseTest, IsZero_BDDNull) {
    EXPECT_FALSE(BDD::Null.is_zero());
}

// --- get_id ---

TEST_F(DDBaseTest, GetId_MatchesGetID_BDD) {
    BDD b(1);
    EXPECT_EQ(b.get_id(), b.GetID());
}

TEST_F(DDBaseTest, GetId_MatchesGetID_ZDD) {
    ZDD z(1);
    EXPECT_EQ(z.get_id(), z.GetID());
}

TEST_F(DDBaseTest, GetId_BDDFalse) {
    EXPECT_EQ(BDD::False.get_id(), bddfalse);
}

TEST_F(DDBaseTest, GetId_BDDTrue) {
    EXPECT_EQ(BDD::True.get_id(), bddtrue);
}

TEST_F(DDBaseTest, GetId_ZDDEmpty) {
    EXPECT_EQ(ZDD::Empty.get_id(), bddempty);
}

TEST_F(DDBaseTest, GetId_ZDDSingle) {
    EXPECT_EQ(ZDD::Single.get_id(), bddsingle);
}

// --- top ---

TEST_F(DDBaseTest, Top_MatchesOldTop_BDD) {
    bddvar v = bddnewvar();
    BDD x = BDD_ID(bddprime(v));
    EXPECT_EQ(x.top(), x.Top());
}

TEST_F(DDBaseTest, Top_MatchesOldTop_ZDD) {
    bddvar v = bddnewvar();
    ZDD z = ZDD_ID(bddchange(bddsingle, v));
    EXPECT_EQ(z.top(), z.Top());
}

TEST_F(DDBaseTest, Top_Terminal_BDD) {
    EXPECT_EQ(BDD::False.top(), static_cast<bddvar>(0));
    EXPECT_EQ(BDD::True.top(), static_cast<bddvar>(0));
}

// --- raw_size ---

TEST_F(DDBaseTest, RawSize_Terminal) {
    EXPECT_EQ(BDD::False.raw_size(), 0u);
    EXPECT_EQ(BDD::True.raw_size(), 0u);
    EXPECT_EQ(ZDD::Empty.raw_size(), 0u);
    EXPECT_EQ(ZDD::Single.raw_size(), 0u);
}

TEST_F(DDBaseTest, RawSize_SingleVar_BDD) {
    bddvar v = bddnewvar();
    BDD x = BDD_ID(bddprime(v));
    EXPECT_EQ(x.raw_size(), 1u);
}

TEST_F(DDBaseTest, RawSize_SingleVar_ZDD) {
    bddvar v = bddnewvar();
    ZDD z = ZDD_ID(bddchange(bddsingle, v));
    EXPECT_EQ(z.raw_size(), 1u);
}

TEST_F(DDBaseTest, RawSize_MatchesOldSize_BDD) {
    bddvar v1 = bddnewvar();
    bddvar v2 = bddnewvar();
    BDD x = BDD_ID(bddprime(v1));
    BDD y = BDD_ID(bddprime(v2));
    BDD z = x & y;
    EXPECT_EQ(z.raw_size(), z.Size());
}

// --- GC protection ---

TEST_F(DDBaseTest, GCProtection_CopyConstruct) {
    bddvar v = bddnewvar();
    BDD x = BDD_ID(bddprime(v));
    BDD y(x);
    EXPECT_EQ(x.get_id(), y.get_id());
}

TEST_F(DDBaseTest, GCProtection_MoveConstruct) {
    bddvar v = bddnewvar();
    BDD x = BDD_ID(bddprime(v));
    bddp id = x.get_id();
    BDD y(std::move(x));
    EXPECT_EQ(y.get_id(), id);
    EXPECT_EQ(x.get_id(), bddnull);
}

TEST_F(DDBaseTest, GCProtection_CopyAssign) {
    bddvar v = bddnewvar();
    BDD x = BDD_ID(bddprime(v));
    BDD y;
    y = x;
    EXPECT_EQ(x.get_id(), y.get_id());
}

TEST_F(DDBaseTest, GCProtection_MoveAssign) {
    bddvar v = bddnewvar();
    BDD x = BDD_ID(bddprime(v));
    bddp id = x.get_id();
    BDD y;
    y = std::move(x);
    EXPECT_EQ(y.get_id(), id);
    EXPECT_EQ(x.get_id(), bddnull);
}

// --- sizeof check ---

TEST_F(DDBaseTest, SizeOf_BDD) {
    EXPECT_EQ(sizeof(BDD), sizeof(bddp));
}
