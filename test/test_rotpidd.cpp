#include "rotpidd.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <set>

using namespace kyotodd;

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

/* ---- LeftRot overlapping case (src/rotpidd.cpp:121-124) ---- */
TEST_F(RotPiDDTest, LeftRotOverlappingCase) {
    /* Exercises the overlapping branch of left_rot where v <= y < u < x.
     * Compute id.LeftRot(x, y).LeftRot(u, v) and compare against a
     * brute-force cyclic-left-shift of positions.
     *
     * Composition convention (see from_vector reverse-order build):
     *   r.LeftRot(a, b) = r ∘ LeftRot(a, b), so in id.LeftRot(x, y).LeftRot(u, v)
     *   the RIGHTMOST call is applied first to the identity. */
    for (int i = 0; i < 5; ++i) RotPiDD_NewVar();

    const int x = 5, y = 2, u = 4, v = 1;
    ASSERT_TRUE(v <= y && y < u && u < x);  /* overlapping precondition */

    RotPiDD id(1);
    RotPiDD p = id.LeftRot(x, y).LeftRot(u, v);
    auto perms = p.RotPiDDToVectorOfPerms();
    ASSERT_EQ(perms.size(), 1u);

    /* Brute force (LeftRot(u,v) applied first, then LeftRot(x,y)):
     *   [1,2,3,4,5] -> LeftRot(4,1) -> [2,3,4,1,5]
     *   [2,3,4,1,5] -> LeftRot(5,2) -> [2,4,1,5,3] */
    std::vector<int> expected = {2, 4, 1, 5, 3};
    EXPECT_EQ(perms[0], expected);
}

/* ---- LeftRot contained case (src/rotpidd.cpp:125-128) ---- */
TEST_F(RotPiDDTest, LeftRotContainedCase) {
    /* Exercises the contained branch of left_rot where y < v <= u < x.
     * Compute id.LeftRot(x, y).LeftRot(u, v) and compare against a
     * brute-force cyclic-left-shift of positions. */
    for (int i = 0; i < 5; ++i) RotPiDD_NewVar();

    const int x = 5, y = 1, u = 4, v = 2;
    ASSERT_TRUE(y < v && v <= u && u < x);  /* contained precondition */

    RotPiDD id(1);
    RotPiDD p = id.LeftRot(x, y).LeftRot(u, v);
    auto perms = p.RotPiDDToVectorOfPerms();
    ASSERT_EQ(perms.size(), 1u);

    /* Brute force (LeftRot(u,v) applied first, then LeftRot(x,y)):
     *   [1,2,3,4,5] -> LeftRot(4,2) -> [1,3,4,2,5]
     *   [1,3,4,2,5] -> LeftRot(5,1) -> [3,4,2,5,1] */
    std::vector<int> expected = {3, 4, 2, 5, 1};
    EXPECT_EQ(perms[0], expected);
}

/* ---- LeftRot overlapping + contained algebraic identity ---- */
TEST_F(RotPiDDTest, LeftRotOverlappingContainedCrossVerify) {
    /* Cross-verification:
     *   Overlapping branch uses  LeftRot(u,v) o LeftRot(x,y) =
     *                            LeftRot(x,y+1) o LeftRot(u-1,v).
     *   Contained   branch uses  LeftRot(u,v) o LeftRot(x,y) =
     *                            LeftRot(x,y)   o LeftRot(u-1,v-1).
     * The LHS path runs through one branch and the RHS path through the
     * other, so mismatched results would reveal a branch-specific bug. */
    for (int i = 0; i < 5; ++i) RotPiDD_NewVar();
    RotPiDD id(1);

    /* Overlapping LHS (v=1 <= y=2 < u=4 < x=5)
     *   triggers overlapping branch.
     * RHS (id.LeftRot(u-1, v).LeftRot(x, y+1)) applies LeftRot(x=5, y+1=3)
     * to a RotPiDD whose top variable is LeftRot(3, 1), i.e. u'=5 > y'=1
     * and v'=3 > y'=1 -> contained branch. */
    RotPiDD lhs_ov = id.LeftRot(5, 2).LeftRot(4, 1);
    RotPiDD rhs_ov = id.LeftRot(3, 1).LeftRot(5, 3);
    EXPECT_EQ(lhs_ov, rhs_ov);

    /* Contained LHS (y=1 < v=2 <= u=4 < x=5) triggers contained branch.
     * RHS (id.LeftRot(u-1, v-1).LeftRot(x, y)) applies LeftRot(5, 1)
     * to a RotPiDD whose top variable is LeftRot(3, 1), i.e. v'=1 <= y'=1
     * -> overlapping branch. */
    RotPiDD lhs_ct = id.LeftRot(5, 1).LeftRot(4, 2);
    RotPiDD rhs_ct = id.LeftRot(3, 1).LeftRot(5, 1);
    EXPECT_EQ(lhs_ct, rhs_ct);
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

    /* r1 * r2 means apply r1 first, then r2 */
    RotPiDD comp = r1 * r2;
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

    /* Verify individual permutation parity */
    /* Even: {1,2,3}=id, {2,3,1}=(123), {3,1,2}=(132) */
    RotPiDD id   = RotPiDD::VECtoRotPiDD({1,2,3});
    RotPiDD p231 = RotPiDD::VECtoRotPiDD({2,3,1});
    RotPiDD p312 = RotPiDD::VECtoRotPiDD({3,1,2});
    EXPECT_EQ(id   & even, id);
    EXPECT_EQ(p231 & even, p231);
    EXPECT_EQ(p312 & even, p312);
    EXPECT_EQ(id   & odd, RotPiDD(0));
    EXPECT_EQ(p231 & odd, RotPiDD(0));
    EXPECT_EQ(p312 & odd, RotPiDD(0));

    /* Odd: {1,3,2}=(23), {2,1,3}=(12), {3,2,1}=(13) */
    RotPiDD p132 = RotPiDD::VECtoRotPiDD({1,3,2});
    RotPiDD p213 = RotPiDD::VECtoRotPiDD({2,1,3});
    RotPiDD p321 = RotPiDD::VECtoRotPiDD({3,2,1});
    EXPECT_EQ(p132 & odd, p132);
    EXPECT_EQ(p213 & odd, p213);
    EXPECT_EQ(p321 & odd, p321);
    EXPECT_EQ(p132 & even, RotPiDD(0));
    EXPECT_EQ(p213 & even, RotPiDD(0));
    EXPECT_EQ(p321 & even, RotPiDD(0));
}

TEST_F(RotPiDDTest, OddEven_EvenLengthLeftRot) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    /* LeftRot(3, 1) = 2 transpositions = even permutation */
    RotPiDD p = RotPiDD(1).LeftRot(3, 1);
    EXPECT_EQ(p.Card(), 1u);
    EXPECT_EQ(p.Odd().Card(), 0u);   // even perm -> not odd
    EXPECT_EQ(p.Even().Card(), 1u);  // even perm -> is even
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

// --- Terminal API without NewVar ---

TEST_F(RotPiDDTest, TerminalTopXY_NoNewVar) {
    RotPiDD empty(0);
    RotPiDD identity(1);
    EXPECT_EQ(empty.TopX(), 0);
    EXPECT_EQ(empty.TopY(), 0);
    EXPECT_EQ(identity.TopX(), 0);
    EXPECT_EQ(identity.TopY(), 0);
}

TEST_F(RotPiDDTest, OddEven_Terminal_NoNewVar) {
    RotPiDD identity(1);
    RotPiDD odd = identity.Odd();
    EXPECT_EQ(odd.Card(), 0u);  // identity is even
    RotPiDD even = identity.Even();
    EXPECT_EQ(even.Card(), 1u);
}

/* ---- contradictionMaximization ---- */
TEST_F(RotPiDDTest, ContradictionMaximization_NExceeds63) {
    RotPiDD_NewVar();
    RotPiDD identity(1);
    unsigned long long int used_set = 0;
    std::vector<int> unused_list;
    for (int i = 1; i <= 64; ++i) unused_list.push_back(i);
    std::unordered_map<
        std::pair<bddp, unsigned long long int>,
        long long int, RotPiDD::hash_func> hash;
    std::vector<std::vector<int>> w(65, std::vector<int>(65, 0));
    EXPECT_THROW(identity.contradictionMaximization(used_set, unused_list, 64, hash, w),
                 std::invalid_argument);
}

TEST_F(RotPiDDTest, ContradictionMaximization_Small) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    RotPiDD p = RotPiDD::VECtoRotPiDD({2, 1, 3}) + RotPiDD::VECtoRotPiDD({1, 3, 2});
    unsigned long long int used_set = 0;
    std::vector<int> unused_list = {1, 2, 3};
    std::unordered_map<
        std::pair<bddp, unsigned long long int>,
        long long int, RotPiDD::hash_func> hash;
    // w[i][j] = 1 for all i,j: any permutation scores the same
    std::vector<std::vector<int>> w(4, std::vector<int>(4, 1));
    long long int result = p.contradictionMaximization(used_set, unused_list, 3, hash, w);
    EXPECT_GE(result, 0);
}

/* ---- XOfLev zero-initialization after resize ---- */
TEST_F(RotPiDDTest, XOfLev_ZeroInitAfterResize) {
    for (int i = 0; i < 7; i++) {
        RotPiDD_NewVar();
    }
    EXPECT_GT(RotPiDD_VarTableSize, 16);
    int toplev = static_cast<int>(bddlevofvar(bddvarused()));
    for (int lev = toplev + 1; lev < RotPiDD_VarTableSize; lev++) {
        EXPECT_EQ(RotPiDD_XOfLev[lev], 0);
    }
}

/* ---- Enum/Enum2 output ---- */
TEST_F(RotPiDDTest, Enum_Output) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD id(1);
    RotPiDD rot = id.LeftRot(2, 1);
    RotPiDD set = id + rot;
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    set.Enum();
    std::cout.rdbuf(old);
    std::string result = oss.str();
    EXPECT_FALSE(result.empty());
    EXPECT_NE(result.find("+"), std::string::npos);
}

/* ---- Move semantics ---- */
TEST_F(RotPiDDTest, MoveConstructor) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD a = RotPiDD(1).LeftRot(2, 1);
    uint64_t expected_card = a.Card();
    RotPiDD b(std::move(a));
    EXPECT_EQ(b.Card(), expected_card);
}

TEST_F(RotPiDDTest, MoveAssignment) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD a = RotPiDD(1).LeftRot(2, 1);
    uint64_t expected_card = a.Card();
    RotPiDD b;
    b = std::move(a);
    EXPECT_EQ(b.Card(), expected_card);
}

TEST_F(RotPiDDTest, Enum2_Output) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD id(1);
    RotPiDD rot = id.LeftRot(2, 1);
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    rot.Enum2();
    std::cout.rdbuf(old);
    std::string result = oss.str();
    EXPECT_FALSE(result.empty());
}

/* ---- Error handling: exceptions instead of std::exit ---- */
TEST_F(RotPiDDTest, LeftRotThrowsOnOutOfRange) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD id(1);
    EXPECT_THROW(id.LeftRot(0, 1), std::invalid_argument);
    EXPECT_THROW(id.LeftRot(1, 0), std::invalid_argument);
    EXPECT_THROW(id.LeftRot(3, 1), std::invalid_argument);
    EXPECT_THROW(id.LeftRot(1, 3), std::invalid_argument);
}

TEST_F(RotPiDDTest, SwapThrowsOnOutOfRange) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD id(1);
    EXPECT_THROW(id.Swap(0, 1), std::invalid_argument);
    EXPECT_THROW(id.Swap(3, 1), std::invalid_argument);
}

TEST_F(RotPiDDTest, ReverseThrowsOnOutOfRange) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD id(1);
    EXPECT_THROW(id.Reverse(0, 1), std::invalid_argument);
    EXPECT_THROW(id.Reverse(3, 1), std::invalid_argument);
}

TEST_F(RotPiDDTest, CofactThrowsOnOutOfRange) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD id(1);
    EXPECT_THROW(id.Cofact(0, 1), std::invalid_argument);
    EXPECT_THROW(id.Cofact(3, 1), std::invalid_argument);
}

/* ---- save_svg ---- */
TEST_F(RotPiDDTest, SaveSvgString) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    RotPiDD id(1);
    RotPiDD r = id.LeftRot(2, 1);  // rotation (2,1)
    RotPiDD perm = r + id;          // {identity, rotation}

    std::string svg = perm.save_svg();
    EXPECT_TRUE(svg.find("<svg") != std::string::npos);
    EXPECT_TRUE(svg.find("</svg>") != std::string::npos);
    EXPECT_EQ(svg, perm.GetZDD().save_svg());
}

TEST_F(RotPiDDTest, SaveSvgStream) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    RotPiDD id(1);
    RotPiDD r = id.Swap(1, 2);

    std::ostringstream oss;
    r.save_svg(oss);
    EXPECT_EQ(oss.str(), r.save_svg());
}

TEST_F(RotPiDDTest, SaveSvgWithParams) {
    RotPiDD_NewVar();
    RotPiDD_NewVar();

    RotPiDD id(1);
    SvgParams params;
    params.mode = DrawMode::Raw;
    params.draw_zero = false;

    std::string svg = id.save_svg(params);
    EXPECT_TRUE(svg.find("<svg") != std::string::npos);
    EXPECT_EQ(svg, id.GetZDD().save_svg(params));
}

TEST_F(RotPiDDTest, SaveSvgTerminal) {
    RotPiDD empty(0);
    std::string svg = empty.save_svg();
    EXPECT_TRUE(svg.find("<svg") != std::string::npos);
    EXPECT_EQ(svg, empty.GetZDD().save_svg());
}

TEST_F(RotPiDDTest, SvgVarNameMap) {
    RotPiDD_NewVar();  // element 1
    RotPiDD_NewVar();  // element 2
    RotPiDD_NewVar();  // element 3

    std::map<bddvar, std::string> m = RotPiDD::svg_var_name_map();
    // Check that the map contains rotation labels
    bool found_21 = false, found_31 = false, found_32 = false;
    for (const auto& kv : m) {
        if (kv.second == "(2,1)") found_21 = true;
        if (kv.second == "(3,1)") found_31 = true;
        if (kv.second == "(3,2)") found_32 = true;
    }
    EXPECT_TRUE(found_21);
    EXPECT_TRUE(found_31);
    EXPECT_TRUE(found_32);

    // Use in SVG rendering
    RotPiDD id(1);
    RotPiDD r = id.LeftRot(2, 1) + id;
    SvgParams params;
    params.var_name_map = m;
    std::string svg = r.save_svg(params);
    EXPECT_TRUE(svg.find("<svg") != std::string::npos);
    EXPECT_TRUE(svg.find("(2,1)") != std::string::npos);
}

TEST_F(RotPiDDTest, RemoveMaxNonPositiveK) {
    // remove_max(k) with k <= 0 must return self without recursing into
    // negative-k land where left_rot would receive invalid arguments.
    for (int i = 1; i < 4; ++i) RotPiDD_NewVar();
    RotPiDD p(1);
    p = p.LeftRot(3, 1) + p.LeftRot(3, 2) + p;
    EXPECT_EQ(p.remove_max(0), p);
    EXPECT_EQ(p.remove_max(-1), p);
    EXPECT_EQ(p.remove_max(-100), p);
}

/* ---- Execution mode dispatch (_rec / _iter) ---- */

namespace {

const BddExecMode kRotModes[] = {BDD_EXEC_RECURSIVE, BDD_EXEC_ITERATIVE, BDD_EXEC_AUTO};

// All permutations of {1..n}, encoded one by one through from_vector.
RotPiDD rot_all_perms(int n) {
    std::vector<int> v(n);
    for (int i = 0; i < n; ++i) v[i] = i + 1;
    RotPiDD all(0);
    do {
        all += RotPiDD::from_vector(v);
    } while (std::next_permutation(v.begin(), v.end()));
    return all;
}

std::string rot_enum_str(const RotPiDD& p, BddExecMode mode, bool form2) {
    std::ostringstream oss;
    std::streambuf* old = std::cout.rdbuf(oss.rdbuf());
    try {
        if (form2) p.enumerate2(mode); else p.enumerate(mode);
    } catch (...) {
        std::cout.rdbuf(old);
        throw;
    }
    std::cout.rdbuf(old);
    return oss.str();
}

// { LeftRot(x, y) } for every rotation level 1..max_level: a ZDD chain with
// one node per level, so every RotPiDD recursion walks all of them.
RotPiDD rot_chain(bddvar max_level) {
    bddp f = bddempty;
    for (bddvar v = 1; v <= max_level; ++v) f = ZDD::getnode_raw(v, f, bddsingle);
    return RotPiDD(ZDD_ID(f));
}

long long rot_contmax(const RotPiDD& p, int n,
                      const std::vector< std::vector<int> >& w, BddExecMode mode) {
    std::vector<int> unused_list;
    for (int i = 1; i <= n; ++i) unused_list.push_back(i);
    RotPiDD::contmax_hash hash;
    return p.contradictionMaximization(0ULL, unused_list, n, hash, w, mode);
}

}  // namespace

TEST_F(RotPiDDTest, ExecModesAgree) {
    for (int i = 0; i < 5; ++i) RotPiDD_NewVar();  // one spare element for insert()
    RotPiDD all = rot_all_perms(4);
    ASSERT_EQ(all.card(), 24u);
    RotPiDD f = all.left_rot(3, 1) - RotPiDD(1).left_rot(2, 1) - RotPiDD(1);
    RotPiDD g = RotPiDD(1).left_rot(4, 2) + RotPiDD(1).left_rot(3, 2).left_rot(2, 1);
    std::vector<int> perm;
    perm.push_back(3); perm.push_back(1); perm.push_back(4); perm.push_back(2);
    std::vector< std::vector<int> > w(5, std::vector<int>(5, 0));
    for (int i = 0; i < 5; ++i)
        for (int j = 0; j < 5; ++j) w[i][j] = (i * 7 + j * 3) % 5 - 2;

    // Reference values via the operator / Auto forms.
    RotPiDD ref_lr = f.left_rot(4, 1);
    RotPiDD ref_swap = f.swap(1, 3);
    RotPiDD ref_rev = f.reverse(1, 4);
    RotPiDD ref_cof = f.cofact(2, 3);
    RotPiDD ref_odd = f.odd();
    RotPiDD ref_even = f.even();
    RotPiDD ref_mul = f * g;
    RotPiDD ref_order = f.order(1, 3);
    RotPiDD ref_inv = f.inverse();
    RotPiDD ref_ins = f.insert(2, 3);
    RotPiDD ref_rm = f.remove_max(2);
    RotPiDD ref_norm = all.normalizeRotPiDD(2);
    RotPiDD ref_fv = RotPiDD::from_vector(perm);
    std::vector< std::vector<int> > ref_perms = f.to_vector_of_perms();
    RotPiDD ref_ext = f.extract_one();
    std::string ref_e1 = rot_enum_str(f, BDD_EXEC_AUTO, false);
    std::string ref_e2 = rot_enum_str(f, BDD_EXEC_AUTO, true);
    std::vector<int> ul;
    for (int i = 1; i <= 4; ++i) ul.push_back(i);
    std::unordered_map< std::pair<bddp, unsigned long long int>, long long int,
                        RotPiDD::hash_func > h0;
    long long ref_cm = f.contradictionMaximization(0ULL, ul, 4, h0, w);
    EXPECT_EQ(ref_odd + ref_even, f);
    EXPECT_EQ(ref_inv.inverse(), f);
    EXPECT_EQ(ref_perms.size(), static_cast<size_t>(f.card()));

    for (int i = 0; i < 3; ++i) {
        BddExecMode m = kRotModes[i];
        bddgc();  // clear the operation cache so each mode really recomputes
        EXPECT_EQ(f.left_rot(4, 1, m), ref_lr);
        EXPECT_EQ(f.swap(1, 3, m), ref_swap);
        EXPECT_EQ(f.reverse(1, 4, m), ref_rev);
        EXPECT_EQ(f.cofact(2, 3, m), ref_cof);
        EXPECT_EQ(f.odd(m), ref_odd);
        EXPECT_EQ(f.even(m), ref_even);
        EXPECT_EQ(f.multiply(g, m), ref_mul);
        EXPECT_EQ(f.multiply(g), ref_mul);
        EXPECT_EQ(f.order(1, 3, m), ref_order);
        EXPECT_EQ(f.inverse(m), ref_inv);
        EXPECT_EQ(f.insert(2, 3, m), ref_ins);
        EXPECT_EQ(f.remove_max(2, m), ref_rm);
        EXPECT_EQ(all.normalizeRotPiDD(2, m), ref_norm);
        EXPECT_EQ(RotPiDD::from_vector(perm, m), ref_fv);
        EXPECT_EQ(f.to_vector_of_perms(m), ref_perms);
        EXPECT_EQ(f.extract_one(m), ref_ext);
        EXPECT_EQ(rot_enum_str(f, m, false), ref_e1);
        EXPECT_EQ(rot_enum_str(f, m, true), ref_e2);
        EXPECT_EQ(rot_contmax(f, 4, w, m), ref_cm);
        // Error paths and trivial cases are mode independent.
        EXPECT_THROW(f.left_rot(0, 1, m), std::invalid_argument);
        EXPECT_THROW(f.cofact(1, 9, m), std::invalid_argument);
        EXPECT_THROW(rot_contmax(f, 64, w, m), std::invalid_argument);
        EXPECT_EQ(RotPiDD(-1).left_rot(1, 2, m), RotPiDD(-1));
        EXPECT_EQ(RotPiDD(-1).odd(m), RotPiDD(-1));
        EXPECT_EQ(f.multiply(RotPiDD(1), m), f);
        EXPECT_EQ(RotPiDD(1).multiply(f, m), f);
        EXPECT_EQ(RotPiDD(0).inverse(m), RotPiDD(0));
        EXPECT_EQ(f.remove_max(0, m), f);
    }
}

TEST_F(RotPiDDTest, ExecModesDeep) {
    const int n = 130;  // elements; one spare so insert() can grow x to 130
    for (int i = 0; i < n; ++i) RotPiDD_NewVar();
    const bddvar levels = 129 * 128 / 2;  // every rotation with x <= 129
    ASSERT_GT(static_cast<int>(levels), BDD_RecurLimit);
    ASSERT_EQ(RotPiDD_Lev_XY(129, 1), static_cast<int>(levels));
    RotPiDD chain = rot_chain(levels);
    const double cnt = static_cast<double>(levels);
    ASSERT_EQ(chain.get_zdd().count(), cnt);
    ASSERT_EQ(chain.top_x(), 129);

    // left_rot: forced recursion must hit the limit, iterative must succeed.
    EXPECT_THROW(chain.left_rot(2, 1, BDD_EXEC_RECURSIVE), std::overflow_error);
    RotPiDD lr = chain.left_rot(2, 1, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(lr.get_zdd().count(), cnt);
    EXPECT_EQ(lr, chain.left_rot(2, 1));
    EXPECT_EQ(lr.left_rot(2, 1, BDD_EXEC_ITERATIVE), chain);  // involution

    // swap / reverse (composites over left_rot and multiply)
    EXPECT_THROW(chain.swap(1, 3, BDD_EXEC_RECURSIVE), std::overflow_error);
    RotPiDD sw = chain.swap(1, 3, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(sw.get_zdd().count(), cnt);
    EXPECT_EQ(sw, chain.swap(1, 3));
    // (reverse(1, 3) would reuse the swap(1, 3) composition cached just above,
    // so use a range whose reversal is not a single swap.)
    EXPECT_THROW(chain.reverse(1, 4, BDD_EXEC_RECURSIVE), std::overflow_error);
    RotPiDD rv = chain.reverse(1, 4, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(rv.get_zdd().count(), cnt);
    EXPECT_EQ(rv, chain.reverse(1, 4));

    // cofact: LeftRot(x, y) fixes position 1 iff y != 1 (128 rotations have y == 1).
    EXPECT_THROW(chain.cofact(1, 1, BDD_EXEC_RECURSIVE), std::overflow_error);
    RotPiDD c = chain.cofact(1, 1, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(c, chain.cofact(1, 1));
    EXPECT_EQ(c.get_zdd().count(), cnt - 128.0);

    // odd / even: LeftRot(x, y) is odd iff x - y is odd -> 64 * 65 of them.
    EXPECT_THROW(chain.odd(BDD_EXEC_RECURSIVE), std::overflow_error);
    RotPiDD o = chain.odd(BDD_EXEC_ITERATIVE);
    EXPECT_EQ(o.get_zdd().count(), 64.0 * 65.0);
    EXPECT_EQ(o + chain.even(BDD_EXEC_ITERATIVE), chain);
    EXPECT_EQ(o, chain.odd());

    // multiply: recursion runs over the right operand.
    RotPiDD t = RotPiDD(1).left_rot(2, 1);
    EXPECT_THROW(t.multiply(chain, BDD_EXEC_RECURSIVE), std::overflow_error);
    RotPiDD m = t.multiply(chain, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(m, t * chain);
    EXPECT_EQ(m.get_zdd().count(), cnt);

    // order: exactly one of pi(1) < pi(2) and pi(2) < pi(1) holds.
    EXPECT_THROW(chain.order(1, 2, BDD_EXEC_RECURSIVE), std::overflow_error);
    RotPiDD o12 = chain.order(1, 2, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(o12, chain.order(1, 2));
    RotPiDD o21 = chain.order(2, 1, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(o12 + o21, chain);
    EXPECT_EQ(o12 & o21, RotPiDD(0));

    // inverse
    EXPECT_THROW(chain.inverse(BDD_EXEC_RECURSIVE), std::overflow_error);
    RotPiDD inv = chain.inverse(BDD_EXEC_ITERATIVE);
    EXPECT_EQ(inv.get_zdd().count(), cnt);
    EXPECT_EQ(inv, chain.inverse());
    EXPECT_EQ(inv.inverse(BDD_EXEC_ITERATIVE), chain);

    // insert
    EXPECT_THROW(chain.insert(1, 1, BDD_EXEC_RECURSIVE), std::overflow_error);
    RotPiDD ins = chain.insert(1, 1, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(ins, chain.insert(1, 1));
    EXPECT_EQ(ins.get_zdd().count(), cnt);

    // remove_max / normalizeRotPiDD
    EXPECT_THROW(chain.remove_max(1, BDD_EXEC_RECURSIVE), std::overflow_error);
    EXPECT_EQ(chain.remove_max(1, BDD_EXEC_ITERATIVE), chain.remove_max(1));
    EXPECT_THROW(chain.normalizeRotPiDD(1, BDD_EXEC_RECURSIVE), std::overflow_error);
    RotPiDD nm = chain.normalizeRotPiDD(1, BDD_EXEC_ITERATIVE);
    EXPECT_EQ(nm, chain.normalizeRotPiDD(1));
    EXPECT_EQ(nm, RotPiDD(1));  // every rotation label is discarded

    // extract_one only follows 1-edges, so both modes succeed and agree.
    RotPiDD one_r = chain.extract_one(BDD_EXEC_RECURSIVE);
    RotPiDD one_i = chain.extract_one(BDD_EXEC_ITERATIVE);
    EXPECT_EQ(one_r, one_i);
    EXPECT_EQ(one_i.get_zdd().count(), 1.0);
    EXPECT_EQ(one_i & chain, one_i);

    // to_vector_of_perms
    EXPECT_THROW(chain.to_vector_of_perms(BDD_EXEC_RECURSIVE), std::overflow_error);
    std::vector< std::vector<int> > perms = chain.to_vector_of_perms(BDD_EXEC_ITERATIVE);
    EXPECT_EQ(perms.size(), static_cast<size_t>(levels));
    EXPECT_EQ(perms, chain.to_vector_of_perms());

    // enumerate / enumerate2
    EXPECT_THROW(rot_enum_str(chain, BDD_EXEC_RECURSIVE, false), std::overflow_error);
    std::string e1 = rot_enum_str(chain, BDD_EXEC_ITERATIVE, false);
    EXPECT_EQ(e1, rot_enum_str(chain, BDD_EXEC_AUTO, false));
    EXPECT_THROW(rot_enum_str(chain, BDD_EXEC_RECURSIVE, true), std::overflow_error);
    std::string e2 = rot_enum_str(chain, BDD_EXEC_ITERATIVE, true);
    EXPECT_EQ(e2, rot_enum_str(chain, BDD_EXEC_AUTO, true));
    size_t seps = 0;
    for (size_t pos = e2.find(" + "); pos != std::string::npos; pos = e2.find(" + ", pos + 3)) ++seps;
    EXPECT_EQ(seps + 1, static_cast<size_t>(levels));
    EXPECT_NE(e2.find("(129:1)"), std::string::npos);
    EXPECT_NE(e2.find("(2:1)"), std::string::npos);
}

// --- count / exact_count ------------------------------------------------

TEST_F(RotPiDDTest, CountAndExactCount) {
    for (int i = 0; i < 5; ++i) RotPiDD_NewVar();
    RotPiDD all = rot_all_perms(4);
    EXPECT_EQ(all.card(), 24u);
    EXPECT_EQ(all.count(), 24.0);
    EXPECT_EQ(all.exact_count(), bigint::BigInt(24));
    RotPiDD f = all - RotPiDD(1);
    EXPECT_EQ(f.count(), 23.0);
    EXPECT_EQ(f.exact_count().to_string(), "23");
    EXPECT_EQ(RotPiDD(0).count(), 0.0);
    EXPECT_EQ(RotPiDD(0).exact_count(), bigint::BigInt(0));
    EXPECT_EQ(RotPiDD(1).count(), 1.0);
    EXPECT_EQ(RotPiDD(1).exact_count(), bigint::BigInt(1));
}

TEST_F(RotPiDDTest, CountAndExactCountDeep) {
    const int n = 130;
    for (int i = 0; i < n; ++i) RotPiDD_NewVar();
    const bddvar levels = 129 * 128 / 2;  // > BDD_RecurLimit
    RotPiDD chain = rot_chain(levels);
    // card() wraps the recursive bddcard and hits the depth limit, while
    // count() / exact_count() switch to the iterative counters.
    EXPECT_THROW(chain.card(), std::overflow_error);
    EXPECT_EQ(chain.count(), static_cast<double>(levels));
    EXPECT_EQ(chain.exact_count(), bigint::BigInt(static_cast<unsigned long long>(levels)));
}

/* ---- Permutation interop ---- */
TEST_F(RotPiDDTest, FromPermutationRoundTrip) {
    for (int i = 0; i < 4; ++i) RotPiDD_NewVar();
    Permutation p({2, 3, 1, 4});
    RotPiDD r = RotPiDD::from_permutation(p);
    EXPECT_EQ(r, RotPiDD::from_vector(p.to_vector()));
    EXPECT_EQ(r.count(), 1.0);
    std::vector<Permutation> perms = r.to_permutations();
    ASSERT_EQ(perms.size(), 1u);
    EXPECT_EQ(perms[0], p);
    EXPECT_EQ(perms[0].size(), 4);
    EXPECT_EQ(perms[0].to_vector(), p.to_vector());
}

TEST_F(RotPiDDTest, FromPermutationShorterThanVarCount) {
    for (int i = 0; i < 5; ++i) RotPiDD_NewVar();
    Permutation p({2, 1});                 // positions 3..5 are fixed points
    RotPiDD r = RotPiDD::from_permutation(p);
    EXPECT_EQ(r, RotPiDD(1).left_rot(2, 1));
    std::vector<Permutation> perms = r.to_permutations();
    ASSERT_EQ(perms.size(), 1u);
    EXPECT_EQ(perms[0].size(), 5);         // padded to the variable count
    EXPECT_EQ(perms[0], p);                // but equal as permutations
    EXPECT_EQ(RotPiDD::from_permutation(Permutation()), RotPiDD(1));
    EXPECT_EQ(RotPiDD::from_permutation(Permutation::identity(3)), RotPiDD(1));
}

TEST_F(RotPiDDTest, FromPermutationRejectsTooLarge) {
    for (int i = 0; i < 3; ++i) RotPiDD_NewVar();
    EXPECT_THROW(RotPiDD::from_permutation(Permutation({2, 3, 4, 1})),
                 std::invalid_argument);
}

TEST_F(RotPiDDTest, ToPermutationsOfSet) {
    for (int i = 0; i < 3; ++i) RotPiDD_NewVar();
    RotPiDD all = rot_all_perms(3);
    std::vector<Permutation> perms = all.to_permutations();
    ASSERT_EQ(perms.size(), 6u);
    std::set<Permutation> distinct(perms.begin(), perms.end());
    EXPECT_EQ(distinct.size(), 6u);
    /* Same order and content as to_vector_of_perms() */
    std::vector< std::vector<int> > vecs = all.to_vector_of_perms();
    ASSERT_EQ(vecs.size(), perms.size());
    for (size_t i = 0; i < vecs.size(); ++i) EXPECT_EQ(perms[i].to_vector(), vecs[i]);
    EXPECT_TRUE(RotPiDD(0).to_permutations().empty());
    ASSERT_EQ(RotPiDD(1).to_permutations().size(), 1u);
    EXPECT_EQ(RotPiDD(1).to_permutations()[0], Permutation::identity(3));
}

TEST_F(RotPiDDTest, MultiplyMatchesPermutationComposition) {
    for (int i = 0; i < 3; ++i) RotPiDD_NewVar();
    Permutation a({2, 3, 1}), b({1, 3, 2});
    RotPiDD A = RotPiDD::from_permutation(a);
    RotPiDD B = RotPiDD::from_permutation(b);
    /* A * B applies A first, then B, exactly like Permutation::operator*:
       (a * b)(i) = b(a(i)) gives [3 2 1], while b first gives [2 1 3]. */
    EXPECT_EQ((a * b).to_vector(), (std::vector<int>{3, 2, 1}));
    EXPECT_EQ((b * a).to_vector(), (std::vector<int>{2, 1, 3}));
    EXPECT_EQ(A * B, RotPiDD::from_permutation(a * b));
    EXPECT_EQ(B * A, RotPiDD::from_permutation(b * a));
    EXPECT_NE(A * B, B * A);
    std::vector<Permutation> perms = (A * B).to_permutations();
    ASSERT_EQ(perms.size(), 1u);
    EXPECT_EQ(perms[0].to_vector(), (std::vector<int>{3, 2, 1}));
}

TEST_F(RotPiDDTest, PermutationOpsAgreeWithRotPiDDOpsRandom) {
    for (int i = 0; i < 5; ++i) RotPiDD_NewVar();
    std::srand(99);
    for (int t = 0; t < 30; ++t) {
        std::vector<int> va(5), vb(5);
        for (int i = 0; i < 5; ++i) va[i] = vb[i] = i + 1;
        std::random_shuffle(va.begin(), va.end());
        std::random_shuffle(vb.begin(), vb.end());
        Permutation a(va), b(vb);
        RotPiDD A = RotPiDD::from_permutation(a);
        RotPiDD B = RotPiDD::from_permutation(b);
        EXPECT_EQ(A * B, RotPiDD::from_permutation(a * b));
        EXPECT_EQ(A.inverse(), RotPiDD::from_permutation(a.inverse()));
        EXPECT_EQ(A.odd() != RotPiDD(0), a.is_odd());
        EXPECT_EQ(A.even() != RotPiDD(0), a.is_even());
        std::vector<Permutation> perms = (A + B).to_permutations();
        std::set<Permutation> distinct(perms.begin(), perms.end());
        EXPECT_TRUE(distinct.count(a));
        EXPECT_TRUE(distinct.count(b));
    }
}

TEST_F(RotPiDDTest, PermutationExecModesAgree) {
    for (int i = 0; i < 4; ++i) RotPiDD_NewVar();
    RotPiDD all = rot_all_perms(4);
    Permutation p({3, 1, 4, 2});
    RotPiDD ref_fp = RotPiDD::from_permutation(p);
    std::vector<Permutation> ref_tp = all.to_permutations();
    for (size_t m = 0; m < sizeof(kRotModes) / sizeof(kRotModes[0]); ++m) {
        EXPECT_EQ(RotPiDD::from_permutation(p, kRotModes[m]), ref_fp);
        EXPECT_EQ(all.to_permutations(kRotModes[m]), ref_tp);
    }
}

/* ---- cofact: brute-force semantics ---- */
namespace {

// Delete position u and value v from a one-line permutation w with
// w[u-1] == v: positions above u and values above v shift down by one.
std::vector<int> rot_remove_pos_val(const std::vector<int>& w, int u, int v) {
    std::vector<int> r;
    for (size_t i = 0; i < w.size(); ++i) {
        if (static_cast<int>(i) + 1 == u) continue;
        r.push_back(w[i] > v ? w[i] - 1 : w[i]);
    }
    return r;
}

// Reference cofact(u, v) of a set of permutations given as vectors.
RotPiDD rot_cofact_ref(const std::vector< std::vector<int> >& perms, int u, int v) {
    RotPiDD r(0);
    for (size_t i = 0; i < perms.size(); ++i) {
        if (perms[i][u - 1] != v) continue;
        r += RotPiDD::from_vector(rot_remove_pos_val(perms[i], u, v));
    }
    return r;
}

std::vector< std::vector<int> > all_vectors(int n, int pad_to) {
    std::vector< std::vector<int> > perms;
    std::vector<int> v(n);
    for (int i = 0; i < n; ++i) v[i] = i + 1;
    do {
        std::vector<int> e = v;
        for (int k = n + 1; k <= pad_to; ++k) e.push_back(k);
        perms.push_back(e);
    } while (std::next_permutation(v.begin(), v.end()));
    return perms;
}

}  // namespace

TEST_F(RotPiDDTest, CofactSinglePermutation) {
    for (int i = 0; i < 5; ++i) RotPiDD_NewVar();
    Permutation b({1, 5, 3, 2, 4});
    RotPiDD B = RotPiDD::from_permutation(b);
    for (int u = 1; u <= 5; ++u) {
        for (int v = 1; v <= 5; ++v) {
            RotPiDD c = B.cofact(u, v);
            if (b(u) != v) {
                EXPECT_EQ(c, RotPiDD(0)) << "u=" << u << " v=" << v;
                continue;
            }
            std::vector<Permutation> ps = c.to_permutations();
            ASSERT_EQ(ps.size(), 1u) << "u=" << u << " v=" << v;
            EXPECT_EQ(ps[0], Permutation(rot_remove_pos_val(b.to_vector(), u, v)))
                << "u=" << u << " v=" << v;
        }
    }
    /* Delete position u and value v, shift the rest down. */
    EXPECT_EQ(B.cofact(1, 1).to_permutations()[0], Permutation({4, 2, 1, 3}));
    EXPECT_EQ(B.cofact(2, 5).to_permutations()[0], Permutation({1, 3, 2, 4}));
    EXPECT_EQ(B.cofact(3, 3).to_permutations()[0], Permutation({1, 4, 2, 3}));
    EXPECT_EQ(B.cofact(4, 2).to_permutations()[0], Permutation({1, 4, 2, 3}));
    EXPECT_EQ(B.cofact(5, 4).to_permutations()[0], Permutation({1, 4, 3, 2}));
    /* The top variable's cofactor is the 1-child. */
    EXPECT_EQ(B.cofact(5, 4), RotPiDD(B.get_zdd().OnSet0(B.get_zdd().Top())));
}

TEST_F(RotPiDDTest, CofactMatchesBruteForce) {
    for (int i = 0; i < 5; ++i) RotPiDD_NewVar();
    std::vector< std::vector<int> > perms = all_vectors(5, 5);
    RotPiDD all = rot_all_perms(5);
    ASSERT_EQ(all.count(), 120.0);
    for (int u = 1; u <= 5; ++u) {
        for (int v = 1; v <= 5; ++v) {
            RotPiDD ref = rot_cofact_ref(perms, u, v);
            EXPECT_EQ(ref.count(), 24.0);
            EXPECT_EQ(all.cofact(u, v), ref) << "u=" << u << " v=" << v;
        }
    }
    /* Random subsets, both execution modes. */
    std::srand(2025);
    for (int t = 0; t < 10; ++t) {
        std::vector< std::vector<int> > sub;
        RotPiDD s(0);
        for (size_t i = 0; i < perms.size(); ++i) {
            if (std::rand() % 3 == 0) {
                sub.push_back(perms[i]);
                s += RotPiDD::from_vector(perms[i]);
            }
        }
        for (int u = 1; u <= 5; ++u) {
            for (int v = 1; v <= 5; ++v) {
                RotPiDD ref = rot_cofact_ref(sub, u, v);
                bddgc();  // clear the operation cache so each mode really recomputes
                EXPECT_EQ(s.cofact(u, v, BDD_EXEC_RECURSIVE), ref) << "u=" << u << " v=" << v;
                bddgc();
                EXPECT_EQ(s.cofact(u, v, BDD_EXEC_ITERATIVE), ref) << "u=" << u << " v=" << v;
            }
        }
    }
}

TEST_F(RotPiDDTest, CofactWithSpareVariables) {
    for (int i = 0; i < 7; ++i) RotPiDD_NewVar();   // permutations of 4 inside 7 variables
    std::vector< std::vector<int> > perms = all_vectors(4, 7);
    RotPiDD all = rot_all_perms(4);
    for (int u = 1; u <= 7; ++u) {
        for (int v = 1; v <= 7; ++v) {
            EXPECT_EQ(all.cofact(u, v), rot_cofact_ref(perms, u, v)) << "u=" << u << " v=" << v;
        }
    }
    /* Positions beyond the support are fixed points. */
    EXPECT_EQ(all.cofact(6, 6), all);
    EXPECT_EQ(all.cofact(6, 7), RotPiDD(0));
    EXPECT_EQ(all.cofact(2, 6), RotPiDD(0));
}

TEST_F(RotPiDDTest, CofactChainCountsExtensions) {
    /* Counting permutations with pi(u1) = v1 and pi(u2) = v2 via two
       cofacts (indices shift after the first deletion). */
    for (int i = 0; i < 5; ++i) RotPiDD_NewVar();
    RotPiDD all = rot_all_perms(5);
    RotPiDD c1 = all.cofact(2, 4);          // pi(2) = 4: 24 permutations
    EXPECT_EQ(c1.count(), 24.0);
    /* pi(4) = 1 on the original: position 4 -> 3 after deleting position 2,
       value 1 stays 1 after deleting value 4. */
    RotPiDD c2 = c1.cofact(3, 1);
    EXPECT_EQ(c2.count(), 6.0);
    std::vector< std::vector<int> > perms = all_vectors(5, 5);
    int brute = 0;
    for (size_t i = 0; i < perms.size(); ++i)
        if (perms[i][1] == 4 && perms[i][3] == 1) ++brute;
    EXPECT_EQ(brute, 6);
}
