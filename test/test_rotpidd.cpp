#include "rotpidd.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <set>

class RotPiDDTest : public ::testing::Test {
protected:
    void SetUp() override {
        BDD_Init(1024, UINT64_MAX);
        RotPiDD_TopVar = 0;
        RotPiDD_VarTableSize = 16;
        if (RotPiDD_XOfLev) { delete[] RotPiDD_XOfLev; RotPiDD_XOfLev = 0; }
    }
};

/* ---- RotPiDD_NewVar ---- */
TEST_F(RotPiDDTest, NewVarBasic) {
    EXPECT_EQ(RotPiDD_VarUsed(), 0);
    int v1 = RotPiDD_NewVar();
    EXPECT_EQ(v1, 1);
    EXPECT_EQ(RotPiDD_VarUsed(), 1);

    int v2 = RotPiDD_NewVar();
    EXPECT_EQ(v2, 2);
    EXPECT_EQ(RotPiDD_VarUsed(), 2);

    int v3 = RotPiDD_NewVar();
    EXPECT_EQ(v3, 3);
    EXPECT_EQ(RotPiDD_VarUsed(), 3);
}

/* ---- Level layout ---- */
TEST_F(RotPiDDTest, LevelLayout) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    /* After 4 RotPiDD vars, BDD levels should be:
       lev 1: (2,1), lev 2: (3,2), lev 3: (3,1),
       lev 4: (4,3), lev 5: (4,2), lev 6: (4,1) */
    EXPECT_EQ(RotPiDD_X_Lev(1), 2);
    EXPECT_EQ(RotPiDD_Y_Lev(1), 1);

    EXPECT_EQ(RotPiDD_X_Lev(2), 3);
    EXPECT_EQ(RotPiDD_Y_Lev(2), 2);

    EXPECT_EQ(RotPiDD_X_Lev(3), 3);
    EXPECT_EQ(RotPiDD_Y_Lev(3), 1);

    EXPECT_EQ(RotPiDD_X_Lev(4), 4);
    EXPECT_EQ(RotPiDD_Y_Lev(4), 3);

    EXPECT_EQ(RotPiDD_X_Lev(5), 4);
    EXPECT_EQ(RotPiDD_Y_Lev(5), 2);

    EXPECT_EQ(RotPiDD_X_Lev(6), 4);
    EXPECT_EQ(RotPiDD_Y_Lev(6), 1);
}

/* ---- Identity ---- */
TEST_F(RotPiDDTest, Identity) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    RotPiDD id(1);
    EXPECT_EQ(id.Card(), 1u);

    /* Identity composed with identity is identity */
    RotPiDD r = id * id;
    EXPECT_EQ(r, id);
}

/* ---- LeftRot basic ---- */
TEST_F(RotPiDDTest, LeftRotBasic) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    RotPiDD id(1);

    /* LeftRot(u, u) = identity */
    EXPECT_EQ(id.LeftRot(2, 2), id);

    /* LeftRot(2, 1) should produce a single non-identity permutation */
    RotPiDD r = id.LeftRot(2, 1);
    EXPECT_EQ(r.Card(), 1u);
    EXPECT_NE(r, id);

    /* LeftRot(2, 1) twice should give identity back (it's a swap of 2 elements) */
    RotPiDD r2 = r.LeftRot(2, 1);
    EXPECT_EQ(r2, id);
}

/* ---- VECtoRotPiDD and RotPiDDToVectorOfPerms roundtrip ---- */
TEST_F(RotPiDDTest, VecConversionRoundtrip) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    /* Test a specific permutation [2, 3, 1, 4] */
    std::vector<int> perm = {2, 3, 1, 4};
    RotPiDD r = RotPiDD::VECtoRotPiDD(perm);
    EXPECT_EQ(r.Card(), 1u);

    auto perms = r.RotPiDDToVectorOfPerms();
    ASSERT_EQ(perms.size(), 1u);
    EXPECT_EQ(perms[0], perm);
}

/* ---- Union of permutations ---- */
TEST_F(RotPiDDTest, UnionOfPerms) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    std::vector<int> p1 = {1, 2, 3};
    std::vector<int> p2 = {2, 1, 3};
    std::vector<int> p3 = {3, 2, 1};

    RotPiDD r1 = RotPiDD::VECtoRotPiDD(p1);
    RotPiDD r2 = RotPiDD::VECtoRotPiDD(p2);
    RotPiDD r3 = RotPiDD::VECtoRotPiDD(p3);

    RotPiDD u = r1 + r2 + r3;
    EXPECT_EQ(u.Card(), 3u);

    auto perms = u.RotPiDDToVectorOfPerms();
    ASSERT_EQ(perms.size(), 3u);

    /* Check all three permutations are present */
    std::set< std::vector<int> > permset(perms.begin(), perms.end());
    EXPECT_TRUE(permset.count(p1));
    EXPECT_TRUE(permset.count(p2));
    EXPECT_TRUE(permset.count(p3));
}

/* ---- Composition ---- */
TEST_F(RotPiDDTest, Composition) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    /* [2, 3, 1] composed with [3, 1, 2] = ? */
    std::vector<int> p1 = {2, 3, 1};
    std::vector<int> p2 = {3, 1, 2};
    RotPiDD r1 = RotPiDD::VECtoRotPiDD(p1);
    RotPiDD r2 = RotPiDD::VECtoRotPiDD(p2);

    /* r2 * r1 means apply r1 first, then r2 */
    RotPiDD comp = r2 * r1;
    auto perms = comp.RotPiDDToVectorOfPerms();
    ASSERT_EQ(perms.size(), 1u);

    /* Manual: r2(r1(1))=r2(2)=1, r2(r1(2))=r2(3)=2, r2(r1(3))=r2(1)=3 => [1,2,3] = id */
    std::vector<int> expected = {1, 2, 3};
    EXPECT_EQ(perms[0], expected);
}

/* ---- Inverse ---- */
TEST_F(RotPiDDTest, Inverse) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    std::vector<int> perm = {3, 1, 4, 2};
    RotPiDD r = RotPiDD::VECtoRotPiDD(perm);
    RotPiDD inv = r.Inverse();

    /* r * inv should be identity */
    RotPiDD comp = r * inv;
    auto perms = comp.RotPiDDToVectorOfPerms();
    ASSERT_EQ(perms.size(), 1u);

    std::vector<int> id = {1, 2, 3, 4};
    EXPECT_EQ(perms[0], id);
}

/* ---- Odd / Even ---- */
TEST_F(RotPiDDTest, OddEven) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    /* Build all 6 permutations of [1,2,3] */
    std::vector< std::vector<int> > all_perms = {
        {1,2,3}, {1,3,2}, {2,1,3}, {2,3,1}, {3,1,2}, {3,2,1}
    };

    RotPiDD all(0);
    for (auto& p : all_perms) {
        all += RotPiDD::VECtoRotPiDD(p);
    }
    EXPECT_EQ(all.Card(), 6u);

    RotPiDD odd = all.Odd();
    RotPiDD even = all.Even();

    /* 3 odd, 3 even permutations */
    EXPECT_EQ(odd.Card(), 3u);
    EXPECT_EQ(even.Card(), 3u);

    /* Identity is even */
    RotPiDD id = RotPiDD::VECtoRotPiDD({1,2,3});
    EXPECT_EQ(id & even, id);
    EXPECT_EQ(id & odd, RotPiDD(0));
}

/* ---- Extract_One ---- */
TEST_F(RotPiDDTest, ExtractOne) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    std::vector<int> p1 = {2, 3, 1};
    std::vector<int> p2 = {3, 1, 2};
    RotPiDD r = RotPiDD::VECtoRotPiDD(p1) + RotPiDD::VECtoRotPiDD(p2);
    EXPECT_EQ(r.Card(), 2u);

    RotPiDD one = r.Extract_One();
    EXPECT_EQ(one.Card(), 1u);

    /* The extracted permutation should be in the original set */
    EXPECT_EQ(one & r, one);
}

/* ---- Set operations ---- */
TEST_F(RotPiDDTest, SetOperations) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    RotPiDD a = RotPiDD::VECtoRotPiDD({1,2,3}) + RotPiDD::VECtoRotPiDD({2,1,3});
    RotPiDD b = RotPiDD::VECtoRotPiDD({2,1,3}) + RotPiDD::VECtoRotPiDD({3,2,1});

    /* Intersection */
    RotPiDD inter = a & b;
    EXPECT_EQ(inter.Card(), 1u);

    /* Difference */
    RotPiDD diff = a - b;
    EXPECT_EQ(diff.Card(), 1u);
}

/* ---- normalizePerm ---- */
TEST_F(RotPiDDTest, NormalizePerm) {
    std::vector<int> v = {10, 5, 20, 1};
    RotPiDD::normalizePerm(v);
    std::vector<int> expected = {3, 2, 4, 1};
    EXPECT_EQ(v, expected);
}

/* ---- Swap ---- */
TEST_F(RotPiDDTest, SwapOperation) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    /* Start with identity, swap positions 1 and 3 → [3, 2, 1] */
    RotPiDD id(1);
    RotPiDD swapped = id.Swap(1, 3);
    auto perms = swapped.RotPiDDToVectorOfPerms();
    ASSERT_EQ(perms.size(), 1u);

    std::vector<int> expected = {3, 2, 1};
    EXPECT_EQ(perms[0], expected);
}

/* ---- Cofact ---- */
TEST_F(RotPiDDTest, Cofact) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    /* Build all 6 perms of [1,2,3] */
    RotPiDD all(0);
    std::vector< std::vector<int> > all_perms = {
        {1,2,3}, {1,3,2}, {2,1,3}, {2,3,1}, {3,1,2}, {3,2,1}
    };
    for (auto& p : all_perms) {
        all += RotPiDD::VECtoRotPiDD(p);
    }

    /* Cofact(1, 2): perms where position 1 has value 2 → {[2,1,3], [2,3,1]} */
    RotPiDD cf = all.Cofact(1, 2);
    /* The result should have 2 elements (sub-permutations) */
    EXPECT_EQ(cf.Card(), 2u);
}

/* ---- Composition of sets ---- */
TEST_F(RotPiDDTest, SetComposition) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    /* {id, (12)} * {id, (23)} should have 4 elements */
    RotPiDD a = RotPiDD::VECtoRotPiDD({1,2,3}) + RotPiDD::VECtoRotPiDD({2,1,3});
    RotPiDD b = RotPiDD::VECtoRotPiDD({1,2,3}) + RotPiDD::VECtoRotPiDD({1,3,2});

    RotPiDD comp = b * a;
    /* Results: id*id=id, id*(12)=(12), (23)*id=(23), (23)*(12)=(132) => 4 distinct */
    EXPECT_EQ(comp.Card(), 4u);
}
